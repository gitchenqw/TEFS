
#ifndef _GVFS_COS_H_INCLUDE_
#define _GVFS_COS_H_INCLUDE_

#include <gvfs_config.h>
#include <gvfs_core.h>

typedef struct gvfs_cos_request_s {
	gvfs_pool_t *pool;
	gvfs_http_request_t *request;
	gvfs_connection_t *connection; /* 腾讯COS的连接 */
	gvfs_upstream_t *upstream; /* 继承自gvfs_http_request_t中的upstream */
} gvfs_cos_request_t;

int gvfs_cos_request_init(gvfs_http_request_t *r);
int gvfs_cos_request_connect(const char *cos_domain);

void gvfs_cos_http_init_response(gvfs_event_t *rev);
void gvfs_cos_http_reinit_response(gvfs_http_request_t *r);
void gvfs_cos_http_process_response(gvfs_event_t *rev);
ssize_t gvfs_cos_http_read_response_header(gvfs_http_request_t *r);

void gvfs_cos_http_finalize_response(gvfs_http_request_t *r,
	gvfs_http_response_code code);

void
gvfs_cos_http_read_client_request_body_handler(gvfs_event_t *rev);

void gvfs_cos_http_close_response(gvfs_http_request_t *r);
void gvfs_cos_http_free_response(gvfs_http_request_t *r);
void gvfs_cos_http_close_connection(gvfs_connection_t *c);

#endif /* _GVFS_COS_H_INCLUDE_ */
