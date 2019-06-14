

#ifndef _GVFS_ATOMIC_H_INCLUDED_
#define _GVFS_ATOMIC_H_INCLUDED_


#include <gvfs_config.h>
#include <gvfs_core.h>


typedef volatile unsigned long  gvfs_atomic_t;


#define gvfs_atomic_cmp_set(lock, old, set)                                    \
    __sync_bool_compare_and_swap(lock, old, set)

#define gvfs_atomic_fetch_add(value, add)                                      \
    __sync_fetch_and_add(value, add)


#endif /* _GVFS_ATOMIC_H_INCLUDED_ */
