

#include <gvfs_config.h>
#include <gvfs_core.h>


void
gvfs_timezone_update(void)
{
    time_t      s;
    struct tm  *t;
    char        buf[4];

    s = time(0);

    t = localtime(&s);

    strftime(buf, 4, "%H", t);
}


void
gvfs_localtime(time_t s, gvfs_tm_t *tm)
{
    (void) localtime_r(&s, tm);

    tm->gvfs_tm_mon++;
    tm->gvfs_tm_year += 1900;
}


void
gvfs_libc_localtime(time_t s, struct tm *tm)
{
    (void) localtime_r(&s, tm);
}


void
gvfs_libc_gmtime(time_t s, struct tm *tm)
{
    (void) gmtime_r(&s, tm);
}
