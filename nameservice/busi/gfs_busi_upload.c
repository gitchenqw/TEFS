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

gfs_int32 gfs_busi_upload_parse(gfs_busi_t *busi)
{
    gfs_busi_req_upload_t     *req;
    gfs_busi_rsp_upload_t     *rsp;
    gfs_net_t                 *c;

	c = busi->c;
    req = (gfs_busi_req_upload_t*)busi->reh;
    rsp = (gfs_busi_rsp_upload_t*)busi->rsh;

    req->head.len = ntohl(req->head.len);
    if (req->head.len != 52)
    {
        rsp->head.len = htonl(36);
        rsp->head.id = req->head.id;
        rsp->head.type = req->head.type;
        rsp->head.encrypt = htonl(0);
        rsp->head.rtn = htonl(GFS_BUSI_RTN_MSG_FORMAT_ERR);
        rsp->dsnum = htonl(0);
        rsp->fid = htonl(0);
        rsp->dslist = NULL;
        rsp->separate = htonl(0);

		glog_error("upload from:%s fd:%d id:%u message format error",
				   c->addr_text, c->fd, ntohl(req->head.id));
        return GFS_BUSI_ERR;
    }

    req->head.encrypt = ntohl(req->head.encrypt);

    /* filehash */
    req->fsize = ntohll(req->fsize); /* 文件总大小 */
    req->bnum = ntohl(req->bnum); /* 文件块总个数 */
    req->bindex = ntohl(req->bindex); /* 当前第几个快 */
    
    req->separate = ntohl(req->separate);

	glog_info("upload from:%s fd:%d id:%u fsize:%llu bnum:%u bindex:%d parse ok",
			  c->addr_text, c->fd, ntohl(req->head.id), req->fsize, req->bnum,
			  req->bindex);

    return GFS_OK;
}

gfs_int32       gfs_busi_upload_handle(gfs_busi_t *busi)
{
    gfs_busi_req_head_t      *reh;
    gfs_busi_rsp_head_t      *rsh;
    gfs_busi_req_upload_t    *req;
    gfs_busi_rsp_upload_t    *rsp;
	gfs_net_t                *c;

    gfs_int32                 ret, status;
    gfs_char                  hash[64];
    gfs_net_t*                pnet[3];
    gfs_uint32                num, i;
    gfs_uint64                fid;
    gfs_void                 *pdata;

	c = busi->c;

    reh = busi->reh;
    rsh = busi->rsh;
    req = (gfs_busi_req_upload_t*)reh;
    rsp = (gfs_busi_rsp_upload_t*)rsh;

    rsp->head.len = htonl(36);
    rsp->head.id = req->head.id;
    rsp->head.type = req->head.type;
    rsp->head.encrypt = htonl(0);
    rsp->head.rtn = htonl(GFS_BUSI_RTN_OK);
    rsp->fid = htonl(0);
    rsp->dsnum = htonl(0);
    rsp->dslist = NULL;
    rsp->separate = htonl(0);

    if (req->head.encrypt != 0)
    {
    	glog_error("upload from:%s fd:%d id:%u unsupport encrypt",
    			   c->addr_text, c->fd, ntohl(req->head.id));
        goto PARAM_ERR;
    }

    if (req->bindex < -1 || req->bnum < 1 || req->fsize <= 0)           //-1 all file
    {
    	glog_error("upload from:%s fd:%d id:%u bindex:%d bnum:%u fsize:%llu error",
    			   c->addr_text, c->fd, ntohl(req->head.id), req->bindex,
    			   req->bnum, req->fsize);
        goto PARAM_ERR;
    }

    if (req->bnum > 150)
    {
    	glog_error("upload from:%s fd:%d id:%u bnum:%u > 150 error",
    			   c->addr_text, c->fd, ntohl(req->head.id), req->bnum);
        goto BLOCK_TOO_MUCH;
    }

	/* { 选择最优的三个负载最低的进行下载 */
    num = 3;
    ret = gfs_net_map_gettopn((gfs_net_t**)(&pnet), &num);
    if (ret != GFS_OK)
    {
    	glog_error("gfs_net_map_gettopn() for:%s fd:%d id:%u failed",
    				c->addr_text, c->fd, ntohl(req->head.id));
        goto SYSTEM_ERR;
    }
    rsp->head.len = htonl(ntohl(rsp->head.len) + num*20);
    /* } */

    rsp->dsnum = htonl(num);
    if (rsp->dsnum == 0)
    {
    	glog_info("upload for:%s fd:%d id:%u available ds:%d",
    			  c->addr_text, c->fd, ntohl(req->head.id), num);
        goto NO_DS;
    }
    else
    {
        pdata = gfs_mem_malloc(busi->pool, num*20);      //gfs_busi_ds_t sizeof 20
        if (!pdata)
            goto SYSTEM_ERR;

        rsp->dslist = pdata;
        for (i = 0; i < num; i++)
        {
            memcpy(rsp->dslist[i].ip, &pnet[i]->addr.sin_addr.s_addr, 4);
            rsp->dslist[i].port = htonl(pnet[i]->user.port);
        }
    }

    gfs_hextostr(req->hash, 16, hash);
    hash[32] = '\0';
    fid = 0;
    ret = gfs_mysql_upload(hash, req->fsize, req->bnum, req->bindex, &status, &fid);
    if (ret != SQL_OK)
    {
    	glog_error("gfs_mysql_upload(%s, %llu, %u, %d) for:%s fd:%d failed",
    			   hash, req->fsize, req->bnum, req->bindex, c->addr_text,
    			   c->fd);
        goto SYSTEM_ERR;
    }
    
    if (status == SQL_FILE_EXIT_COMPLETE)
    {
        rsp->head.rtn = htonl(GFS_BUSI_RTN_FILE_FINISHED);
    	glog_info("gfs_mysql_upload(%s, %llu, %u, %d) for:%s fd:%d file exist and complete",
    			   hash, req->fsize, req->bnum, req->bindex, c->addr_text,
    			   c->fd);
        goto BUSI_OK;
    }
    else if (status == SQL_BLOCK_EXIST)
    {
        rsp->head.rtn = htonl(GFS_BUSI_RTN_BLOCK_EXIST);
    	glog_info("gfs_mysql_upload(%s, %llu, %u, %d) for:%s fd:%d file block is exist",
    			   hash, req->fsize, req->bnum, req->bindex, c->addr_text,
    			   c->fd);
        goto BUSI_OK;
    }

    rsp->fid = htonll(fid);

	glog_info("upload from:%s fd:%d handle ok", c->addr_text, c->fd);

BUSI_OK:
    return GFS_OK;
NO_DS:
    rsp->head.rtn = htonl(GFS_BUSI_RTN_NO_DS_USE);
    return GFS_OK;
PARAM_ERR:
    rsp->head.rtn = htonl(GFS_BUSI_RTN_OK);
    return GFS_OK;
SYSTEM_ERR:
    rsp->head.rtn = htonl(GFS_BUSI_RTN_SYS_ERR);
    return GFS_OK;
BLOCK_TOO_MUCH:
    rsp->head.rtn = htonl(GFS_BUSI_RTN_BLOCK_TOO_MUCH);
    return GFS_OK;
}

gfs_int32       gfs_busi_upload_tobuf(gfs_busi_t *busi)
{
    gfs_net_buf_t           *buf;
    gfs_busi_rsp_upload_t   *rsp;
    gfs_uint32               len;
    gfs_void                *pdata;
    gfs_net_t               *c;

	c = busi->c;

    rsp = (gfs_busi_rsp_upload_t*)busi->rsh;
    buf = &busi->sbuf;

    buf->pos = htonl(busi->rsh->len);
    buf->len = buf->pos;
    pdata = gfs_mem_malloc(busi->pool, buf->len);
    if (!pdata)
    {
        gfs_mem_free(rsp->dslist);
        return GFS_BUSI_ERR;
    }

    buf->data = pdata;
    len = 20;
    memcpy(pdata, busi->rsh, len);
    pdata += len;
    memcpy(pdata, &rsp->fid, 8);
    pdata += 8;
    memcpy(pdata, &rsp->dsnum, 4);
    pdata += 4;

    len = ntohl(rsp->dsnum) * 20;      //gfs_busi_ds_t sizeof 20
    memcpy(pdata, rsp->dslist, len);
    pdata += len;

    memset(pdata, 0, sizeof(gfs_uint32));

    gfs_mem_free(rsp->dslist);
    
    glog_info("upload send to:%s fd:%d ok", c->addr_text, c->fd);

    return GFS_OK;
}
