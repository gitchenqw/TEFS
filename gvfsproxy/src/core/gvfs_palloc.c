

#include <gvfs_config.h>
#include <gvfs_core.h>


#define GVFS_ALIGNMENT   sizeof(unsigned long)    /* platform word */

#define gvfs_align_ptr(p, a)                                                   \
    (u_char *) (((uintptr_t) (p) + ((uintptr_t) a - 1)) & ~((uintptr_t) a - 1))


static void *gvfs_palloc_block(gvfs_pool_t *pool, size_t size);
static void *gvfs_palloc_large(gvfs_pool_t *pool, size_t size);


gvfs_pool_t *
gvfs_create_pool(size_t size)
{
    gvfs_pool_t  *p;

    p = gvfs_memalign(GVFS_POOL_ALIGNMENT, size);
    if (p == NULL) {
        return NULL;
    }

    p->d.last = (u_char *) p + sizeof(gvfs_pool_t);
    p->d.end = (u_char *) p + size;
    p->d.next = NULL;
    p->d.failed = 0;

    size = size - sizeof(gvfs_pool_t);
    p->max = (size < GVFS_MAX_ALLOC_FROM_POOL) ? size : GVFS_MAX_ALLOC_FROM_POOL;

    p->current = p;
    p->large = NULL;

    return p;
}


void
gvfs_destroy_pool(gvfs_pool_t *pool)
{
    gvfs_pool_t          *p, *n;
    gvfs_pool_large_t    *l;

    for (l = pool->large; l; l = l->next) {
        if (l->alloc) {
            gvfs_free(l->alloc);
        }
    }

    for (p = pool, n = pool->d.next; /* void */; p = n, n = n->d.next) {
        gvfs_free(p);

        if (n == NULL) {
            break;
        }
    }
}


void
gvfs_reset_pool(gvfs_pool_t *pool)
{
    gvfs_pool_t        *p;
    gvfs_pool_large_t  *l;

    for (l = pool->large; l; l = l->next) {
        if (l->alloc) {
            gvfs_free(l->alloc);
        }
    }

    pool->large = NULL;

    for (p = pool; p; p = p->d.next) {
        p->d.last = (u_char *) p + sizeof(gvfs_pool_t);
    }
}


void *
gvfs_palloc(gvfs_pool_t *pool, size_t size)
{
    u_char      *m;
    gvfs_pool_t  *p;

    if (size <= pool->max) {

        p = pool->current;

        do {
            m = gvfs_align_ptr(p->d.last, GVFS_ALIGNMENT);

            if ((size_t) (p->d.end - m) >= size) {
                p->d.last = m + size;

                return m;
            }

            p = p->d.next;

        } while (p);

        return gvfs_palloc_block(pool, size);
    }

    return gvfs_palloc_large(pool, size);
}


void *
gvfs_pnalloc(gvfs_pool_t *pool, size_t size)
{
    u_char      *m;
    gvfs_pool_t  *p;

    if (size <= pool->max) {

        p = pool->current;

        do {
            m = p->d.last;

            if ((size_t) (p->d.end - m) >= size) {
                p->d.last = m + size;

                return m;
            }

            p = p->d.next;

        } while (p);

        return gvfs_palloc_block(pool, size);
    }

    return gvfs_palloc_large(pool, size);
}


static void *
gvfs_palloc_block(gvfs_pool_t *pool, size_t size)
{
    u_char      *m;
    size_t       psize;
    gvfs_pool_t  *p, *new, *current;

    psize = (size_t) (pool->d.end - (u_char *) pool);

    m = gvfs_memalign(GVFS_POOL_ALIGNMENT, psize);
    if (m == NULL) {
        return NULL;
    }

    new = (gvfs_pool_t *) m;

    new->d.end = m + psize;
    new->d.next = NULL;
    new->d.failed = 0;

    m += sizeof(gvfs_pool_data_t);
    m = gvfs_align_ptr(m, GVFS_ALIGNMENT);
    new->d.last = m + size;

    current = pool->current;

    for (p = current; p->d.next; p = p->d.next) {
        if (p->d.failed++ > 4) {
            current = p->d.next;
        }
    }

    p->d.next = new;

    pool->current = current ? current : new;

    return m;
}


static void *
gvfs_palloc_large(gvfs_pool_t *pool, size_t size)
{
    void               *p;
    gvfs_uint_t         n;
    gvfs_pool_large_t  *large;

    p = gvfs_alloc(size);
    if (p == NULL) {
        return NULL;
    }

    n = 0;

    for (large = pool->large; large; large = large->next) {
        if (large->alloc == NULL) {
            large->alloc = p;
            return p;
        }

        if (n++ > 3) {
            break;
        }
    }

    large = gvfs_palloc(pool, sizeof(gvfs_pool_large_t));
    if (large == NULL) {
        gvfs_free(p);
        return NULL;
    }

    large->alloc = p;
    large->next = pool->large;
    pool->large = large;

    return p;
}


void *
gvfs_pmemalign(gvfs_pool_t *pool, size_t size, size_t alignment)
{
    void              *p;
    gvfs_pool_large_t  *large;

    p = gvfs_memalign(alignment, size);
    if (p == NULL) {
        return NULL;
    }

    large = gvfs_palloc(pool, sizeof(gvfs_pool_large_t));
    if (large == NULL) {
        gvfs_free(p);
        return NULL;
    }

    large->alloc = p;
    large->next = pool->large;
    pool->large = large;

    return p;
}


gvfs_int_t
gvfs_pfree(gvfs_pool_t *pool, void *p)
{
    gvfs_pool_large_t  *l;

    for (l = pool->large; l; l = l->next) {
        if (p == l->alloc) {
            gvfs_free(l->alloc);
            l->alloc = NULL;

            return GVFS_OK;
        }
    }

    return GVFS_DECLINED;
}


void *
gvfs_pcalloc(gvfs_pool_t *pool, size_t size)
{
    void *p;

    p = gvfs_palloc(pool, size);
    if (p) {
        gvfs_memzero(p, size);
    }

    return p;
}

