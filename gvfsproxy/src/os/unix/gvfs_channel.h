
#ifndef _GVFS_CHANNEL_H_INCLUDED_
#define _GVFS_CHANNEL_H_INCLUDED_


#include <gvfs_config.h>
#include <gvfs_core.h>
#include <gvfs_event.h>


typedef int    gvfs_fd_t;


typedef struct {
     gvfs_uint_t  command;
     gvfs_pid_t   pid;
     gvfs_int_t   slot;
     gvfs_fd_t    fd;
} gvfs_channel_t;


gvfs_int_t gvfs_write_channel(gvfs_socket_t s, gvfs_channel_t *ch, size_t size);
gvfs_int_t gvfs_read_channel(gvfs_socket_t s, gvfs_channel_t *ch, size_t size);
gvfs_int_t gvfs_add_channel_event(gvfs_cycle_t *cycle, gvfs_fd_t fd,
    gvfs_int_t event, gvfs_event_handler_pt handler);
void gvfs_close_channel(gvfs_fd_t *fd);


#endif /* _GVFS_CHANNEL_H_INCLUDED_ */
