#include <gfs_config.h>
#include <gfs_global.h>
#include <gfs_log.h>
#include <gfs_event.h>
#include <gfs_mem.h>
#include <gfs_tools.h>
#include <gfs_net_map.h>
#include <gfs_net.h>

gfs_net_loop_t* gfs_net_pool_create(gfs_mem_pool_t *pool, gfs_uint32 num)
{
	gfs_net_loop_t      *loop;
	gfs_mem_pool_t      *p;
	gfs_net_t           *c;
	gfs_net_buf_t       *buf;
	gfs_int32            i, size;

    gfs_errno = 0;

    loop = gfs_mem_malloc(pool, sizeof(gfs_net_loop_t));
    if (NULL == loop) {
    	glog_error("gfs_mem_malloc(%p, %lu) failed", pool,
    			   sizeof(gfs_net_loop_t));
        gfs_errno = GFS_MEMPOOL_MALLOC_ERR;
        return NULL;
    }

    loop->net_num = num;
    loop->used_num = 0;
    loop->free_num = num;
    gfs_queue_init(&loop->free_list);
    gfs_queue_init(&loop->used_list);

    size = sizeof(gfs_net_t);
    for (i = 0; i < num; i++)
    {
        c = gfs_mem_malloc(pool, size);
        if (NULL == c) {
			glog_error("gfs_mem_malloc(%p, %d) failed", pool, size);
            gfs_errno = GFS_MEMPOOL_MALLOC_ERR;
            return NULL;
        }

        memset(c, 0, size);
        c->fd = -1;
        c->ev.data = c;
        c->ev.pfd = &c->fd;
        c->loop = loop;
        gfs_queue_insert_tail(&loop->free_list, &c->qnode);

        p = gfs_mem_pool_create(10);
        if(NULL == p) {
        	glog_error("%s", "gfs_mem_pool_create(10) failed");
            gfs_errno = GFS_MEMPOOL_CREATE_ERR;
            return NULL;
        }
        c->pool = p;

        buf = gfs_mem_malloc(pool, GFS_NET_RECV_BUFFER_SIZE + sizeof(gfs_net_buf_t));
        if (NULL == buf) {
        	glog_error("gfs_mem_malloc(%p, %lu) failed", pool, 
        			   GFS_NET_RECV_BUFFER_SIZE + sizeof(gfs_net_buf_t));
            gfs_errno = GFS_MEMPOOL_MALLOC_ERR;
            return NULL;
        }
        buf->data = (char*)buf + sizeof(gfs_net_buf_t);
        buf->len = GFS_NET_RECV_BUFFER_SIZE;
        c->rbuf = buf;

        buf = gfs_mem_malloc(pool, GFS_NET_SEND_BUFFER_SIZE + sizeof(gfs_net_buf_t));
        if (NULL == buf) {
        	glog_error("gfs_mem_malloc(%p, %lu) failed", pool, 
        			   GFS_NET_SEND_BUFFER_SIZE + sizeof(gfs_net_buf_t));
            gfs_errno = GFS_MEMPOOL_MALLOC_ERR;
            return NULL;
        }
        buf->data = (char*)buf + sizeof(gfs_net_buf_t);
        buf->len = GFS_NET_SEND_BUFFER_SIZE;
        c->sbuf = buf;
    }

    glog_debug("gfs_net_pool_create(%p) success", loop);

    return loop;
}

gfs_void        gfs_net_pool_destory(gfs_net_loop_t *loop)
{
    gfs_net_t       *c;
    gfs_queue_t     *q;
    gfs_void        *p;

    if (!loop)
        return;

    p = loop;
    for (q = gfs_queue_head(&loop->free_list); q != gfs_queue_sentinel(&loop->free_list); q = gfs_queue_next(q))
    {
        c = gfs_queue_data(q, gfs_net_t, qnode);
        gfs_mem_pool_destory(c->pool);
        gfs_mem_free(c);
    }
    for (q = gfs_queue_head(&loop->used_list); q != gfs_queue_sentinel(&loop->used_list); q = gfs_queue_next(q))
    {
        c = gfs_queue_data(q, gfs_net_t, qnode);
        gfs_mem_pool_destory(c->pool);
        gfs_mem_free(c);
    }

    gfs_mem_free(loop);
    glog_info("gfs_net_pool_destory(%p) success", p);
}

static gfs_int32 gfs_net_timer_handle(gfs_void *data)
{
    gfs_net_t   *c;
    gfs_uint64   now;

    c = (gfs_net_t*)data;
    now = gfs_mstime();
    if (now >= c->t_end)
    {
        glog_info("<%s:%d fd:%d> The connection without recv data within %ds, will disconnection.", 
            inet_ntoa(c->addr.sin_addr), ntohs(c->addr.sin_port), c->fd, GFS_CONN_TIMEOUT_MS/1000);
        gfs_net_close(c);
    }

    return GFS_OK;
}

gfs_net_t*      gfs_net_node_get(gfs_net_loop_t *loop, gfs_int32 fd)
{
    gfs_net_t       *c;
    gfs_queue_t     *q;

    if (!loop || gfs_queue_empty(&loop->free_list))
        return NULL;

    q = gfs_queue_head(&loop->free_list);
    gfs_queue_remove(q);
    gfs_queue_insert_head(&loop->used_list, q);
    c = gfs_queue_data(q, gfs_net_t, qnode);

    loop->used_num++;
    loop->free_num--;

    c->fd = fd;
    c->t_start = gfs_mstime();
    c->t_end = c->t_start + GFS_CONN_TIMEOUT_MS;         //two minutes

    memset(&c->ev, 0, sizeof(gfs_event_t));
    c->ev.ready = GFS_NET_READY;
    c->ev.pfd = &c->fd;
    c->ev.data = c;

    memset(&c->user, 0, sizeof(gfs_net_data_t));
    gfs_timer_set(&c->timer, 10000, -1, gfs_net_timer_handle, c);
    gfs_timer_add(gfs_global->eloop->tloop, &c->timer);

    return c;
}

gfs_int32       gfs_net_close(gfs_net_t *c)
{
    gfs_net_loop_t  *loop;
    if (!c)
        return GFS_PARAM_FUN_ERR;

    glog_info("connection for:%s fd:%d closed by peer", c->addr_text, c->fd);
    
    loop = c->loop;

    gfs_event_del(&c->ev);
    close(c->fd);
    memset(&c->addr, 0, sizeof(struct sockaddr_in));
    c->rbuf->hd = c->rbuf->pos = 0;
    c->sbuf->hd = c->sbuf->pos = 0;
    gfs_mem_pool_reset(c->pool);
    c->data = NULL;
    c->fd = -1;
    gfs_timer_del(gfs_global->eloop->tloop, &c->timer);

    gfs_queue_remove(&c->qnode);
    gfs_queue_insert_head(&loop->free_list, &c->qnode);

    if (c->user.ctype == GFS_CLIENT_TYPE_DS)
        gfs_net_map_remove(c->user.dsid);

    gfs_global->client_num--;

    return GFS_OK;
}

gfs_int32 gfs_net_fd_noblock(gfs_int32 fd)
{
	gfs_int32 flags;

    if ((flags = fcntl(fd, F_GETFL)) == -1) {
    	glog_error("fcntl(%d, F_GETFL) failed, error:%s", fd, strerror(errno));
        return GFS_NFD_ERR;
    }
    
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
    	glog_error("fcntl(%d, F_SETFL, %d | O_NONBLOCK) failed, error:%s",
    			   fd, flags, strerror(errno));
        return GFS_NFD_ERR;
    }

    return GFS_OK;
}

gfs_int32       gfs_net_fd_reuse(gfs_int32 fd)
{
    gfs_int32   flag = 1;

    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(gfs_int32)) == -1)
    {
    	glog_error("setsockopt(%d) failed, error:%s", fd, strerror(errno));
        return GFS_NFD_ERR;
    }

    return GFS_OK;
}

gfs_int32       gfs_net_packet_parse_tcp(gfs_net_buf_t *buf, gfs_char **packet, gfs_int32 *plen)
{
    gfs_char        *pdata;
    gfs_char        *pend;
    gfs_uint32       len, type, size, sprt;
    gfs_uint32      *pu;
    gfs_int32        rtn; /* 扫描buffer时 跳过的字节数 */

    pdata = buf->data + buf->hd;
    pend = buf->data + buf->pos;
    size = pend - pdata;
    rtn = 0;
    pu = (gfs_uint32*)pdata;

    while(pdata < pend)
    {
    	/* len-4byte id-4byte type-4byte encrypt-4byte*/
        len = ntohl(pu[0]);
        type = ntohl(pu[2]);

		/* 包长度不合法 跳过 */
        if (len > 2048 || len < 16)
        {
            pdata++;
            rtn++;
            continue;
        }

        if (rtn + len > size)
        {
            if (buf->pos < buf->len)
            {
                pdata++;
                rtn++;
                continue;
            }
            else if (buf->pos == buf->len)
            {
                *packet = NULL;
                *plen = 0;
            }
            else
            {
                *packet = NULL;
                *plen = 0;
                rtn = buf->pos - buf->hd;
            }
        }
        else
        {	/* 检查分隔符 */
            sprt = ntohl(*(gfs_uint32*)(pdata+len-4));
            if(sprt != 0) 
            {
                pdata++;
                rtn++;
                continue;
            }
            else
            {
                *packet = pdata;
                *plen = len;
                rtn += len;
            }
        }

        return rtn;
    }

    *packet = NULL;
    *plen = 0;
    rtn = buf->pos - buf->hd;

    return rtn;
}

gfs_int32           gfs_net_memcopy(gfs_net_buf_t *dst, gfs_net_buf_t *src)
{
    gfs_int32 remain, copylen;

    if(!src || !dst)
    {
        return GFS_ERR;
    }

    remain = dst->len - dst->pos;
    copylen = src->pos - src->hd;

    if(remain > copylen)
        remain = copylen;

    memcpy(dst->data + dst->pos, src->data + src->hd, remain);
    dst->pos += remain;
    src->hd += remain;

    return GFS_OK;
}
