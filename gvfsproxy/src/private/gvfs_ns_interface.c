

#include <gvfs_config.h>
#include <gvfs_core.h>

#include <gvfs_ns_interface.h>
#include <gvfs_upstream.h>
#include <gvfs_ns_file_upload.h>
#include <gvfs_ns_file_download.h>
#include <gvfs_ns_file_delete.h>
#include <gvfs_ns_file_query.h>


static INT32 gvfs_ns_process(gvfs_upstream_t *upstream);

static VOID gvfs_ns_process_response_body(gvfs_event_t *rev);

ssize_t gvfs_ns_read_response_header(gvfs_connection_t *c)
{
	gvfs_event_t             *rev;
	gvfs_upstream_t          *upstream;
	gvfs_buf_t               *buf;
	gvfs_rsp_head_t          *rsp_head;
	UINT8                    *p;
	
	ssize_t                   n, received;
	size_t                    size, rsize;

	rev = c->read;
	upstream = c->data;
	buf = &upstream->buf;

	size = sizeof(gvfs_ns_rsp_head_t);
	received = 0;

	for ( ; ; ) {
		rsize = size - (buf->last - buf->pos);
		
		n = c->recv(c, buf->last, rsize);
		
		if (GVFS_AGAIN == n || GVFS_ERROR == n || 0 == n) {
			return n;
		}

		buf->last += n;
		received += n;

		if (received == rsize) {
			upstream->header_completed = 1;
			break;
		}
	}

	rsp_head = &upstream->rsp_head;
	p = buf->pos;
	
	rsp_head->len = gvfs_rb32(p);
	p += sizeof(UINT32);
	
	rsp_head->id = gvfs_rb32(p);
	p += sizeof(UINT32);
	
	rsp_head->type = gvfs_rb32(p);
	p += sizeof(UINT32);
	
	rsp_head->rtn = gvfs_rb32(p);

	gvfs_log(LOG_INFO, "recv from ns type:%02x len:%u id:%u rtn:%u",
			 rsp_head->type, rsp_head->len, rsp_head->id, rsp_head->rtn);
						
	return n;
}


VOID gvfs_ns_process_response_header(gvfs_event_t *rev)
{
	gvfs_connection_t        *c;
	gvfs_upstream_t          *upstream;
	gvfs_http_request_t      *r;
	gvfs_peer_connection_t   *pc;
	ssize_t                   n;
	gvfs_http_response_code   code;
	
	c = rev->data;
	upstream = c->data;
	pc = &upstream->peer;
	r = pc->data;

	code = HTTP_Ok;
	if (rev->timedout) {
		gvfs_log(LOG_DEBUG, "wait ns for client:%s fd:%d sid:%u timeout",
				 r->connection->addr_text.data, c->fd, upstream->sid);
		code = HTTP_NotAcceptable;
		goto failed;
	}

	for ( ; ; ) {
		n = gvfs_ns_read_response_header(c);

		if (GVFS_AGAIN == n) {
			return;
		}
		else if (GVFS_ERROR == n) {
			gvfs_log(LOG_ERROR, "read ns for client:%s fd:%d sid:%u error",
					 r->connection->addr_text.data, c->fd, upstream->sid);
			code = HTTP_InternalServerError;
			goto failed;
		}
		else if (0 == n) {
			gvfs_log(LOG_INFO, "ns close connection for client:%s fd:%d sid:%u",
							   r->connection->addr_text.data, c->fd,
							   upstream->sid);
			code = HTTP_NotAcceptable;
			goto failed;
		}

		if (upstream->header_completed) {
			rev->handler = gvfs_ns_process_response_body;
			rev->handler(rev);
			return;
		}
	}

failed:
	gvfs_http_finalize_request(r, code);
	gvfs_close_upstream(r);
	return;
}

ssize_t gvfs_ns_read_response_body(gvfs_connection_t *c)
{
	gvfs_event_t             *rev;
	gvfs_upstream_t          *upstream;
	gvfs_buf_t               *buf;
	gvfs_rsp_head_t          *rsp_head;
	
	ssize_t                   n, received;
	size_t                    size, rsize;

	rev = c->read;
	upstream = c->data;
	rsp_head = &upstream->rsp_head;
	buf = &upstream->buf;

	size = rsp_head->len;
	received = 0;

	for ( ; ; ) {
		rsize = size - (buf->last - buf->pos);
		
		n = c->recv(c, buf->last, rsize);
		
		if (GVFS_AGAIN == n || GVFS_ERROR == n || 0 == n) {
			return n;
		}

		buf->last += n;
		received += n;
		
		if (received == rsize) {
			upstream->body_completed = 1;
			break;
		}
	}

	return n;
}

static VOID gvfs_ns_process_response_body(gvfs_event_t *rev)
{
	gvfs_connection_t        *c;
	gvfs_upstream_t          *upstream;
	gvfs_http_request_t      *r;
	gvfs_peer_connection_t   *pc;
	ssize_t                   n;
	gvfs_http_response_code   code;
	
	c = rev->data;
	upstream = c->data;
	pc = &upstream->peer;
	r = pc->data;

	code = HTTP_Ok;
	for ( ; ; ) {
		n = gvfs_ns_read_response_body(c);
		
		if (GVFS_AGAIN == n) {
			return;
		}
		else if (GVFS_ERROR == n) {
			gvfs_log(LOG_ERROR, "read ns for client:%s fd:%d sid:%u error",
					 r->connection->addr_text.data, c->fd, upstream->sid);
			code = HTTP_InternalServerError;
			goto failed;
		}
		else if (0 == n) {
			gvfs_log(LOG_INFO, "ns close connection for client:%s fd:%d sid:%u",
					 		   r->connection->addr_text.data, c->fd,
					 		   upstream->sid);
			code = HTTP_NotAcceptable;
			goto failed;
		}

		if (upstream->body_completed) {
		
			if (GVFS_OK != gvfs_ns_process(upstream)) {
				goto failed;
			}

			if (NS_FILE_DOWNLOAD != upstream->rsp_head.type) {
				gvfs_http_finalize_request(r, code);
			}
			
			return;
		}
	}

failed:
	gvfs_http_finalize_request(r, code);
	gvfs_close_upstream(r);
	return;
}

static INT32 gvfs_ns_process(gvfs_upstream_t *upstream)
{
	gvfs_buf_t             *buf;
	gvfs_rsp_head_t        *rsp_head;
	gvfs_http_request_t    *r;
	UINT32                  type;
	INT32                   rc;

	buf = &upstream->buf;
	rsp_head = &upstream->rsp_head;

	type = rsp_head->type;
	
	rsp_head->data = buf->pos;

	r = upstream->peer.data;
	
	switch (type) {
	
		case NS_FILE_QUERY:
		{
			rsp_head->rsh = gvfs_palloc(upstream->pool, sizeof(gvfs_ns_file_query_rsp_t));
			if (NULL == rsp_head->rsh) {
				return GVFS_ERROR;
			}

			rc = gvfs_ns_file_query_process(r, rsp_head);
			if (GVFS_NS_OK != rc) {
				return GVFS_ERROR;
			}

			rc = gvfs_ns_file_query_completed(r, rsp_head);
			gvfs_close_upstream(r);
			if (GVFS_NS_OK != rc) {
				return GVFS_ERROR;
			}

			break;
		}

		case NS_FILE_UPLOAD:
		{
			rsp_head->rsh = gvfs_palloc(upstream->pool, sizeof(gvfs_ns_file_upload_rsp_t));
			if (NULL == rsp_head->rsh) {
				return GVFS_ERROR;
			}

			rc = gvfs_ns_file_upload_process(r, rsp_head);
			if (GVFS_NS_OK != rc) {
				return GVFS_ERROR;
			}

			rc = gvfs_ns_file_upload_completed(r, rsp_head);
			if (GVFS_NS_OK != rc) {
				return GVFS_ERROR;
			}

			break;
		}

		case NS_FILE_DOWNLOAD:
		{
			rsp_head->rsh = gvfs_palloc(upstream->pool, sizeof(gvfs_ns_file_download_rsp_t));
			if (NULL == rsp_head->rsh) {
				return GVFS_ERROR;
			}

			rc = gvfs_ns_file_download_process(r, rsp_head);
			if (GVFS_NS_OK != rc) {
				return GVFS_ERROR;
			}

			rc = gvfs_ns_file_download_completed(r, rsp_head);
			if (GVFS_NS_OK != rc) {
				return GVFS_ERROR;
			}

			break;
		}

		case NS_FILE_DELETE:
		{
			rsp_head->rsh = gvfs_palloc(upstream->pool, sizeof(gvfs_ns_file_delete_rsp_t));
			if (NULL == rsp_head->rsh) {
				return GVFS_ERROR;
			}

			rc = gvfs_ns_file_delete_process(r, rsp_head);
			if (GVFS_NS_OK != rc) {
				return GVFS_ERROR;
			}

			rc = gvfs_ns_file_delete_completed(r, rsp_head);
			gvfs_close_upstream(r);
			if (GVFS_NS_OK != rc) {
				return GVFS_ERROR;
			}

			break;
		}

		default:
			return GVFS_ERROR;
	}

	return GVFS_OK;
}

