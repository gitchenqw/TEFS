#ifndef __GFS_MYSQL_H__
#define __GFS_MYSQL_H__

#include <gfs_config.h>
#include <gfs_dbpool.h>

#define SQL_OK              0
#define SQL_EMPTY          -1
#define SQL_ARGS_ERR        1
#define SQL_QUERY_ERR       2
#define SQL_FETCH_ERR       3
#define SQL_POOL_ERR        4
#define SQL_INSERT_EXISTS   5
#define SQL_ERROR           6

#define SQL_FILE_NOT_EXIST          20
#define SQL_FILE_EXIT_COMPLETE      13
#define SQL_BLOCK_NOT_EXIST         21
#define SQL_BLOCK_EXIST             12

typedef struct gfs_sql_block_s
{
    gfs_int32       bindex;
    gfs_uint32      dsid;
    gfs_uint64      fid;
}gfs_sql_block_t;

gfs_int32   gfs_mysql_init();

gfs_int32   gfs_mysql_free();

gfs_int32   gfs_mysql_login(gfs_uint32 dsid, gfs_uint64 dsize, gfs_uint64 remain);
gfs_int32   gfs_mysql_upload(gfs_char *fhash, gfs_uint64 fsize, gfs_uint32 bnum, gfs_int32 bindex, gfs_int32 *status, gfs_uint64 *fid);
gfs_int32   gfs_mysql_delfile(gfs_char *fhash);
gfs_int32   gfs_mysql_delblock(gfs_uint64 fid, gfs_uint32 bindex, gfs_uint32 dsid);
gfs_int32   gfs_mysql_cpyblock(gfs_uint64 fid, gfs_uint32 bindex, gfs_uint32 dsid);
gfs_int32   gfs_mysql_download(gfs_mem_pool_t *pool, gfs_char *fhash, gfs_uint32 bindex, gfs_uint64 *fid, gfs_sql_block_t **blist, gfs_uint32 *bnum);

gfs_int32   gfs_mysql_get_delblock(gfs_int32 opt, gfs_uint32 copies, gfs_sql_block_t *blist, gfs_uint32 *bnum);
gfs_int32   gfs_mysql_get_cpyblock(gfs_uint32 copies, gfs_sql_block_t *blist, gfs_uint32 *bnum);

gfs_int32   gfs_mysql_getfsize(gfs_char *fhash, gfs_uint64 *fsize);
gfs_int32   gfs_mysql_set_deling(gfs_uint64 fid, gfs_uint32 bindex, gfs_uint32 dsid);

#endif
