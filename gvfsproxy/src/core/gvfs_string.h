

#ifndef _GVFS_STRING_H_INCLUDED_
#define _GVFS_STRING_H_INCLUDED_


#include <gvfs_config.h>
#include <gvfs_core.h>


typedef struct {
    size_t      len;
    u_char     *data;
} gvfs_str_t;


#define gvfs_string(str)     { sizeof(str) - 1, (u_char *) str }
#define gvfs_null_string     { 0, NULL }
#define gvfs_str_set(str, text)                                               \
    (str)->len = sizeof(text) - 1; (str)->data = (u_char *) text
#define gvfs_str_null(str)   (str)->len = 0; (str)->data = NULL

#define gvfs_tolower(c)      (u_char) ((c >= 'A' && c <= 'Z') ? (c | 0x20) : c)
#define gvfs_toupper(c)      (u_char) ((c >= 'a' && c <= 'z') ? (c & ~0x20) : c)

#define gvfs_strlen(s)       strlen((const char *) s)

#define gvfs_memzero(buf, n)       (void) memset(buf, 0, n)
#define gvfs_memset(buf, c, n)     (void) memset(buf, c, n)

#define gvfs_memcpy(dst, src, n)   (void) memcpy(dst, src, n)
#define gvfs_cpymem(dst, src, n)   (((u_char *) memcpy(dst, src, n)) + (n))


u_char *gvfs_cpystrn(u_char *dst, u_char *src, size_t n);
u_char *gvfs_hex_dump(u_char *dst, u_char *src, size_t len);
void gvfs_hextoi(u_char *dst, size_t len, u_char *line, size_t n);

VOID
gvfs_encode_base64(UCHAR *dst, size_t *dlen, UCHAR *src, size_t slen);

INT32
gvfs_decode_base64(UCHAR *dst, size_t *dlen, UCHAR *src, size_t slen);


#endif /* _GVFS_STRING_H_INCLUDED_ */
