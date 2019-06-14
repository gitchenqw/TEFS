#ifndef __RTB_CONFIG_H__
#define __RTB_CONFIG_H__

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/epoll.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <arpa/inet.h>

#include <pthread.h>
#include <unistd.h>
#include <netdb.h>
#include <stdint.h>
#include <stdarg.h>
#include <time.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <errno.h>

typedef char                gfs_int8;
typedef unsigned char       gfs_uint8;
typedef short               gfs_int16;
typedef unsigned short      gfs_uint16;
typedef int                 gfs_int32;
typedef unsigned int        gfs_uint32;
typedef long long           gfs_int64;
typedef unsigned long long  gfs_uint64;
typedef void                gfs_void;
typedef char                gfs_char;
typedef unsigned char       gfs_uchar;
typedef unsigned long       gfs_uptr;

typedef gfs_int32   (*gfs_handle_fun_ptr)(gfs_void *data);

#define gfs_true                        1
#define gfs_false                       0

#define GFS_HTTP_PTO                    0
#define GFS_CUSTOM_PTO                  1

#define GFS_NET_DS_MAPS                 1024

#define GFS_NET_SEND_BUFFER_SIZE        4096
#define GFS_NET_RECV_BUFFER_SIZE        4096

#define GFS_EVENT_NONE                  0
#define GFS_EVENT_READ                  1
#define GFS_EVENT_WRITE                 2
#define GFS_EVENT_ET                    4

#define GFS_NET_NOREADY                 0
#define GFS_NET_READY                   8

#define GFS_CONN_TIMEOUT_MS             3*60*1000
#define GFS_BUSI_LIFE_MS                10*60*1000

#define GFS_OK                          0           //return OK
#define GFS_ERR                         1
#define GFS_PARAM_FUN_ERR               2           //function param error
#define GFS_PARAM_CFG_ERR               3           //config param error
#define GFS_PARAM_NET_ERR               4           //recv network param error
#define GFS_FOPEN_ERR                   10          //open or create file failed
#define GFS_FIO_ERR                     11          //file IO expression
#define GFS_NFD_ERR                     12          //socket fd expression
#define GFS_EPOLL_ERR                   13          //epoll error

#define GFS_MEMPOOL_CREATE_ERR          100         //memory pool create failed
#define GFS_MEMPOOL_MALLOC_ERR          101         //malloc memory from memory pool failed
#define GFS_LOG_INIT_ERR                110         //log module failed to initialize or not initialized
#define GFS_GLOBAL_GETCFG_ERR           120         //global module get config param failed
#define GFS_GLOBAL_INIT_FAILED          121         //global module init failed
#define GFS_EVENT_NOEVENTS              130         //epoll_wait no events ready
#define GFS_EVENT_EPOLL_WAIT_INTR       131         //epoll_wait return by interrupt
#define GFS_EVENT_NOTIMER_ERR           132         
#define GFS_NET_CLOSE                   140         //TCP closed by remote
#define GFS_NET_AGAIN                   141         //TCP errno again
#define GFS_NET_OUTOF_MEMORY            142         //TCP recv unfinished
#define GFS_NET_LOOP_UNUSED             143         //No resources available in net loop.=
#define GFS_BUSI_ERR                    150


#define GFS_BUSI_RTN_OK                 0
#define GFS_BUSI_RTN_SYS_ERR            1
#define GFS_BUSI_RTN_PARAN_ERR          2
#define GFS_BUSI_RTN_OTHER_ERR          3
#define GFS_BUSI_RTN_UNSUPPORT_ENCRYPT  4
#define GFS_BUSI_RTN_UNKNOWN_MSG        5
#define GFS_BUSI_RTN_MSG_FORMAT_ERR     6
#define GFS_BUSI_RTN_UNKNOWN_CLIENT     7
#define GFS_BUSI_RTN_DSIZE_NOT_ENOUGH   10
#define GFS_BUSI_RTN_NO_DS_USE          11
#define GFS_BUSI_RTN_BLOCK_EXIST        12
#define GFS_BUSI_RTN_FILE_FINISHED      13
#define GFS_BUSI_RTN_FILE_NOT_EXIST     20
#define GFS_BUSI_RTN_BLOCK_NOT_EXIST    21
#define GFS_BUSI_RTN_BLOCK_TOO_MUCH     30

#define GFS_CLIENT_TYPE_PS              0
#define GFS_CLIENT_TYPE_DS              1
#endif
