

#include <gvfs_config.h>
#include <gvfs_core.h>


void
gvfs_shmtx_create(gvfs_shmtx_t *mtx, void *addr)
{
    mtx->lock = addr;
}

gvfs_uint_t
gvfs_shmtx_trylock(gvfs_shmtx_t *mtx)
{
	return (*mtx->lock == 0 && gvfs_atomic_cmp_set(mtx->lock, 0, gvfs_pid));
}

gvfs_uint_t
gvfs_shmtx_unlock(gvfs_shmtx_t *mtx)
{
    return (*mtx->lock == (gvfs_atomic_t)gvfs_pid
    	&& gvfs_atomic_cmp_set(mtx->lock, gvfs_pid, 0));
}
