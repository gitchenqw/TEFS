

#include <gvfs_config.h>
#include <gvfs_core.h>
#include <gvfs_upstream.h>


static void *
gvfs_upstream_module_create_conf(gvfs_cycle_t *cycle);
static char *
gvfs_upstream_module_init_conf(gvfs_cycle_t *cycle);
static INT32 
gvfs_upstream_module_init(gvfs_cycle_t *cycle); 

static gvfs_int_t gvfs_upstream_init_request(gvfs_http_request_t *r);

const CHAR *ns_ip_addr = NULL;
INT32       ns_port    = 0;

gvfs_upstream_conf_t *g_gvfs_ucf = NULL;

static gvfs_command_t gvfs_upstream_commands[] = {
	{"upstream", GET_CONF_CURRENT,
	 NULL, 0},
	 
	{"port", GET_CONF_CHILD,
	 gvfs_conf_set_num_slot, offsetof(gvfs_upstream_conf_t, port)},
	 
	{"ip_addr", GET_CONF_NEXT,
	 gvfs_conf_set_str_slot, offsetof(gvfs_upstream_conf_t, ip_addr)}
};

gvfs_module_t gvfs_upstream_module = {
	.name         = "gvfs_upstream_module",
	.create_conf  = gvfs_upstream_module_create_conf,
	.init_conf    = gvfs_upstream_module_init_conf,
	.init_module  = gvfs_upstream_module_init,
	.init_process = NULL,
	.exit_process = NULL,
	.exit_master  = NULL,
};

gvfs_int_t gvfs_upstream_create(gvfs_http_request_t *r,
	const CHAR *ip_addr, INT32 port)
{
	gvfs_upstream_t		      *upstream;
	socklen_t				   addrlen;
	struct sockaddr_in		   addr;
	
	upstream = r->upstream;

	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = inet_addr(ip_addr);

	addrlen = sizeof(addr);
	gvfs_memcpy(upstream->peer.sockaddr, &addr, addrlen);
	upstream->peer.socklen = addrlen;

	snprintf((char *)upstream->peer.addr_text.data, 32,
			"%s:%d",
			ip_addr, port);

	return GVFS_OK;
}


gvfs_int_t gvfs_upstream_connect(gvfs_http_request_t *r,
    gvfs_upstream_t *upstream)
{
	gvfs_connection_t    *c, *cc;
    gvfs_int_t            rc;

    rc = gvfs_event_connect_peer(&upstream->peer);

    if (GVFS_OK != rc) {
    	c = r->connection;
    	gvfs_log(LOG_ERROR, "connect to peer:%s for client:%s fd:%d failed",
    						upstream->peer.addr_text.data, c->addr_text.data,
    						c->fd);
    	return GVFS_ERROR;
    }

	c = upstream->peer.connection;
	cc = r->connection;
	c->data = upstream;
	
    gvfs_log(LOG_INFO, "connect to peer:%s fd:%d for client:%s fd:%d success",
    					c->addr_text.data, c->fd, cc->addr_text.data, cc->fd);
    					
    return GVFS_OK;
}

static gvfs_int_t gvfs_upstream_init_request(gvfs_http_request_t *r)
{
	gvfs_upstream_t        *upstream;

	upstream = r->upstream;

	return gvfs_upstream_connect(r, upstream);
}


gvfs_int_t gvfs_upstream_init(gvfs_http_request_t *r, const CHAR *ip_addr,
	INT32 port)
{
	if (GVFS_OK != gvfs_upstream_create(r, ip_addr, port)) {
		return GVFS_ERROR;
	}
	
	return gvfs_upstream_init_request(r);
}

static INT32 gvfs_upstream_module_init(gvfs_cycle_t *cycle)
{
#if 0
	gvfs_pool_t        *pool;
    struct addrinfo    *res;
    struct sockaddr_in *ai_addr_in;
    CHAR               *addr_text;

    CHAR                service[8];
    struct addrinfo     hints;
    
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_flags = AI_CANONNAME;
	hints.ai_family = AF_UNSPEC;   
	hints.ai_socktype = SOCK_STREAM;

	pool = cycle->pool;
	snprintf(service, sizeof(service), "%u", g_gvfs_ucf->port);

	if (0 != getaddrinfo(g_gvfs_ucf->ip_addr, service, &hints, &res)) {
		gvfs_log(LOG_ERROR, "getaddrinfo(\"%s\") failed, error:%s",
				 g_gvfs_ucf->ip_addr, gvfs_strerror(errno));
		return GVFS_ERROR;
	}

	ai_addr_in = (struct sockaddr_in *) res->ai_addr;
    addr_text = hper_pcalloc(pool, INET_ADDRSTRLEN);
    if (NULL == inet_ntop(res->ai_family, &ai_addr_in->sin_addr, addr_text,
    					  INET_ADDRSTRLEN))
    {
    	gvfs_log(LOG_ERROR, "inet_ntop(\"%s\") failed, error:%s",
    			 g_gvfs_ucf->ip_addr, gvfs_strerror(errno));
    			   
    	freeaddrinfo(res);
    	return GVFS_ERROR;
    }
    
    ns_ip_addr = addr_text;
    ns_port = g_gvfs_ucf->port;
	freeaddrinfo(res);
	
#endif

	ns_ip_addr = g_gvfs_ucf->ip_addr;
	ns_port = g_gvfs_ucf->port;
	
	return GVFS_OK;
}

static void *
gvfs_upstream_module_create_conf(gvfs_cycle_t *cycle)
{
	if (g_gvfs_ucf) {
		return g_gvfs_ucf;
	}

	g_gvfs_ucf = gvfs_palloc(cycle->pool, sizeof(gvfs_upstream_conf_t));

	return g_gvfs_ucf;
}

static char *
gvfs_upstream_module_init_conf(gvfs_cycle_t *cycle)
{
	gvfs_conf_t 	*conf;
	gvfs_command_t	*cmd;
	size_t			 i, cmd_size;

	conf = cycle->conf;

	cmd_size = sizeof(gvfs_upstream_commands) / sizeof(gvfs_upstream_commands[0]);

	for (i = 0; i < cmd_size; i++) {
		cmd = &gvfs_upstream_commands[i];

		conf = gvfs_get_conf_specific(conf, cmd->name, cmd->type);
		if (NULL == conf) {
			continue;
		}
		
		if (cmd->set) {
			cmd->set(conf, cmd, g_gvfs_ucf);
		}
	}

	return GVFS_CONF_OK;
}


void gvfs_free_upstream(gvfs_upstream_t *upstream)
{
	gvfs_pool_t              *pool;
	gvfs_connection_t        *c;
	
	c = upstream->peer.connection;
	
	pool = c->pool;

	gvfs_destroy_pool(pool);
}

void gvfs_close_upstream(gvfs_http_request_t *r)
{
	gvfs_connection_t *c;
	gvfs_upstream_t   *upstream;

	upstream = r->upstream;
	c = upstream->peer.connection;
	if (NULL == c) {
		return;
	}

	if (NULL != upstream->download_body) {
		gvfs_http_free_request_body_buf(upstream->download_body);
		upstream->download_body = NULL;
	}
	
	gvfs_free_upstream(upstream);

	gvfs_close_connection(c);
	upstream->peer.connection = NULL;
}

