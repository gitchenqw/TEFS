#include <gfs_config.h>
#include <gfs_log.h>
#include <gfs_daemon.h>

gfs_int32 gfs_daemon_init()
{
    pid_t pid = fork();
    if(pid < 0)
    {
    	glog_error("fork() failed, error:%s", strerror(errno));
        return GFS_DAEMON_ERROR;
    }
    else if(pid > 0)
    {
        exit(0);        //parent exit;
    }

    //child continues
    setsid();        //become session leader
    chdir("~");      //change working directory
    umask(0);        //clear file mode creation mask
    close(0);        //close stdin
    close(1);        //close stdout
    close(2);        //close stderr

    open("/dev/null", O_RDONLY);
    open("/dev/null", O_RDWR);
    open("/dev/null", O_RDWR);

    return GFS_DAEMON_OK;
}

gfs_int32   gfs_daemon()
{
    gfs_int32 flag;

    flag = gfs_daemon_init();

    if(flag)
    {
        exit(0);
    }

    return GFS_DAEMON_OK;
}
