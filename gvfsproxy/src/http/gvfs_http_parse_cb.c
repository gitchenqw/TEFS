

#include <gvfs_config.h>
#include <gvfs_core.h>

static int gvfs_http_on_message_begin(gvfs_http_parse_t *http_p);
static int gvfs_http_on_url(gvfs_http_parse_t *http_p, const char *at,
	size_t length);
static int gvfs_http_on_status(gvfs_http_parse_t *http_p, const char *at,
	size_t length);
static int gvfs_http_on_headers_complete(gvfs_http_parse_t *http_p);
static int gvfs_http_on_body(gvfs_http_parse_t *http_p,
	const char *at, size_t length);
static int gvfs_http_on_message_complete(gvfs_http_parse_t *http_p);

/* 所有回调接口,返回值为0表示成功,返回值非0表示失败 */
gvfs_http_parse_cb_t gvfs_http_parse_cb = {
	.on_message_begin    = gvfs_http_on_message_begin,
	.on_url              = gvfs_http_on_url,           
	.on_status           = gvfs_http_on_status,
	.on_header_field     = NULL,     
	.on_header_value     = NULL,    
	.on_headers_complete = gvfs_http_on_headers_complete,
	.on_body			 = gvfs_http_on_body,             
	.on_message_complete = gvfs_http_on_message_complete
};

static int
gvfs_http_on_message_begin(gvfs_http_parse_t *http_p)
{
	gvfs_http_request_t *http_r;

	http_r = http_p->data;
	http_r->begin_parsed = 1;
	
	return 0;
}

static int
gvfs_http_on_url(gvfs_http_parse_t *http_p, const char *at,
	size_t length)
{
	gvfs_http_request_t  *r;
	gvfs_http_parse_url_t url;

	r = http_p->data;

	r->request_url = (char *) at;
	r->request_url_len  = length;
	
	gvfs_http_parse_url(r->request_url, r->request_url_len, 0, &url);
	
	gvfs_http_url_dump(r->request_url, &url, &r->query_string,
					   &r->query_string_len, UF_QUERY);
	gvfs_http_url_dump(r->request_url, &url, &r->request_path,
					   &r->request_path_len, UF_PATH);

	return 0;
}

static int
gvfs_http_on_status(gvfs_http_parse_t *http_p, const char *at,
	size_t length)
{
	return 0;
}

static int
gvfs_http_on_headers_complete(gvfs_http_parse_t *http_p)
{
	gvfs_http_request_t *http_r;

	http_r = http_p->data;

	http_r->method = http_p->method;
	http_r->headers_parse_completed = 1;
	http_r->should_keep_alive = gvfs_http_should_keep_alive(http_p);
	
	return 0;
}


static int
gvfs_http_on_body(gvfs_http_parse_t *http_p,
	const char *at, size_t length)
{
	gvfs_http_request_t *r;

	r = http_p->data;
	
	/* 处理消息体部分 */
	if (r->body_with_headers) {
		r->body_in_headers = (char *) at;
		r->body_in_headers_len = length;
	}
	
	r->body_is_final = gvfs_http_body_is_final(http_p);
	return 0;
}


static int
gvfs_http_on_message_complete(gvfs_http_parse_t *http_p)
{
	gvfs_http_request_t *http_r;

	http_r = http_p->data;

	http_r->message_parse_completed = 1;
	return 0;
}

