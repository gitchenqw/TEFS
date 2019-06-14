

#ifndef _GVFS_EVENT_CONNECT_H_
#define _GVFS_EVENT_CONNECT_H_


#include <gvfs_config.h>
#include <gvfs_core.h>


typedef struct gvfs_peer_connection_s gvfs_peer_connection_t;

struct gvfs_peer_connection_s {
	gvfs_connection_t *connection;    /* 对端连接                 */
	
    struct sockaddr   *sockaddr;      /* 对端套接字地址            */
    socklen_t          socklen;       /* 对端套接字地址长度        */ 
	gvfs_str_t         addr_text;     /* 对端地址ip:port字符串形式 */
    
    VOID              *data;          /* 指向gvfs_http_request_t   */       
	INT32              rcvbuf;        /* 接收缓冲区的大小          */
	INT32              sendbuf;
};

gvfs_int_t gvfs_event_connect_peer(gvfs_peer_connection_t *pc);


#endif
