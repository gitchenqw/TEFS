

#include <gvfs_config.h>
#include <gvfs_core.h>

#include <gvfs_upstream.h>

static gvfs_int_t
gvfs_cos_http_do_read_client_request_body(gvfs_http_request_t *r);

static gvfs_int_t
gvfs_cos_http_write_request_body(gvfs_http_request_t *r);

static gvfs_int_t
gvfs_cos_http_write_request_body_to_file(gvfs_http_request_t *r);


ssize_t gvfs_cos_http_read_request_body(gvfs_http_request_t *r)
{
	ssize_t            n;
	gvfs_connection_t *c;
	gvfs_event_t      *rev;
	gvfs_buf_t        *buf;
	
	c = r->connection;
	rev = c->read;
	buf = r->request_body;

	/* 删除rev时,rev->active置0,recv到EAGIN时,rev->ready置0 */
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

void
gvfs_cos_http_read_client_request_body_handler(gvfs_event_t *rev)
{
	gvfs_connection_t   *c;
	gvfs_http_request_t *r;
	gvfs_cos_request_t  *cos_r;
	
	c = rev->data;
	cos_r = c->data;
	r = cos_r->request;
	
	if (rev->timedout) {
		gvfs_log(LOG_DEBUG, "client:%s fd:%d timeout",
							c->addr_text.data, c->fd);
		gvfs_http_close_request(r);
		return;
	}

	/* 每次有数据到来就更新链接的超时时间为当前时间2min之后 */
	gvfs_event_add_timer(rev, 120000);
		
	gvfs_cos_http_do_read_client_request_body(r);

	return;
}

static gvfs_int_t
gvfs_cos_http_do_read_client_request_body(gvfs_http_request_t *r)
{
	INT32				code;
	ssize_t 			n;
	gvfs_connection_t  *c, *pc;
	gvfs_http_parse_t  *p;
	gvfs_buf_t		   *request_body;

	c = r->connection;
	p = r->parser;
	
	code = HTTP_Ok;
	request_body = r->request_body;

	for ( ; ;) {
	
		if (request_body->last == request_body->end) {

			pc = r->upstream->peer.connection;
			
			if (NULL != pc) {
				gvfs_cos_http_write_request_body(r);
			}
			else {
				gvfs_cos_http_write_request_body_to_file(r);
			}
			
			request_body->pos = request_body->last = request_body->start;
		}

		n = gvfs_cos_http_read_request_body(r);
		
		if (GVFS_AGAIN == n) {
			return GVFS_OK;
		}
		else if (GVFS_ERROR == n) {
			code = HTTP_InternalServerError;
			goto to_client;
		}
		else if (0 == n) {
			gvfs_log(LOG_INFO, "client: %s fd:%d closed by peer",
							   c->addr_text.data, c->fd);
							   
			gvfs_http_close_request(r);
			return GVFS_OK;
		}

		r->recv_size += n;

		if (r->body_size == r->recv_size) {

			pc = r->upstream->peer.connection;
			
			if (NULL != pc) {
				gvfs_event_add_timer(pc->read, 30000);
				gvfs_cos_http_write_request_body(r);
			}
			else {
				gvfs_cos_http_write_request_body_to_file(r);
			}

			gvfs_log(LOG_INFO, "recv for client:%s fd:%d size:%lu done",
								c->addr_text.data, c->fd, r->send_size);
			return GVFS_OK;
		}
	}

to_client:
	gvfs_http_finalize_request(r, code);
	return GVFS_OK;
}

static gvfs_int_t
gvfs_cos_http_write_request_body(gvfs_http_request_t *r)
{
	gvfs_connection_t  *pc;
	gvfs_upstream_t    *upstream;
	gvfs_buf_t         *request_body;
	size_t              size;
	ssize_t             n;

	upstream = r->upstream;
	pc = upstream->peer.connection;
	request_body = r->request_body;

	size = request_body->last - request_body->pos;

	n = gvfs_completed_send(pc, request_body->pos, size);
	if (GVFS_OK == n) {
		r->send_size += size;
		return GVFS_OK;
	}

	gvfs_log(LOG_ERROR, "send to ds:%s fd:%d for client:%s fd:%d size:%lu failed",
						upstream->peer.addr_text.data,pc->fd,
						r->connection->addr_text.data, r->connection->fd,
						size);
						
	return GVFS_ERROR;
}

static gvfs_int_t
gvfs_cos_http_write_request_body_to_file(gvfs_http_request_t *r)
{
	UINT8      *data;
	gvfs_buf_t *request_body;
	size_t      size, nsize;
	ssize_t     n;
	
	if (0 == r->ffd) {
		r->ffd = open(r->file_name, O_WRONLY | O_CREAT, 0755);
		if (0 > r->ffd) {
			gvfs_log(LOG_ERROR, "open(\"%s\") for client:%s fd:%d failed, "
								"error: %s",
								r->file_name, r->connection->addr_text.data,
								r->connection->fd, gvfs_strerror(errno));
			return GVFS_ERROR;
		}
		
		gvfs_log(LOG_DEBUG, "open(\"%s\") ffd:%d for client:%s fd:%d success",
							r->file_name, r->ffd, r->connection->addr_text.data,
							r->connection->fd);
	}

	request_body = r->request_body;

	size = nsize = request_body->last - request_body->pos;
	data = request_body->start;

	while (0 != size && 0 != (n = write(r->ffd, data, size)))
	{
		if (-1 == n) {
			if (EINTR == errno) {
				continue;
			}
			gvfs_log(LOG_ERROR, "write(\"%s\") size:%lu for client:%s fd:%d "
								"failed, error:%s",
								r->file_name, nsize,
								r->connection->addr_text.data,
								r->connection->fd, gvfs_strerror(errno));
			return GVFS_ERROR;
		}
		
		if (n > 0) {
			r->send_size += n;
		}
		
		data += n;
		size -= n;
	}
	
	return GVFS_OK;
}
/* } */
