#ifndef __GFS_EVENT_H__
#define __GFS_EVENT_H__
#include <gfs_config.h>
#include <gfs_queue.h>
#include <gfs_timer.h>
#include <gfs_mem.h>

typedef struct gfs_event_s gfs_event_t;
typedef struct gfs_event_loop_s gfs_event_loop_t;

struct gfs_event_s
{
    gfs_int32               *pfd;
    gfs_int32                flag;
    gfs_int32                ready;
    gfs_int32                isgfs;
    gfs_queue_t              qnode;
    gfs_timer_t              timer;
    gfs_void                *data;
    gfs_event_loop_t        *loop;
    gfs_handle_fun_ptr       read;
    gfs_handle_fun_ptr       write;
    gfs_handle_fun_ptr       close;
};

struct gfs_event_loop_s
{
    gfs_int32           epfd;
    struct epoll_event *epev;
    gfs_uint32          evnum;
    gfs_queue_t         delay;
    gfs_timer_loop_t   *tloop;
};

gfs_event_loop_t    *gfs_event_loop_create(gfs_mem_pool_t *pool, gfs_uint32 num);
gfs_void             gfs_event_loop_destory(gfs_event_loop_t *loop);
gfs_int32            gfs_event_loop_run(gfs_event_loop_t *loop);

gfs_int32            gfs_event_add(gfs_event_t *ev, gfs_int32 flag);
gfs_int32            gfs_event_del(gfs_event_t *ev);


#endif

