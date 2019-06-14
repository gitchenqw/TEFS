

#ifndef _GVFS_ZLIB_H_INCLUDE_
#define _GVFS_ZLIB_H_INCLUDE_


#include <gvfs_config.h>
#include <gvfs_core.h>


INT32
gvfs_compress(UCHAR *dst, size_t *dlen, const UCHAR *src, size_t slen);

INT32
gvfs_uncompress(UCHAR *dst, size_t *dlen, const UCHAR *src, size_t slen);


#endif /* _GVFS_ZLIB_H_INCLUDE_ */

