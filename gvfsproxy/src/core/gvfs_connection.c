#include <gvfs_config.h>
#include <gvfs_core.h>
#include <gvfs_event.h>



gvfs_connection_t *
gvfs_get_connection(gvfs_socket_t s)
{
    gvfs_uint_t         instance;
    gvfs_event_t       *rev, *wev;
    gvfs_connection_t  *c;

    c = gvfs_cycle->free_connections;

    if (c == NULL) {
        gvfs_log(LOG_ERROR, "%lu worker_connections are not enough",
                      gvfs_cycle->connection_n);
        return NULL;
    }

    gvfs_cycle->free_connections = c->data;
    gvfs_cycle->free_connection_n--;

    rev = c->read;
    wev = c->write;

    gvfs_memzero(c, sizeof(gvfs_connection_t));

    c->read = rev;
    c->write = wev;
    c->fd = s;

    instance = rev->instance;

    gvfs_memzero(rev, sizeof(gvfs_event_t));
    gvfs_memzero(wev, sizeof(gvfs_event_t));
	INIT_LIST_HEAD(&rev->lh);
	INIT_LIST_HEAD(&wev->lh);

    rev->instance = !instance;
    wev->instance = !instance;

    rev->data = c;
    wev->data = c;

    wev->write = 1;

    return c;
}

void gvfs_free_connection(gvfs_connection_t *c)
{
    c->data = gvfs_cycle->free_connections;
    gvfs_cycle->free_connections = c;
    gvfs_cycle->free_connection_n++;
}

void gvfs_close_connection(gvfs_connection_t *c)
{
	gvfs_err_t        err;
	gvfs_socket_t     fd;

	if (-1 == c->fd) {
		gvfs_log(LOG_WARN, "%s", "connection already closed");
		return;
	}

	if (c->read->timer_set) {
		gvfs_event_del_timer(c->read);
	}

	if (c->write->timer_set) {
		gvfs_event_del_timer(c->write);
	}

	/* close(fd)之后, epoll自动将其从事件集合中删除 */
	gvfs_epoll_del_connection(c, GVFS_CLOSE_EVENT);

	if (!list_empty(&c->read->lh)) {
		list_del(&c->read->lh);
	}

	if (!list_empty(&c->write->lh)) {
		list_del(&c->write->lh);
	}

	gvfs_free_connection(c);

	fd = c->fd;
	c->fd = (gvfs_socket_t) -1;

	if (-1 == gvfs_close_socket(fd)) {
		err = errno;
		gvfs_log(LOG_ERROR, "close(%d) socket failed, error: %s",
							fd, gvfs_strerror(err));
	}

	gvfs_log(LOG_INFO, "close fd:%d", fd);
}


gvfs_listening_t *
gvfs_create_listening(gvfs_cycle_t *cycle)
{
	gvfs_listening_t           *ls;
    struct sockaddr            *sa;
	socklen_t                   addrlen;
	struct sockaddr_in          addr;
	
	addr.sin_family = AF_INET;
	addr.sin_port = htons(g_gvfs_hccf->port);
	addr.sin_addr.s_addr = inet_addr(g_gvfs_hccf->ip_addr);

	addrlen = sizeof(addr);
	
	ls = gvfs_array_push(&cycle->listening);
	if (ls == NULL) {
		return NULL;
	}
	
	gvfs_memzero(ls, sizeof(gvfs_listening_t));
	
    sa = gvfs_palloc(cycle->pool, addrlen);
    if (sa == NULL) {
        return NULL;
    }

    gvfs_memcpy(sa, &addr, addrlen);

    ls->fd = (gvfs_socket_t) -1;
    
    ls->sockaddr = sa;
    ls->socklen = addrlen;
    ls->addr_text.data = (UCHAR *) g_gvfs_hccf->ip_addr;
    ls->addr_text.len = strlen(g_gvfs_hccf->ip_addr);

    ls->type = SOCK_STREAM;
    ls->backlog = 511;
    ls->rcvbuf = -1;
    ls->sndbuf = -1;

    return ls;
}

gvfs_int_t
gvfs_open_listening_sockets(gvfs_cycle_t *cycle)
{
	gvfs_listening_t 	*ls;
	gvfs_uint_t          i;
	gvfs_socket_t        s;
	gvfs_err_t           err;
    int                  reuseaddr;

	ls = cycle->listening.elts;
	for (i = 0; i < cycle->listening.nelts; i++) {
		if (-1 != ls[i].fd) {
			continue;
		}

		s = gvfs_socket(ls[i].sockaddr->sa_family, ls[i].type, 0);
		if (s == -1) {
			err = errno;
			gvfs_log(LOG_ERROR, "socket(\"%s\") failed, error: %s",
								ls[i].addr_text.data, gvfs_strerror(err));
			return GVFS_ERROR;
		}

		reuseaddr = 1;
		if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR,
					   (const void *) &reuseaddr, sizeof(int))
			== -1)
		{
			err = errno;
			gvfs_log(LOG_ERROR, "setsockopt(SO_REUSEADDR) %s failed, error: %s",
						  ls[i].addr_text.data, gvfs_strerror(err));
		
			if (gvfs_close_socket(s) == -1) {
				err = errno;
				gvfs_log(LOG_ERROR, "close(\"%s\") failed, error: %s",
									ls[i].addr_text.data, gvfs_strerror(err));
			}
		
			return GVFS_ERROR;
		}

		
		if (bind(s, ls[i].sockaddr, ls[i].socklen) == -1) {
			err = errno;
		
			gvfs_log(LOG_ERROR, "bind(\"%s\") failed, error: %s",
								ls[i].addr_text.data, gvfs_strerror(err));
		
			if (gvfs_close_socket(s) == -1) {
				err = errno;
				gvfs_log(LOG_ERROR, "close(\"%s\") failed, error: %s",
									ls[i].addr_text.data, gvfs_strerror(err));
			}
		
			return GVFS_ERROR;
		}

		
		if (listen(s, ls[i].backlog) == -1) {
			err = errno;
			gvfs_log(LOG_ERROR, "listen(\"%s\") backlog %d failed, error: %s",
						        ls[i].addr_text.data, ls[i].backlog,
						        gvfs_strerror(err));
		
			if (gvfs_close_socket(s) == -1) {
				err = errno;
				gvfs_log(LOG_ERROR, "close(\"%s\") failed, error: %s",
									ls[i].addr_text.data, gvfs_strerror(err));
			}
		
			return GVFS_ERROR;
		}
		
		ls[i].listen = 1;
		
		ls[i].fd = s;

	}

	return GVFS_OK;	
}

void
gvfs_configure_listening_sockets(gvfs_cycle_t *cycle)
{
	int                         timeout;
    gvfs_uint_t                 i;
    gvfs_listening_t           *ls;
    
    ls = cycle->listening.elts;
    for (i = 0; i < cycle->listening.nelts; i++) {
        if (ls[i].rcvbuf != -1) {
            if (setsockopt(ls[i].fd, SOL_SOCKET, SO_RCVBUF,
                           (const void *) &ls[i].rcvbuf, sizeof(int))
                == -1)
            {
                gvfs_log(LOG_WARN, "setsockopt(SO_RCVBUF, %d) %s, ignored",
                              		ls[i].rcvbuf, ls[i].addr_text.data);
            }
        }

        if (ls[i].sndbuf != -1) {
            if (setsockopt(ls[i].fd, SOL_SOCKET, SO_SNDBUF,
                           (const void *) &ls[i].sndbuf, sizeof(int))
                == -1)
            {
                gvfs_log(LOG_WARN, "setsockopt(SO_SNDBUF, %d) %s, ignored",
                              		ls[i].sndbuf, ls[i].addr_text.data);
            }
        }

        if (ls[i].listen) {
            if (listen(ls[i].fd, ls[i].backlog) == -1) {
                gvfs_log(LOG_WARN, "listen() to %s, backlog %d failed, ignored",
                              	   ls[i].addr_text.data, ls[i].backlog);
            }
        }

		timeout = 10;
		if (setsockopt(ls[i].fd, IPPROTO_TCP, TCP_DEFER_ACCEPT,
					   &timeout, sizeof(int))
			== -1)
		{
			gvfs_log(LOG_WARN,
				"setsockopt(TCP_DEFER_ACCEPT, %d) for %s failed, ignored",
				timeout, ls[i].addr_text.data);
		
			continue;
		}
    }
}

void
gvfs_close_listening_sockets(gvfs_cycle_t *cycle)
{
    gvfs_uint_t         i;
    gvfs_err_t          err;
    gvfs_listening_t   *ls;
    gvfs_connection_t  *c;

	gvfs_accept_mutex_held = 0;
	gvfs_use_accept_mutex = 0;
	
    ls = cycle->listening.elts;
    for (i = 0; i < cycle->listening.nelts; i++) {
    	c = ls[i].connection;
        if (c && c->read->active) {
        
            gvfs_epoll_del_event(c->read, GVFS_READ_EVENT, 0);
            gvfs_free_connection(c);
            c->fd = (gvfs_socket_t) -1;
        }

		
        gvfs_log(LOG_INFO, "close listening %s fd: %d ", ls[i].addr_text.data,
        					ls[i].fd);
                       
        if (gvfs_close_socket(ls[i].fd) == -1) {
        	err = errno;
			gvfs_log(LOG_ERROR, "close(\"%s\") failed, error: %s",
								ls[i].addr_text.data, gvfs_strerror(err));
        }

        ls[i].fd = (gvfs_socket_t) -1;
    }
}



gvfs_int_t
gvfs_connection_error(gvfs_err_t err)
{
	/* 链接被重置当对端关闭连接处理 */
    if (err == ECONNRESET) {
        return 0;
    }

    return GVFS_ERROR;
}
