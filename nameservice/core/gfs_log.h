
#ifndef __GFS_LOG_H__
#define __GFS_LOG_H__

/* { add by chenqianwu */
#include <zlog.h>
/* } */
#include <gfs_config.h>

#define DEBUG           1
#define INFO            2
#define WARN            3
#define ERROR           4

#define gfs_log_debug(format, args...)   gfs_log_real(DEBUG, format, ##args)
#define gfs_log_info(format, args...)       gfs_log_real(INFO, format, ##args)
#define gfs_log_warn(format, args...)       gfs_log_real(WARN, format, ##args)
#define gfs_log_error(format, args...)      gfs_log_real(ERROR, format, ##args)

gfs_int32 gfs_log_init();
gfs_void  gfs_log_destory();

gfs_int32 gfs_log_real(gfs_int32 level, const gfs_char* format, ...);

/* { add by chenqianwu */

#define GFS_LOG_CONF_PATH         "conf/zlog.conf"
#define GFS_LOG_CATEGORY_NAME     "gfs"

extern zlog_category_t *zc;

#define glog_debug(format, args...) zlog_debug(zc, format, ##args)
#define glog_info(format, args...) zlog_info(zc, format, ##args)
#define glog_notice(format, args...) zlog_notice(zc, format, ##args)
#define glog_warn(format, args...) zlog_warn(zc, format, ##args)
#define glog_error(format, args...) zlog_error(zc, format, ##args)
#define glog_fatal(format, args...) zlog_fatal(zc, format, ##args)

int gfs_init_log(const char *zlog_conf_file, const char *name);

/* } */

#endif
