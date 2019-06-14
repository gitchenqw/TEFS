

#ifndef _GVFS_CONFIG_H_INCLUDED_
#define _GVFS_CONFIG_H_INCLUDED_


#include <gvfs_linux_config.h>


typedef intptr_t        gvfs_int_t;
typedef uintptr_t       gvfs_uint_t;
typedef intptr_t        gvfs_flag_t;

#define GVFS_INT32_LEN   sizeof("-2147483648") - 1
#define GVFS_INT64_LEN   sizeof("-9223372036854775808") - 1


#endif /* _GVFS_CONFIG_H_INCLUDED_ */
