

#include <gvfs_config.h>
#include <gvfs_core.h>


gvfs_int_t   gvfs_ncpu;
gvfs_int_t   gvfs_max_sockets;


gvfs_int_t
gvfs_os_init(void)
{
	struct rlimit  rlmt;
	
    gvfs_init_setproctitle();

    gvfs_pagesize = getpagesize();

	/* { 获取CPU核心数,将进程与特定的CPU绑定,提高程序的性能 */
    if (0 == gvfs_ncpu) {
        gvfs_ncpu = sysconf(_SC_NPROCESSORS_ONLN); 
    }

    if (1 > gvfs_ncpu) {
        gvfs_ncpu = 1;
    }
    /* } */

    if (-1 == getrlimit(RLIMIT_NOFILE, &rlmt)) {
        gvfs_log(LOG_ERROR, "getrlimit(RLIMIT_NOFILE) failed), error: %s",
        					gvfs_strerror(errno));
        
        return GVFS_ERROR;
    }

    gvfs_max_sockets = (gvfs_int_t) rlmt.rlim_cur;
    
    return GVFS_OK;
}

gvfs_int_t
gvfs_daemon(void)
{
    int  fd;

    switch (fork()) {
    case -1:
        return GVFS_ERROR;

    case 0:
        break;

    default:
        exit(0);
    }

    gvfs_pid = getpid();

    if (setsid() == -1) {
        return GVFS_ERROR;
    }

    umask(0);

    fd = open("/dev/null", O_RDWR);
    if (fd == -1) {
        return GVFS_ERROR;
    }

    if (dup2(fd, STDIN_FILENO) == -1) {
        return GVFS_ERROR;
    }

    if (dup2(fd, STDOUT_FILENO) == -1) {
        return GVFS_ERROR;
    }

    if (fd > STDERR_FILENO) {
        if (close(fd) == -1) {
            return GVFS_ERROR;
        }
    }

    return GVFS_OK;
}
