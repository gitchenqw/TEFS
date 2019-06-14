

#ifndef _GVFS_ARRAY_H_INCLUDED_
#define _GVFS_ARRAY_H_INCLUDED_


#include <gvfs_config.h>
#include <gvfs_core.h>


struct gvfs_array_s {
	void         *elts;
	gvfs_uint_t   nelts;
	size_t        size;
	gvfs_uint_t   nalloc;
	gvfs_pool_t  *pool;
};


gvfs_array_t *gvfs_array_create(gvfs_pool_t *p, gvfs_uint_t n, size_t size);
void gvfs_array_destroy(gvfs_array_t *a);
void *gvfs_array_push(gvfs_array_t *a);
void *gvfs_array_push_n(gvfs_array_t *a, gvfs_uint_t n);


static inline gvfs_int_t
gvfs_array_init(gvfs_array_t *array, gvfs_pool_t *pool, gvfs_uint_t n, size_t size)
{
    /*
     * set "array->nelts" before "array->elts", otherwise MSVC thinks
     * that "array->nelts" may be used without having been initialized
     */

    array->nelts = 0;
    array->size = size;
    array->nalloc = n;
    array->pool = pool;

    array->elts = gvfs_palloc(pool, n * size);
    if (array->elts == NULL) {
        return GVFS_ERROR;
    }

    return GVFS_OK;
}


#endif /* _GVFS_ARRAY_H_INCLUDED_ */
