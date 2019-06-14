

#include <gvfs_config.h>
#include <gvfs_core.h>


volatile gvfs_msec_t        gvfs_current_msec;


VOID gvfs_time_update(VOID)
{
	time_t                sec;
	gvfs_msec_t           msec;
	struct timeval        tv;

	gvfs_gettimeofday(&tv);

	sec = tv.tv_sec;
	msec = tv.tv_usec / 1000;

	gvfs_current_msec = (gvfs_msec_t) sec * 1000 + msec;
}
