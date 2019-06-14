

#ifndef _GVFS_UPSTREAM_H_INCLUDE_
#define _GVFS_UPSTREAM_H_INCLUDE_


#include <gvfs_config.h>
#include <gvfs_core.h>

extern const CHAR *ns_ip_addr;
extern INT32       ns_port;

typedef struct gvfs_upstream_conf_s {
	CHAR	 *ip_addr;
	INT32     port;
} gvfs_upstream_conf_t;

struct gvfs_rsp_head_s
{
	VOID		  *data;
	VOID          *rsh;
    unsigned int   len;
    unsigned int   id;
    unsigned int   type;
    unsigned int   encrypt;
    unsigned int   rtn;
};

struct gvfs_upstream_s {
	gvfs_pool_t                       *pool; /* 继承自gvfs_connection_t或者重新分配 */     
	gvfs_buf_t                        *download_body;
	gvfs_buf_t                         buf;  /* 该buf用于实现发送端分包发送的情况   */
	gvfs_peer_connection_t             peer; /* 和对端的连接结构                    */
	gvfs_rsp_head_t                    rsp_head;

	UINT32                             sid; 
	UINT16                             header_completed; /* 标志位: 消息头已经解析完成 */
	UINT16                             body_completed;   /* 标志位: 消息体已经解析完成 */
};


extern gvfs_upstream_conf_t *g_gvfs_ucf;

    
#endif

