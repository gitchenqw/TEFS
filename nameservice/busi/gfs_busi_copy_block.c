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

gfs_int32 gfs_busi_copy_block_parse(gfs_busi_t *busi)
{
	gfs_busi_nte_cpy_block_t *req;
	gfs_net_t                *c;

	c = busi->c;
	
    req = (gfs_busi_nte_cpy_block_t*)busi->reh;
    req->head.len = ntohl(req->head.len);
    if (req->head.len != 44) {
    	glog_error("copy block notice from:%s fd:%d id:%u message format error",
    			   c->addr_text, c->fd, ntohl(req->head.id));
        return GFS_BUSI_ERR;
    }

	req->head.encrypt = ntohl(req->head.encrypt);
	req->head.id = ntohl(req->head.id);
	req->dsid = ntohl(req->dsid);
	req->remain = ntohll(req->remain);
	req->separate = ntohl(req->separate);

	glog_info("copy block notice from:%s fd:%d id:%u dsid:%u remain:%llu parse ok",
			  c->addr_text, c->fd, req->head.id, req->dsid, req->remain);

    return GFS_OK;
}

gfs_int32 gfs_busi_copy_block_handle(gfs_busi_t *busi)
{
    gfs_busi_req_head_t      *reh;
    gfs_busi_nte_cpy_block_t *req;

    gfs_int32                 ret;
    gfs_net_t                *c;
    gfs_uint64                fid;
    gfs_uint32                bindex;

    reh = busi->reh;
    req = (gfs_busi_nte_cpy_block_t*)reh;
    c = (gfs_net_t*) busi->c;

    if (req->head.encrypt != 0) {
    	glog_warn("copy block notice from:%s fd:%d unsupport encrypt, ignore",
    			   c->addr_text, c->fd);
        return GFS_OK;
    }

    memcpy(&fid, req->bid, 8); /* 头8个字节为field id */
    memcpy(&bindex, &req->bid[8], 4); /* 后4个字节为块索引 */
    fid = ntohll(fid);
    bindex = ntohl(bindex);
    
    ret = gfs_mysql_cpyblock(fid, bindex, req->dsid);
    if (ret != SQL_OK) {
    	glog_error("gfs_mysql_cpyblock(%llu, %u, %u) failed for:%s fd:%d id:%u",
    			   fid, bindex, req->dsid, c->addr_text, c->fd, req->head.id);
        return GFS_ERR;
    }

	glog_info("copy block notice from:%s fd:%d id:%u dsid:%u handle ok",
			  c->addr_text, c->fd, req->head.id, req->dsid);

    return GFS_OK;
}

gfs_int32 gfs_busi_copy_block_tobuf(gfs_busi_t *busi)
{
    return GFS_OK;
}

