#include <gfs_config.h>
#include <gfs_global.h>
#include <gfs_event.h>
#include <gfs_net.h>
#include <gfs_net_tcp.h>
#include <gfs_busi.h>
#include <gfs_net.h>
#include <gfs_net_tcp.h>
#include <gfs_event.h>
#include <gfs_log.h>
#include <gfs_handle.h>


gfs_int32   gfs_handle_init()
{
    gfs_net_tcpser_t    *gfs;
    gfs_event_t         *ev;

    gfs = gfs_global->tcpser;
    ev = &gfs->ev;
    ev->ready = GFS_NET_READY;
    ev->isgfs = gfs_true;
    ev->data = gfs;
    ev->pfd = &gfs->fd;
    ev->loop = gfs_global->eloop;
    ev->read = (gfs_handle_fun_ptr)gfs_net_tcp_accept;
    ev->write = NULL;
    ev->close = NULL;
    gfs->handle = (gfs_handle_fun_ptr)gfs_handle_tcp;

    gfs_event_add(ev, GFS_EVENT_READ);

    return GFS_OK;
}

gfs_int32 gfs_handle_tcp(gfs_net_t *c)
{
    gfs_event_t     *ev;

    ev = &c->ev;
    ev->pfd = &c->fd;
    ev->data = c;
    ev->ready = GFS_NET_READY;
    ev->isgfs = gfs_false;
    ev->loop = gfs_global->eloop;
    ev->read = (gfs_handle_fun_ptr)gfs_handle_tcp_recv;
    ev->write = (gfs_handle_fun_ptr)gfs_handle_tcp_send;
    ev->close = (gfs_handle_fun_ptr)gfs_handle_tcp_close;

    c->parse = gfs_net_packet_parse_tcp;
    c->handle = (gfs_handle_fun_ptr)gfs_handle_tcp_parse;

    gfs_event_add(ev, GFS_EVENT_READ | GFS_EVENT_ET);

    return GFS_OK;
}

gfs_int32   gfs_handle_tcp_recv(gfs_event_t *ev)
{
    gfs_net_t       *c;
    gfs_int32        rtn;

    c = (gfs_net_t*)ev->data;
    rtn = gfs_net_tcp_recv(c);
    if (rtn == GFS_NET_CLOSE) {
    	/* { modify by chenqianwu */
        // ev->close(ev);
        /* } */
        gfs_handle_tcp_close(ev);
    }
    else {
    	/* { modify by chenqianwu */
        // c->handle(c);
        /* } */
        gfs_handle_tcp_parse(c);
    }
        

    return rtn;
}

gfs_int32   gfs_handle_tcp_send(gfs_event_t *ev)
{
    gfs_net_t       *c;
    gfs_int32        rtn;
    gfs_net_buf_t   *sbuf;

    c = (gfs_net_t*)ev->data;
    sbuf = c->sbuf;
    rtn = gfs_net_tcp_send(c);
    if (rtn == GFS_NET_CLOSE) {
    	/* { modify by chenqianwu */
        // ev->close(ev);
        /* } */
        gfs_handle_tcp_close(ev);
    }
    else if (rtn == GFS_NET_AGAIN)
    {
        c->ev.ready &= ~GFS_NET_READY;
        gfs_event_add(ev, GFS_EVENT_WRITE | GFS_EVENT_ET);
    }
    else if (rtn == GFS_OK && !(c->ev.ready&GFS_NET_READY))
    {
        c->ev.ready |= GFS_NET_READY;
        gfs_event_add(ev, GFS_EVENT_READ | GFS_EVENT_ET);
    }

    if (ev->ready & GFS_EVENT_WRITE)
    {
        ev->ready &= ~GFS_EVENT_WRITE;
        c->handle(c);
    }

    memmove(sbuf->data, sbuf->data + sbuf->hd, sbuf->pos - sbuf->hd);
    sbuf->pos -= sbuf->hd;
    sbuf->hd = 0;

    return rtn;
}

gfs_int32   gfs_handle_tcp_close(gfs_event_t *ev)
{
    gfs_net_t       *c;

    c = (gfs_net_t*)ev->data;
    gfs_net_close(c);

    return GFS_OK;
}

gfs_int32   gfs_handle_tcp_parse(gfs_net_t *c)
{
	gfs_net_buf_t       *rbuf, *sbuf;
	gfs_char            *packet;
	gfs_int32            plen, rtn, ret;

	rbuf = c->rbuf;
	sbuf = c->sbuf;
    
    while (1)
    {
        plen = 0;
        packet = NULL;

		rtn = gfs_net_packet_parse_tcp(rbuf, &packet, &plen);
        /* { modify by chenqianwu */
        // rtn = c->parse(rbuf, &packet, &plen);
		/* } */
        
        rbuf->hd += rtn;
        if (!packet)
            break;
            
        ret = gfs_busi_handle(c, packet, plen);
		if (GFS_OK != ret) {
			return GFS_OK;
		}
        
    }

    if (rbuf->hd > rbuf->pos || rbuf->pos > rbuf->len || rbuf->hd > rbuf->len)
    {
        glog_error("tcp rbuf error. pos %d, hd %d, len %d\n", rbuf->pos, rbuf->hd, rbuf->len);
        rbuf->pos = rbuf->hd = 0;
    }
    else
    {
        memmove(rbuf->data, rbuf->data + rbuf->hd, rbuf->pos - rbuf->hd);
        rbuf->pos -= rbuf->hd;
        rbuf->hd = 0;
    }

    return GFS_OK;
}
