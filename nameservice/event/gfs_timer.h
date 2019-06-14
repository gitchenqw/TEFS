#ifndef __GFS_TIMER_H__
#define __GFS_TIMER_H__
#include <gfs_config.h>
#include <gfs_rbtree.h>

typedef struct gfs_timer_s
{
    gfs_rbtree_node_t       node;
    gfs_int32               secs;       
    gfs_int32               times;      //run times: -1 unlimited times
    gfs_uint32              now;        //now time;
    gfs_handle_fun_ptr      handle;
    gfs_void               *param;
    gfs_void               *data;
}gfs_timer_t;

typedef struct gfs_timer_loop_s
{
    gfs_rbtree_t            gfsree;
    gfs_rbtree_node_t       sentinel;
    gfs_timer_t            *list;
    gfs_uint32              num;
}gfs_timer_loop_t;

gfs_timer_loop_t*   gfs_timer_loop_create();
gfs_void            gfs_timer_loop_destory(gfs_timer_loop_t* loop);
gfs_void            gfs_timer_loop_run(gfs_timer_loop_t* loop);


//msecs must be greater than 10
gfs_void    gfs_timer_set(gfs_timer_t *timer, gfs_int32 msecs, gfs_int32 times, gfs_handle_fun_ptr handle, gfs_void *param);
gfs_int32   gfs_timer_add(gfs_timer_loop_t* loop, gfs_timer_t *timer);
gfs_void    gfs_timer_del(gfs_timer_loop_t* loop, gfs_timer_t *timer);

#endif