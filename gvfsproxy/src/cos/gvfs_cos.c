
#include <gvfs_config.h>
#include <gvfs_core.h>

#define GVFS_COS_MEM_POOL_SIZE 4096

int gvfs_cos_request_connect(const char *cos_domain);

static INT32 gvfs_cos_uri_parse(CHAR *input, CHAR *uri,
	INT32 uri_len)
{
	INT32 icnt               = 0;
	CHAR  *pfilehash         = NULL;
	static CHAR *sfilehash   = "uri=";

	pfilehash   = strstr(input, sfilehash);
	
	if (NULL == pfilehash) {
		return GVFS_ERROR;
	}

	icnt = 0;
	pfilehash += strlen(sfilehash);
	while ('\0' != *pfilehash && icnt < uri_len) {
		snprintf(uri + icnt, 2, "%s", pfilehash);
		pfilehash++;
		icnt++;
	}

	return GVFS_OK;
}

int gvfs_cos_request_init(gvfs_http_request_t *r)
{
	int fd, rc;
	size_t size;
	gvfs_pool_t *pool;
	gvfs_cos_request_t *cos_request;
	gvfs_connection_t *c;
	gvfs_event_t *rev;
	char *addr_text;
	char *domain_start;
	char *domain_end;

	char cos_domain[128];
	char cos_request_buf[1024];
	
	char query_str[512] = {0};
	char cos_uri[1024] = {0};

	
	snprintf(query_str, r->query_string_len + 1, "%s", r->query_string);
	rc = gvfs_cos_uri_parse(query_str, cos_uri, 1024);
	if (GVFS_OK != rc) {
		gvfs_log(LOG_ERROR, "gvfs_cos_uri_parse(\"%s\") failed",
				 query_str);
		return GVFS_ERROR;
	}

	/* 从uri中解析出腾讯cos的域名 */
	domain_start = strstr(cos_uri, "http://");
	if (NULL == domain_start) {
		gvfs_log(LOG_ERROR, "cos uri:%s no 'http://' error", cos_uri);
		return GVFS_ERROR;
	}

	domain_start += strlen("http://");
	domain_end = strstr(domain_start, "/");
	if (NULL == domain_start) {
		gvfs_log(LOG_ERROR, "cos uri:%s no '/' error", domain_start);
		return GVFS_ERROR;
	}
	
	/* 截取uri中腾讯cos的域名 */
	size = domain_end - domain_start + 1;
	snprintf(cos_domain, size, "%s", domain_start);
	
	fd = gvfs_cos_request_connect(cos_domain);
	if (-1 == fd) {
		gvfs_log(LOG_ERROR, "gvfs_cos_request_connect(\"%s\") failed",
				 cos_domain);
		return GVFS_ERROR;
	}

	c = gvfs_get_connection(fd);
	if (NULL == c) {
		gvfs_close_socket(fd);
		gvfs_log(LOG_ERROR, "gvfs_get_connection(%d) failed", fd);
		return GVFS_ERROR;
	}

	pool = gvfs_create_pool(GVFS_COS_MEM_POOL_SIZE);
	if (NULL == pool) {
		gvfs_close_socket(fd);
		gvfs_log(LOG_ERROR, "gvfs_create_pool(%d) for fd:%d failed",
				 GVFS_COS_MEM_POOL_SIZE, fd);
		return GVFS_ERROR;
	}

	addr_text = gvfs_palloc(pool, size);
	snprintf(addr_text, size, "%s", domain_start);

	c->addr_text.len = size;
	c->addr_text.data = (u_char *) addr_text;

	cos_request = gvfs_pcalloc(pool, sizeof(gvfs_cos_request_t));
	if (NULL == cos_request) {
		gvfs_log(LOG_ERROR, "gvfs_pcalloc(%p, %lu) for fd:%d failed", pool,
				 sizeof(gvfs_cos_request_t), fd);
				 
		gvfs_close_socket(fd);
		gvfs_destroy_pool(pool);
		return GVFS_ERROR;
	}
	
	c->pool = pool;
	c->data = cos_request;
	cos_request->pool = pool;
	cos_request->connection = c;

	/* 发送http下载请求到cos,把事件添加到epool中 */
	size = snprintf(cos_request_buf, 1024,
		"GET %s HTTP/1.0\r\n"
		"User-Agent: gvfs_proxy/1.0.0\r\n"
		"Accept: */*\r\n"
		"Host: %s\r\n"
		"Connection: Keep-Alive\r\n\r\n",
		domain_end, cos_domain);

	c->recv = gvfs_unix_recv;
	c->send = gvfs_unix_send;
	if (GVFS_OK != gvfs_completed_send(c, (u_char *) cos_request_buf, size)) {
		gvfs_log(LOG_ERROR, "send to cos:%s fd:%d failed", cos_domain, fd);

		gvfs_close_socket(fd);
		gvfs_destroy_pool(pool);
		return GVFS_ERROR;
	}
	
	gvfs_log(LOG_INFO, "send to cos:%s fd:%d\n%s", cos_domain, c->fd,
			 cos_request_buf);

	
	cos_request->upstream = gvfs_pcalloc(pool, sizeof(gvfs_upstream_t));
	if (NULL == cos_request->upstream) {
		gvfs_close_socket(fd);
		gvfs_destroy_pool(pool);
		return GVFS_ERROR;
	}

	
	cos_request->upstream->peer.addr_text.len = 32;
	cos_request->upstream->peer.addr_text.data = gvfs_palloc(c->pool, 32);
	if (NULL == r->upstream->peer.addr_text.data) {
		gvfs_close_socket(fd);
		gvfs_destroy_pool(pool);
		return GVFS_ERROR;
	}

	gvfs_memcpy(cos_request->upstream->peer.addr_text.data,
		r->upstream->peer.addr_text.data, 32);
	gvfs_memcpy(cos_request->upstream, r->upstream, sizeof(gvfs_upstream_t));

	/* 添加到epool中进行处理,使用水平触发方式 */
	rev = c->read;
	rev->handler = gvfs_cos_http_init_response;
	if (!rev->active) {
		if (GVFS_OK != gvfs_epoll_add_event(rev, GVFS_READ_EVENT,
			GVFS_LEVEL_EVENT))
		{
			gvfs_close_socket(fd);
			gvfs_destroy_pool(pool);
			return GVFS_ERROR;
		}
	}

	return GVFS_OK;
}

int gvfs_cos_request_connect(const char *cos_domain)
{
	int sockfd = -1;
	char port[16] = {0};
    struct addrinfo *res = NULL, *cos_addr = NULL;
    struct addrinfo hints;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_flags = AI_CANONNAME;
	hints.ai_family = AF_UNSPEC;   
	hints.ai_socktype = SOCK_STREAM;
	
	snprintf(port, sizeof(port), "%d", 80);
	if (0 != getaddrinfo(cos_domain, port, &hints, &cos_addr)) {
		gvfs_log(LOG_ERROR, "getaddrinfo(\"%s\", %s) failed, error:%s",
				 cos_domain, port, gvfs_strerror(errno));
    	return sockfd;
    }
	
    res = cos_addr;
    do {
        sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
        if (0 > sockfd) {
            continue;
        }
        
        if (0 == connect(sockfd, res->ai_addr, res->ai_addrlen)) {
            break;
        }

        close(sockfd);
    } while((res = res->ai_next) != NULL);

	freeaddrinfo(cos_addr);

	return sockfd;
}

