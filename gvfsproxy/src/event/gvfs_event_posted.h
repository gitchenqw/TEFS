

#ifndef _GVFS_EVENT_POSTED_H_INCLUDED_
#define _GVFS_EVENT_POSTED_H_INCLUDED_


#include <gvfs_config.h>
#include <gvfs_core.h>
#include <gvfs_event.h>


#define gvfs_locked_post_event(ev, queue) \
	gvfs_list_add_tail(&ev->lh, queue)
									 
#define gvfs_delete_posted_event(ev) \
	list_del(&ev->lh)

void gvfs_event_process_posted(struct gvfs_list_head *posted);


extern struct gvfs_list_head gvfs_posted_accept_events;
extern struct gvfs_list_head gvfs_posted_events;


#endif /* _GVFS_EVENT_POSTED_H_INCLUDED_ */
