#ifndef __GFS_GLOBAL_H__
#define __GFS_GLOBAL_H__
#include <gfs_config.h>
#include <gfs_mem.h>
#include <gfs_net.h>
#include <gfs_event.h>
#include <gfs_net_tcp.h>

typedef struct gfs_global_s
{
    gfs_char                bind_ip[32]; /* 监听的ip地址        */
    gfs_uint16              tport;       /* 监听的端口,默认8082 */
    gfs_int16               timeout;
    gfs_int32               max_num;     /* 服务支持的最大连接数 */
    gfs_int32               client_num;  /* 已连接的用户数       */
    gfs_int8                log_level;
    gfs_uint32              copies;      /* 文件快备份个数       */
    gfs_uint32              block_size;  /* 默认文件块大小       */

    gfs_int32               mysql_conn;        /* mysql 连接池个数 */
    gfs_char                mysql_host[32];    /* mysql ip */
    gfs_uint16              mysql_port;        /* mysql 的端口 */
    gfs_char                mysql_db_name[32]; /* 数据库名称 */
    gfs_char                mysql_user[32];    /* 数据库用户名 */
    gfs_char                mysql_pwd[32];     /* 数据库密码 */

    gfs_mem_pool_t         *pool;
    gfs_net_loop_t         *nloop;
    gfs_event_loop_t       *eloop;
    gfs_net_tcpser_t       *tcpser;
}gfs_global_t;

gfs_int32   gfs_global_init();
gfs_void    gfs_global_destory();

gfs_int32   gfs_global_reload_cfg();

extern gfs_global_t *gfs_global;
extern gfs_int32     gfs_errno;
#endif
