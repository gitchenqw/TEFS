

#include <gvfs_config.h>
#include <gvfs_core.h>
#include <gvfs_proxy.h>

volatile gvfs_cycle_t *gvfs_cycle;
gvfs_core_conf_t      *g_gvfs_ccf = NULL;


gvfs_cycle_t *
gvfs_init_cycle(char *conf_file)
{
	void               *rv;
    gvfs_cycle_t       *cycle;
    gvfs_pool_t        *pool;
    gvfs_module_t      *module = NULL;
    
    pool = gvfs_create_pool(GVFS_DEFAULT_POOL_SIZE);
    if (NULL == pool) {
    	gvfs_log(LOG_ERROR, "gvfs_create_pool(%d) failed", GVFS_DEFAULT_POOL_SIZE);
        return NULL;
    }

    cycle = gvfs_palloc(pool, sizeof(gvfs_cycle_t));
    if (NULL == cycle) {
    	gvfs_destroy_pool(pool);
        return NULL;
    }

    cycle->pool = pool;

    cycle->listening.elts = gvfs_palloc(pool, 10 * sizeof(gvfs_listening_t));
    if (NULL == cycle->listening.elts) {
    	gvfs_destroy_pool(pool);
        return NULL;
    }
    cycle->listening.nelts = 0;
    cycle->listening.size = sizeof(gvfs_listening_t);
    cycle->listening.nalloc = 10;
    cycle->listening.pool = pool;
    
    cycle->conf_file = conf_file;

    cycle->conf = gvfs_pcalloc(pool, sizeof(void*));

	/* 解析配置文件 */
    if (GVFS_OK != gvfs_conf_parse(cycle)) {
    	gvfs_destroy_pool(pool);
        return NULL;
    }
    
    while ((module = gvfs_module_next(module))) {
			/* 创建配置文件存储空间 */
			if (module->create_conf) {
				rv = module->create_conf(cycle);
				if (NULL == rv) {
					gvfs_log(LOG_ERROR, "create_conf(\"%s\") failed",
						module->name);
					gvfs_destroy_pool(pool);
					return NULL;
				}
			}

			/* 根据配置文件配置,初始化配置项 */
			if (module->init_conf
				&& GVFS_CONF_OK != module->init_conf(cycle)) {
					gvfs_log(LOG_ERROR, "init_conf(\"%s\") failed",
						module->name);
					gvfs_destroy_pool(pool);
					return NULL;
			}
			
			if (module->init_module
				&& GVFS_OK != module->init_module(cycle)) {
					gvfs_log(LOG_ERROR, "init_module(\"%s\") failed",
						module->name);
					/* fatal */
					exit(1);
			}
			
	}

	/*  打开监听套接字 */
    if (gvfs_open_listening_sockets(cycle) != GVFS_OK) {
    	gvfs_log(LOG_ERROR, "%s", "gvfs_open_listening_sockets() failed");
        exit(1);
    }

    gvfs_configure_listening_sockets(cycle);
    
    return cycle;
}

gvfs_int_t gvfs_create_pidfile(char *name)
{
    size_t       len;
    gvfs_uint_t  create;
    gvfs_file_t  file;
    u_char       pid[GVFS_INT64_LEN + 2];

    gvfs_memzero(&file, sizeof(gvfs_file_t));
	file.name.data = (u_char *) name;

	create = O_CREAT | O_EXCL;

	file.fd = gvfs_open_file(file.name.data, O_RDWR, create, GVFS_FILE_DEFAULT_ACCESS);
	if (GVFS_INVALID_FILE == file.fd) {
		if (EEXIST == errno) {
			create = 0;
			file.fd = gvfs_open_file(name, O_RDWR, 0, GVFS_FILE_DEFAULT_ACCESS);
			if (GVFS_INVALID_FILE == file.fd) {
				gvfs_log(LOG_ERROR, "open(\"%s\") failed, error: %s",
									name, gvfs_strerror(errno));
				return GVFS_ERROR;
			}
			goto create_pidfile;
		} else {
			gvfs_log(LOG_ERROR, "open(\"%s\") failed, error: %s",
								name, gvfs_strerror(errno));
			return GVFS_ERROR;
		}
	}
	
create_pidfile:
	
	if (0 != gvfs_trylock_fd(file.fd)) {
		printf("proce:%s is running\n", GVFS_VER);
		return GVFS_ERROR;
	}
	
	if (-1 == fstat(file.fd, &file.info)) {
		gvfs_log(LOG_ERROR, "fstat(\"%s\") failed, error: %s",
							name, gvfs_strerror(errno));
		return GVFS_ERROR;
	}
	
	len = snprintf((char *)pid, GVFS_INT64_LEN + 2, "%d", gvfs_pid);
	if (0 != file.info.st_size) {
		ftruncate(file.fd, 0);
	}

	gvfs_write_file(&file, pid, len, 0);
	
	return GVFS_OK;
}

gvfs_int_t
gvfs_signal_process(char *sig)
{
    ssize_t            n;
    gvfs_int_t         pid;
    gvfs_file_t        file;
    u_char             buf[GVFS_INT64_LEN + 2];

    file.name.data = (u_char *)GVFS_PID_PATH;
    
    file.fd = gvfs_open_file(file.name.data, O_RDONLY, 0, GVFS_FILE_DEFAULT_ACCESS);

    if (file.fd == GVFS_INVALID_FILE) {
        gvfs_log(LOG_ERROR, "open() \"%s\" failed, error: %s",
        					file.name.data, gvfs_strerror(errno));
        return 1;
    }

    n = gvfs_read_file(&file, buf, GVFS_INT64_LEN + 2, 0);
    if (close(file.fd) == GVFS_FILE_ERROR) {
        gvfs_log(LOG_ERROR, "close() \"%s\" failed, error: %s", 
        					file.name.data, gvfs_strerror(errno));
    }
    if (n == GVFS_ERROR) {
        return 1;
    }

    pid = atoi((char *)buf);
    return gvfs_os_signal_process(sig, pid);

}

