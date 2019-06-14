
#ifndef _GVFS_STATUS_H_INCLUDE_
#define _GVFS_STATUS_H_INCLUDE_

#include <gvfs_config.h>
#include <gvfs_core.h>

typedef enum gvfs_http_request_status_e
{
    GVFS_UPSTREAM_NS_DONE = 0x00000001,
    GVFS_UPSTREAM_DS_DONE = 0x00000010,
    GVFS_UPSTREAM_ERROR   = 0x00000100, 
} gvfs_http_request_status_t;

#define gvfs_set_status(status, open, value)\
    ((status) = ((status) & (~(value))) | (!!(open) ? (value) : 0))

#define gvfs_set_upstream_ns_done(status, open)\
    gvfs_set_status(status, open, GVFS_UPSTREAM_NS_DONE)
#define gvfs_set_upstream_ds_done(status, open)\
	gvfs_set_status(status, open, GVFS_UPSTREAM_DS_DONE)
#define gvfs_set_upstream_error(status, open)\
    gvfs_set_status(status, open, GVFS_UPSTREAM_ERROR)


#define gvfs_get_status(status, value) (!!((status) & (value)))

#define gvfs_get_upstream_ns_done(status)\
	gvfs_get_status(status, GVFS_UPSTREAM_NS_DONE)
#define gvfs_get_upstream_ds_done(status)\
	gvfs_get_status(status, GVFS_UPSTREAM_DS_DONE)
	
#define gvfs_get_upstream_error(status)\
	gvfs_get_status(status, GVFS_UPSTREAM_ERROR)

#endif
