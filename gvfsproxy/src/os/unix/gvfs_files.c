

#include <gvfs_config.h>
#include <gvfs_core.h>
#include <gvfs_channel.h>


ssize_t
gvfs_read_file(gvfs_file_t *file, u_char *buf, size_t size, off_t offset)
{
    ssize_t  n;

    if (file->sys_offset != offset) {
        if (lseek(file->fd, offset, SEEK_SET) == -1) {
            gvfs_log(LOG_ERROR, "lseek() \"%s\" failed, error: %s",
            					file->name.data, gvfs_strerror(errno));
            return GVFS_ERROR;
        }

        file->sys_offset = offset;
    }

    n = read(file->fd, buf, size);

    if (n == -1) {
        gvfs_log(LOG_ERROR, "read() \"%s\" failed, error: %s",
        					file->name.data, gvfs_strerror(errno));
        return GVFS_ERROR;
    }

    file->sys_offset += n;

    file->offset += n;

    return n;
}

ssize_t
gvfs_write_file(gvfs_file_t *file, u_char *buf, size_t size, off_t offset)
{
    ssize_t  n, written;

    written = 0;

    if (file->sys_offset != offset) {
        if (lseek(file->fd, offset, SEEK_SET) == -1) {
            gvfs_log(LOG_ERROR, "lseek() \"%s\" failed, error: %s",
            					file->name.data, gvfs_strerror(errno));
            return GVFS_ERROR;
        }

        file->sys_offset = offset;
    }

    for ( ;; ) {
        n = write(file->fd, buf + written, size);

        if (n == -1) {
            gvfs_log(LOG_ERROR, "write() \"%s\" failed, error: %s",
            					file->name.data, gvfs_strerror(errno));
            return GVFS_ERROR;
        }

        file->offset += n;
        written += n;

        if ((size_t) n == size) {
            return written;
        }

        size -= n;
    }
}

gvfs_err_t
gvfs_trylock_fd(gvfs_fd_t fd)
{
    struct flock  fl;

    fl.l_start = 0;
    fl.l_len = 0;
    fl.l_pid = 0;
    fl.l_type = F_WRLCK;
    fl.l_whence = SEEK_SET;

	/* 如果无法获取独占写锁,则立即返回 */
    if (fcntl(fd, F_SETLK, &fl) == -1) { /* fcntl: 改变已经打开的文件的性质 */
        return errno;
    }

    return 0;
}

gvfs_err_t
gvfs_lock_fd(gvfs_fd_t fd)  /* 这两个函数是有区别的,注意fcntl的第二个参数 */
{
    struct flock  fl;

    fl.l_start = 0;
    fl.l_len = 0;
    fl.l_pid = 0;
    fl.l_type = F_WRLCK;
    fl.l_whence = SEEK_SET;

	/* 如果无法获取独占写锁,则使进程休眠等待,直到获得锁唤醒进程 */
    if (fcntl(fd, F_SETLKW, &fl) == -1) {
        return errno;
    }

    return 0;
}

gvfs_err_t
gvfs_unlock_fd(gvfs_fd_t fd)
{
    struct flock  fl;

    fl.l_start = 0;
    fl.l_len = 0;
    fl.l_pid = 0;
    fl.l_type = F_UNLCK;
    fl.l_whence = SEEK_SET;

    if (fcntl(fd, F_SETLK, &fl) == -1) {
        return  errno;
    }

    return 0;
}
