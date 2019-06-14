

#ifndef _GVFS_BASE64_H_INCLUDE_
#define _GVFS_BASE64_H_INCLUDE_


VOID
gvfs_encode_base64(UCHAR *dst, size_t *dlen, UCHAR *src, size_t slen);

INT32
gvfs_decode_base64(UCHAR *dst, size_t *dlen, UCHAR *src, size_t slen);


#endif /* _GVFS_BASE64_H_INCLUDE_ */

