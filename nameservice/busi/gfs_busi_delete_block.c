#include <gfs_config.h>
#include <gfs_log.h>
#include <gfs_mem.h>
#include <gfs_global.h>
#include <gfs_net.h>
#include <gfs_tools.h>
#include <gfs_net_map.h>
#include <gfs_handle.h>
#include <gfs_sql.h>
#include <gfs_busi.h>

gfs_int32       gfs_busi_delete_block_parse(gfs_busi_t *busi)
{
    gfs_busi_nte_delblock_t     *req;
    gfs_net_t                   *c;

    c = busi->c;

    req = (gfs_busi_nte_delblock_t*)busi->reh;
    req->head.len = ntohl(req->head.len);
    if (req->head.len != 40)
    {
    	glog_error("delete notice from:%s fd:%d id:%u message format error",
    			   c->addr_text, c->fd, ntohl(req->head.id));
        return GFS_BUSI_ERR;
    }

    req->head.encrypt = ntohl(req->head.encrypt);
    req->remain = ntohll(req->remain);
    req->separate = ntohl(req->separate);

	glog_info("delete notice from:%s fd:%d id:%u disk remain:%llu parse ok",
			  c->addr_text, c->fd, ntohl(req->head.id), req->remain);

    return GFS_OK;
}

gfs_int32       gfs_busi_delete_block_handle(gfs_busi_t *busi)
{
    gfs_busi_req_head_t      *reh;
    gfs_busi_nte_delblock_t  *req;

    gfs_int32                 ret;
    gfs_uint64                fid;
    gfs_uint32                bindex;
    gfs_net_t                *c;

    reh = busi->reh;
    req = (gfs_busi_nte_delblock_t*)reh;
    c = busi->c;

    if (req->head.encrypt != 0)
    {
    	glog_error("delete notice from:%s fd:%d id:%u unsupport encrypt",
    			   c->addr_text, c->fd, ntohl(req->head.id));
        return GFS_OK;
    }

    memcpy(&fid, req->bid, 8);
    memcpy(&bindex, &req->bid[8], 4);
    fid = ntohll(fid);
    bindex = ntohl(bindex);
    ret = gfs_mysql_delblock(fid, bindex, c->user.dsid);
    if (ret != SQL_OK)
    {
    	glog_error("delete notice from:%s fd:%d id:%u for gfs_mysql_delblock(%llu, %u, %u) failed",
    			   c->addr_text, c->fd, ntohl(req->head.id), fid, bindex, c->user.dsid);
        return GFS_OK;
    }

	glog_info("delete notice from:%s fd:%d id:%u delete fid:%llu bindex:%u handle ok",
			  c->addr_text, c->fd, ntohl(req->head.id), fid, bindex);

    return GFS_OK;
}

gfs_int32       gfs_busi_delete_block_tobuf(gfs_busi_t *busi)
{
    return GFS_OK;
}
