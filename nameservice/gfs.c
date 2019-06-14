#include <gfs_config.h>
#include <gfs_log.h>
#include <gfs_global.h>
#include <gfs_handle.h>
#include <gfs_event.h>
#include <gfs_net.h>
#include <gfs_net_tcp.h>
#include <gfs_net_map.h>
#include <gfs_daemon.h>
#include <gfs_sql.h>
#include <gfs_busi.h>

gfs_void run(gfs_event_loop_t *eloop)
{
    glog_debug("worker process:%d ship way start", getpid());
    
    while (1) {
        gfs_event_loop_run(eloop);
    }
    
    exit(0);
}

gfs_int32 main(int argc, char **argv)
{
    gfs_event_loop_t    *eloop;
    gfs_net_loop_t      *nloop;
    pid_t                pid;

	gfs_init_log(GFS_LOG_CONF_PATH, GFS_LOG_CATEGORY_NAME);
    
	/* { modify by chenqianwu */
	// gfs_log_init();
	/* } */

	if (argc == 1) {
    	gfs_daemon();
    }
    
    if (GFS_OK != gfs_global_init()) {
    	glog_error("%s", "gfs_global_init() failed");
    	return -1;
    }
    
    if (GFS_OK != gfs_net_map_init()) {
    	glog_error("%s", "gfs_net_map_init() failed");
    	return -1;
    }

    eloop = gfs_event_loop_create(gfs_global->pool, gfs_global->max_num);
    if (!eloop) {
    	glog_error("gfs_event_loop_create(%p, %d) failed", gfs_global->pool,
    			   gfs_global->max_num);
        return gfs_errno;
    }
    gfs_global->eloop = eloop;
    
    nloop = gfs_net_pool_create
    (gfs_global->pool, gfs_global->max_num);
    if (!nloop) {
    	glog_error("gfs_net_pool_create(%p, %d) failed", gfs_global->pool,
    			   gfs_global->max_num);
        return gfs_errno;
	}
    gfs_global->nloop = nloop;

    gfs_global->tcpser = gfs_net_tcp_listen(gfs_global->pool, gfs_global->tport,
    										gfs_global->bind_ip);
	if (NULL == gfs_global) {
		glog_error("gfs_net_tcp_listen(%p, %u, %s) failed", gfs_global->pool,
				   gfs_global->tport, gfs_global->bind_ip);
		return -1;
	}
    
    gfs_handle_init();
    
    if (SQL_OK != gfs_mysql_init()) {
    	glog_error("%s", "gfs_mysql_init() failed");
    }
    
    gfs_busi_init();
    
MYFORK:
	if (argc == 1) {
	
	    pid = fork();    
	    if (pid > 0) {
	        if (wait(NULL) > 0) {
	            glog_warn("worker process:%d is died", pid);
	            goto MYFORK;
	        }
	        else  {
	            glog_error("wait() failed, error:%s", strerror(errno));
	        }
	    }
	    else if (pid == 0) {
	        run(eloop);
	    }
	    else {
	        glog_error("fork() failed, error:%s", strerror(errno));
	    }
	}
	
    run(eloop);

    gfs_net_tcp_unliten(gfs_global->tcpser);
    gfs_net_pool_destory(nloop);
    gfs_event_loop_destory(eloop);

    gfs_mysql_free();
    gfs_global_destory();
    
    glog_info("%s", "gfs shutdown success");
    
    gfs_log_destory();

    return GFS_OK;
}
