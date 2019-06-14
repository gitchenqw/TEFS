

#ifndef _GVFS_HTTP_CONFIG_H_INCLUDE_
#define _GVFS_HTTP_CONFIG_H_INCLUDE_


#define HTTP_POOL_SIZE          4096
#define HTTP_BACKLOG            512
#define HTTP_SENDBUF            65536
#define HTTP_RECVBUF            16384

#define HTTP_LISTEN_PORT        8080
#define HTTP_SERVICE_IP         "0.0.0.0"

#define HTTP_MAIN_CHUNK_NUM     16
#define HTTP_MAIN_CHUNK_SIZE    1024

#define HTTP_CONNECTION_TIMEOUT 3000 

#endif /* _GVFS_HTTP_CONFIG_H_INCLUDE_ */