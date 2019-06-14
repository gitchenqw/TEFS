#ifndef __GFS_BUSI_H__
#define __GFS_BUSI_H__
#include <gfs_config.h>
#include <gfs_mem.h>
#include <gfs_net.h>
#include <gfs_busi_types.h>

typedef struct gfs_busi_s gfs_busi_t;
typedef gfs_int32   (*gfs_busi_handle_ptr)(gfs_busi_t *busi);

struct gfs_busi_s
{
    gfs_uint32          type;
    gfs_uint32          id;
    gfs_uint32          stat;
    gfs_busi_req_head_t *reh;
    gfs_busi_rsp_head_t *rsh;/* 是实际的rsp数据字段的大小 */

    gfs_mem_pool_t     *pool;
    gfs_net_buf_t       rbuf; /* 存放收到的数据,长度等于实际数据长度 */
    gfs_net_buf_t       sbuf; 

    gfs_void           *c; /* 保存的是该业务对应的连接 */
    gfs_void           *data;
    gfs_busi_handle_ptr parse;
};

gfs_int32       gfs_busi_handle(gfs_net_t *c, gfs_void *data, gfs_uint32 len);

//parse
gfs_int32       gfs_busi_login_parse(gfs_busi_t *busi);
gfs_int32       gfs_busi_upload_parse(gfs_busi_t *busi);
gfs_int32       gfs_busi_download_parse(gfs_busi_t *busi);
gfs_int32       gfs_busi_delete_file_parse(gfs_busi_t *busi);

gfs_int32       gfs_busi_delete_block_parse(gfs_busi_t *busi);
gfs_int32       gfs_busi_copy_block_parse(gfs_busi_t *busi);
gfs_int32       gfs_busi_heartbeat_parse(gfs_busi_t *busi);

gfs_int32       gfs_busi_getfsize_parse(gfs_busi_t *busi);

//handle
gfs_int32       gfs_busi_login_handle(gfs_busi_t *busi);
gfs_int32       gfs_busi_upload_handle(gfs_busi_t *busi);
gfs_int32       gfs_busi_download_handle(gfs_busi_t *busi);
gfs_int32       gfs_busi_delete_file_handle(gfs_busi_t *busi);

gfs_int32       gfs_busi_delete_block_handle(gfs_busi_t *busi);
gfs_int32       gfs_busi_copy_block_handle(gfs_busi_t *busi);
gfs_int32       gfs_busi_heartbeat_handle(gfs_busi_t *busi);

gfs_int32       gfs_busi_getfsize_handle(gfs_busi_t *busi);

//tobuf
gfs_int32       gfs_busi_login_tobuf(gfs_busi_t *busi);
gfs_int32       gfs_busi_upload_tobuf(gfs_busi_t *busi);
gfs_int32       gfs_busi_download_tobuf(gfs_busi_t *busi);
gfs_int32       gfs_busi_delete_file_tobuf(gfs_busi_t *busi);

gfs_int32       gfs_busi_delete_block_tobuf(gfs_busi_t *busi);
gfs_int32       gfs_busi_copy_block_tobuf(gfs_busi_t *busi);
gfs_int32       gfs_busi_heartbeat_tobuf(gfs_busi_t *busi);

gfs_int32       gfs_busi_getfsize_tobuf(gfs_busi_t *busi);

//gfs_int32       gfs_busi_notice_delete_file(gfs_uint64 fileid);
//gfs_int32       gfs_busi_notice_copy_block(gfs_uint64 blockid);

gfs_int32       gfs_busi_notice_handle(gfs_void *data);
gfs_int32       gfs_busi_init();
#endif
