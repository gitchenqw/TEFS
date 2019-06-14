

#ifndef _GVFS_HTTP_CORE_MODULE_H_INCLUDE_
#define _GVFS_HTTP_CORE_MODULE_H_INCLUDE_


#include <gvfs_config.h>
#include <gvfs_core.h>


typedef struct {
	gvfs_uint_t    port;
	CHAR*          ip_addr;
	
	gvfs_uint_t    chunk_num;
	gvfs_uint_t    chunk_size;

	gvfs_uint_t    timeout;
} gvfs_http_core_conf_t;


extern gvfs_module_t gvfs_http_core_module;
extern gvfs_http_core_conf_t *g_gvfs_hccf;


#endif /* _GVFS_HTTP_CORE_MODULE_H_INCLUDE_ */
