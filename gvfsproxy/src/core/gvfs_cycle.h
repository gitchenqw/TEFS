

#ifndef _GVFS_CYCLE_H_INCLUDED_
#define _GVFS_CYCLE_H_INCLUDED_


#include <gvfs_config.h>
#include <gvfs_core.h>


#define GVFS_CYCLE_POOL_SIZE     16384


struct gvfs_cycle_s {
    gvfs_conf_t               *conf;
    gvfs_pool_t               *pool;
    gvfs_connection_t         *free_connections;
    gvfs_uint_t                free_connection_n;
    gvfs_array_t               listening;
    gvfs_uint_t                connection_n; /* events:worker_connections*/
    gvfs_int_t                 lock_file;
    gvfs_connection_t         *connections;  /* connection_n * gvfs_connection_t */
    gvfs_event_t              *read_events;
    gvfs_event_t              *write_events;
	char                      *conf_file;
};

typedef struct gvfs_core_conf_s
{
	gvfs_flag_t 			daemon;         
	gvfs_flag_t				master;         
	gvfs_int_t				worker_processes;
	
	gvfs_uint_t				cpu_affinity_n;
	u_long				   *cpu_affinity;
}gvfs_core_conf_t;


gvfs_cycle_t *gvfs_init_cycle(char *conf_file);
gvfs_int_t gvfs_create_pidfile(char *name);
gvfs_int_t gvfs_signal_process(char *sig);


extern volatile gvfs_cycle_t *gvfs_cycle;
extern gvfs_module_t          gvfs_core_module;
extern gvfs_core_conf_t      *g_gvfs_ccf;


#endif /* _GVFS_CYCLE_H_INCLUDED_ */
