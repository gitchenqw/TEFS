#include <gfs_config.h>
#include <gfs_tools.h>


static gfs_int32 hex_num[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8,
  9, 0, 0, 0, 0, 0, 0, 0,
  10,11,12,13,14,15, 0, 0, 
  0, 0, 0, 0, 0, 0, 0, 0, 
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  10,11,12,13,14,15  };


gfs_int32 gfs_strtohex( gfs_char* str, gfs_int32 slen, gfs_uchar* hex )
{
    gfs_int32 hf, hs;
    gfs_int32 i;
    gfs_int32 half;

    half = slen / 2;
    for( i = 0; i < half; ++i )
    {
        hf = str[ 2 * i ];
        hs = str[ 2 * i + 1 ];

        hf -= 0x30;
        hs -= 0x30;

        hf = hex_num[ hf ];
        hs = hex_num[ hs ];

        hf *= 0x10;
        hex[ i ] = hf + hs;
    }

    return half;
}

static gfs_int32 hex_str[] = { '0', '1', '2', '3', '4', '5', '6','7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f' };

gfs_int32 gfs_hextostr( gfs_uchar* hex, gfs_int32 hlen, gfs_char* str )
{
    gfs_int32 hf, hs;
    gfs_int32 i;

    for( i = 0; i< hlen; ++i )
    {
        hf = hex[ i ] / 16;
        hs = hex[ i ] % 16;

        str[ 2 * i ] = hex_str[ hf ];
        str[ 2 * i + 1 ] = hex_str[ hs ];
    }

    return 2*hlen;
}

gfs_uint64 gfs_ustime()
{
    struct timeval tv; 
    long long ust; 

    gettimeofday(&tv, NULL);
    ust = ((long long)tv.tv_sec)*1000000;
    ust += tv.tv_usec;
    return ust; 
} 

gfs_int32   gfs_string_value(gfs_char *str, gfs_char *key, gfs_char *value, gfs_uint32 vlen, gfs_char* split)
{
    gfs_char    *p, *q;
    gfs_char     tmp[32];
    gfs_int32    len;

    if (!str || !key || !value || !split)
        return GFS_PARAM_FUN_ERR;

    sprintf(tmp, "%s=", key);
    p = strstr(str, tmp);
    if (!p)
        return GFS_ERR;

    p += strlen(tmp);
    q = strpbrk(p, split);
    if (p && !q)
    {
        strcpy(value, p);
    }
    else if (p && q)
    {
        len = q - p;
        if (len >= vlen)
            return GFS_ERR;
        memcpy(value, p, len);
        value[len] = '\0';
    }
    else
        return GFS_ERR;

    return GFS_OK;
}

gfs_int32   gfs_url_parse(gfs_char *str, gfs_char *url)
{
    gfs_int32       hf, hs;
    gfs_char       *pstr,*purl;
    if (!str || !url)
        return GFS_PARAM_FUN_ERR;

    pstr = str;
    purl = url;

    while(*pstr != '\0')
    {
        if (*pstr == '%')
        {
            hf = *(++pstr);
            hs = *(++pstr);

            hf -= 0x30;
            hs -= 0x30;

            hf = hex_num[ hf ];
            hs = hex_num[ hs ];

            hf *= 0x10;
            *purl++ = hf + hs;
            pstr++;
        }
        else
            *purl++ = *pstr++;
    }
    *purl = '\0';

    return GFS_OK;
}


