

#ifndef _GVFS_UTILS_H_INCLUDE_
#define _GVFS_UTILS_H_INCLUDE_


#include <gvfs_config.h>
#include <gvfs_core.h>


UINT32 gvfs_rb32(UINT8 *p);
UINT64 gvfs_rb64(UINT8 *p);

UINT32 gvfs_rl32(UINT8 *p);
UINT64 gvfs_rl64(UINT8 *p);

UINT32 gvfs_get_randid(VOID);
VOID gvfs_srand(VOID);

INT32 gvfs_string_value(CHAR *buf, CHAR *key, CHAR *value, UINT32 vlen,
	CHAR *split);

#endif
