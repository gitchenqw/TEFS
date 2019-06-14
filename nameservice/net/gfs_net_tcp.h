#ifndef __GFS_NET_TCP_H__
#define __GFS_NET_TCP_H__
#include <gfs_config.h>
#include <gfs_net.h>
#include <gfs_event.h>

typedef struct gfs_net_tcpser_s
{
    gfs_int32           fd;
    gfs_event_t         ev;
    gfs_uint16          port;
    gfs_char            ip[32];
    gfs_handle_fun_ptr  handle;
}gfs_net_tcpser_t;

gfs_net_tcpser_t*   gfs_net_tcp_listen(gfs_mem_pool_t *pool, gfs_uint16 port, gfs_char *bindaddr);
gfs_void            gfs_net_tcp_unliten(gfs_net_tcpser_t *gfs);
gfs_int32           gfs_net_tcp_accept(gfs_event_t *ev);

gfs_int32       gfs_net_tcp_recv(gfs_net_t *c);
gfs_int32       gfs_net_tcp_send(gfs_net_t *c);

#endif

