

#ifndef _GVFS_SOCKET_H_INCLUDED_
#define _GVFS_SOCKET_H_INCLUDED_


#include <gvfs_config.h>


typedef int  gvfs_socket_t;


#define gvfs_socket          socket
#define gvfs_close_socket    close


int gvfs_nonblocking(gvfs_socket_t s);
int gvfs_blocking(gvfs_socket_t s);



#endif /* _GVFS_SOCKET_H_INCLUDED_ */
