

#ifndef _GVFS_TIMES_H_INCLUDE_
#define _GVFS_TIMES_H_INCLUDE_


#include <gvfs_config.h>
#include <gvfs_core.h>


extern volatile gvfs_msec_t        gvfs_current_msec;


VOID gvfs_time_update(VOID);


#endif
