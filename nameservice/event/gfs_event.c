#include <gfs_config.h>
#include <gfs_global.h>
#include <gfs_log.h>
#include <gfs_mem.h>
#include <gfs_net.h>
#include <gfs_net_tcp.h>
#include <gfs_net_map.h>
#include <gfs_event.h>
#include <gfs_handle.h>

gfs_event_loop_t  *gfs_event_loop_create(gfs_mem_pool_t *pool, gfs_uint32 num)
{
    gfs_event_loop_t    *loop;
    gfs_int32            size;
    struct epoll_event  *ev;

    gfs_errno = 0;

    pool = gfs_global->pool;
    size = sizeof(gfs_event_loop_t);
    loop = gfs_mem_malloc(pool, size);
    if (!loop)
    {
        glog_error("gfs_mem_malloc(%p, %d) failed", pool, size);
        gfs_errno = GFS_MEMPOOL_MALLOC_ERR;
        return NULL;
    }
    
    loop->evnum = num;
    size = sizeof(struct epoll_event)*loop->evnum;
    ev = gfs_mem_malloc(pool, size);
    if (!ev)
    {
        glog_error("gfs_mem_malloc(%p, %d) failed", pool, size);
        gfs_mem_free(loop);
        gfs_errno = GFS_MEMPOOL_MALLOC_ERR;
        return NULL;
    }
    loop->epev = ev;

    loop->epfd = epoll_create(1024);    //1024 is just an hint for the kernel
    if (loop->epfd < 0)
    {
        glog_error("epoll_create(1024) failed, error:%s", strerror(errno));
        gfs_mem_free(loop);
        gfs_mem_free(ev);
        gfs_errno = GFS_EPOLL_ERR;
        return NULL;
    }

    gfs_queue_init(&loop->delay);
    loop->tloop = gfs_timer_loop_create();
    if (!loop->tloop)
    {
        gfs_mem_free(loop);
        gfs_mem_free(ev);
        return NULL;
    }

    glog_debug("gfs_event_loop_create(%p) success", loop);

    return loop;
}

gfs_void gfs_event_loop_destory(gfs_event_loop_t *loop)
{
    gfs_void        *p;

    if (!loop)
        return;

    p = loop;
    close(loop->epfd);
    gfs_mem_free(loop->epev);
    gfs_timer_loop_destory(loop->tloop);
    gfs_mem_free(loop);
    glog_info("gfs_event_loop_destory(%p) success", p);
}

static gfs_void gfs_event_delay_run(gfs_event_loop_t *loop)
{
    gfs_queue_t     *q;
    gfs_event_t     *ev;
    gfs_int32        rtn;

    for (q = gfs_queue_head(&loop->delay); q != gfs_queue_sentinel(&loop->delay); q = gfs_queue_next(q))
    {
        ev = gfs_queue_data(q, gfs_event_t, qnode);
        if (ev->ready&GFS_NET_READY)
        {
            if (ev->ready&GFS_EVENT_WRITE)
            	/* { modify by chenqianwu */
                // ev->write(ev);
                /* } */
                gfs_handle_tcp_send(ev);
            if (ev->ready&GFS_EVENT_READ)
            {
            	/* { modify by chenqianwu */
                // rtn = ev->read(ev);
                /* } */
                rtn = gfs_handle_tcp_recv(ev);
                if (rtn == GFS_NET_OUTOF_MEMORY)
                    continue;
                else if(rtn == GFS_NET_AGAIN)
                    ev->ready &= ~GFS_EVENT_READ;
            }
            gfs_queue_remove(q);
        }
    }
}

gfs_int32 gfs_event_loop_run(gfs_event_loop_t *loop)
{
    gfs_int32       nfds, i, revents;
    gfs_event_t    *ev;

	/* 定时器事件的执行 */
    gfs_timer_loop_run(loop->tloop);

    /* 延迟队列事件的执行 */
    gfs_event_delay_run(loop);

    nfds = epoll_wait(loop->epfd, loop->epev, loop->evnum, 100);
    if (nfds == -1)
    {
        switch(errno)
        {
        case EBADF:
        case EFAULT:
        case EINVAL:
        	glog_error("epoll_wait() failed, error:%s", strerror(errno));
            return GFS_EPOLL_ERR;
        case EINTR:
            return GFS_EVENT_EPOLL_WAIT_INTR;
        }
    }
    else if (nfds == 0) {
        return GFS_EVENT_NOEVENTS;
    }

    for (i = 0; i < nfds; i++)
    {
        ev = loop->epev[i].data.ptr;
        revents = loop->epev[i].events;
        if(revents & (EPOLLERR | EPOLLHUP | EPOLLRDHUP))
        {
            glog_warn("epoll_wait() error on fd:%d ev:%08X", *ev->pfd, revents);
			/* { modify by chenqianwu */
			// ev->close(ev);
			/* } */
			gfs_handle_tcp_close(ev);
            continue;
        }

        if (ev->isgfs) { /* accept事件 */
        	/* { modify by chenqianwu */
            // ev->read(ev);
            /* } */
            
            gfs_net_tcp_accept(ev);
        }
        else /* 非accept事件 */
        {
            if (!(ev->ready&GFS_EVENT_READ))
            {
                gfs_queue_insert_tail(&ev->loop->delay, &ev->qnode);
            }

            if (revents & EPOLLIN)
                ev->ready |= GFS_EVENT_READ;
            if (revents & EPOLLOUT)
                ev->ready |= GFS_EVENT_WRITE;
        }
    }

    return GFS_OK;
}


gfs_int32 gfs_event_add(gfs_event_t *ev, gfs_int32 flag)
{
    struct epoll_event  ee;
    gfs_int32           op;

    op = (ev->flag == GFS_EVENT_NONE) ? EPOLL_CTL_ADD : EPOLL_CTL_MOD;
    ee.events = 0;
    if (flag & GFS_EVENT_READ)
        ee.events |= EPOLLIN;
    if (flag & GFS_EVENT_WRITE)
        ee.events |= EPOLLOUT;
    if (flag & GFS_EVENT_ET)
        ee.events |= EPOLLET;

    ee.data.ptr = ev;
    ev->flag = ee.events;
    if (epoll_ctl(ev->loop->epfd, op, *ev->pfd, &ee) == -1)
    {
		glog_error("epoll_ctl(\"%s\", %d) failed, error: %s",
				   op == EPOLL_CTL_MOD ? "MOD" : "ADD",
				   *ev->pfd, strerror(errno));
        return GFS_EPOLL_ERR;
    }

    return GFS_OK;
}

gfs_int32 gfs_event_del(gfs_event_t *ev)
{
    if (epoll_ctl(ev->loop->epfd, EPOLL_CTL_DEL, *ev->pfd, 0) == -1)
    {
		glog_error("epoll_ctl(\"EPOLL_CTL_DEL\", %d) failed, error: %s",
				   *ev->pfd, strerror(errno));
        return GFS_EPOLL_ERR;
    }

    return GFS_OK;
}
