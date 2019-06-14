

#include <gvfs_config.h>
#include <gvfs_core.h>


gvfs_int_t gvfs_event_connect_peer(gvfs_peer_connection_t *pc)
{
    gvfs_err_t         err;
    gvfs_socket_t      s;
    gvfs_event_t      *rev, *wev;
    gvfs_connection_t *c;

    s = socket(pc->sockaddr->sa_family, SOCK_STREAM, 0);

    if (-1 == s) {
    	err = errno;
    	
    	gvfs_log(LOG_ERROR, "socket() to peer: %s failed, error: %s",
    						pc->addr_text.data, gvfs_strerror(err));
    	return GVFS_ERROR;
    }

    c = gvfs_get_connection(s);

    if (NULL == c) {
    	if (-1 == gvfs_close_socket(s)) {
    	
    		err = errno;
    		
    		gvfs_log(LOG_ERROR, "close(%d) failed, error: %s",
    							s, gvfs_strerror(err));
    		return GVFS_ERROR;
    	}

    	gvfs_log(LOG_ERROR, "get connection for peer: %s failed",
    						pc->addr_text.data);
    	return GVFS_ERROR;
    }

	pc->rcvbuf = 16384;
    if (pc->rcvbuf) {
        if (setsockopt(s, SOL_SOCKET, SO_RCVBUF,
                       (const void *) &pc->rcvbuf, sizeof(int)) == -1)
        {
            gvfs_log(LOG_ERROR, "setsockopt(SO_RCVBUF) for peer: %s failed",
            					pc->addr_text.data);
            goto failed;
        }
    }

	
	pc->sendbuf = 65536;
    if (pc->sendbuf) {
        if (setsockopt(s, SOL_SOCKET, SO_SNDBUF,
                       (const void *) &pc->sendbuf, sizeof(int)) == -1)
        {
            gvfs_log(LOG_ERROR, "setsockopt(SO_SNDBUF) for peer: %s failed",
            					pc->addr_text.data);
            goto failed;
        }
    }

    c->recv = gvfs_unix_recv;
    c->send = gvfs_unix_send;

    rev = c->read;
    wev = c->write;

	c->addr_text.data = pc->addr_text.data;
	c->addr_text.len = pc->addr_text.len;
	
    pc->connection = c;

	/* 可将nonblocking提前设置,判断EINPROGRESS返回成功,添加到epoll和定时器中判断 */
	while (-1 == connect (s, pc->sockaddr, pc->socklen) && errno != EISCONN) {
		if (EINTR != errno) {
			gvfs_log(LOG_ERROR, "connect to peer: %s failed, error: %s",
				pc->addr_text.data, gvfs_strerror(errno));
			goto failed;	
		}
	}
    
    if (-1 == gvfs_nonblocking(s)) {
    	err = errno;
    	
        gvfs_log(LOG_ERROR, "nonblocking(%d) for peer: %s failed, error: %s",
        					s, pc->addr_text.data, gvfs_strerror(err));

        goto failed;
    }

    
	return GVFS_OK;
	
failed:

	gvfs_close_connection(c);
	pc->connection = NULL;

	return GVFS_ERROR;
}
