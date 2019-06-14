#ifndef __GFS_HANDLE_H__
#define __GFS_HANDLE_H__
#include <gfs_config.h>
#include <gfs_net.h>
#include <gfs_event.h>

gfs_int32   gfs_handle_init();

gfs_int32   gfs_handle_tcp(gfs_net_t *c);
gfs_int32   gfs_handle_tcp_recv(gfs_event_t *ev);
gfs_int32   gfs_handle_tcp_send(gfs_event_t *ev);
gfs_int32   gfs_handle_tcp_close(gfs_event_t *ev);
gfs_int32   gfs_handle_tcp_parse(gfs_net_t *c);

#endif

