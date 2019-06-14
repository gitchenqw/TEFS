
#include <gvfs_config.h>
#include <gvfs_core.h>
#include <gvfs_proxy.h>

extern CHAR **environ;

static INT32 gvfs_get_options(INT32 argc, CHAR *const *argv);
static gvfs_int_t gvfs_save_argv(int argc, char *const *argv);
static void *gvfs_core_module_create_conf(gvfs_cycle_t *cycle);
static char *gvfs_core_module_init_conf(gvfs_cycle_t *cycle);

static gvfs_command_t gvfs_core_commands[] = {
	{"daemon", GET_CONF_CURRENT,
	 gvfs_conf_set_flag_slot, offsetof(gvfs_core_conf_t, daemon)},
	 
	{"master", GET_CONF_CURRENT,
	 gvfs_conf_set_flag_slot, offsetof(gvfs_core_conf_t, master)},

	{"worker_processes", GET_CONF_CURRENT,
	 gvfs_conf_set_num_slot, offsetof(gvfs_core_conf_t, worker_processes)}
};

gvfs_module_t gvfs_core_module = {
	.name         = "gvfs_core_module",
	.create_conf  = gvfs_core_module_create_conf,
	.init_conf    = gvfs_core_module_init_conf,
	.init_module  = NULL,
	.init_process = NULL,
	.exit_process = NULL,
	.exit_master  = NULL,
};

static gvfs_uint_t  gvfs_show_help;
static gvfs_uint_t  gvfs_show_version;
static u_char      *gvfs_conf_file;
static char        *gvfs_signal;

static char 	  **gvfs_os_environ;

INT32 main(INT32 argc, CHAR **argv)
{
	gvfs_cycle_t     *cycle;

    if (GVFS_OK != gvfs_get_options(argc, argv)) {
        return 1;
    }

	if (gvfs_show_version) { /* hv */
		printf("gvfs_proxy version: " GVFS_VER "\n");

		if (gvfs_show_help) { /* h */
			printf("Usage: gvfs_proxy [-hv] [-s signal] [-c filename] \n"
				   "Options:\n"
				   "  -h		: this help\n"
				   "  -v		: show version and exit\n"
				   "  -s signal	: send signal to a master process: stop, quit\n"
				   "  -c filename	: set configuration file\n"
				   );
		}
		return 0;
	}

	gvfs_pid = getpid();

	if (GVFS_OK != gvfs_init_log(GVFS_LOG_CONF_PATH, GVFS_LOG_CATEGORY_NAME)) {
		return 1;
	}
	
	/* 注册gvfs所有模块 */
	gvfs_register_all();
	
	if (GVFS_OK != gvfs_strerror_init()) {
		return 1;
	}

    if (GVFS_OK != gvfs_save_argv(argc, argv)) {
        return 1;
    }

    if (GVFS_OK != gvfs_os_init()) {
        return 1;
    }
    
    if (gvfs_signal) {
		return gvfs_signal_process(gvfs_signal);
    }
    
	/* { 解析处理配置文件,初始化模块,并打开监听套接字 */
    cycle = gvfs_init_cycle(GVFS_CONF_PATH);
    if (NULL == cycle) {
    	return 1;
    }
    /* } */

    gvfs_cycle = cycle;
	gvfs_cycle->connection_n = g_gvfs_ecf->connections;
    if (GVFS_OK != gvfs_init_signals()) {
        return 1;
    }

	if (g_gvfs_ccf->daemon) {
        if (GVFS_OK != gvfs_daemon()) {
            return 1;
        }

        gvfs_daemonized = 1;
		if (GVFS_OK != gvfs_create_pidfile(GVFS_PID_PATH)) {
			return 1;
		}
	}

	if (g_gvfs_ccf->master) {
    	gvfs_master_process_cycle(cycle);
    } else {
    	gvfs_single_process_cycle(cycle);
    }

    return 0;
}


static INT32
gvfs_get_options(INT32 argc, CHAR *const *argv)
{
	u_char	   *p;
	INT32    	i;

	for (i = 1; i < argc; i++) {

		p = (u_char *) argv[i];

		if (*p++ != '-') {
			printf("invalid option: \"%s\"\n", argv[i]);
			return GVFS_ERROR;
		}

		while (*p) {

			switch (*p++) {

			case 'h':
				gvfs_show_version = 1;
				gvfs_show_help = 1;
				break;

			case 'v':
				gvfs_show_version = 1;
				break;

			/* 支持 -c/usr/local/gvfs_proxy/conf/gvfs_proxy.conf 和 */
			/* -c /usr/local/gvfs_proxy/conf/gvfs_proxy.conf两种参数方式 */
			case 'c':
				if (*p) {
					gvfs_conf_file = p;
					goto next;
				}

				if (argv[++i]) {
					gvfs_conf_file = (u_char *)argv[i];
					goto next;
				}

				printf("option \"-c\" requires parameter(gvfs conf file)\n");
				return GVFS_ERROR;

			case 's':
				if (*p) {
					gvfs_signal = (char *) p;

				} else if (argv[++i]) {
					gvfs_signal = argv[i];

				} else {
					printf("option \"-s\" requires parameter(stop or quit)\n");
					return GVFS_ERROR;
				}

				if (strcmp(gvfs_signal, "stop") == 0
					|| strcmp(gvfs_signal, "quit") == 0)
				{
					gvfs_process = GVFS_PROCESS_SIGNALLER;
					goto next;
				}

				printf("invalid option: \"-s %s\"\n", gvfs_signal);
				return GVFS_ERROR;

			default:
				printf("invalid option: \"%c\"\n", *(p - 1));
				return GVFS_ERROR;
			}
		}

	next:

		continue;
	}

	return GVFS_OK;
}

static void *gvfs_core_module_create_conf(gvfs_cycle_t *cycle)
{
	if (g_gvfs_ccf) {
		return g_gvfs_ccf;
	}

	g_gvfs_ccf = gvfs_palloc(cycle->pool, sizeof(gvfs_core_conf_t));

	g_gvfs_ccf->daemon = FALSE;
	g_gvfs_ccf->master = FALSE;
	
	return g_gvfs_ccf;
}

static char *gvfs_core_module_init_conf(gvfs_cycle_t *cycle)
{
	gvfs_conf_t    *cf;
	gvfs_command_t *cmd;
	size_t          i, cmds_size;

	cmds_size = sizeof(gvfs_core_commands) / sizeof(gvfs_core_commands[0]);

	for (i = 0; i < cmds_size; i++) {
		cmd = &gvfs_core_commands[i];

		cf = gvfs_get_conf_specific(cycle->conf, cmd->name, cmd->type);
		if (NULL == cf) {
			continue;
		}

		if (cmd->set) {
			cmd->set(cf, cmd, g_gvfs_ccf);
		}
	}
	
	return GVFS_CONF_OK;
}

static gvfs_int_t
gvfs_save_argv(int argc, char *const *argv)
{
    size_t     len;
    gvfs_int_t  i;

    gvfs_os_argv = (char **) argv;
    gvfs_argc = argc;

    gvfs_argv = gvfs_alloc((argc + 1) * sizeof(char *));
    if (gvfs_argv == NULL) {
        return GVFS_ERROR;
    }

    for (i = 0; i < argc; i++) {
        len = gvfs_strlen(argv[i]) + 1;

		// 直接走malloc
        gvfs_argv[i] = gvfs_alloc(len);
        if (gvfs_argv[i] == NULL) {
            return GVFS_ERROR;
        }

		// 内部实现拷贝len字节后再拷贝'\0'
        (void) gvfs_cpystrn((u_char *) gvfs_argv[i], (u_char *) argv[i], len);
    }

    gvfs_argv[i] = NULL;

    gvfs_os_environ = environ;

    return GVFS_OK;
}
