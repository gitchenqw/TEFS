

#include <gvfs_config.h>
#include <gvfs_core.h>
#include <gvfs_event.h>
#include <gvfs_channel.h>


static void gvfs_start_worker_processes(gvfs_cycle_t *cycle, gvfs_int_t n,
    gvfs_int_t type);
static void gvfs_pass_open_channel(gvfs_cycle_t *cycle, gvfs_channel_t *ch);
static void gvfs_signal_worker_processes(gvfs_cycle_t *cycle, int signo);
static gvfs_uint_t gvfs_reap_children(gvfs_cycle_t *cycle);
static void gvfs_master_process_exit(gvfs_cycle_t *cycle);
static void gvfs_worker_process_cycle(gvfs_cycle_t *cycle, void *data);
static void gvfs_worker_process_init(gvfs_cycle_t *cycle, gvfs_uint_t priority);
static void gvfs_worker_process_exit(gvfs_cycle_t *cycle);
static void gvfs_channel_handler(gvfs_event_t *ev);

gvfs_uint_t    gvfs_process;
gvfs_pid_t     gvfs_pid;
gvfs_uint_t    gvfs_threaded;

sig_atomic_t   gvfs_reap; // 有子进程意外结束
sig_atomic_t   gvfs_sigio; 
sig_atomic_t   gvfs_sigalrm;
sig_atomic_t   gvfs_terminate;
sig_atomic_t   gvfs_quit;
gvfs_uint_t    gvfs_exiting;

gvfs_uint_t    gvfs_daemonized;


u_long         cpu_affinity;
static u_char  master_process[] = "master process";


static gvfs_int_t gvfs_set_master_proctitle(gvfs_cycle_t *cycle);


static gvfs_int_t
gvfs_set_master_proctitle(gvfs_cycle_t *cycle)
{
	char              *title;
	u_char            *p;
	size_t             size;
	gvfs_int_t         i;
    
    size = sizeof(master_process);

    for (i = 0; i < gvfs_argc; i++) {
        size += gvfs_strlen(gvfs_argv[i]) + 1; 
    }

    title = gvfs_pnalloc(cycle->pool, size);
    if (NULL == title) {
    	return GVFS_ERROR;
    }

    p = gvfs_cpymem(title, master_process, sizeof(master_process) - 1);
    for (i = 0; i < gvfs_argc; i++) {
		*p++ = ' ';
		p = gvfs_cpystrn(p, (u_char *) gvfs_argv[i], size);
    }

    gvfs_setproctitle(title);

    return GVFS_OK;
}

void
gvfs_master_process_cycle(gvfs_cycle_t *cycle)
{
    sigset_t           set;
	gvfs_uint_t        live;
	
    sigemptyset(&set);
    sigaddset(&set, SIGCHLD);
    sigaddset(&set, SIGALRM);
    sigaddset(&set, SIGIO);
    sigaddset(&set, SIGINT);
    sigaddset(&set, SIGTERM);
    sigaddset(&set, SIGQUIT);

    if (-1 == sigprocmask(SIG_BLOCK, &set, NULL)) {
    	gvfs_log(LOG_ERROR, "%s", "sigprocmask() failed");
    }

    sigemptyset(&set);

	if (GVFS_OK != gvfs_set_master_proctitle(cycle)) {
		gvfs_log(LOG_ERROR, "%s", "gvfs_set_master_proctitle() failed");
		return;
	}

    gvfs_start_worker_processes(cycle, g_gvfs_ccf->worker_processes,
                                GVFS_PROCESS_RESPAWN);
	live = 1;
	
    for ( ;; ) {
    
    	sigsuspend(&set);

    	if (gvfs_reap) {
    		gvfs_reap = 0;
    		live = gvfs_reap_children(cycle);
    	}

		// 这里有bug,如何处理排队的信号,-s quit时存在父进程先于子进程退出的情况
    	if (!live && (gvfs_terminate || gvfs_quit)) {
    		gvfs_master_process_exit(cycle);
    	}
    	
    	if (gvfs_terminate) {
			gvfs_signal_worker_processes(cycle, SIGTERM);
    	}

    	if (gvfs_quit) {
    		gvfs_signal_worker_processes(cycle, SIGQUIT);
    	}
    }
}


void
gvfs_single_process_cycle(gvfs_cycle_t *cycle)
{
	gvfs_module_t *module = NULL;

	while ((module = gvfs_module_next(module))) {
		if (module->init_process) {
			if (module->init_process(cycle) == GVFS_ERROR) {
				gvfs_log(LOG_ERROR, "init_process(\"%s\") failed",
									module->name);
				exit(2);
			}
		}
	}

	module = NULL;
	for ( ;; ) {
	
		gvfs_process_events_and_timers(cycle);
		
		if (gvfs_terminate || gvfs_quit) {
			while ((module = gvfs_module_next(module))) {
				if (module->exit_master) {
					module->exit_master(cycle);
				}
			}
			
			gvfs_master_process_exit(cycle);
		}
	}
}


static void
gvfs_start_worker_processes(gvfs_cycle_t *cycle, gvfs_int_t n, gvfs_int_t type)
{
    gvfs_int_t      i;
    gvfs_channel_t  ch;

    ch.command = GVFS_CMD_OPEN_CHANNEL;

    for (i = 0; i < n; i++) {

        gvfs_spawn_process(cycle, gvfs_worker_process_cycle, NULL,
                          "worker process", type);

        ch.pid = gvfs_processes[gvfs_process_slot].pid;
        ch.slot = gvfs_process_slot;
        ch.fd = gvfs_processes[gvfs_process_slot].channel[0];

        gvfs_pass_open_channel(cycle, &ch); 
    }
}

static void
gvfs_pass_open_channel(gvfs_cycle_t *cycle, gvfs_channel_t *ch)
{
    gvfs_int_t  i;

    for (i = 0; i < gvfs_last_process; i++) {

		/* skip当前创建的worker进程和异常的worker进程 */
        if (i == gvfs_process_slot
            || gvfs_processes[i].pid == -1
            || gvfs_processes[i].channel[0] == -1)
        {
            continue;
        }

        gvfs_log(LOG_DEBUG,
        		 "pass channel slot:%ld pid:%d fd:%d to slot:%ld pid:%d fd:%d",
                 ch->slot, ch->pid, ch->fd, i, gvfs_processes[i].pid,
                 gvfs_processes[i].channel[0]);

		/* 发送消息给除刚创建之外的其它worker进程 */
        gvfs_write_channel(gvfs_processes[i].channel[0], ch,
        				   sizeof(gvfs_channel_t));
    }
}


static void
gvfs_signal_worker_processes(gvfs_cycle_t *cycle, int signo)
{
	char           *action;
    gvfs_int_t      i;
    int      		err;
    gvfs_channel_t  ch;

	action = NULL;
	
    switch (signo) {

    case SIGQUIT:
    	action = "SIGQUIT";
        ch.command = GVFS_CMD_QUIT;
        break;

    case SIGTERM:
    	action = "SIGTERM";
        ch.command = GVFS_CMD_TERMINATE;
        break;

    default:
        ch.command = 0;
    }

    ch.fd = -1;


    for (i = 0; i < gvfs_last_process; i++) {


        if (gvfs_processes[i].pid == -1) {
            continue;
        }

        if (ch.command) {

			gvfs_log(LOG_WARN, "send:%s to child:%ld pid:%d exited:%u respawn:%u",
								action,
								i,
								gvfs_processes[i].pid,
								gvfs_processes[i].exited,
								gvfs_processes[i].respawn);
        
            if (gvfs_write_channel(gvfs_processes[i].channel[0],
                                  &ch, sizeof(gvfs_channel_t))
                == GVFS_OK)
            {
                continue;
            }
        }


		
        gvfs_log(LOG_WARN, "kill (pid: %d, signo: %d)",
        					gvfs_processes[i].pid,
        					signo);

        if (kill(gvfs_processes[i].pid, signo) == -1) {
            err = errno;
            gvfs_log(LOG_ERROR, "kill(%d, %d) failed, error: %s",
            					gvfs_processes[i].pid,
            					signo, gvfs_strerror(err));

            if (err == ESRCH) {
                gvfs_processes[i].exited = 1;
                gvfs_reap = 1;
            }

            continue;
        }

    }
}


static gvfs_uint_t
gvfs_reap_children(gvfs_cycle_t *cycle)
{
    gvfs_int_t         i, n;
    gvfs_uint_t        live;
    gvfs_channel_t     ch;

    ch.command = GVFS_CMD_CLOSE_CHANNEL;
    ch.fd = -1;

    live = 0;
    for (i = 0; i < gvfs_last_process; i++) {

		/* 有关进程管理的日志,全部输出到warnning日志中,方便定位 */
        gvfs_log(LOG_WARN, "reap child: %ld pid: %d exited:%u respawn:%u",
							i,
							gvfs_processes[i].pid,
							gvfs_processes[i].exited,
							gvfs_processes[i].respawn);
                       
        if (gvfs_processes[i].pid == -1) {
            continue;
        }

		/* exited在waitpid中置为1,表示子进程已经终止 */
        if (gvfs_processes[i].exited) {

			/* 子进程已经退出,父进程要关闭其套接字,也要通知其它子进程关闭 */
			gvfs_close_channel(gvfs_processes[i].channel);
			gvfs_processes[i].channel[0] = -1;
			gvfs_processes[i].channel[1] = -1;

			
			ch.pid = gvfs_processes[i].pid;
			ch.slot = i;

			for (n = 0; n < gvfs_last_process; n++) {
			
				if (gvfs_processes[n].exited
					|| gvfs_processes[n].pid == -1
					|| gvfs_processes[n].channel[0] == -1)
				{
					continue;
				}

				gvfs_log(LOG_DEBUG, "pass close channel s:%ld pid:%d to pid:%d",
							   		ch.slot, ch.pid, gvfs_processes[n].pid);
							   		
				gvfs_write_channel(gvfs_processes[n].channel[0], &ch,
								   sizeof(gvfs_channel_t));
			}
			
			/* respawn在waitpid中当exit(2)时置为0,表示进程正常终止,不再重新创建 */
            if (gvfs_processes[i].respawn
                && !gvfs_terminate
                && !gvfs_quit)
            {

                if (gvfs_spawn_process(cycle, gvfs_processes[i].proc,
                                       gvfs_processes[i].data,
                                       gvfs_processes[i].name, i)
                    == GVFS_INVALID_PID)
                {
                    gvfs_log(LOG_WARN, "could not respawn %s",
                                  		gvfs_processes[i].name);
                    continue;
                }

                ch.command = GVFS_CMD_OPEN_CHANNEL;
                ch.pid = gvfs_processes[gvfs_process_slot].pid;
                ch.slot = gvfs_process_slot;
                ch.fd = gvfs_processes[gvfs_process_slot].channel[0];

                gvfs_pass_open_channel(cycle, &ch);

                live = 1;
                
				gvfs_log(LOG_INFO, "respawn child: %ld new pid: %d success",
									i, ch.pid);
									
                continue;
            }

            if (i == gvfs_last_process - 1) {
                gvfs_last_process--;
            } else {
                gvfs_processes[i].pid = -1;
            }

        }
    }

    return live;
}

static void
gvfs_master_process_exit(gvfs_cycle_t *cycle)
{
    gvfs_module_t *module = NULL;

    while ((module = gvfs_module_next(module))) {
        if (module->exit_master) {
            module->exit_master(cycle);
        }
    }

    gvfs_destroy_pool(cycle->pool);
    exit(0);
}


static void
gvfs_worker_process_cycle(gvfs_cycle_t *cycle, void *data)
{
	gvfs_process = GVFS_PROCESS_WORKER;

	gvfs_worker_process_init(cycle, 1);
	
	gvfs_setproctitle("worker process");

	/* worker进程主循环 */
	for ( ;; ) {

		/* 事件处理接口 */
		gvfs_process_events_and_timers(cycle);

		/* worker子进程直接退出 */
		if (gvfs_terminate) {
			gvfs_worker_process_exit(cycle);
		}

		/* worker子进程退出前,处理完正在处理的请求再退出 */
		if (gvfs_quit) {
			gvfs_quit = 0;
			
			/* 修改进程名称 */
			gvfs_setproctitle("worker process is shutting down");
			gvfs_worker_process_exit(cycle);
			if (!gvfs_exiting) {
				// gvfs_close_listening_sockets(cycle);
				gvfs_exiting = 1;
			}
		}
	}
}



static void
gvfs_worker_process_init(gvfs_cycle_t *cycle, gvfs_uint_t priority)
{
	sigset_t		  set;
	gvfs_int_t		  n;
	gvfs_module_t    *module = NULL;
	
	sigemptyset(&set);

	if (-1 == sigprocmask(SIG_SETMASK, &set, NULL)) {
		gvfs_log(LOG_ERROR, "%s", "sigprocmask() failed");
	}

	while ((module = gvfs_module_next(module))) {
		if (module->init_process
			&& GVFS_ERROR == module->init_process(cycle)) {
				gvfs_log(LOG_ERROR, "init_process(\"%s\") failed",
									module->name);
				exit(2);
		}
		
	}

	/* 关闭从master进程继承下来的UNIX套接口的写端 */
	for (n = 0; n < gvfs_last_process; n++) {

		if (gvfs_processes[n].pid == -1) {
			continue;
		}

		/* 跳过当前进程 */
		if (n == gvfs_process_slot) {
			continue;
		}

		if (gvfs_processes[n].channel[1] == -1) {
			continue;
		}

		if (close(gvfs_processes[n].channel[1]) == -1) {
			gvfs_log(LOG_ERROR, "close() channel failed, error: %s",
								gvfs_strerror(errno));
		}
	}

	/* 关闭当前进程的写端 */
	if (close(gvfs_processes[gvfs_process_slot].channel[0]) == -1) {
		gvfs_log(LOG_WARN, "close() channel failed, error: %s, ignore",
							gvfs_strerror(errno));
	}

	if (gvfs_add_channel_event(cycle, gvfs_channel, GVFS_READ_EVENT,
							   gvfs_channel_handler)
		== GVFS_ERROR)
	{
		exit(2);
	}
}


static void
gvfs_worker_process_exit(gvfs_cycle_t *cycle)
{
	gvfs_log(LOG_WARN, "worker proc:%d exit(2)", gvfs_pid);
	exit(2);
}


static void
gvfs_channel_handler(gvfs_event_t *ev)
{
    gvfs_int_t          n;
    gvfs_channel_t      ch;
    gvfs_connection_t  *c;

    c = ev->data;

    for ( ;; ) {

        n = gvfs_read_channel(c->fd, &ch, sizeof(gvfs_channel_t));


        if (n == GVFS_ERROR) {

			// gvfs_del_conn(c, 0);
			// gvfs_close_connection(c);
            return;
        }


        if (n == GVFS_AGAIN) {
            return;
        }

        switch (ch.command) {
        
		
        case GVFS_CMD_QUIT:
            gvfs_quit = 1;
            break;

        case GVFS_CMD_TERMINATE:
            gvfs_terminate = 1;
            break;

        case GVFS_CMD_OPEN_CHANNEL:

            gvfs_log(LOG_INFO, "get channel slot:%ld pid:%d fd:%d",
                           	   ch.slot, ch.pid, ch.fd);

            gvfs_processes[ch.slot].pid = ch.pid;
            gvfs_processes[ch.slot].channel[0] = ch.fd;
            break;

        case GVFS_CMD_CLOSE_CHANNEL:

            gvfs_log(LOG_INFO, "close channel slot:%ld pid:%d our:%d fd:%d",
                           	   ch.slot, ch.pid, gvfs_processes[ch.slot].pid,
                               gvfs_processes[ch.slot].channel[0]);

            if (close(gvfs_processes[ch.slot].channel[0]) == -1) {
                gvfs_log(LOG_ERROR, "close() channel failed, error: %s",
                					gvfs_strerror(errno));
            }

            gvfs_processes[ch.slot].channel[0] = -1;
            break;
        }
    }
}

