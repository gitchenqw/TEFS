#include <gfs_config.h>
#include <gfs_log.h>
#include <gfs_mem.h>
#include <gfs_global.h>
#include <gfs_net.h>
#include <gfs_tools.h>
#include <gfs_net_map.h>
#include <gfs_handle.h>
#include <gfs_busi.h>

gfs_int32 gfs_busi_heartbeat_parse(gfs_busi_t *busi)
{
    gfs_busi_heartbeat_t     *req;
	gfs_net_t                *c;

	c = busi->c;
	
    req = (gfs_busi_heartbeat_t*)busi->reh;
    req->head.len = ntohl(req->head.len);
    if (req->head.len != 28)
    {
    	glog_error("heartbeat from:%s fd:%d id:%u message format error",
    			   c->addr_text, c->fd, ntohl(req->head.id));
        return GFS_BUSI_ERR;
    }

    req->head.encrypt = ntohl(req->head.encrypt);
    req->type = ntohl(req->type);
    req->weighted = ntohl(req->weighted);
    
	glog_debug("heartbeat from:%s fd:%d id:%u dsid:%u weighted:%u parse ok",
			  c->addr_text, c->fd, ntohl(req->head.id), c->user.dsid, req->weighted);

    return GFS_OK;
}

gfs_int32 gfs_busi_heartbeat_handle(gfs_busi_t *busi)
{
    gfs_busi_req_head_t      *reh;
    gfs_busi_heartbeat_t     *req;

    gfs_net_t                *c;

    reh = busi->reh;
    req = (gfs_busi_heartbeat_t*)reh;
    c = (gfs_net_t*)busi->c;

    if (req->head.encrypt != 0)
    {
    	glog_error("heartbeat from:%s fd:%d id:%u unsupport encrypt",
    			   c->addr_text, c->fd, ntohl(req->head.id));
        return GFS_OK;
    }
    if (c->user.ctype == GFS_CLIENT_TYPE_PS)
    {
    	glog_info("heartbeat from:%s fd:%d id:%u is proxy service",
    			  c->addr_text, c->fd, ntohl(req->head.id));
        return GFS_OK;
    }
    
    if (req->weighted < 0 || req->weighted > 100)
    {
    	glog_error("heartbeat from:%s fd:%d id:%u weight:%u error",
    			   c->addr_text, c->fd, ntohl(req->head.id), req->weighted);
        return GFS_OK;
    }

    c->user.weight = req->weighted;
    
	glog_debug("heartbeat from:%s fd:%d id:%u handle ok",
			  c->addr_text, c->fd, ntohl(req->head.id));

    return GFS_OK;
}

gfs_int32 gfs_busi_heartbeat_tobuf(gfs_busi_t *busi)
{
    return GFS_OK;
}
