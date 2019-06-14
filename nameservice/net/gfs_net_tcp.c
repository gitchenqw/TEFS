#include <gfs_config.h>
#include <gfs_net.h>
#include <gfs_event.h>
#include <gfs_log.h>
#include <gfs_global.h>
#include <gfs_tools.h>
#include <gfs_net_tcp.h>
#include <gfs_handle.h>

gfs_net_tcpser_t* gfs_net_tcp_listen(gfs_mem_pool_t *pool, gfs_uint16 port,
	gfs_char *bindaddr)
{
	gfs_int32           fd;
	struct sockaddr_in  addr;
	gfs_net_tcpser_t   *gfs;

	fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd == -1) {
    	glog_error("socket() failed, error:%s", strerror(errno));
    	return NULL;
    }

    if (GFS_OK != gfs_net_fd_noblock(fd)) {
    	glog_error("gfs_net_fd_noblock(%d) failed", fd);
    	return NULL;
    }
    
    if (GFS_OK != gfs_net_fd_reuse(fd)) {
    	glog_error("gfs_net_fd_reuse(%d) failed", fd);
    	return NULL;
    }

    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(bindaddr);
    if (bind(fd, (struct sockaddr*)&addr, sizeof(addr)) == -1)
    {
        close(fd);
        glog_error("bind(%d) failed, error:%s", fd, strerror(errno));
        return NULL;
    }

    if (listen(fd, gfs_global->max_num * 2) == -1)
    {
        close(fd);
        glog_error("listen(%d, %d) failed, error:%s", fd,
        		   gfs_global->max_num *2, strerror(errno));
        return NULL;
    }

    gfs = gfs_mem_malloc(pool, sizeof(gfs_net_tcpser_t));
    if (!gfs)
    {
        close(fd);
        glog_error("gfs_mem_malloc(%p, %lu) failed", pool,
        		   sizeof(gfs_net_tcpser_t));
		return NULL;
    }

    gfs->fd = fd;
    strcpy(gfs->ip, bindaddr);
    gfs->handle = NULL;
    gfs->port = port;
    memset(&gfs->ev, 0, sizeof(gfs_event_t));

	glog_debug("listen(%s, %d) success", bindaddr, port);
	
    return gfs;
}

gfs_void    gfs_net_tcp_unliten(gfs_net_tcpser_t *gfs)
{
    if (!gfs)
        return;

    if (gfs->fd > 0) {
        gfs_event_del(&gfs->ev);
        close(gfs->fd);
        glog_info("close(%d) for listen port:%d", gfs->fd, gfs->port);
    }
    
    gfs_mem_free(gfs);
}


gfs_int32 gfs_net_tcp_accept(gfs_event_t *ev)
{
	struct sockaddr_in      addr;
	gfs_int32               fd, size;
	gfs_net_tcpser_t       *gfs;
	gfs_net_t              *c;

	gfs = (gfs_net_tcpser_t *)ev->data;
	size = sizeof(struct sockaddr_in);
    while (1)
    {
        fd = accept(gfs->fd, (struct sockaddr*)&addr, (socklen_t*)&size);
        if (fd == -1)
        {
            switch(errno)
            {
            case EAGAIN:
                return GFS_NET_AGAIN;
            case EINTR:
            case ECONNABORTED:
                continue;
            default:
            	glog_error("accept() failed, error:%s", strerror(errno));
                return GFS_NFD_ERR;
            }
        }

        if (gfs_global->client_num >= gfs_global->max_num) {
            close(fd);
            glog_warn("current connection:%d has reached max:%d, ignore",
            		  gfs_global->client_num, gfs_global->max_num);
            continue;
        }

        gfs_net_fd_noblock(fd);
        c = gfs_net_node_get(gfs_global->nloop, fd);
        if (NULL == c) {
            glog_warn("gfs_net_node_get(%p, %d) failed, for:%s, ignore",
            		  gfs_global->nloop, fd, inet_ntoa(addr.sin_addr));
            return GFS_NET_LOOP_UNUSED;
        }

        memcpy(&c->addr, &addr, sizeof(struct sockaddr_in));

		memset(c->addr_text, 0, sizeof(c->addr_text));
		if (NULL == inet_ntop(AF_INET, &(addr.sin_addr), c->addr_text, INET_ADDRSTRLEN))
		{
			glog_warn("inet_ntop() for:%s fd:%d failed, error:%s, ignore",
					  inet_ntoa(addr.sin_addr), fd, strerror(errno));
		}
		
        gfs_global->client_num++;
		glog_info("accept from:%s:%d fd:%d current connection:%d",
				  c->addr_text, ntohs(addr.sin_port), fd,
				  gfs_global->client_num);

		/* { modify by chenqianwu */
		// gfs->handle(c);
		/* } */
		
		gfs_handle_tcp(c);
    }

    return GFS_OK;
}

gfs_int32           gfs_net_tcp_recv(gfs_net_t *c)
{
    gfs_net_buf_t      *buf;
    gfs_int32           remain;
    gfs_int32           rlen;

    buf = c->rbuf;
    remain = buf->len - buf->pos;
    while(remain > 0)
    {
        rlen = recv(c->fd, buf->data + buf->pos, remain, 0);
        if (rlen == 0)
            return GFS_NET_CLOSE;
        else if (rlen == -1)
        {
            switch(errno)
            {
            case EINTR:
                continue;
            case EAGAIN:        //EWOULDBLOCK
                c->t_end = gfs_mstime() + GFS_CONN_TIMEOUT_MS;
                return GFS_NET_AGAIN;
            case ECONNRESET:
                glog_error("remote host:%s fd:%d reset the connection",
                		   c->addr_text, c->fd);
                return GFS_NET_CLOSE;
            case ECONNREFUSED:
                glog_error("remote host:%s fd:%d refused to allow the connection",
                		   c->addr_text, c->fd);
                return GFS_NET_CLOSE;
            default:
                glog_error("recv(fd:%d, len:%d) failed, error:%s", c->fd, remain,
                		   strerror(errno));
                return GFS_NET_CLOSE;
            }
        }
        
        glog_debug("recv %d bytes from:%s fd:%d", rlen, c->addr_text, c->fd);

        buf->pos += rlen;
        remain -= rlen;
    }

    c->t_end = gfs_mstime() + GFS_CONN_TIMEOUT_MS;
    return GFS_NET_OUTOF_MEMORY;
}

gfs_int32 gfs_net_tcp_send(gfs_net_t *c)
{
    gfs_net_buf_t       *buf;
    gfs_int32            rlen, remain;

    buf = c->sbuf;
    remain = buf->pos - buf->hd;
    while(remain > 0)
    {
        rlen = send(c->fd, buf->data + buf->hd, remain, MSG_NOSIGNAL);
        if (rlen == -1)
        {
            switch(errno)
            {
            case EINTR:
                continue;
            case EAGAIN: // EWOULDBLOCK
            	glog_warn("send(%d) failed, error:%s, ignore", c->fd,
            			  strerror(errno));
                return GFS_NET_AGAIN;
            case ECONNRESET:
            	glog_error("send(%d) failed, error:%s", c->fd, strerror(errno));
                return GFS_NET_CLOSE;
            case EPIPE:
            	glog_error("send(%d) failed, error:%s", c->fd, strerror(errno));
                return GFS_NET_CLOSE;
            default:
                glog_error("send(fd:%d size:%d)failed, error:%s\n", c->fd,
                		   remain, strerror(errno));
                return GFS_NET_CLOSE;
            }
        }
        
        glog_debug("send to:%s fd:%d size:%d success", c->addr_text, c->fd,
        		   rlen);
        		   
        buf->hd += rlen;
        remain -= rlen;
    }

    return GFS_OK;
}
