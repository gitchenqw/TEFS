
#include <gvfs_config.h>
#include <gvfs_core.h>
#include <gvfs_proxy.h>

#include <gvfs_http_query.h>
#include <gvfs_http_upload.h>
#include <gvfs_http_download.h>
#include <gvfs_http_delete.h>


void gvfs_http_init_connection(gvfs_connection_t *c)
{
	gvfs_event_t *rev;

	rev = c->read;
	
	rev->handler = gvfs_http_init_request;
	c->write->handler = NULL;
	
	if (rev->ready) {
		/* { 多进程模式,为了让其它进程快速获得accept锁,将事件延后处理 */
		if (gvfs_use_accept_mutex) {
			gvfs_locked_post_event(rev, &gvfs_posted_events);
			return;
		}
		/* } */

		/* { 单进程模式,为调试使用 */
		gvfs_http_init_request(rev);
		/* } */
	}
}

void gvfs_http_init_request(gvfs_event_t *rev)
{
	gvfs_http_request_t *r;
	gvfs_connection_t 	*c;
	gvfs_buf_t          *headers_in, *headers_out;
	socklen_t			 addrlen;
	struct sockaddr_in   addr;

	c = rev->data;

	r = gvfs_pcalloc(c->pool, sizeof(gvfs_http_request_t));
	if (r == NULL) {
		goto failed;
	}

	r->body_with_headers = 1;
	r->upstream_busy = 0;

	/* { HTTP请求头和响应头各分配1K的缓冲区 */
	headers_in = &r->headers_in;
	headers_in->size = HTTP_MAIN_CHUNK_SIZE;
	headers_in->start = gvfs_pcalloc(c->pool, HTTP_MAIN_CHUNK_SIZE);
	headers_in->end = headers_in->start + HTTP_MAIN_CHUNK_SIZE;
	headers_in->pos = headers_in->last = headers_in->start;

	headers_out = &r->headers_out;
	headers_out->size = HTTP_MAIN_CHUNK_SIZE;
	headers_out->start = gvfs_pcalloc(c->pool, HTTP_MAIN_CHUNK_SIZE);
    headers_out->end = headers_out->start + HTTP_MAIN_CHUNK_SIZE;
	headers_out->pos = headers_out->last = headers_out->start;
	/* } */

	/* { 分配和初始化HTTP协议解析器 */
	r->parser = gvfs_pcalloc(c->pool, sizeof(gvfs_http_parse_t));
	if (NULL == r->parser) {
		goto failed;
	}
	r->parser->data = r;
	gvfs_http_parse_init(r->parser, HTTP_REQUEST);
	/* } */

	/* { 分配和初始化upstream */
	r->upstream = gvfs_pcalloc(c->pool, sizeof(gvfs_upstream_t));
	if (NULL == r->upstream) {
		goto failed;
	}
	
	addrlen = sizeof(addr);
	r->upstream->peer.sockaddr = gvfs_palloc(c->pool, addrlen);
	if (NULL == r->upstream->peer.sockaddr) {
		goto failed;
	}

	/* 这里分配的内存保存ip:port的字符串形式,方便日志打印 */
	r->upstream->peer.addr_text.len = 32;
	r->upstream->peer.addr_text.data = gvfs_palloc(c->pool, 32);
	if (NULL == r->upstream->peer.addr_text.data) {
		goto failed;
	}
	/* } */
	
	c->data = r;
	r->connection = c;
	r->pool = c->pool;
	
	rev->handler = gvfs_http_process_request;

	rev->handler(rev);

	return;
	
failed:
	gvfs_http_close_connection(c);
	return;
}

void gvfs_http_reinit_request(gvfs_http_request_t *r)
{
	gvfs_buf_t        *headers;

	r->body_size = 0;
	r->recv_size = 0;
	r->send_size = 0;
	
	r->body_with_headers = 1;
	r->body_in_headers_len = 0;

	headers = &r->headers_in;
	headers->pos = headers->last = headers->start;
	gvfs_memzero(headers->start, HTTP_MAIN_CHUNK_SIZE);
	
	headers = &r->headers_out;
	headers->pos = headers->last = headers->start;

	gvfs_http_parse_init(r->parser, HTTP_REQUEST);
}

INT32 gvfs_http_request_handler(gvfs_http_request_t *r)
{
	gvfs_connection_t  *pc;
	CHAR               *rp;
	INT32               rc;
	UINT32              rp_len;
	INT32               code;

	pc = r->upstream->peer.connection;
	if (NULL != pc) {
		return GVFS_OK;
	}

	rp = r->request_path + 1; /* skip '/' */
	rp_len = r->request_path_len - 1;

	if (0 == rp_len) {
		return GVFS_ERROR;
	}
	
	if (0 == strncmp(rp, GVFS_QUERY, rp_len)){
		rc = gvfs_ns_query_request(r);
	}
	else if (0 == strncmp(rp, GVFS_UPLOAD, rp_len)) {
		rc = gvfs_ns_upload_request(r);
	}
	else if (0 == strncmp(rp, GVFS_DOWNLOAD, rp_len)) {
		rc = gvfs_ns_download_request(r);
	}
	else if (0 == strncmp(rp, GVFS_DELETE, rp_len)) {
		rc = gvfs_ns_delete_request(r);
	}
	else if (0 == strncmp(rp, GVFS_COS, rp_len)) {
		rc = gvfs_cos_request_init(r);
		if (GVFS_OK == rc) {
			r->upstream_busy = 1;
			code = HTTP_Ok;
		} else {
			code = HTTP_ServiceUnavailable;
		}
		gvfs_http_finalize_request(r, code);
	}
	else {
		return GVFS_ERROR;
	}

	if (GVFS_OK != rc) {
		return GVFS_ERROR;
	}

	return GVFS_OK;
}

void gvfs_http_process_request(gvfs_event_t *rev)
{
	gvfs_connection_t   *c, *pc;
	gvfs_http_request_t *r;
	gvfs_http_parse_t   *p;
	gvfs_buf_t          *headers_in;
	char                *header_end;
	INT32                code;
	ssize_t              n;
	size_t               len;

	c = rev->data;
	r = c->data;
	p = r->parser;
	
	code = HTTP_Ok;

	/* { 超时关闭连接,一般设置为2分钟 */
	if (rev->timedout) {
		gvfs_log(LOG_INFO, "client:%s fd:%d timeout",
							c->addr_text.data, c->fd);
							
		gvfs_http_close_request(r);
		return;
	}
	/* } */

	headers_in = &r->headers_in;
	gvfs_event_add_timer(rev, 120000);
	
	for ( ; ;) {
	
		n = gvfs_http_read_request_header(r);
		
		if (n == GVFS_AGAIN) {
			return;
		}
		else if (n == GVFS_ERROR) {
			code = HTTP_InternalServerError;
			goto to_client;
		} else if (n == 0) {
			gvfs_log(LOG_INFO, "client:%s fd:%d closed by peer",
							   c->addr_text.data, c->fd);
							   
			gvfs_http_close_request(r);
			return;
		}

		/* { 解析HTTP协议 */
		len = headers_in->last - headers_in->pos;
		n = gvfs_http_parse(p, &gvfs_http_parse_cb, (char *) headers_in->pos, len);
		if (len != n) {
			gvfs_log(LOG_ERROR, "bad request for client:%s fd:%d, "
								"name:%s desc:%s", c->addr_text.data, c->fd,
					 			gvfs_http_errno_name(p->http_errno),
					 			gvfs_http_errno_description(p->http_errno));
					 			
			gvfs_http_finalize_request(r, HTTP_BadRequest);
			gvfs_http_close_request(r);
			
			return;
		}

		headers_in->pos += n;
		/* } */
		
		if (r->headers_parse_completed) {
		
			header_end = strstr((char *) headers_in->start, "\r\n\r\n");
			if (NULL != header_end) {
				header_end += strlen("\r\n");
				*header_end = 0;
			}
			
			gvfs_log(LOG_INFO, "recv from:%s fd:%d\n%s",
								c->addr_text.data, c->fd, headers_in->start);

			r->body_with_headers = 0;
			r->body_size = p->body_size;

			/* 根据URL区分业务 */
			if (GVFS_OK != gvfs_http_request_handler(r)) {
				code = HTTP_Unauthorized;
				goto to_client;
			}
			
			if (0 == p->body_size) {
				continue;
			}
			
			/* { 有消息体,为消息体分配接收缓冲区 */
			r->request_body = gvfs_http_get_request_buf_body();
			if (NULL == r->request_body) {
				gvfs_log(LOG_INFO, "no buf for client:%s fd:%d",
								   c->addr_text.data, c->fd);
				code = HTTP_ServiceUnavailable;
				goto to_client;
			}
			/* } */
			
			/* { 将epoll模型改为水平触发,把边缘触发模式删除 */
			if (rev->active) {
				if (GVFS_OK != gvfs_epoll_del_event(rev, GVFS_READ_EVENT,
					GVFS_CLEAR_EVENT))
				{
					code = HTTP_InternalServerError;
					goto to_client;
				}
			}
			/* } */

			/* { TCP段中同时包含完整的HTTP消息头和部分消息体 */
			if (r->body_in_headers_len) {
				pc = r->upstream->peer.connection;
				if (NULL != pc) {
					gvfs_completed_send(pc, (UINT8 *) r->body_in_headers,
										r->body_in_headers_len);
				}
				
				if (r->body_size > r->body_in_headers_len) {
					r->body_size -= r->body_in_headers_len;
				}
				
				gvfs_log(LOG_INFO, "client:%s fd:%d body with headers:%u bytes",
									 c->addr_text.data, c->fd, 
									 r->body_in_headers_len);
			}
			/* } */
			
			rev->handler = gvfs_http_read_client_request_body_handler;
			rev->handler(rev);
		}
	}

to_client:
	gvfs_http_finalize_request(r, code);
	gvfs_http_free_request(r);
}

ssize_t gvfs_http_read_request_header(gvfs_http_request_t *r)
{
	gvfs_connection_t *c;
	gvfs_event_t      *rev;
	gvfs_buf_t        *headers_in;
	ssize_t            n;

	c = r->connection;
	rev = c->read;
	headers_in = &r->headers_in;
	
	if (rev->ready) {
		n = c->recv(c, headers_in->last, headers_in->end - headers_in->last);
	} else {
		n = GVFS_AGAIN;
	}

	if (n == GVFS_AGAIN) {
        if (!rev->active && !rev->ready) {
            if (gvfs_epoll_add_event(rev, GVFS_READ_EVENT, GVFS_CLEAR_EVENT)
                == GVFS_ERROR)
            {
                return GVFS_ERROR;
            }
        }
        return GVFS_AGAIN;
	}

	if (0 < n) {
		headers_in->last += n;
	}

	return n;
}

void gvfs_http_close_request(gvfs_http_request_t *r)
{

	gvfs_connection_t *c;

	c = r->connection;

	if (r->ffd) {
		close(r->ffd);
	}

	if (0 == r->upstream_busy) {
		gvfs_close_upstream(r);
	}
	
	gvfs_http_free_request(r);
	gvfs_http_close_connection(c);
}

void gvfs_http_free_request(gvfs_http_request_t *r)
{
	if (r->request_body) {
		gvfs_http_free_request_body_buf(r->request_body);
		r->request_body = NULL;
	}
}

void gvfs_http_close_connection(gvfs_connection_t *c)
{
    gvfs_pool_t  *pool;

    pool = c->pool;

    gvfs_close_connection(c);

    gvfs_destroy_pool(pool);
}

void gvfs_http_finalize_request(gvfs_http_request_t *r,
	gvfs_http_response_code code)
{
	gvfs_http_parse_t        *p;
	gvfs_connection_t        *c;
	u_char                   *data;
	gvfs_buf_t               *headers_out;
	char                      body[HTTP_MAIN_CHUNK_SIZE];
	size_t                    len, size;

	c = r->connection;
	p = r->parser;

	headers_out = &r->headers_out;

	/* { 如果在NS阶段已经封包则直接发送 */
	if (headers_out->last > headers_out->pos) {
		goto send;
	}
	/* } */

	size = snprintf(body, HTTP_MAIN_CHUNK_SIZE, 
					"<html>\r\n"
					    "<head>\r\n"
					    "<title>Welcome to gvfs</title>\r\n"
					    "</head>\r\n"
					    
					    "<body bgcolor=\"white\" text=\"black\">\r\n"
					       "<center><h1>%s!</h1></center>\r\n"
					    "</body>\r\n"
					"</html>\r\n",
					gvfs_http_reason_phrase(code));

	
	gvfs_http_response_headers(headers_out, code, size);

	len = headers_out->end - headers_out->last;
	size = snprintf((CHAR *) headers_out->last, len,
 				      "%s", body);
 				      
 	headers_out->last += size;

send:
	data = headers_out->pos;
	size = headers_out->last - headers_out->pos;
	
	gvfs_completed_send(c, data, size);

	gvfs_log(LOG_INFO, "send to client:%s fd:%d \n%s",
						c->addr_text.data, c->fd, data);

	gvfs_http_reinit_request(r);
	
	return;
}

