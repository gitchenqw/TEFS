#ifndef __GFS_NET_H__
#define __GFS_NET_H__
#include <gfs_config.h>
#include <gfs_event.h>
#include <gfs_mem.h>
#include <gfs_timer.h>
#include <gfs_queue.h>

typedef struct gfs_net_loop_s gfs_net_loop_t;
typedef struct gfs_net_buf_s gfs_net_buf_t;
typedef gfs_int32 (*gfs_net_packet_parse)(gfs_net_buf_t *buf, gfs_char **packet, gfs_int32 *plen);

struct gfs_net_buf_s
{
    gfs_void        *data;
	gfs_uint32       len;           //buffer len
	gfs_uint32       pos;           // end of data
	gfs_uint32       hd;            // start of data
};

typedef struct gfs_net_data_s
{
    gfs_char            user[32];
    gfs_char            pwd[32];
    gfs_int8            state;              //login state
    gfs_int32           ctype;
    gfs_uint32          dsid;
    gfs_uint64          resize;             //remain size
    gfs_uint64          dsize;              //disk size
    gfs_uint32          cfft;               //disk coefficient
    gfs_uint32          weight;
    gfs_uint32          port;
}gfs_net_data_t;

typedef struct gfs_net_s
{
	gfs_int32           fd;
	gfs_event_t         ev;
	struct sockaddr_in  addr;
	char addr_text[32];

	gfs_net_data_t      user;

    gfs_uint64               t_start;            //milliseconds
    gfs_uint64               t_end;              //milliseconds
    gfs_mem_pool_t          *pool;               // 默认40k
    gfs_net_buf_t           *rbuf;               //recv buffer,默认4096个字节
    gfs_net_buf_t           *sbuf;               //send buffer,默认4096个字节
    gfs_queue_t              qnode;
    gfs_net_loop_t          *net;
    gfs_void                *data;
    gfs_net_packet_parse     parse;
    gfs_handle_fun_ptr       handle;
    gfs_timer_t              timer;
    gfs_net_loop_t          *loop;
}gfs_net_t;

struct gfs_net_loop_s
{
    gfs_queue_t         free_list;
    gfs_queue_t         used_list;
    gfs_uint32          net_num;
    gfs_uint32          free_num;
    gfs_uint32          used_num;
};

gfs_net_loop_t*     gfs_net_pool_create(gfs_mem_pool_t *pool, gfs_uint32 num);
gfs_void            gfs_net_pool_destory(gfs_net_loop_t *loop);

gfs_net_t*          gfs_net_node_get(gfs_net_loop_t *loop, gfs_int32 fd);
gfs_int32           gfs_net_close(gfs_net_t *c);

gfs_int32           gfs_net_fd_noblock(gfs_int32 fd);
gfs_int32           gfs_net_fd_reuse(gfs_int32 fd);

gfs_int32           gfs_net_packet_parse_tcp(gfs_net_buf_t *buf, gfs_char **packet, gfs_int32 *plen);

gfs_int32           gfs_net_memcopy(gfs_net_buf_t *dst, gfs_net_buf_t *src);
#endif
