

#ifndef _GVFS_EVENT_H_INCLUDED_
#define _GVFS_EVENT_H_INCLUDED_


#include <gvfs_config.h>
#include <gvfs_core.h>


struct gvfs_event_s {
	void                  *data;              /* 指向gvfs_connection_t */
    unsigned               write:1;           /* 标志位:为1时表示连接处于可发生数据状态 */
    unsigned         	   accept:1;          /* 标志位:为1时表示此事件可以建立新的连接 */
    unsigned               instance:1;        /* 标志位:用于区分事件是否为过期事件 */
    unsigned               active:1;          /* 标志位:为1时表示事件是活跃的 */
    unsigned               ready:1;           /* 标志位:为1时表示事件已经准备就绪*/
    unsigned               eof:1;             /* 标志位:为1时表示当前的字节流已经结束 */
    unsigned               error:1;           /* 标志位:为1时表示在处理过程中遇到错误 */
    unsigned               timedout:1;        /* 标志位:表示这个事件已经超时 */
    unsigned               deferred_accept:1; /* 标志位:延迟建立连接 */
    unsigned               channel:1;         /* 标志位:表示该事件是UNIX协议域套接字 */
    unsigned               timer_set:1;       /* 标志位:表示该事件在定时器中 */
    gvfs_event_handler_pt  handler;           /* 分阶段处理逻辑的函数指针 */
    gvfs_rbtree_node_t     timer;             /* 红黑树节点,由于构成红黑树定时器 */
    struct gvfs_list_head  lh;                /* 链接节点,用于构成事件处理队列 */
};

typedef struct {
    gvfs_uint_t    connections;
    gvfs_uint_t    events;
} gvfs_event_conf_t;


extern gvfs_atomic_t         *gvfs_connection_counter;
extern sig_atomic_t           gvfs_event_timer_alarm;

extern gvfs_atomic_t         *gvfs_accept_mutex_ptr;
extern gvfs_shmtx_t           gvfs_accept_mutex;
extern gvfs_uint_t            gvfs_use_accept_mutex;
extern gvfs_uint_t            gvfs_accept_mutex_held;


extern gvfs_module_t           gvfs_event_core_module;

extern gvfs_event_conf_t     *g_gvfs_ecf;


#define GVFS_READ_EVENT     EPOLLIN     /* 00000001 */
#define GVFS_WRITE_EVENT    EPOLLOUT    /* 00000004 */
#define GVFS_LEVEL_EVENT    0           
#define GVFS_CLEAR_EVENT    EPOLLET     /* 80000000 */

#define GVFS_CLOSE_EVENT    1


#define GVFS_UPDATE_TIME    1
#define GVFS_POST_EVENTS    2


void gvfs_process_events_and_timers(gvfs_cycle_t *cycle);


gvfs_int_t gvfs_epoll_init(gvfs_cycle_t *cycle);
void gvfs_epoll_done(gvfs_cycle_t *cycle);


gvfs_int_t gvfs_epoll_add_event(gvfs_event_t *ev, gvfs_int_t event,
	gvfs_uint_t flags);
gvfs_int_t gvfs_epoll_del_event(gvfs_event_t *ev, gvfs_int_t event,
	gvfs_uint_t flags);

gvfs_int_t gvfs_epoll_add_connection(gvfs_connection_t *c);
gvfs_int_t gvfs_epoll_del_connection(gvfs_connection_t *c,
	gvfs_uint_t flags);

gvfs_int_t gvfs_epoll_process_events(gvfs_cycle_t *cycle,
	gvfs_uint_t timer, gvfs_uint_t flags);


void gvfs_event_accept(gvfs_event_t *ev);
gvfs_int_t gvfs_trylock_accept_mutex(gvfs_cycle_t *cycle);


#endif /* _GVFS_EVENT_H_INCLUDED_ */
