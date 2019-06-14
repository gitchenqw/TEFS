

#include <gvfs_config.h>
#include <gvfs_core.h>
#include <gvfs_http_config.h>


gvfs_http_core_conf_t *g_gvfs_hccf = NULL;

static void *gvfs_http_core_module_create_conf(gvfs_cycle_t *cycle);
static char *gvfs_http_core_module_init_conf(gvfs_cycle_t *cycle);

static INT32 gvfs_http_core_module_init(gvfs_cycle_t *cycle); 
static INT32 gvfs_http_process_init(gvfs_cycle_t *cycle);

static gvfs_command_t gvfs_http_commands[] = {
	{"http", GET_CONF_CURRENT,
	 NULL, 0},
	 
	{"port", GET_CONF_CHILD,
	 gvfs_conf_set_num_slot, offsetof(gvfs_http_core_conf_t, port)},
	 
	{"ip_addr", GET_CONF_NEXT,
	 gvfs_conf_set_str_slot, offsetof(gvfs_http_core_conf_t, ip_addr)},
	 
	{"chunk_num", GET_CONF_NEXT,
	 gvfs_conf_set_num_slot, offsetof(gvfs_http_core_conf_t, chunk_num)},

	{"chunk_size", GET_CONF_NEXT,
	 gvfs_conf_set_num_slot, offsetof(gvfs_http_core_conf_t, chunk_size)},

	{"connection_timeout", GET_CONF_NEXT,
	 gvfs_conf_set_num_slot, offsetof(gvfs_http_core_conf_t, timeout)}
};

gvfs_module_t gvfs_http_module = {
	.name         = "hper_http_module",
	.create_conf  = gvfs_http_core_module_create_conf,
	.init_conf    = gvfs_http_core_module_init_conf,
	.init_module  = gvfs_http_core_module_init,
	.init_process = gvfs_http_process_init,
	.exit_process = NULL,
	.exit_master  = NULL,
};

static INT32 gvfs_http_core_module_init(gvfs_cycle_t *cycle)
{
    gvfs_listening_t           *ls;

    ls = gvfs_create_listening(cycle);
    if (NULL == ls) {
    	return GVFS_ERROR;
    }

	ls->pool_size = HTTP_POOL_SIZE;
	ls->backlog = HTTP_BACKLOG;
	ls->sndbuf = HTTP_SENDBUF;
	ls->rcvbuf = HTTP_RECVBUF;
	
    ls->handler = gvfs_http_init_connection;
    
    return GVFS_OK;
}

static INT32 gvfs_http_process_init(gvfs_cycle_t *cycle)
{
	if (GVFS_OK != gvfs_http_create_request_body_buf_list(
		g_gvfs_hccf->chunk_size * g_gvfs_hccf->chunk_num, g_gvfs_ecf->connections))
	{
		gvfs_log(LOG_ERROR, "gvfs_http_create_request_body_buf_list(%lu, %ld) failed",
							g_gvfs_hccf->chunk_size * g_gvfs_hccf->chunk_num,
							g_gvfs_ecf->connections);
		return GVFS_ERROR;
	}
	
	gvfs_log(LOG_DEBUG, "gvfs_http_create_request_body_buf_list(%lu, %ld) success",
						g_gvfs_hccf->chunk_size * g_gvfs_hccf->chunk_num,
						g_gvfs_ecf->connections);									   
    return GVFS_OK;
}


static void *
gvfs_http_core_module_create_conf(gvfs_cycle_t * cycle)
{
	if (g_gvfs_hccf) {
		return g_gvfs_hccf;
	}

	g_gvfs_hccf = gvfs_palloc(cycle->pool, sizeof(gvfs_http_core_conf_t));

	g_gvfs_hccf->port = HTTP_LISTEN_PORT;
	g_gvfs_hccf->ip_addr = HTTP_SERVICE_IP;
	g_gvfs_hccf->chunk_num = HTTP_MAIN_CHUNK_NUM;
	g_gvfs_hccf->chunk_size = HTTP_MAIN_CHUNK_SIZE;
	g_gvfs_hccf->timeout = HTTP_CONNECTION_TIMEOUT;
	
	return g_gvfs_hccf;
}


static char *
gvfs_http_core_module_init_conf(gvfs_cycle_t * cycle)
{
	gvfs_conf_t    *cf;
	gvfs_command_t *cmd;
	size_t		   i, cmds_size;

	cmds_size = sizeof(gvfs_http_commands) / sizeof(gvfs_http_commands[0]);
	cf = cycle->conf;
	
	for (i = 0; i < cmds_size; i++) {
		cmd = &gvfs_http_commands[i];

		cf = gvfs_get_conf_specific(cf, cmd->name, cmd->type);
		if (NULL == cf) {
			continue;
		}
		
		if (cmd->set) {
			cmd->set(cf, cmd, g_gvfs_hccf);
		}
	}
	
	return GVFS_CONF_OK;
}
