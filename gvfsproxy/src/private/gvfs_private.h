

#ifndef _GVFS_PRIVATE_H_INCLUDE_
#define _GVFS_PRIVATE_H_INCLUDE_


#include <gvfs_config.h>
#include <gvfs_core.h>

typedef struct gvfs_upstream_s        gvfs_upstream_t;
typedef struct gvfs_rsp_head_s        gvfs_rsp_head_t;


#include <gvfs_ns_interface.h>
#include <gvfs_ds_interface.h>

#include <gvfs_upstream.h>

#include <gvfs_http_query.h>
#include <gvfs_http_upload.h>
#include <gvfs_http_download.h>
#include <gvfs_http_delete.h>


#define GVFS_QUERY    "storage/v1/query"
#define GVFS_UPLOAD   "storage/v1/upload"
#define GVFS_DOWNLOAD "storage/v1/download"
#define GVFS_DELETE   "storage/v1/delete"

#define GVFS_COS      "cos"


gvfs_int_t gvfs_upstream_create(gvfs_http_request_t *r,
	const CHAR *ip_addr, INT32 port);

gvfs_int_t gvfs_upstream_init(gvfs_http_request_t *r,
	const CHAR *ip_addr, INT32 port);

gvfs_int_t gvfs_upstream_connect(gvfs_http_request_t *r,
    gvfs_upstream_t *upstream);
    
void gvfs_free_upstream(gvfs_upstream_t *upstream);

void gvfs_close_upstream(gvfs_http_request_t *r);

VOID gvfs_upstream_to_http_response(gvfs_http_request_t *r,
	gvfs_http_response_code code, CHAR *http_body);
	
#endif
