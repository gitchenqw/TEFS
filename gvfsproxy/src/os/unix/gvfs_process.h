

#ifndef _GVFS_PROCESS_H_INCLUDED_
#define _GVFS_PROCESS_H_INCLUDED_


#include <gvfs_setproctitle.h>


typedef pid_t       gvfs_pid_t;


#define GVFS_INVALID_PID           -1
#define GVFS_PROCESS_RESPAWN       -3
#define GVFS_MAX_PROCESSES          1024


typedef void (*gvfs_spawn_proc_pt) (gvfs_cycle_t *cycle, void *data);


typedef struct {
	gvfs_pid_t          pid;        /* 进程ID */
	int                 status;     /* waitpid获取的进程状态 */
	gvfs_socket_t       channel[2]; /* sockpair通信的句柄 */

	gvfs_spawn_proc_pt  proc;       /* 子进程的循环方式,在父进程创建子进程时调用 */
	void               *data;       /* 传递给gvfs_spawn_proc_pt的第二个参数 */
	char               *name;       /* 显示的进程名称 */

	unsigned            respawn:1;  /* 表示该进程是可以重新创建的,当调用exit(2)时,该标志置为0 */
	unsigned            exited:1;   /* 进程已经退出,waitpid的地方置该标志位 */
} gvfs_process_t;


gvfs_pid_t gvfs_spawn_process(gvfs_cycle_t *cycle,
    gvfs_spawn_proc_pt proc, void *data, char *name, gvfs_int_t respawn);
gvfs_int_t gvfs_init_signals(void);


extern int             gvfs_argc;
extern char          **gvfs_argv;
extern char          **gvfs_os_argv;

extern gvfs_pid_t      gvfs_pid;
extern gvfs_socket_t   gvfs_channel;
extern gvfs_int_t      gvfs_process_slot;   // 当前操作的进程在process数组中的下标
extern gvfs_int_t      gvfs_last_process;   // process数组有意义的最大下标
extern gvfs_process_t  gvfs_processes[GVFS_MAX_PROCESSES];


#endif /* _GVFS_PROCESS_H_INCLUDED_ */
