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


gfs_int32       gfs_busi_getfsize_parse(gfs_busi_t *busi)
{
    gfs_busi_req_getfsize_t     *req;
    gfs_busi_rsp_getfsize_t     *rsp;
    gfs_net_t                   *c;

	c = busi->c;
    req = (gfs_busi_req_getfsize_t*)busi->reh;
    rsp = (gfs_busi_rsp_getfsize_t*)busi->rsh;

    req->head.len = ntohl(req->head.len);
    if (req->head.len != 36)
    {
        rsp->head.len = htonl(36);
        rsp->head.id = req->head.id;
        rsp->head.type = req->head.type;
        rsp->head.encrypt = htonl(0);
        rsp->head.rtn = htonl(GFS_BUSI_RTN_MSG_FORMAT_ERR);
        rsp->separate = htonl(0);

		glog_error("getfsize from:%s fd:%d id:%u message format error",
				   c->addr_text, c->fd, ntohl(req->head.id));
        return GFS_BUSI_ERR;
    }

    req->head.encrypt = ntohl(req->head.encrypt);
    req->separate = ntohl(req->separate);

	glog_info("getfsize from:%s fd:%d id:%u parse ok",
			  c->addr_text, c->fd, ntohl(req->head.id));

    return GFS_OK;
}

gfs_int32       gfs_busi_getfsize_handle(gfs_busi_t *busi)
{
    gfs_busi_req_head_t      *reh;
    gfs_busi_rsp_head_t      *rsh;
    gfs_busi_req_getfsize_t  *req;
    gfs_busi_rsp_getfsize_t  *rsp;
    gfs_net_t                *c;

    gfs_int32                 ret;
    gfs_char                  hash[64];
    gfs_uint64                fsize;

	c = busi->c;
    reh = busi->reh;
    rsh = busi->rsh;
    req = (gfs_busi_req_getfsize_t*)reh;
    rsp = (gfs_busi_rsp_getfsize_t*)rsh;

    rsp->head.len = htonl(36);
    rsp->head.id = req->head.id;
    rsp->head.type = req->head.type;
    rsp->head.encrypt = htonl(0);
    rsp->head.rtn = htonl(GFS_BUSI_RTN_OK);
    rsp->bsize = htonl(gfs_global->block_size);
    rsp->fsize = htonll(0);
    rsp->separate = htonl(0);

    gfs_hextostr(req->hash, 16, hash);
    hash[32] = '\0';

    ret = gfs_mysql_getfsize(hash, &fsize);
    if (ret != SQL_OK)
    {
    	glog_error("getfsize from:%s fd:%d for gfs_mysql_getfsize(\"%s\") failed",
    			   c->addr_text, c->fd, hash);
        goto NOFILE;
    }

    rsp->fsize = htonll(fsize);
	glog_info("getfsize from:%s fd:%d id:%u for file:%s fsize:%llu handle ok",
			  c->addr_text, c->fd, ntohl(req->head.id), hash, fsize);

    return GFS_OK;
NOFILE:
    rsp->head.rtn = htonl(GFS_BUSI_RTN_FILE_NOT_EXIST);
    return GFS_OK;
}

gfs_int32       gfs_busi_getfsize_tobuf(gfs_busi_t *busi)
{
    gfs_void                *data, *p;
    gfs_busi_rsp_getfsize_t *rsp;
    gfs_net_t               *c;

	c = busi->c;
    busi->sbuf.pos = ntohl(busi->rsh->len);
    busi->sbuf.len = busi->sbuf.pos;
    data = gfs_mem_malloc(busi->pool, busi->rsh->len);
    if (!data)
        return GFS_BUSI_ERR;

    rsp = (gfs_busi_rsp_getfsize_t*)busi->rsh;
    busi->sbuf.data = data;

    memcpy(busi->sbuf.data, busi->rsh, 20);
    p = data + 20;
    memcpy(p, &rsp->fsize, 8);
    p += 8;
    memcpy(p, &rsp->bsize, 4);
    p += 4;
    memset(p, 0, 4);

    glog_info("getfsize from:%s fd:%d id:%u send ok",
    		  c->addr_text, c->fd, ntohl(rsp->head.id));

    return GFS_OK;
}
