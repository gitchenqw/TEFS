

#include <gvfs_config.h>
#include <gvfs_core.h>


u_char *
gvfs_cpystrn(u_char *dst, u_char *src, size_t n)
{
    if (n == 0) {
        return dst;
    }

    while (--n) {
        *dst = *src;

        if (*dst == '\0') {
            return dst;
        }

        dst++;
        src++;
    }

    *dst = '\0';

    return dst;
}


u_char *
gvfs_hex_dump(u_char *dst, u_char *src, size_t len)
{
    static u_char  hex[] = "0123456789abcdef";

    while (len--) {
        *dst++ = hex[*src >> 4];
        *dst++ = hex[*src++ & 0xf];
    }

    return dst;
}


void 
gvfs_hextoi(u_char *dst, size_t len, u_char *line, size_t n)
{
	UINT32     count;
    u_char     c, ch, value;

	count = value = 0;
	
    for ( ; n--; line++) {

    	
    	if (2 == count) {

    		if (0 < len) {
    			*dst++ = value;
    		}
    		
    		count = 0;
    		value = 0;
    		len--;
    	}

    	count++;
    	
    	ch = *line;

		if (ch >= '0' && ch <= '9') {
			value = value * 16 + (ch - '0');
			continue;
		}

		c = (u_char) (ch | 0x20);
		
        if (c >= 'a' && c <= 'f') {
            value = value * 16 + (c - 'a' + 10);
            continue;
        }
    }

	if (0 < len) {
    	*dst = value;
    }
}


static INT32
gvfs_decode_base64_internal(UCHAR *dst, size_t *dlen, UCHAR *src,
	size_t slen, const UCHAR *basis)
{
    size_t          len;
    UCHAR          *d, *s;

    for (len = 0; len < slen; len++) {
        if (src[len] == '=') {
            break;
        }

        if (basis[src[len]] == 77) {
            return GVFS_ERROR;
        }
    }

    if (len % 4 == 1) {
        return GVFS_ERROR;
    }

    s = src;
    d = dst;

    while (len > 3) {
        *d++ = (UCHAR) (basis[s[0]] << 2 | basis[s[1]] >> 4);
        *d++ = (UCHAR) (basis[s[1]] << 4 | basis[s[2]] >> 2);
        *d++ = (UCHAR) (basis[s[2]] << 6 | basis[s[3]]);

        s += 4;
        len -= 4;
    }

    if (len > 1) {
        *d++ = (UCHAR) (basis[s[0]] << 2 | basis[s[1]] >> 4);
    }

    if (len > 2) {
        *d++ = (UCHAR) (basis[s[1]] << 4 | basis[s[2]] >> 2);
    }

    *dlen = d - dst;

    return GVFS_OK;
}


VOID
gvfs_encode_base64(UCHAR *dst, size_t *dlen, UCHAR *src, size_t slen)
{
    UCHAR        *d, *s;
    size_t        len;
    static UCHAR  basis64[] =
            "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    len = slen;
    s = src;
    d = dst;

    while (len > 2) {
        *d++ = basis64[(s[0] >> 2) & 0x3f];
        *d++ = basis64[((s[0] & 3) << 4) | (s[1] >> 4)];
        *d++ = basis64[((s[1] & 0x0f) << 2) | (s[2] >> 6)];
        *d++ = basis64[s[2] & 0x3f];

        s += 3;
        len -= 3;
    }

    if (len) {
        *d++ = basis64[(s[0] >> 2) & 0x3f];

        if (len == 1) {
            *d++ = basis64[(s[0] & 3) << 4];
            *d++ = '=';

        } else {
            *d++ = basis64[((s[0] & 3) << 4) | (s[1] >> 4)];
            *d++ = basis64[(s[1] & 0x0f) << 2];
        }

        *d++ = '=';
    }

    *dlen = d - dst;
}


INT32
gvfs_decode_base64(UCHAR *dst, size_t *dlen, UCHAR *src, size_t slen)
{
    static UCHAR   basis64[] = {
        77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77,
        77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77,
        77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 62, 77, 77, 77, 63,
        52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 77, 77, 77, 77, 77, 77,
        77,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
        15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 77, 77, 77, 77, 77,
        77, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
        41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 77, 77, 77, 77, 77,

        77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77,
        77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77,
        77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77,
        77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77,
        77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77,
        77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77,
        77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77,
        77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77, 77
    };

    return gvfs_decode_base64_internal(dst, dlen, src, slen, basis64);
}

