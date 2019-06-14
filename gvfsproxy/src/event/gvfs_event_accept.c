

#include <gvfs_config.h>
#include <gvfs_core.h>
#include <gvfs_event.h>


static gvfs_int_t gvfs_enable_accept_events(gvfs_cycle_t *cycle);
static gvfs_int_t gvfs_disable_accept_events(gvfs_cycle_t *cycle);
static void gvfs_close_accepted_connection(gvfs_connection_t *c);


void
gvfs_event_accept(gvfs_event_t *ev)
{
    socklen_t           socklen;
    gvfs_err_t          err;
    gvfs_socket_t       s;
    gvfs_event_t       *rev, *wev;
    gvfs_listening_t   *ls;
    gvfs_connection_t  *c, *lc;
    char               *addr_text;
	struct sockaddr_in  sa;

    lc = ev->data;
    ls = lc->listening;
    
    ev->ready = 0;

    socklen = sizeof(struct sockaddr_in);

    s = accept(lc->fd, (struct sockaddr *) &sa, &socklen);

    if (s == -1) {
        err = errno;

        if (err == EAGAIN) {
            gvfs_log(LOG_INFO, "accept() on %s not ready",
            				   ls->addr_text.data);
            return;
        }

        
        gvfs_log(LOG_ERROR, "accept() on %s failed, error: %s",
        					ls->addr_text.data, gvfs_strerror(err));

        return;
    }

    c = gvfs_get_connection(s);
    if (c == NULL) {
        if (gvfs_close_socket(s) == -1) {
        	err = errno;
            gvfs_log(LOG_ERROR, "close(%d) failed, error: %s",
            					s, gvfs_strerror(err));
        }

        return;
    }

	/* ls->pool_size = HTTP_POOL_SIZE = 4096 */
    c->pool = gvfs_create_pool(ls->pool_size);
    if (c->pool == NULL) {
        gvfs_close_accepted_connection(c);
        return;
    }

    addr_text = gvfs_palloc(c->pool, INET_ADDRSTRLEN);
    if (NULL == inet_ntop(AF_INET, &(sa.sin_addr), addr_text, INET_ADDRSTRLEN))
    {
    	err = errno;
    	gvfs_log(LOG_WARN, "inet_ntop() for fd: %d failed, error: %s, ignore",
    						s, gvfs_strerror(err));
    }
    
    c->addr_text.len = INET_ADDRSTRLEN;        /* INET_ADDRSTRLEN = 16 Byte */
    c->addr_text.data = (u_char *) addr_text;
    
	if (gvfs_nonblocking(s) == -1) {
		gvfs_log(LOG_ERROR, "ioctl(FIONBIO) fd:%d failed", s);
		gvfs_close_accepted_connection(c);
		return;
	}
	
    c->recv = gvfs_unix_recv;
    c->send = gvfs_unix_send;
	c->sent = 0;
    c->listening = ls;


    rev = c->read;
    wev = c->write;

    wev->ready = 1;

    if (ev->deferred_accept) {
        rev->ready = 1;
    }

    gvfs_log(LOG_INFO, "accept from:%s fd:%d", ls->addr_text.data, c->fd);
    					
	if (NULL == ls->handler) {
		gvfs_close_accepted_connection(c);
		return;
	}
	
    ls->handler(c);
}



gvfs_int_t
gvfs_trylock_accept_mutex(gvfs_cycle_t *cycle)
{
    if (gvfs_shmtx_trylock(&gvfs_accept_mutex)) {

    	
        if (gvfs_accept_mutex_held) {
            return GVFS_OK;
        }
		
        if (gvfs_enable_accept_events(cycle) == GVFS_ERROR) {
            gvfs_shmtx_unlock(&gvfs_accept_mutex);
            return GVFS_ERROR;
        }

        gvfs_accept_mutex_held = 1;

        return GVFS_OK;
    }
    

    if (gvfs_accept_mutex_held) {
    
        if (gvfs_disable_accept_events(cycle) == GVFS_ERROR) {
            return GVFS_ERROR;
        }

        gvfs_accept_mutex_held = 0;
    }
	
    return GVFS_OK;
}


static gvfs_int_t
gvfs_enable_accept_events(gvfs_cycle_t *cycle)
{
    gvfs_uint_t         i;
    gvfs_listening_t   *ls;
    gvfs_connection_t  *c;

    ls = cycle->listening.elts;
    for (i = 0; i < cycle->listening.nelts; i++) {

        c = ls[i].connection;
		
        if (gvfs_epoll_add_event(c->read, GVFS_READ_EVENT, GVFS_LEVEL_EVENT) == GVFS_ERROR) {
            return GVFS_ERROR;
        }
    }

    return GVFS_OK;
}


static gvfs_int_t
gvfs_disable_accept_events(gvfs_cycle_t *cycle)
{
    gvfs_uint_t         i;
    gvfs_listening_t   *ls;
    gvfs_connection_t  *c;

    ls = cycle->listening.elts;
    for (i = 0; i < cycle->listening.nelts; i++) {

        c = ls[i].connection;

        if (!c->read->active) {
            continue;
        }
		
        if (gvfs_epoll_del_event(c->read, GVFS_READ_EVENT, 0)
            == GVFS_ERROR)
        {
            return GVFS_ERROR;
        }
    }

    return GVFS_OK;
}


static void
gvfs_close_accepted_connection(gvfs_connection_t *c)
{
    gvfs_socket_t  fd;

    gvfs_free_connection(c);

    fd = c->fd;
    c->fd = (gvfs_socket_t) -1;

    if (gvfs_close_socket(fd) == -1) {
        gvfs_log(LOG_ERROR, "close(%d) failed", fd);
    }

    if (c->pool) {
        gvfs_destroy_pool(c->pool);
    }
}

