
#ifndef _GVFS_PALLOC_H_INCLUDED_
#define _GVFS_PALLOC_H_INCLUDED_


#include <gvfs_config.h>
#include <gvfs_core.h>


#define GVFS_MAX_ALLOC_FROM_POOL  (gvfs_pagesize - 1)

#define GVFS_DEFAULT_POOL_SIZE    (16 * 1024)

#define GVFS_POOL_ALIGNMENT       16

typedef struct gvfs_pool_large_s    gvfs_pool_large_t;

struct gvfs_pool_large_s {
	gvfs_pool_large_t    *next;
	void                 *alloc;
};

typedef struct {
	u_char               *last;
	u_char               *end;
	gvfs_pool_t          *next;
	gvfs_uint_t           failed;
} gvfs_pool_data_t;

struct gvfs_pool_s {
    gvfs_pool_data_t       d;
    size_t                 max;
    gvfs_pool_t           *current;
    gvfs_pool_large_t     *large;
};

gvfs_pool_t *gvfs_create_pool(size_t size);
void gvfs_destroy_pool(gvfs_pool_t *pool);
void gvfs_reset_pool(gvfs_pool_t *pool);

void *gvfs_palloc(gvfs_pool_t *pool, size_t size);
void *gvfs_pnalloc(gvfs_pool_t *pool, size_t size);
void *gvfs_pcalloc(gvfs_pool_t *pool, size_t size);
void *gvfs_pmemalign(gvfs_pool_t *pool, size_t size, size_t alignment);
gvfs_int_t gvfs_pfree(gvfs_pool_t *pool, void *p);


#endif /* _GVFS_PALLOC_H_INCLUDED_ */
