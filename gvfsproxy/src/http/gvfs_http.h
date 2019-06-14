

#ifndef _GVFS_HTTP_H_INCLUDE_
#define _GVFS_HTTP_H_INCLUDE_


#include <gvfs_config.h>
#include <gvfs_core.h>

typedef struct gvfs_buf_s              gvfs_buf_t;
typedef struct gvfs_ipv4_addr_s        gvfs_ipv4_addr_t;
typedef struct gvfs_http_request_s     gvfs_http_request_t;
typedef enum gvfs_http_response_code_e gvfs_http_response_code;


void gvfs_http_init_connection(gvfs_connection_t *c);
void gvfs_http_init_request(gvfs_event_t *rev);
void gvfs_http_reinit_request(gvfs_http_request_t *r);
void gvfs_http_process_request(gvfs_event_t *rev);
ssize_t gvfs_http_read_request_header(gvfs_http_request_t *r);

void gvfs_http_read_client_request_body_handler(gvfs_event_t *r);

void gvfs_http_finalize_request(gvfs_http_request_t *r,
	gvfs_http_response_code code);

void gvfs_http_close_request(gvfs_http_request_t *r);
void gvfs_http_free_request(gvfs_http_request_t *r);
void gvfs_http_close_connection(gvfs_connection_t *c);



#endif
