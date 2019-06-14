

#ifndef _GVFS_TIME_H_INCLUDED_
#define _GVFS_TIME_H_INCLUDED_


#include <gvfs_config.h>
#include <gvfs_core.h>


typedef gvfs_uint_t      gvfs_msec_t;
typedef gvfs_int_t       gvfs_msec_int_t;

typedef struct tm             gvfs_tm_t;

#define gvfs_tm_sec            tm_sec
#define gvfs_tm_min            tm_min
#define gvfs_tm_hour           tm_hour
#define gvfs_tm_mday           tm_mday
#define gvfs_tm_mon            tm_mon
#define gvfs_tm_year           tm_year
#define gvfs_tm_wday           tm_wday
#define gvfs_tm_isdst          tm_isdst

#define gvfs_tm_sec_t          int
#define gvfs_tm_min_t          int
#define gvfs_tm_hour_t         int
#define gvfs_tm_mday_t         int
#define gvfs_tm_mon_t          int
#define gvfs_tm_year_t         int
#define gvfs_tm_wday_t         int


#define gvfs_tm_gmtoff         tm_gmtoff
#define gvfs_tm_zone           tm_zone


#define gvfs_timezone(isdst) (- (isdst ? timezone + 3600 : timezone) / 60)


void gvfs_timezone_update(void);
void gvfs_localtime(time_t s, gvfs_tm_t *tm);
void gvfs_libc_localtime(time_t s, struct tm *tm);
void gvfs_libc_gmtime(time_t s, struct tm *tm);

#define gvfs_gettimeofday(tp)  (void) gettimeofday(tp, NULL);
#define gvfs_msleep(ms)        (void) usleep(ms * 1000)
#define gvfs_sleep(s)          (void) sleep(s)


#endif /* _GVFS_TIME_H_INCLUDED_ */
