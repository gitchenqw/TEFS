

#ifndef _GVFS_NS_FILE_QUERY_H_INCLUDE_
#define _GVFS_NS_FILE_QUERY_H_INCLUDE_


#include <gvfs_config.h>
#include <gvfs_core.h>

#include <gvfs_ns_interface.h>

INT32 gvfs_ns_file_query_process(gvfs_http_request_t *r,
	gvfs_rsp_head_t *rsp_head);
	
INT32 gvfs_ns_file_query_completed(gvfs_http_request_t *r,
	gvfs_rsp_head_t *rsp_head);

#endif
