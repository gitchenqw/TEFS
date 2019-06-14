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

#define MAX_COPIES  10

typedef struct dsblist_s
{
    gfs_uint64      fid;
    gfs_int32       bindex;
    gfs_uint32      dsid[MAX_COPIES];
    gfs_uint32      dsnum;
}dsblist_t;

gfs_int32 gfs_busi_notice_delete_block(gfs_uint32 dsid, gfs_uint64 fid, gfs_int32 bindex)
{
    gfs_net_t               *net;
    gfs_uchar               msg[32];
    gfs_net_buf_t           buf;
    gfs_uint32             *pmsg;
    gfs_uint64              nfid;
    gfs_int32               nbindex, ret;
    gfs_event_t            *ev;

    net = gfs_net_map_get(dsid);
    if(net == NULL) {
        return GFS_BUSI_ERR;
    }

    ret = gfs_mysql_set_deling(fid, bindex, dsid);
    if (ret != SQL_OK) {
        return GFS_BUSI_ERR;
    }

    buf.pos = buf.len = 32;
    buf.hd = 0;
    buf.data = msg;

    ev = &net->ev;
    nfid = htonll(fid);
    nbindex = htonl(bindex);

    pmsg = (gfs_uint32*)msg;
    pmsg[0] = htonl(32);
    pmsg[1] = htonl(time(NULL));
    pmsg[2] = htonl(0x05);
    pmsg[3] = htonl(0);
    memcpy(&pmsg[4], &nfid, 8);
    memcpy(&pmsg[6], &nbindex, 4);

    pmsg[7] = htonl(0);

    while (buf.hd < buf.pos)
    {
        gfs_net_memcopy(net->sbuf, &buf);
        gfs_handle_tcp_send(ev);
        glog_notice("send to:%s fd:%d type:%02x success", net->addr_text,
        		    net->fd, 0x05);
		/* { modify by chenqianwu */
        // ev->write(ev);
        /* } */
    }

    return GFS_OK;
}

gfs_int32       gfs_busi_notice_copy_block(gfs_uint32 dsid_src, gfs_net_t *c_dst, gfs_uint64 fid, gfs_int32 bindex)
{
    gfs_net_t               *net;
    gfs_uchar               msg[72];
    gfs_net_buf_t           buf;
    gfs_uint32             *pmsg;
    gfs_uint64              nfid;
    gfs_int32               nbindex;
    gfs_event_t            *ev;

    net = gfs_net_map_get(dsid_src);
    if(net == NULL)
        return GFS_BUSI_ERR;

    buf.pos = buf.len = 72;
    buf.hd = 0;
    buf.data = msg;

    ev = &net->ev;
    nfid = htonll(fid);
    nbindex = htonl(bindex);

    pmsg = (gfs_uint32*)msg;
    pmsg[0] = htonl(72);
    pmsg[1] = htonl(time(NULL));
    pmsg[2] = htonl(0x07);
    pmsg[3] = htonl(0);
    memcpy(&pmsg[4], &nfid, 8);
    memcpy(&pmsg[6], &nbindex, 4);
    memcpy(&pmsg[7],  &net->addr.sin_addr.s_addr, 4);
    pmsg[11] = htonl(net->user.port);
    memcpy(&pmsg[12],  &c_dst->addr.sin_addr.s_addr, 4);
    pmsg[16] = htonl(c_dst->user.port);
    pmsg[17] = htonl(0);

    while (buf.hd < buf.pos)
    {
        gfs_net_memcopy(net->sbuf, &buf);
        gfs_handle_tcp_send(ev);

        glog_notice("send to:%s fd:%d type:%02x success", net->addr_text,
        		    net->fd, 0x07);
		/* { modify by chenqianwu */
        // ev->write(ev);
        /* } */
    }

    return GFS_OK;
}

gfs_int32       gfs_busi_check_del_block()
{
    gfs_sql_block_t      blist[50];
    gfs_uint32           bnum, ret;
    gfs_uint32           i, j, isfind, num;
    dsblist_t            dsblist[50];
    gfs_uint32           dsbnum;

    ret = gfs_mysql_get_delblock(0, gfs_global->copies, blist, &bnum);
    if (ret != SQL_OK || bnum < 1) { /* bnum±íÊ¾´ýÉ¾³ýµÄ¿é */
        return GFS_ERR;
	}
	
    for (i = 0; i < bnum; i++) {
        gfs_busi_notice_delete_block(blist[i].dsid, blist[i].fid, blist[i].bindex);
    }

    ret = gfs_mysql_get_delblock(1, gfs_global->copies, blist, &bnum);
    if (ret != SQL_OK || bnum < 1) {
        return GFS_ERR;
    }

    dsbnum = 0;
    for (i = 0; i < bnum; i++)
    {
        isfind = 0;
        for (j = 0; j < dsbnum; j++) {
            if (blist[i].fid == dsblist[j].fid && blist[i].bindex == dsblist[j].bindex)
            {
                isfind = 1;
                dsblist[j].dsid[dsblist[j].dsnum++] = blist[i].dsid;
                break;
            }
        }
        
        if (!isfind)
        {
            dsblist[dsbnum].fid = blist[i].fid;
            dsblist[dsbnum].bindex = blist[i].bindex;
            dsblist[dsbnum].dsnum = 0;
            dsblist[dsbnum].dsid[dsblist[dsbnum].dsnum++] = blist[i].dsid;
            dsbnum++;
        }
    }

    for (i = 0; i < dsbnum; i++)
    {
        num = dsblist[i].dsnum - gfs_global->copies;
        for (j = 0; j < num; j++)
            gfs_busi_notice_delete_block(dsblist[i].dsid[j], dsblist[i].fid, dsblist[i].bindex);
    }

    return GFS_OK;
}

//gfs_sql_block_t **pblist, gfs_uint32 pbnum
gfs_void        gfs_busi_cpys_handle(dsblist_t *dsb, gfs_uint32 copies, gfs_net_t **pnet, gfs_uint32 pnnum)
{
    gfs_int32       i, j, num, exist;

    num = copies - dsb->dsnum;
    for (i = 0; i< pnnum; i++)
    {
        if (num <1)
            break;

        exist = 0;
        for (j = 0; j < dsb->dsnum; j++)
        {
            if (dsb->dsid[j] == pnet[i]->user.dsid)
            {
                exist = 1;
                break;
            }
        }

        if (exist)
            continue;

        gfs_busi_notice_copy_block(dsb->dsid[0], pnet[i], dsb->fid, dsb->bindex);
        num--;
    }
}

gfs_int32       gfs_busi_check_cpy_block()
{
    gfs_sql_block_t      blist[50];
    gfs_uint32           bnum, num, ret;
    gfs_net_t*           pnet[10];
    gfs_uint32           i, j, isfind;
    dsblist_t            dsblist[50];
    gfs_uint32           dsbnum;

    bnum = 0;

    ret = gfs_mysql_get_cpyblock(gfs_global->copies, blist, &bnum);
    if (ret != SQL_OK || bnum < 1) {
        return GFS_ERR;
    }

    num = 10;
    ret = gfs_net_map_gettopn((gfs_net_t**)(&pnet), &num);
    if (ret != GFS_OK || num < 2) {
        return GFS_ERR;
    }

    dsbnum = 0;
    for (i = 0; i < bnum; i++)
    {
        isfind = 0;
        for (j = 0; j < dsbnum; j++)
        {
            if (blist[i].fid == dsblist[j].fid && blist[i].bindex == dsblist[j].bindex)
            {
                isfind = 1;
                dsblist[j].dsid[dsblist[j].dsnum++] = blist[i].dsid;
                break;
            }
        }
        if (!isfind)
        {
            dsblist[dsbnum].fid = blist[i].fid;
            dsblist[dsbnum].bindex = blist[i].bindex;
            dsblist[dsbnum].dsnum = 0;
            dsblist[dsbnum].dsid[dsblist[dsbnum].dsnum++] = blist[i].dsid;
            dsbnum++;
        }
    }

    for (i = 0; i < dsbnum; i++) {
        gfs_busi_cpys_handle(&dsblist[i], gfs_global->copies, pnet, num);
    }

    return GFS_OK;
}

gfs_int32 gfs_busi_notice_handle(gfs_void *data)
{
    gfs_busi_check_cpy_block();
    gfs_busi_check_del_block();
    return GFS_OK;
}
