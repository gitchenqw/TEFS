#ifndef __GFS_DBPOOL_H__
#define __GFS_DBPOOL_H__
#include <mysql/mysql.h>
#include <mysql/errmsg.h>
#include <gfs_config.h>
#include <gfs_mem.h>
#include <gfs_log.h>
#include <gfs_global.h>

#define ERROR_LIMIT 100

typedef struct gfs_mysql_s
{
    MYSQL               *conn; /* mysql的通信数据结构 */
    MYSQL_RES           *res;  /* mysql返回的数据集   */
    gfs_int32            used;      //used:1, unused:0
    struct gfs_mysql_s  *next;
}gfs_mysql_t;

typedef struct gfs_mysql_pool_s
{
    gfs_mysql_t         *conn;
    gfs_int32            conn_num;
    gfs_int32            used_num;
    gfs_char             host[32];
    gfs_uint16           port;
    gfs_char             db[16];
    gfs_char             user[16];
    gfs_char             pwd[16];
    gfs_mem_pool_t      *pool;  /* 初始化的时候 pool为NULL */
}gfs_mysql_pool_t;



/* create connects pool to the mysql */
gfs_mysql_pool_t*       gfs_mysql_pool_create();

/* free all the connects in the pool */
gfs_void                gfs_mysql_pool_free();

/* fetch a connect fd from the pool */
gfs_mysql_t*            gfs_mysql_get();

/* put back the connect fd to the pool */
/* returen  0:ok  other:error */
gfs_int32               gfs_mysql_put(gfs_mysql_t *mysql);

gfs_int32               gfs_mysql_pool_current_num();

#endif