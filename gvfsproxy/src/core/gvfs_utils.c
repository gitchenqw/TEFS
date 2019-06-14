

#include <gvfs_config.h>
#include <gvfs_core.h>


INT32 gvfs_r8(UINT8 *p)
{
    return *p;
}

UINT32 gvfs_rb16(UINT8 *p)
{
    UINT32 val;

    val = gvfs_r8(p) << 8;
    p++;

    val |= gvfs_r8(p);

    return val;
}

UINT32 gvfs_rb24(UINT8 *p)
{
    UINT32 val;

    val = gvfs_rb16(p) << 16;
    p++;
    p++;

    val |= gvfs_r8(p);

    return val;
}

UINT32 gvfs_rb32(UINT8 *p)
{
    UINT32 val;

    val = gvfs_rb16(p) << 16;
    p++;
    p++;

    val |= gvfs_rb16(p);

    return val;
}

UINT64 gvfs_rb64(UINT8 *p)
{
	UINT64 val;

	val = (UINT64)gvfs_rb32(p) << 32;
	p++;
	p++;
	p++;
	p++;

	val |= (UINT64)gvfs_rb32(p);

	return val;
}

UINT32 gvfs_rl16(UINT8 *p)
{
    UINT32 val;
    
    val = gvfs_r8(p);
    p++;
    
    val |= gvfs_r8(p) << 8;
    
    return val;
}

UINT32 gvfs_rl24(UINT8 *p)
{
    UINT32 val;
    
    val = gvfs_rl16(p);
    p++;
    p++;
    
    val |= gvfs_r8(p) << 16;
    
    return val;
}

UINT32 gvfs_rl32(UINT8 *p)
{
    UINT32 val;
    val = gvfs_rl16(p);
    p++;
    p++;
    
    val |= gvfs_rl16(p) << 16;
    
    return val;
}

UINT64 gvfs_rl64(UINT8 *p)
{
    UINT64 val;
    
    val = (UINT64)gvfs_rl32(p);
    p++;
    p++;
    p++;
	p++;
    
    val |= (UINT64)gvfs_rl32(p) << 32;
    
    return val;
}

static UINT32 fetch_and_add(volatile UINT32 *operand, INT32 incr)
{
    INT32 result;
    asm __volatile__ (
        "lock xaddl %0,%1\n"
        : "=r"(result), "=m"(*(UINT32 *)operand)
        : "0"(incr)
        : "memory"
        );
        
    return result;
}

UINT32 gvfs_get_randid(VOID)
{
    static UINT32 randid = 0;
    
    if (randid == 0) {
        randid = time(0);
    }
    
    return fetch_and_add(&randid, 1);
}

VOID gvfs_srand(VOID)
{
    INT32 result = 0;
    static INT32 randid = -1;

    if (-1 == randid) {
        randid = time(NULL);
    }

    asm __volatile__ (
        "lock xaddl %0,%1\n"
        : "=r"(result), "=m"(randid)
        : "0"(1)
        : "memory"
        );

    srand(result);
}

INT32 gvfs_string_value(CHAR *buf, CHAR *key, CHAR *value, UINT32 vlen,
	CHAR *split)
{
	CHAR  *p;
	CHAR  *q;
	CHAR   key_buf[64];
	size_t len;

	snprintf(key_buf, 64, "%s=", key);

	p = strstr(buf, key_buf);
	if (NULL == p) {
		gvfs_log(LOG_ERROR, "buf:%s no key:%s error", buf, key);
		return GVFS_ERROR;
	}

	p += strlen(key_buf);
	q = strpbrk(p, split);
	if (NULL == q) {
		strncpy(value, p, vlen);
	}
	else {
		len = q - p;
		if (len >= vlen) {
			
			return GVFS_ERROR;
		}

		memcpy(value, p, len);
		value[len] = '\0';
	}
	
    return GVFS_OK;
}
