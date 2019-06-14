

#include <gvfs_config.h>
#include <gvfs_core.h>
#include <gvfs_event.h>
#include <gvfs_channel.h>


typedef struct gvfs_signal_s {
    int     signo;
    char   *signame;
    char   *name;
    void  (*handler)(int signo);
} gvfs_signal_t;


static void gvfs_signal_handler(int signo);
static void gvfs_process_get_status(void);


int               gvfs_argc;    /* 保存命令行参数argc */
char            **gvfs_argv;    /* 保存命令行参数argv */
char            **gvfs_os_argv; /* 指向argv           */


gvfs_socket_t     gvfs_channel;
gvfs_int_t        gvfs_process_slot;
gvfs_int_t        gvfs_last_process;
gvfs_process_t    gvfs_processes[GVFS_MAX_PROCESSES];


/*
 *	SIGTERM 可以捕获的终止信号  终止进程
 *	SIGQUIT 用户产生退出符  终止进程
 *	SIGALRM alarm() send 终止进程
 *	SIGINT  用户产生中断符 终止进程
 *	SIGIO   异步IO事件 终止进程
 *	SIGCHLD 子进程终止 忽略
 *	SIGSYS  进程试图执行无效的系统条用 终止进程
 *	SIGPIPE 向无读取进程的管道写入,如向一个关闭的fd中写入数据 终止进程
 */
gvfs_signal_t  signals[] = {

    { SIGTERM, "SIGTERM",          "stop", gvfs_signal_handler },
    { SIGQUIT, "SIGQUIT",          "quit", gvfs_signal_handler },
    { SIGALRM, "SIGALRM",          "",     gvfs_signal_handler },
    { SIGINT,  "SIGINT",           "",     gvfs_signal_handler },
    { SIGIO,   "SIGIO",            "",     gvfs_signal_handler },
    { SIGCHLD, "SIGCHLD",          "",     gvfs_signal_handler },
    { SIGSYS,  "SIGSYS, SIG_IGN",  "",     SIG_IGN             },
    { SIGPIPE, "SIGPIPE, SIG_IGN", "",     SIG_IGN             },
    { 0,       NULL,               "",     NULL                }
};


gvfs_pid_t
gvfs_spawn_process(gvfs_cycle_t *cycle, gvfs_spawn_proc_pt proc, void *data,
    char *name, gvfs_int_t respawn)
{
	u_long      on;
	gvfs_pid_t  pid;
	gvfs_int_t  s;
	
	/* respawn大于等于0表示子进程已经异常终止,父进程重新创建子进程 */
    if (respawn >= 0) {
        s = respawn;
    } else { /* master进程创建worker进程 */
        for (s = 0; s < gvfs_last_process; s++) {
            if (gvfs_processes[s].pid == -1) {
                break;
            }
        }

		if (s == GVFS_MAX_PROCESSES) {
			return GVFS_INVALID_PID;
		}
    }

    if (socketpair(AF_UNIX, SOCK_STREAM, 0, gvfs_processes[s].channel) == -1)
    {
        gvfs_log(LOG_ERROR, "socketpair() failed while spawning \"%s\"", name);
        return GVFS_INVALID_PID;
    }

    if (gvfs_nonblocking(gvfs_processes[s].channel[0]) == -1) {
        gvfs_log(LOG_ERROR, "failed while spawning \"%s\"", name);
        gvfs_close_channel(gvfs_processes[s].channel);
        return GVFS_INVALID_PID;
    }

    if (gvfs_nonblocking(gvfs_processes[s].channel[1]) == -1) {
        gvfs_log(LOG_ERROR, "failed while spawning \"%s\"", name);
        gvfs_close_channel(gvfs_processes[s].channel);
        return GVFS_INVALID_PID;
    }

    on = 1;
    if (ioctl(gvfs_processes[s].channel[0], FIOASYNC, &on) == -1) {
        gvfs_log(LOG_ERROR, "ioctl(FIOASYNC) failed while spawning \"%s\"",
        	name);
        gvfs_close_channel(gvfs_processes[s].channel);
        return GVFS_INVALID_PID;
    }

    if (fcntl(gvfs_processes[s].channel[0], F_SETOWN, gvfs_pid) == -1) {
        gvfs_log(LOG_ERROR, "fcntl(F_SETOWN) failed while spawning \"%s\"",
			name);
        gvfs_close_channel(gvfs_processes[s].channel);
        return GVFS_INVALID_PID;
    }

    gvfs_channel = gvfs_processes[s].channel[1];
    
    gvfs_process_slot = s;

    pid = fork();
    switch (pid) {

    case -1:
        gvfs_log(LOG_ERROR, "fork() failed while spawning \"%s\"", name);
        gvfs_close_channel(gvfs_processes[s].channel);
        return GVFS_INVALID_PID;

    case 0:
        gvfs_pid = getpid();
        proc(cycle, data);
        break;

    default:
        break;
    }
    
	gvfs_log(LOG_INFO, "master proc: %d create worker(%d) success",
						gvfs_pid, pid);

    gvfs_processes[s].pid = pid;
    gvfs_processes[s].exited = 0;

	/* 如果是子进程异常终止后,父进程重新创建的子进程,直接返回 */
    if (0 <= respawn) {
    	return pid;
    }
    
    gvfs_processes[s].proc = proc;
    gvfs_processes[s].data = data;
    gvfs_processes[s].name = name;
    gvfs_processes[s].respawn = 1;

    if (s == gvfs_last_process) {
        gvfs_last_process++;
    }

    return pid;
}


gvfs_int_t 
gvfs_init_signals(void)
{
    gvfs_signal_t     *sig;
    struct sigaction   sa;

    for (sig = signals; sig->signo != 0; sig++) {
        gvfs_memzero(&sa, sizeof(struct sigaction));
        sa.sa_handler = sig->handler;
        sigemptyset(&sa.sa_mask);
        if (sigaction(sig->signo, &sa, NULL) == -1) {
            gvfs_log(LOG_ERROR, "sigaction(\"%s\") failed, error: %s",
            					sig->signame, gvfs_strerror(errno));
            					
            return GVFS_ERROR;
        }
    }

    return GVFS_OK;
}

void
gvfs_signal_handler(int signo)
{
	char            *action;
	gvfs_signal_t   *sig;

    for (sig = signals; sig->signo != 0; sig++) {
        if (sig->signo == signo) {
            break;
        }
    }

    action = "";

    switch (gvfs_process) {

    case GVFS_PROCESS_MASTER: 
    case GVFS_PROCESS_SINGLE: 
        switch (signo) {

        case SIGQUIT:
            gvfs_quit = 1;
            action = ", shutting down";
            break;

        case SIGTERM:
        case SIGINT:
            gvfs_terminate = 1;
            action = ", exiting";
            break;

        case SIGALRM:
            gvfs_sigalrm = 1;
            break;

        case SIGIO:
            gvfs_sigio = 1;
            break;

        case SIGCHLD:
            gvfs_reap = 1;
            break;
        }

        break;

    case GVFS_PROCESS_WORKER:
        switch (signo) {

        case SIGQUIT:
            gvfs_quit = 1;
            action = ", shutting down";
            break;

        case SIGTERM:
        case SIGINT:
            gvfs_terminate = 1;
            action = ", exiting";
            break;
        }

        break;
    }

    gvfs_log(LOG_WARN, "signal %d (%s) received%s", signo, sig->signame, action);

    if (signo == SIGCHLD) {
        gvfs_process_get_status();
    }
    
}


static void
gvfs_process_get_status(void)
{
    int               status;
    char             *process;
    gvfs_pid_t        pid;
    int               err;
    gvfs_int_t        i;
    gvfs_uint_t       one;

    one = 0;

    for ( ;; ) {
    	/* -1:等待任一一个子进程,此时与wait等效 */
        pid = waitpid(-1, &status, WNOHANG);

		/* waitpid设置了WNOHANG,但发觉没有子进程可搜集,则返回0 */
        if (pid == 0) { 
            return;
        }

        if (pid == -1) {
            err = errno;

            if (err == EINTR) {
                continue;
            }

            if (err == ECHILD && one) {
                return;
            }

            gvfs_log(LOG_ERROR, "waitpid() failed, error: %s",
            	gvfs_strerror(errno));
            return;
        }


        if (gvfs_accept_mutex_ptr) {

            /*
             * unlock the accept mutex if the abnormally exited process
             * held it
             */

            gvfs_atomic_cmp_set(gvfs_accept_mutex_ptr, pid, 0);
        }


        one = 1;
        process = "unknown process";

        for (i = 0; i < gvfs_last_process; i++) {
            if (gvfs_processes[i].pid == pid) {
                gvfs_processes[i].status = status;
                gvfs_processes[i].exited = 1;
                process = gvfs_processes[i].name;
                break;
            }
        }

        if (WTERMSIG(status)) {
#ifdef WCOREDUMP
            gvfs_log(LOG_WARN, "%s %d exited on signal %d%s",
                          process, pid, WTERMSIG(status),
                          WCOREDUMP(status) ? " (core dumped)" : "");
#else
            gvfs_log(LOG_WARN, "%s %d exited on signal %d",
                          process, pid, WTERMSIG(status));
#endif

        } else {
            gvfs_log(LOG_WARN, "%s %d exited with code %d",
                          process, pid, WEXITSTATUS(status));
        }

        if (WEXITSTATUS(status) == 2 && gvfs_processes[i].respawn) {
            gvfs_log(LOG_WARN, "%s %d exited with fatal code %d "
                          "and cannot be respawned",
                          process, pid, WEXITSTATUS(status));
            gvfs_processes[i].respawn = 0;
        }
    }
}


gvfs_int_t
gvfs_os_signal_process(char *name, gvfs_int_t pid)
{
    gvfs_signal_t  *sig;

    for (sig = signals; sig->signo != 0; sig++) {
        if (strcmp(name, sig->name) == 0) {
            if (kill(pid, sig->signo) != -1) {
                return 0;
            }
        }
    }

    return 1;
}
