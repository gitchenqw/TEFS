

#include <gvfs_config.h>
#include <gvfs_core.h>
#include <gvfs_event.h>
#include <gvfs_event_posted.h>


#define DEFAULT_CONNECTIONS  512


static INT32 gvfs_event_module_init(gvfs_cycle_t *cycle);
static INT32  gvfs_event_process_init(gvfs_cycle_t *cycle);

static void *gvfs_event_create_conf(gvfs_cycle_t *cycle);
static char *gvfs_event_init_conf(gvfs_cycle_t *cycle);


static gvfs_atomic_t   connection_counter = 1;
gvfs_atomic_t         *gvfs_connection_counter = &connection_counter;

gvfs_event_conf_t     *g_gvfs_ecf = NULL;

sig_atomic_t           gvfs_event_timer_alarm;

gvfs_atomic_t         *gvfs_accept_mutex_ptr;
gvfs_shmtx_t           gvfs_accept_mutex;
gvfs_uint_t            gvfs_use_accept_mutex;
gvfs_uint_t            gvfs_accept_mutex_held;


static gvfs_command_t gvfs_event_commands[] = {
	{"events", GET_CONF_CURRENT,
	 NULL, 0},
	 
	{"worker_connections", GET_CONF_CHILD,
	 gvfs_conf_set_num_slot, offsetof(gvfs_event_conf_t, connections)},
	 
	{"epoll_events", GET_CONF_NEXT,
	 gvfs_conf_set_num_slot, offsetof(gvfs_event_conf_t, events)}
};

gvfs_module_t gvfs_event_module = {
	.name         = "gvfs_event_module",
	.create_conf  = gvfs_event_create_conf,
	.init_conf    = gvfs_event_init_conf,
	.init_module  = gvfs_event_module_init,
	.init_process = gvfs_event_process_init,
	.exit_process = NULL,
	.exit_master  = NULL,
};

static INT32
gvfs_event_module_init(gvfs_cycle_t *cycle)
{
	u_char              *shared;
	gvfs_shm_t           shm;
	size_t               size, cl;

	cl = 128;

    size = cl      /* gvfs_accept_mutex */
           + cl;   /* gvfs_connection_counter */

    
    shm.size = size;
    shm.name.len = sizeof("gvfs_shared_zone");
    shm.name.data = (u_char *)"gvfs_shared_zone";

    if (gvfs_shm_alloc(&shm) != GVFS_OK) {
        return GVFS_ERROR;
    }

    shared = shm.addr;

    gvfs_accept_mutex_ptr = (gvfs_atomic_t *)shared;
    gvfs_accept_mutex.spin = (gvfs_uint_t) -1;

	gvfs_shmtx_create(&gvfs_accept_mutex, (gvfs_shmtx_sh_t *)shared);

	gvfs_connection_counter = (gvfs_atomic_t *)(shared + 1 * cl);

	(void) gvfs_atomic_cmp_set(gvfs_connection_counter, 0, 1);

	return GVFS_OK;
}

void
gvfs_timer_signal_handler(int signo)
{
    gvfs_event_timer_alarm = 1;
}


static INT32 
gvfs_event_process_init(gvfs_cycle_t *cycle)
{
	gvfs_uint_t           i;
	gvfs_err_t            err;
    gvfs_listening_t     *ls;
    gvfs_event_t         *rev, *wev;
    gvfs_connection_t    *c, *next;
	struct sigaction      sa;
	struct itimerval      itv;

	INIT_LIST_HEAD(&gvfs_posted_accept_events);
	INIT_LIST_HEAD(&gvfs_posted_events);
	
	if (g_gvfs_ccf->master && 1 < g_gvfs_ccf->worker_processes) {
        gvfs_use_accept_mutex = 1;
        gvfs_accept_mutex_held = 0;
	}
	
    gvfs_event_timer_init();

	if (GVFS_OK != gvfs_epoll_init(cycle)) {
		gvfs_log(LOG_ERROR, "%s", "gvfs_epoll_init() failed");
							
		exit(2);
	}

	gvfs_memzero(&sa, sizeof(struct sigaction));
	sa.sa_handler = gvfs_timer_signal_handler;
	sigemptyset(&sa.sa_mask);
	
	if (sigaction(SIGALRM, &sa, NULL) == -1) {
		gvfs_log(LOG_ERROR, "sigaction(SIGALRM) failed, error: %s",
							gvfs_strerror(errno));
							
		return GVFS_ERROR;
	}
	
	itv.it_interval.tv_sec = 0;
	itv.it_interval.tv_usec = 100000;
	
	itv.it_value.tv_sec = 0;
	itv.it_value.tv_usec = 100000;
	
	if (setitimer(ITIMER_REAL, &itv, NULL) == -1) {
		gvfs_log(LOG_ERROR, "setitimer(ITIMER_REAL) failed, error: %s",
							gvfs_strerror(errno));
							
		return GVFS_ERROR;
	}
	
	cycle->connections =
		gvfs_alloc(sizeof(gvfs_connection_t) * cycle->connection_n);
	if (cycle->connections == NULL) {
		err = errno;
		
		gvfs_log(LOG_ERROR,
			"gvfs_alloc(%lu *%lu) for cycle->connections failed, error: %s",
			sizeof(gvfs_connection_t), cycle->connection_n, gvfs_strerror(err));
			
		return GVFS_ERROR;
	}

	c = cycle->connections;

	cycle->read_events = gvfs_alloc(sizeof(gvfs_event_t) * cycle->connection_n);
	if (cycle->read_events == NULL) {
		err = errno;
		
		gvfs_log(LOG_ERROR,
			"gvfs_alloc(%lu *%lu) for cycle->read_events failed, error: %s",
			sizeof(gvfs_event_t), cycle->connection_n, gvfs_strerror(err));
			
		return GVFS_ERROR;
	}

	rev = cycle->read_events;
	
	for (i = 0; i < cycle->connection_n; i++) {
		rev[i].instance = 1;
	}

	cycle->write_events = gvfs_alloc(sizeof(gvfs_event_t) * cycle->connection_n);
	if (cycle->write_events == NULL) {
		err = errno;
		
		gvfs_log(LOG_ERROR,
			"gvfs_alloc(%lu *%lu) for cycle->write_events failed, error: %s",
			sizeof(gvfs_event_t), cycle->connection_n, gvfs_strerror(err));
			
		return GVFS_ERROR;
	}

	wev = cycle->write_events;
	
	i = cycle->connection_n;
	next = NULL;

	do {
		i--;

		c[i].data = next;
		c[i].read = &cycle->read_events[i];
		c[i].write = &cycle->write_events[i];
		c[i].fd = (gvfs_socket_t) -1;

		next = &c[i];

	} while (i);
	
	cycle->free_connections = next;
	cycle->free_connection_n = cycle->connection_n;
	
    ls = cycle->listening.elts;
    for (i = 0; i < cycle->listening.nelts; i++) {
    	c = gvfs_get_connection(ls[i].fd);
    	
        if (c == NULL) {
            return GVFS_ERROR;
        }
        
        c->listening = &ls[i]; 
        ls[i].connection = c;

        rev = c->read;

        rev->accept = 1;
		rev->deferred_accept = 1;
		
		rev->handler = gvfs_event_accept;

		if (gvfs_use_accept_mutex) {
			continue;
		}
		
		if (gvfs_epoll_add_event(rev, GVFS_READ_EVENT, GVFS_CLEAR_EVENT) == GVFS_ERROR) {
			return GVFS_ERROR;
		}
    }
    
	return GVFS_OK;
}

void
gvfs_process_events_and_timers(gvfs_cycle_t *cycle)
{
    gvfs_uint_t  flags;
	gvfs_uint_t	 timer; 
	gvfs_uint_t  delta;

	timer = (gvfs_uint_t) -1;
	flags = 0;
    
	if (gvfs_use_accept_mutex) {
	
		if (GVFS_ERROR == gvfs_trylock_accept_mutex(cycle)) {
			return;
		}

		if (gvfs_accept_mutex_held) {
			flags = GVFS_POST_EVENTS;
		} else {
			timer = 100;
		}
	}

	delta = gvfs_current_msec;
	
	gvfs_epoll_process_events(cycle, timer, flags);

	delta = gvfs_current_msec - delta;

	/* 
	 * gvfs_epoll_process_events有可能在极短的时间内完成,gvfs_current_msec没有
	 * 更新,这样可以提高性能	
	 */
	if (delta) {
		gvfs_event_expire_timers();
	}
	
	/* 处理accept事件 */
	if (!list_empty(&gvfs_posted_accept_events)) {
		gvfs_event_process_posted(&gvfs_posted_accept_events);
	}

	/*
	 * 处理完accept时间就释放锁,accept事件一般耗时比较短,而非accept事件耗时比较
     * 长
	 */
    if (gvfs_accept_mutex_held) {
        gvfs_shmtx_unlock(&gvfs_accept_mutex);
    }

    /* 处理非accept事件 */
    if (!list_empty(&gvfs_posted_events)) {
    	gvfs_event_process_posted(&gvfs_posted_events);
    }

}


static void *
gvfs_event_create_conf(gvfs_cycle_t *cycle)
{
	if (g_gvfs_ecf) {
		return g_gvfs_ecf;
	}

	g_gvfs_ecf = gvfs_palloc(cycle->pool, sizeof(gvfs_event_conf_t));

	g_gvfs_ecf->connections = DEFAULT_CONNECTIONS;
	g_gvfs_ecf->events      = DEFAULT_CONNECTIONS;

	return g_gvfs_ecf;
}


static char * gvfs_event_init_conf(gvfs_cycle_t *cycle)
{
	gvfs_conf_t 	*conf;
	gvfs_command_t	*cmd;
	size_t			 i, cmd_size;

	conf = cycle->conf;

	cmd_size = sizeof(gvfs_event_commands) / sizeof(gvfs_event_commands[0]);

	for (i = 0; i < cmd_size; i++) {
		cmd = &gvfs_event_commands[i];

		conf = gvfs_get_conf_specific(conf, cmd->name, cmd->type);
		if (NULL == conf) {
			continue;
		}
		
		if (cmd->set) {
			cmd->set(conf, cmd, g_gvfs_ecf);
		}
	}

	return GVFS_CONF_OK;
}

