

#ifndef _GVFS_ALLOC_H_INCLUDED_
#define _GVFS_ALLOC_H_INCLUDED_


#include <gvfs_config.h>
#include <gvfs_core.h>


#define gvfs_free          free


void *gvfs_alloc(size_t size);
void *gvfs_calloc(size_t size);
void *gvfs_memalign(size_t alignment, size_t size);


extern gvfs_uint_t  gvfs_pagesize;


#endif /* _GVFS_ALLOC_H_INCLUDED_ */
