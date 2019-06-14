

#ifndef _GVFS_BUF_H_INCLUDE_
#define _GVFS_BUF_H_INCLUDE_


#include <gvfs_config.h>
#include <gvfs_core.h>


/*
 * @size 由配置文件配置,推荐1024Byte
 */
struct gvfs_buf_s{
	gvfs_pool_t    *pool;
	
	u_char 		   *data;         /* buffer */
	size_t          size;         /* buffer size */
	
	u_char         *pos;          /* start of data */
	u_char         *last;         /* end of data */
	u_char         *start;        /* start of buffer */
	u_char         *end;          /* end of buffer */

	struct gvfs_list_head lh;  /* buffer 链表节点 */
};


INT32 gvfs_http_create_request_body_buf_list(UINT32 size, UINT32 nelts);
gvfs_buf_t *gvfs_http_get_request_buf_body(VOID);
VOID gvfs_http_free_request_body_buf(gvfs_buf_t *request_body_buf);


#endif /* _GVFS_BUF_H_INCLUDE_ */
