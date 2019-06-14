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

gfs_int32 gfs_busi_login_parse(gfs_busi_t *busi)
{
    gfs_busi_req_login_t     *req;
    gfs_busi_rsp_login_t     *rsp;
    gfs_net_t                *c;

    c = busi->c;

    req = (gfs_busi_req_login_t*)busi->reh;
    rsp = (gfs_busi_rsp_login_t*)busi->rsh;

    req->head.len = ntohl(req->head.len);
    if (req->head.len != 48)
    {
        rsp->head.len = htonl(28);
        rsp->head.id = req->head.id;
        rsp->head.type = req->head.type;
        rsp->head.encrypt = 0;
        rsp->head.rtn = htonl(GFS_BUSI_RTN_MSG_FORMAT_ERR);
        rsp->bsize = htonl(gfs_global->block_size);
        rsp->separate = 0;

		glog_error("login from:%s fd:%d message error", c->addr_text, c->fd);
        return GFS_BUSI_ERR;
    }

    req->head.encrypt = ntohl(req->head.encrypt);
    
    req->type = ntohl(req->type);
    req->dsid = ntohl(req->dsid);
    req->remain = ntohll(req->remain);
    req->disk = ntohll(req->disk);
    req->port = ntohl(req->port);
    
    req->separate = ntohl(req->separate);

	glog_info("login from:%s fd:%d dsid:%u remain:%llu disk:%llu port:%u parse ok",
			  c->addr_text, c->fd, req->dsid, req->remain, req->disk, req->port);
			  
    return GFS_OK;
}

gfs_int32 gfs_busi_login_handle(gfs_busi_t *busi)
{
    gfs_busi_req_head_t      *reh;
    gfs_busi_rsp_head_t      *rsh;
    gfs_busi_req_login_t     *req;
    gfs_busi_rsp_login_t     *rsp;
    gfs_int32                 ret;
    gfs_net_t                *c;

    reh = busi->reh;
    rsh = busi->rsh;
    req = (gfs_busi_req_login_t*)reh;
    rsp = (gfs_busi_rsp_login_t*)rsh;

    rsp->head.len = htonl(28);
    rsp->head.id = req->head.id;
    rsp->head.type = req->head.type;
    rsp->head.encrypt = 0;
    rsp->head.rtn = htonl(GFS_BUSI_RTN_OK);
    rsp->bsize = htonl(gfs_global->block_size);
    rsp->separate = 0;
    
    c = (gfs_net_t*)busi->c;

    if (req->head.encrypt != 0) {
    	glog_error("login from:%s fd:%d unsupport encrypt",
    			  c->addr_text, c->fd);
        goto PARAM_ERR;
    }

	/* PS不需要给NS心跳,该接口暂时无用 */
    if (req->type == GFS_CLIENT_TYPE_PS) { 
    	glog_info("login from:%s fd:%d success, type is proxy service",
    			  c->addr_text, c->fd);
        goto BUSI_OK;
    }
    else if (req->type != GFS_CLIENT_TYPE_DS) {
    	glog_error("login from:%s fd:%d unknow type:%u error",
    			  c->addr_text, c->fd, req->type);
        goto UNKNOWN_CLIENT_TYPE;
    }
    else if (req->remain > req->disk) {
    	glog_error("login from:%s fd:%d disk remain:%llu > size:%llu error",
    			  c->addr_text, c->fd, req->remain, req->disk);
        goto PARAM_ERR;
    }

    ret = gfs_mysql_login(req->dsid, req->disk, req->remain);
    if (ret != SQL_OK) {
    	glog_error("gfs_mysql_login(%u, %llu, %llu) for:%s fd:%d failed",
    			   req->dsid, req->disk, req->remain, c->addr_text, c->fd);
        goto SYSTEM_ERR;
    }

    c->user.ctype = req->type;
    c->user.dsid = req->dsid;
    c->user.resize = req->remain;
    c->user.dsize = req->disk;
    c->user.cfft = (req->remain * 100)/(req->disk);
    c->user.port = req->port;

    ret = gfs_net_map_add(req->dsid, c);
    if (ret != GFS_OK) {
    	glog_error("gfs_net_map_add(%u) for:%s failed", req->dsid, c->addr_text);
        goto SYSTEM_ERR;
    }

	glog_info("login from:%s fd:%d handle ok", c->addr_text, c->fd);

BUSI_OK:
    return GFS_OK;
PARAM_ERR:
    rsp->head.rtn = htonl(GFS_BUSI_RTN_OK);
    return GFS_OK;
UNKNOWN_CLIENT_TYPE:
    rsp->head.rtn = htonl(GFS_BUSI_RTN_UNKNOWN_CLIENT);
    return GFS_OK;
SYSTEM_ERR:
    rsp->head.rtn = htonl(GFS_BUSI_RTN_SYS_ERR);
    return GFS_OK;
}

gfs_int32 gfs_busi_login_tobuf(gfs_busi_t *busi)
{
    gfs_void        *data;
    gfs_net_t       *c;

    busi->sbuf.pos = ntohl(busi->rsh->len);
    busi->sbuf.len = busi->sbuf.pos;
    data = gfs_mem_malloc(busi->pool, busi->rsh->len);
    if (!data)
        return GFS_BUSI_ERR;

    busi->sbuf.data = data;
    memcpy(busi->sbuf.data, busi->rsh, busi->sbuf.pos);

	c = busi->c;
	
    glog_info("login send to:%s fd:%d ok", c->addr_text, c->fd);

    return GFS_OK;
}


