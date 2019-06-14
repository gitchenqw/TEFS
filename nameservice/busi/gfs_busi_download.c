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

gfs_int32 gfs_busi_download_parse(gfs_busi_t *busi)
{
    gfs_busi_req_download_t     *req;
    gfs_busi_rsp_download_t     *rsp;
    gfs_net_t                   *c;

    c = busi->c;

    req = (gfs_busi_req_download_t*)busi->reh;
    rsp = (gfs_busi_rsp_download_t*)busi->rsh;

    req->head.len = ntohl(req->head.len);
    if (req->head.len != 40)
    {
        rsp->head.len = htonl(36);
        rsp->head.id = req->head.id;
        rsp->head.type = req->head.type;
        rsp->head.encrypt = htonl(0);
        rsp->head.rtn = htonl(GFS_BUSI_RTN_MSG_FORMAT_ERR);
        rsp->pnum = htons(1);
        rsp->pindex = htons(0);
        rsp->bnum = htonl(0);
        rsp->separate = htonl(0);

		glog_error("download from:%s fd:%d id:%u message format error",
				   c->addr_text, c->fd, ntohl(req->head.id));
        return GFS_BUSI_ERR;
    }

    req->head.encrypt = ntohl(req->head.encrypt);
    req->bindex = ntohl(req->bindex);

	glog_info("download from:%s fd:%d id:%u bindex:%d parse ok",
			  c->addr_text, c->fd, ntohl(req->head.id), req->bindex);

    return GFS_OK;
}

gfs_int32       gfs_busi_download_handle(gfs_busi_t *busi)
{
    gfs_busi_req_head_t      *reh;
    gfs_busi_rsp_head_t      *rsh;
    gfs_busi_req_download_t  *req;
    gfs_busi_rsp_download_t  *rsp;
    gfs_net_t                *c;

    gfs_int32                 ret, copies;
    gfs_char                  hash[64];
    gfs_uint32                bnum;
    gfs_uint64                fid;
    gfs_sql_block_t          *blist;

	c = busi->c;
    reh = busi->reh;
    rsh = busi->rsh;
    req = (gfs_busi_req_download_t*)reh;
    rsp = (gfs_busi_rsp_download_t*)rsh;

    rsp->head.len = htonl(36);
    rsp->head.id = req->head.id;
    rsp->head.type = req->head.type;
    rsp->head.encrypt = htonl(0);
    rsp->head.rtn = htonl(GFS_BUSI_RTN_OK);

    rsp->pnum = htons(1);
    rsp->pindex = htons(0);
    rsp->bnum = htonl(0);
    rsp->blist = NULL;
    rsp->separate = htonl(0);

    copies = gfs_global->copies;

    if (req->head.encrypt != 0)
    {
    	glog_error("download from:%s fd:%d id:%u unsupport encrypt",
    			   c->addr_text, c->fd, ntohl(req->head.id));
        goto PARAM_ERR;
    }

    gfs_hextostr(req->hash, 16, hash);
    hash[32] = '\0';

    if (req->bindex < -1)           //-1 all file
    {
		glog_error("download from:%s fd:%d id:%u bindex:%d error",
				   c->addr_text, c->fd, ntohl(req->head.id), req->bindex);
        goto PARAM_ERR;
    }

    fid = 0;
    bnum = 0;
    ret = gfs_mysql_download(busi->pool, hash, req->bindex, &fid, &blist, &bnum);
    if (ret == SQL_EMPTY )
    {
    	glog_info("download from:%s fd:%d id:%u file(hash:%s, bindex:%d) not exist",
    			  c->addr_text, c->fd, ntohl(req->head.id), hash, req->bindex);
        goto NOFILE;
    }
    else if (ret == SQL_ERROR)
    {
    	glog_info("download from:%s fd:%d id:%u gfs_mysql_download(hash:%s bindex:%d) error",
    			  c->addr_text, c->fd, ntohl(req->head.id), hash, req->bindex);
        goto SYSTEM_ERR;
    }

    rsp->fid = htonll(fid);
    if (bnum < 1 || blist == NULL)
    {
        glog_info("download from:%s fd:%d id:%u file(hash:%s, bindex:%d) bnum:%u",
        		  c->addr_text, c->fd, ntohl(req->head.id), hash, req->bindex, bnum);
        goto NOBLOCK;
    }

    //if (bnum > 150)
    //{
    //    glog_error("%s file(hash %s,bindex %d) block number too much.", gfs_busi_req_tostr(busi), hash, req->bindex);
    //    goto SYSTEM_ERR;
    //}
    rsp->bnum = htonl(bnum);
    busi->data = blist;

    glog_info("download from:%s fd:%d id:%u for file(hash:%s, bindex:%d) fid:%llu bnum:%u handle ok",
    		  c->addr_text, c->fd, ntohl(req->head.id), hash, req->bindex, fid, bnum);

    return GFS_OK;
PARAM_ERR:
    rsp->head.rtn = htonl(GFS_BUSI_RTN_OK);
    return GFS_OK;
SYSTEM_ERR:
    rsp->head.rtn = htonl(GFS_BUSI_RTN_SYS_ERR);
    return GFS_OK;
NOFILE:
    rsp->head.rtn = htonl(GFS_BUSI_RTN_FILE_NOT_EXIST);
    return GFS_OK;
NOBLOCK:
    rsp->head.rtn = htonl(GFS_BUSI_RTN_BLOCK_NOT_EXIST);
    return GFS_OK;
}

gfs_int32 gfs_busi_download_tobuf(gfs_busi_t *busi)
{
    gfs_net_buf_t           *buf;
    gfs_busi_rsp_download_t *rsp;
    gfs_uint32               len, bnum, i, b;
    gfs_void                *pdata;
    gfs_sql_block_t         *blist;
    gfs_net_t               *net;
    gfs_net_t               *c;
    gfs_uint32               dsnum;
    gfs_int32                bindex;

    rsp = (gfs_busi_rsp_download_t*)busi->rsh;
    blist = (gfs_sql_block_t*)busi->data;
    bnum = ntohl(rsp->bnum);

	c = busi->c;
    buf = &busi->sbuf;
    buf->pos = htonl(busi->rsh->len);
    buf->len = buf->pos;
    if (bnum == 0) {
        len = buf->len;
    }
    else if (bnum > 0) {
        len = bnum*24 + 36;
    }

	/* { 在此分配发送缓冲区 */
    pdata = gfs_mem_malloc(busi->pool, len);
    if (!pdata)
    {
        gfs_mem_free(blist);
        return GFS_BUSI_ERR;
    }

    buf->data = pdata;
	/* } */
	
    memcpy(pdata, busi->rsh, 20);
    pdata += 20;
    
    memcpy(pdata, &rsp->fid, 8);
    pdata += 8;

    rsp->blist = (gfs_busi_binfo_t*)(pdata + 4);

    dsnum = b = 0;
    bindex = 0;
    for (i = 0; i < bnum; i++)
    {
        if (dsnum >= gfs_global->copies)
        {
            if (rsp->blist[b-1].bindex == htonl(blist[i].bindex))
                continue;
            else
                dsnum = 0;
        }

        if (b > 150)
            break;

        rsp->blist[b].bindex = htonl(blist[i].bindex);
        net = gfs_net_map_get(blist[i].dsid);
        if (net == NULL) {
        	glog_info("download from:%s fd:%d id:%u can not get dsid:%u from map",
        			  c->addr_text, c->fd, ntohl(rsp->head.id), blist[i].dsid);
            continue;
        }

        memcpy(rsp->blist[b].ip, &net->addr.sin_addr.s_addr, 4);
        rsp->blist[b].port = htonl(net->user.port);//net->addr.sin_port<<16;

        b++;

        if (bindex != blist[i].bindex)
        {
            bindex = blist[i].bindex;
            dsnum = 1;
        }
        else
            dsnum++;

    }

    rsp->bnum = htonl(b);
    memcpy(pdata, &rsp->bnum, 4);
    pdata += 4;
    len = b*24;         //24=sizeof gfs_busi_binfo_t
    pdata += len;
    memset(pdata, 0, 4);

    len  += ntohl(rsp->head.len);
    rsp->head.len = htonl(len);
    memcpy(buf->data, &rsp->head.len, 4);
    buf->len = buf->pos = len;

    gfs_mem_free(blist);
    
    glog_info("download from:%s fd:%d id:%u, bnum:%u send ok",
    		  c->addr_text, c->fd, ntohl(rsp->head.id), b);

    return GFS_OK;
}
