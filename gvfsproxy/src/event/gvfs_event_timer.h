

#ifndef _GVFS_EVENT_TIMER_H_INCLUDED_
#define _GVFS_EVENT_TIMER_H_INCLUDED_


#include <gvfs_config.h>
#include <gvfs_core.h>


#define GVFS_TIMER_INFINITE  (gvfs_msec_t) -1

#define GVFS_TIMER_LAZY_DELAY  300

VOID gvfs_event_timer_init();
VOID gvfs_event_expire_timers(VOID);


extern  gvfs_rbtree_t  gvfs_event_timer_rbtree;


void gvfs_event_del_timer(gvfs_event_t *ev);

void gvfs_event_add_timer(gvfs_event_t *ev, gvfs_msec_t timer);


#endif /* _GVFS_EVENT_TIMER_H_INCLUDED_ */
