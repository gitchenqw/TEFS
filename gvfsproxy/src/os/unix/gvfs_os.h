

#ifndef _GVFS_OS_H_INCLUDED_
#define _GVFS_OS_H_INCLUDED_


#include <gvfs_config.h>
#include <gvfs_core.h>


typedef ssize_t (*gvfs_recv_pt)(gvfs_connection_t *c, u_char *buf, size_t size);
typedef ssize_t (*gvfs_send_pt)(gvfs_connection_t *c, u_char *buf, size_t size);


gvfs_int_t gvfs_os_init(void);
gvfs_int_t gvfs_daemon(void);
gvfs_int_t gvfs_os_signal_process(char *sig, gvfs_int_t pid);


ssize_t gvfs_unix_recv(gvfs_connection_t *c, u_char *buf, size_t size);
ssize_t gvfs_udp_unix_recv(gvfs_connection_t *c, u_char *buf, size_t size);
ssize_t gvfs_unix_send(gvfs_connection_t *c, u_char *buf, size_t size);


INT32 gvfs_completed_send(gvfs_connection_t *c, UINT8 *data,
	size_t size);

extern gvfs_int_t    gvfs_ncpu;
extern gvfs_int_t    gvfs_max_sockets;


#endif /* _GVFS_OS_H_INCLUDED_ */
