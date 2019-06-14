

#include <gvfs_config.h>
#include <gvfs_core.h>


gvfs_uint_t  gvfs_pagesize;


void *gvfs_alloc(size_t size)
{
    void       *p;
    gvfs_err_t  err;

    p = malloc(size);
    if (NULL == p) {
    	err = errno;
        gvfs_log(LOG_ERROR, "malloc(%lu) failed, error: %s",
        					size, gvfs_strerror(err));
    }

    return p;
}

void *gvfs_calloc(size_t size)
{
    void  *p;

    p = gvfs_alloc(size);

    if (p) {
        gvfs_memzero(p, size);
    }

    return p;
}

void *gvfs_memalign(size_t alignment, size_t size)
{
    void  *p;
    int    err;

    err = posix_memalign(&p, alignment, size);

    if (err) {
    	gvfs_log(LOG_ERROR, "memalign(%lu, %lu) failed", alignment, size);
        p = NULL;
    }

    return p;
}
