

#ifndef _GVFS_PROCESS_CYCLE_H_INCLUDED_
#define _GVFS_PROCESS_CYCLE_H_INCLUDED_


#include <gvfs_config.h>
#include <gvfs_core.h>


#define GVFS_CMD_OPEN_CHANNEL   1
#define GVFS_CMD_CLOSE_CHANNEL  2
#define GVFS_CMD_QUIT           3
#define GVFS_CMD_TERMINATE      4


#define GVFS_PROCESS_SINGLE     0
#define GVFS_PROCESS_MASTER     1
#define GVFS_PROCESS_SIGNALLER  2
#define GVFS_PROCESS_WORKER     3


void gvfs_master_process_cycle(gvfs_cycle_t *cycle);
void gvfs_single_process_cycle(gvfs_cycle_t *cycle);


extern gvfs_uint_t      gvfs_process;
extern gvfs_pid_t       gvfs_pid;
extern gvfs_uint_t      gvfs_daemonized;
extern gvfs_uint_t      gvfs_threaded;
extern gvfs_uint_t      gvfs_exiting;

extern sig_atomic_t    gvfs_reap;
extern sig_atomic_t    gvfs_sigio;
extern sig_atomic_t    gvfs_sigalrm;
extern sig_atomic_t    gvfs_quit;
extern sig_atomic_t    gvfs_terminate;


#endif /* _GVFS_PROCESS_CYCLE_H_INCLUDED_ */
