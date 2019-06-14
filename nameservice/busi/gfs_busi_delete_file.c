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

gfs_int32 gfs_busi_delete_file_parse(gfs_busi_t *busi)
{
    gfs_busi_req_delfile_t     *req;
    gfs_busi_rsp_delfile_t     *rsp;
    gfs_net_t                  *c;

    c = busi->c;

    req = (gfs_busi_req_delfile_t*)busi->reh;
    rsp = (gfs_busi_rsp_delfile_t*)busi->rsh;

    req->head.len = ntohl(req->head.len);
    if (req->head.len != 36)
    {
        rsp->head.len = htonl(24);
        rsp->head.id = req->head.id;
        rsp->head.type = req->head.type;
        rsp->head.encrypt = htonl(0);
        rsp->head.rtn = htonl(GFS_BUSI_RTN_MSG_FORMAT_ERR);
        rsp->separate = htonl(0);

		glog_error("delete from:%s fd:%d id:%u message format error",
				   c->addr_text, c->fd, ntohl(req->head.id));
        return GFS_BUSI_ERR;
    }

    req->head.encrypt = ntohl(req->head.encrypt);
    req->separate = ntohl(req->separate);

	glog_info("delete from:%s fd:%d id:%u parse ok",
			  c->addr_text, c->fd, ntohl(req->head.id));
    return GFS_OK;
}

gfs_int32 gfs_busi_delete_file_handle(gfs_busi_t *busi)
{
    gfs_busi_req_head_t      *reh;
    gfs_busi_rsp_head_t      *rsh;
    gfs_busi_req_delfile_t   *req;
    gfs_busi_rsp_delfile_t   *rsp;
    gfs_net_t                *c;

    gfs_int32                 ret;
    gfs_char                  hash[64];

    reh = busi->reh;
    rsh = busi->rsh;
    req = (gfs_busi_req_delfile_t*)reh;
    rsp = (gfs_busi_rsp_delfile_t*)rsh;

    rsp->head.len = htonl(24);
    rsp->head.id = req->head.id;
    rsp->head.type = req->head.type;
    rsp->head.encrypt = htonl(0);
    rsp->head.rtn = htonl(GFS_BUSI_RTN_OK);
    rsp->separate = htonl(0);

    c = busi->c;

    if (req->head.encrypt != 0)
    {
    	glog_error("delete from:%s fd:%d id:%u unsupport encrypt",
    				c->addr_text, c->fd, ntohl(req->head.id));
        goto PARAM_ERR;
    }

    gfs_hextostr(req->hash, 16, hash);
    hash[32] = '\0';
    ret = gfs_mysql_delfile(hash);
    if (ret == SQL_FILE_NOT_EXIST)
    {
    	glog_info("delete from:%s for:%d gfs_mysql_delfile(\"%s\") not exist",
    			  c->addr_text, c->fd, hash);
        goto FILE_NOT_EXIT;
    }
    if (ret != SQL_OK)
    {
    	glog_info("delete from:%s for:%d gfs_mysql_delfile(\"%s\") error",
    			  c->addr_text, c->fd, hash);
        goto SYSTEM_ERR;
    }

	glog_info("delete from:%s fd:%d id:%u delete file(hash:%s) handle ok",
			  c->addr_text, c->fd, ntohl(req->head.id), hash);

    return GFS_OK;
PARAM_ERR:
    rsp->head.rtn = htonl(GFS_BUSI_RTN_OK);
    return GFS_OK;
SYSTEM_ERR:
    rsp->head.rtn = htonl(GFS_BUSI_RTN_SYS_ERR);
    return GFS_OK;
FILE_NOT_EXIT:
    rsp->head.rtn = htonl(GFS_BUSI_RTN_FILE_NOT_EXIST);
    return GFS_OK;
}

gfs_int32 gfs_busi_delete_file_tobuf(gfs_busi_t *busi)
{
    gfs_void        *data;
    gfs_net_t       *c;

    c = busi->c;

    busi->sbuf.pos = ntohl(busi->rsh->len);
    busi->sbuf.len = busi->sbuf.pos;
    data = gfs_mem_malloc(busi->pool, busi->sbuf.len);
    if (!data)
        return GFS_BUSI_ERR;

    busi->sbuf.data = data;
    memcpy(busi->sbuf.data, busi->rsh, busi->sbuf.len);

    glog_info("delete from:%s fd:%d send ok", c->addr_text, c->fd);

    return GFS_OK;
}

