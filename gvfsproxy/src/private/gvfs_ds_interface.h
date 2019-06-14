

#ifndef _GVFS_DS_INTERFACE_H_INCLUDE_
#define _GVFS_DS_INTERFACE_H_INCLUDE_


#include <gvfs_config.h>
#include <gvfs_core.h>


#define DS_BLOCK_UPLOAD         0x0101
#define DS_BLOCK_DOWNLOAD       0x0102


typedef struct gvfs_ds_req_head_s
{
    unsigned int  len;
    unsigned int  id;
    unsigned int  type;
    unsigned int  encrypt;
} gvfs_ds_req_head_t;

typedef struct gvfs_ds_rsp_head_s
{
    unsigned int  len;
    unsigned int  id;
    unsigned int  type;
    unsigned int  encrypt;
    unsigned int  rtn;
} gvfs_ds_rsp_head_t;


VOID gvfs_ds_process_respodse_header(gvfs_event_t *rev);


#endif /* _GVFS_NS_INTERFACE_H_INCLUDE_ */

