#ifndef __GFS_BUSI_TYPES_H__
#define __GFS_BUSI_TYPES_H__
#include <gfs_config.h>

typedef struct gfs_busi_ds_s
{
    gfs_uchar   ip[16];
    gfs_uint32  port;
}gfs_busi_ds_t;

typedef struct gfs_busi_binfo_s
{
    gfs_uint32      bindex;
    gfs_uint32      port;
    gfs_uchar       ip[16];
}gfs_busi_binfo_t;

typedef struct gfs_busi_req_head_s
{
    gfs_uint32  len;
    gfs_uint32  id;
    gfs_uint32  type;
    gfs_uint32  encrypt;
}gfs_busi_req_head_t;

typedef struct gfs_busi_rsp_head_s
{
    gfs_uint32  len;
    gfs_uint32  id;
    gfs_uint32  type;
    gfs_uint32  rtn;
    gfs_uint32  encrypt;
}gfs_busi_rsp_head_t;

typedef struct gfs_busi_rep_login_s
{
    gfs_busi_req_head_t head;
    gfs_uint32          type; /* 0:ps端 1:ds端 */
    gfs_uint32          dsid; /* dataserver id dataserver配置需要不一样 */
    gfs_uint64          remain; /* 磁盘剩余容量 */
    gfs_uint64          disk; /* 磁盘总容量 */
    gfs_uint32          port; /* DS监听的端口 */
    gfs_uint32          separate;
}gfs_busi_req_login_t;

typedef struct gfs_busi_rsp_login_s
{
    gfs_busi_rsp_head_t head;
    gfs_uint32          bsize; /* 告诉dataserver块大小 */
    gfs_uint32          separate;
}gfs_busi_rsp_login_t;

typedef struct gfs_busi_req_upload_s
{
    gfs_busi_req_head_t head;
    gfs_uchar           hash[16]; /* 文件hash */
    gfs_uint64          fsize; /* 文件大小 */
    gfs_uint32          bnum; /* 文件块总数 */
    gfs_int32           bindex; /* 文件块索引 */
    gfs_uint32          separate;
}gfs_busi_req_upload_t;

typedef struct gfs_busi_rsp_upload_s
{
    gfs_busi_rsp_head_t head;
    gfs_uint64          fid;
    gfs_uint32          dsnum;
    gfs_busi_ds_t      *dslist;
    gfs_uint32          separate;
}gfs_busi_rsp_upload_t;

typedef struct gfs_busi_req_download_s
{
    gfs_busi_req_head_t head;
    gfs_uchar           hash[16];
    gfs_int32           bindex;
    gfs_uint32          separate;
}gfs_busi_req_download_t;

typedef struct gfs_busi_rsp_download_s
{
    gfs_busi_rsp_head_t head;
    gfs_uint16          pnum;
    gfs_uint16          pindex;
    gfs_uint64          fid;
    gfs_uint32          bnum;
    gfs_busi_binfo_t   *blist;
    gfs_uint32          separate;
}gfs_busi_rsp_download_t;

typedef struct gfs_busi_req_delfile_s
{
    gfs_busi_req_head_t head;
    gfs_uchar           hash[16];
    gfs_uint32          separate;
}gfs_busi_req_delfile_t;

typedef struct gfs_busi_rsp_delfile_s
{
    gfs_busi_rsp_head_t head;
    gfs_uint32          separate;
}gfs_busi_rsp_delfile_t;

typedef struct gfs_busi_nte_delblock_s      //0x06
{
    gfs_busi_req_head_t head;
    gfs_uchar           bid[12];
    gfs_uint64          remain;
    gfs_uint32          separate;
}gfs_busi_nte_delblock_t;

typedef struct gfs_busi_nte_cpyblock_s
{
    gfs_busi_req_head_t head;
    gfs_uint32          dsid;
    gfs_uchar           bid[12];
    gfs_uint64          remain;
    gfs_uint32          separate;
}gfs_busi_nte_cpy_block_t;

typedef struct gfs_busi_heartbeat_s
{
    gfs_busi_req_head_t head;
    gfs_uint32          type;
    gfs_uint32          weighted;
    gfs_uint32          separate;
}gfs_busi_heartbeat_t;

typedef struct gfs_busi_req_getfsize_s
{
    gfs_busi_req_head_t head;
    gfs_uchar           hash[16];
    gfs_uint32          separate;
}gfs_busi_req_getfsize_t;

typedef struct gfs_busi_rsp_getfsize_s
{
    gfs_busi_rsp_head_t head;
    gfs_uint64          fsize;
    gfs_uint32          bsize;
    gfs_uint32          separate;
}gfs_busi_rsp_getfsize_t;

#endif