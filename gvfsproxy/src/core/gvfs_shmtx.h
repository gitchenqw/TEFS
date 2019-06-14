

#ifndef _GVFS_SHMTX_H_INCLUDED_
#define _GVFS_SHMTX_H_INCLUDED_


#include <gvfs_config.h>
#include <gvfs_core.h>


typedef struct {
    gvfs_atomic_t   lock;
} gvfs_shmtx_sh_t;

typedef struct {
    gvfs_atomic_t  *lock;
    gvfs_uint_t     spin;
} gvfs_shmtx_t;


void gvfs_shmtx_create(gvfs_shmtx_t *mtx, void *addr);
gvfs_uint_t gvfs_shmtx_trylock(gvfs_shmtx_t *mtx);
gvfs_uint_t gvfs_shmtx_unlock(gvfs_shmtx_t *mtx);


#endif /* _GVFS_SHMTX_H_INCLUDED_ */
