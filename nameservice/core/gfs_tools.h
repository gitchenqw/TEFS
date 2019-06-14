#ifndef __GFS_TOOLS_H__
#define __GFS_TOOLS_H__

gfs_int32   gfs_strtohex( gfs_char* str, gfs_int32 slen, gfs_uchar* hex );
gfs_int32   gfs_hextostr( gfs_uchar* hex, gfs_int32 hlen, gfs_char* str );

gfs_int32   gfs_string_value(gfs_char *str, gfs_char *key, gfs_char *value, gfs_uint32 vlen, gfs_char* split);
gfs_int32   gfs_url_parse(gfs_char *str, gfs_char *url);
gfs_uint64  gfs_ustime();
#define     gfs_mstime() (gfs_ustime()/1000)

static inline gfs_uint64  ntohll(gfs_uint64 val){return ((((unsigned long long)htonl((int)(((val)<<32)>>32)))<<32)|(unsigned int)htonl((int)((val)>>32)));}
static inline gfs_uint64  htonll(gfs_uint64 val){return ((((unsigned long long)htonl((int)(((val)<<32)>>32)))<<32)|(unsigned int)htonl((int)((val)>>32)));}
#endif //__TOOLS_H__
