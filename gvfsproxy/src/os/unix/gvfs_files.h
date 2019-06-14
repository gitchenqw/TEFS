

#ifndef _GVFS_FILES_H_INCLUDE_
#define _GVFS_FILES_H_INCLUDE_


#include <gvfs_config.h>
#include <gvfs_core.h>
#include <gvfs_channel.h>

#define GVFS_INVALID_FILE         -1
#define GVFS_FILE_ERROR           -1

#define GVFS_FILE_DEFAULT_ACCESS  0644

#define gvfs_open_file(name, mode, create, access)                            \
    open((const char *) name, mode|create, access)


ssize_t gvfs_read_file(gvfs_file_t *file, u_char *buf, size_t size,
						   off_t offset);
ssize_t gvfs_write_file(gvfs_file_t *file, u_char *buf, size_t size,
						   off_t offset);
						   
gvfs_err_t gvfs_trylock_fd(gvfs_fd_t fd);
gvfs_err_t gvfs_lock_fd(gvfs_fd_t fd);
gvfs_err_t gvfs_unlock_fd(gvfs_fd_t fd);


#endif
