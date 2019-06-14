

#include <gvfs_config.h>
#include <gvfs_core.h>

gvfs_int_t
gvfs_shm_alloc(gvfs_shm_t *shm)
{
    shm->addr = (u_char *) mmap(NULL, shm->size,
                                PROT_READ|PROT_WRITE,
                                MAP_ANON|MAP_SHARED, -1, 0);

    if (shm->addr == MAP_FAILED) {
        gvfs_log(LOG_ERROR, "mmap(MAP_ANON|MAP_SHARED, %lu) failed", shm->size);
        return GVFS_ERROR;
    }

    return GVFS_OK;
}


void
gvfs_shm_free(gvfs_shm_t *shm)
{
    if (munmap((void *) shm->addr, shm->size) == -1) {
        gvfs_log(LOG_ERROR, "munmap(%p, %lu) failed", shm->addr, shm->size);
    }
}
