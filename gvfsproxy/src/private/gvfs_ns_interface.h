

#ifndef _GVFS_NS_INTERFACE_H_INCLUDE_
#define _GVFS_NS_INTERFACE_H_INCLUDE_


#include <gvfs_config.h>
#include <gvfs_core.h>

#define GVFS_NS_OK        0
#define GVFS_NS_ERROR     1

#define NS_FILE_UPLOAD           0x0002
#define NS_FILE_DOWNLOAD         0x0003
#define NS_FILE_DELETE           0x0004
#define NS_FILE_QUERY            0X000a

#define NS_MEM_POOL_SIZE         4096
#define NS_MAX_MSG_SIZE          16384


typedef struct gvfs_ns_req_head_s
{
    unsigned int  len;
    unsigned int  id;
    unsigned int  type;
} gvfs_ns_req_head_t;


typedef struct gvfs_ns_rsp_head_s
{
    unsigned int  len;
    unsigned int  id;
    unsigned int  type;
    unsigned int  rtn;
} gvfs_ns_rsp_head_t;


typedef struct gvfs_ns_interface_s {
	gvfs_pool_t           *pool;
	
	gvfs_ns_rsp_head_t    *rsh;
	UINT8                 *data;
	
	UINT32                 len;
	UINT32                 id;
	UINT32                 type;
	UINT32                 rtn;
} gvfs_ns_interface_t;


typedef struct gvfs_ns_file_upload_rsp_s {
	gvfs_ns_rsp_head_t head;
	
	UINT32	            encrypt; 	 /* 0:未加密 1:加密 */
	UINT64              file_id;     /* 文件ID */
	UINT32              ds_number;   /* DS服务器个数 */
	gvfs_ipv4_addr_t   *ds_addr;
		
	unsigned int	    separate;	
} gvfs_ns_file_upload_rsp_t;


typedef struct gvfs_ds_block_info_s {
	UINT32              block_index;
	UINT32              port;
	UINT8              *ip;
} gvfs_ds_block_info_t;

typedef struct gvfs_ns_file_download_rsp_s {
	gvfs_ns_rsp_head_t    head;
	
	unsigned int	      encrypt; 	   /* 0:未加密 1:加密 */
	UINT64                file_id;
	unsigned int          block_number;
	gvfs_ds_block_info_t *block_info;

	unsigned int		  separate;	
} gvfs_ns_file_download_rsp_t;


typedef struct gvfs_ns_file_delete_rsp_s {
	gvfs_ns_rsp_head_t head;
	
	unsigned int	   encrypt; 	 /* 0:未加密 1:加密 */
	
	unsigned int	   separate;	
	
} gvfs_ns_file_delete_rsp_t;

typedef struct gvfs_ns_file_query_rsp_s {
	gvfs_ns_rsp_head_t        head;

	UINT32                    encrypt;
	UINT64                    file_size;
	UINT32                    block_size;
	UINT32                    separate;
} gvfs_ns_file_query_rsp_t;

VOID gvfs_ns_process_response_header(gvfs_event_t *rev);

#endif /* _GVFS_NS_INTERFACE_H_INCLUDE_ */
