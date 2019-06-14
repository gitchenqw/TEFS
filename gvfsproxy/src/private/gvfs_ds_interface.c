

#include <gvfs_config.h>
#include <gvfs_core.h>

#include <gvfs_ds_interface.h>
#include <gvfs_upstream.h>
#include <gvfs_ds_file_upload.h>
#include <gvfs_ds_file_download.h>


static INT32 gvfs_ds_process(gvfs_upstream_t *upstream);

static VOID gvfs_ds_process_respodse_body(gvfs_event_t *rev);


static gvfs_int_t
gvfs_ds_write_download_data(gvfs_http_request_t *r)
{
	gvfs_connection_t  *c;
	gvfs_upstream_t    *upstream;
	gvfs_buf_t         *request_body;
	size_t              size;
	ssize_t             n;

	upstream = r->upstream;
	c = r->connection;

	request_body = upstream->download_body;

	size = request_body->last - request_body->pos;
	
	n = gvfs_completed_send(c, request_body->pos, size);
	if (GVFS_OK == n) {
		return GVFS_OK;
	}

	gvfs_log(LOG_ERROR, "send to client:%s fd:%d size:%lu failed",
						r->connection->addr_text.data, r->connection->fd,
						size);
						
	return GVFS_ERROR;
}


ssize_t gvfs_ds_read_respodse_header(gvfs_connection_t *c)
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

	size = sizeof(gvfs_ds_rsp_head_t);
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

	rsp_head->encrypt = gvfs_rb32(p);
	p += sizeof(UINT32);
	
	rsp_head->rtn = gvfs_rb32(p);
	
	gvfs_log(LOG_DEBUG, "recv from ds:%s fd:%d type:%02x len:%u id:%u rtn:%u",
			 c->addr_text.data, c->fd, rsp_head->type, rsp_head->len,
			 rsp_head->id, rsp_head->rtn);
						
	return n;
}


VOID gvfs_ds_process_respodse_header(gvfs_event_t *rev)
{
	gvfs_connection_t        *c;
	gvfs_upstream_t          *upstream;
	gvfs_http_request_t      *r;
	gvfs_peer_connection_t   *pc;
	gvfs_rsp_head_t          *rsp_head;	
	ssize_t                   n;
	gvfs_http_response_code   code;
	
	c = rev->data;
	upstream = c->data;
	pc = &upstream->peer;
	r = pc->data;

	code = HTTP_Ok;
	if (rev->timedout) {
		gvfs_log(LOG_DEBUG, "wait ds:%s fd:%d for cliend:%s fd:%d sid:%u timeout",
				 pc->addr_text.data, c->fd, r->connection->addr_text.data,
				 r->connection->fd, upstream->sid);
		code = HTTP_NotAcceptable;
		goto end;
	}

	if (rev->timer_set) {
		gvfs_event_del_timer(rev);
	}
	
	for ( ; ; ) {
		n = gvfs_ds_read_respodse_header(c);

		if (GVFS_AGAIN == n) {
			return;
		}
		else if (GVFS_ERROR == n) {
			gvfs_log(LOG_ERROR, "read ds:%s for client:%s fd:%d sid:%u error",
					 pc->addr_text.data, r->connection->addr_text.data, c->fd,
					 upstream->sid);
			code = HTTP_InternalServerError;
			goto end;
		}
		else if (0 == n) {
			gvfs_log(LOG_INFO, "ds:%s close connection for client:%s fd:%d "
								"sid:%u", pc->addr_text.data,
								r->connection->addr_text.data,
								c->fd, upstream->sid);
			code = HTTP_NotAcceptable;				   
			goto end;
		}

		if (upstream->header_completed) {
		
			rsp_head = &upstream->rsp_head;

			if (DS_BLOCK_UPLOAD == rsp_head->type) {
				gvfs_ds_process(upstream);
				goto end;
			}

			else if (DS_BLOCK_DOWNLOAD == rsp_head->type) {
			
				r->body_size = r->end - r->start;
				
				gvfs_ds_process(upstream);
				
				upstream->download_body = gvfs_http_get_request_buf_body();
				if (NULL == upstream->download_body) {
				
					code = HTTP_ServiceUnavailable;
					
					r->headers_out.pos = r->headers_out.last = 
										  r->headers_out.start;
										  
					gvfs_log(LOG_INFO, "no buf for client:%s fd:%d",
									   r->connection->addr_text.data,
									   r->connection->fd);
									   
					goto end;
				}
				
				gvfs_http_finalize_request(r, code);

				/* { 将epoll模型改为水平触发,把边缘触发模式删除 */
				if (rev->active) {
					if (GVFS_OK != gvfs_epoll_del_event(rev, GVFS_READ_EVENT,
						GVFS_CLEAR_EVENT))
					{
						code = HTTP_InternalServerError;
						goto end;
					}
				}
				/* } */
				r->body_size = r->end - r->start;
				rev->handler = gvfs_ds_process_respodse_body;
				rev->handler(rev);
				return;
			}
			else {
				code = HTTP_NotImplemented;
				goto end;
			}
			
		}
	}

end:
	gvfs_http_finalize_request(r, code);
	gvfs_close_upstream(r);
	return;
}


ssize_t gvfs_ds_read_respodse_body(gvfs_connection_t *c)
{
	gvfs_event_t             *rev;
	gvfs_upstream_t          *upstream;
	gvfs_buf_t               *buf;
	gvfs_rsp_head_t          *rsp_head;
	gvfs_http_request_t      *r;
	gvfs_peer_connection_t   *pc;
	
	ssize_t                   n;

	rev = c->read;
	upstream = c->data;
	rsp_head = &upstream->rsp_head;
	buf = upstream->download_body;

	pc = &upstream->peer;
	r = pc->data;

	if (rev->ready) {
		n = c->recv(c, buf->last, buf->end - buf->last);
	} else {
		n = GVFS_AGAIN;
	}

	if (n == GVFS_AGAIN) {
        if (!rev->active && !rev->ready) {
            if (gvfs_epoll_add_event(rev, GVFS_READ_EVENT, GVFS_LEVEL_EVENT)
                == GVFS_ERROR)
            {
                return GVFS_ERROR;
            }
        }
        return GVFS_AGAIN;
	}
	
	if (0 < n) {
		buf->last += n;
	}
	
	return n;
}


static VOID gvfs_ds_process_respodse_body(gvfs_event_t *rev)
{
	gvfs_connection_t        *c;
	gvfs_upstream_t          *upstream;
	gvfs_http_request_t      *r;
	gvfs_peer_connection_t   *pc;
	gvfs_buf_t               *buf;
	ssize_t                   n;
	gvfs_http_response_code   code;
	
	c = rev->data;
	upstream = c->data;
	pc = &upstream->peer;
	r = pc->data;

	code = HTTP_Ok;
	buf = upstream->download_body;
	
	for ( ; ; ) {
	
		if (buf->last == buf->end) {
		
			gvfs_ds_write_download_data(r);
			
			buf->pos = buf->last = buf->start;
		}
		
		n = gvfs_ds_read_respodse_body(c);
		
		if (GVFS_AGAIN == n) {
			return;
		}
		else if (GVFS_ERROR == n) {
			gvfs_log(LOG_ERROR, "read ds for client:%s fd:%d sid:%u error",
					 			r->connection->addr_text.data, c->fd,
					 			upstream->sid);
			goto end;
		}
		else if (0 == n) {
			gvfs_log(LOG_INFO, "ds:%s fd:%d for client:%s fd:%d sid:%u close "
							   "connection", pc->addr_text.data,
							   pc->connection->fd,
							   r->connection->addr_text.data, c->fd,
							   upstream->sid);
			goto end;
		}
		
		r->recv_size += n;

		/* 根据http body中Tencent COS的URI到Tencent COS去取数据 */

		if (r->body_size == r->recv_size) {
		
			gvfs_ds_write_download_data(r);
			
			gvfs_log(LOG_INFO, "recv data from ds:%s fd:%d size:%lu done",
								pc->addr_text.data, pc->connection->fd,
								r->recv_size);
								
			goto end;
		}
	}

end:
	if (!(r->connection->read->timer_set)) {
		gvfs_event_add_timer(r->connection->read, 120000);
	}
	
	gvfs_close_upstream(r);
	
	return;
}


static INT32 gvfs_ds_process(gvfs_upstream_t *upstream)
{
	gvfs_rsp_head_t     *rsp_head;
	gvfs_http_request_t *r;
	UINT32               type;

	rsp_head = &upstream->rsp_head;
	type = rsp_head->type;
	
	r = upstream->peer.data;
	
	switch (type) {
		case DS_BLOCK_UPLOAD:
		{
			gvfs_ds_file_upload_completed(r, rsp_head);
			break;
		}

		case DS_BLOCK_DOWNLOAD:
		{
			gvfs_ds_file_download_completed(r, rsp_head);
			break;
		}

		default:
			return GVFS_ERROR;
	}
	
	return GVFS_OK;
}

