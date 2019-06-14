#include <zlib.h>

#include <gvfs_config.h>
#include <gvfs_core.h>


INT32
gvfs_compress(UCHAR *dst, size_t *dlen, const UCHAR *src, size_t slen)
{
	INT32  rc;

	rc = compress(dst, dlen, (const Bytef*) src, slen);
	if (Z_OK != rc) {
		return GVFS_ERROR;
	}

	return GVFS_OK;
}

INT32
gvfs_uncompress(UCHAR *dst, size_t *dlen, const UCHAR *src, size_t slen)
{
	INT32  rc;

	rc = uncompress(dst, dlen, src, slen);
	if (Z_OK != rc) {
		return GVFS_ERROR;
	}

	return GVFS_OK;
}
