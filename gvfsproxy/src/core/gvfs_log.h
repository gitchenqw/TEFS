
#ifndef _GVFS_LOG_H_INCLUDE_
#define _GVFS_LOG_H_INCLUDE_

#include <zlog.h>

typedef enum gvfs_log_level_e{
	LOG_DEBUG  = 0,
	LOG_INFO   = 1,
	LOG_NOTICE = 2,
	LOG_WARN   = 3,
	LOG_ERROR  = 4,
	LOG_FATAL  = 5
} gvfs_log_level_t; 

extern zlog_category_t *zc;

#define gvfs_log(level, format, args...) \
do {\
	switch (level) {\
	case LOG_DEBUG:\
		zlog_debug(zc, format, ##args);\
		break; \
	case LOG_INFO:\
		zlog_info(zc, format, ##args);\
		break;\
	case LOG_NOTICE:\
		zlog_notice(zc, format, ##args);\
		break;\
	case LOG_WARN:\
		zlog_warn(zc, format, ##args);\
		break;\
	case LOG_ERROR:\
		zlog_error(zc, format, ##args);\
		break;\
	case LOG_FATAL:\
		zlog_fatal(zc, format, ##args);\
		break;\
	default:\
		break;\
	}\
} while (0)\

gvfs_int_t gvfs_init_log(const char *zlog_conf_file, const char *name);

#endif
