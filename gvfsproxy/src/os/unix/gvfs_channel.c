

#include <gvfs_config.h>
#include <gvfs_core.h>
#include <gvfs_channel.h>


gvfs_int_t
gvfs_write_channel(gvfs_socket_t s, gvfs_channel_t *ch, size_t size)
{
	ssize_t             n;
	int                 err;
	struct iovec        iov[1];
	struct msghdr       msg;

	union {
		struct cmsghdr  cm;
		char            space[CMSG_SPACE(sizeof(int))];
	} cmsg;

    if (ch->fd == -1) {
        msg.msg_control = NULL;
        msg.msg_controllen = 0;

    } else { 
        msg.msg_control = (caddr_t) &cmsg;
        msg.msg_controllen = sizeof(cmsg);

        cmsg.cm.cmsg_len = CMSG_LEN(sizeof(int));
        cmsg.cm.cmsg_level = SOL_SOCKET;
        cmsg.cm.cmsg_type = SCM_RIGHTS;

		gvfs_memcpy(CMSG_DATA(&cmsg.cm), &ch->fd, sizeof(int));
    }

    msg.msg_flags = 0;

    iov[0].iov_base = (char *) ch;
    iov[0].iov_len = size;

    msg.msg_name = NULL;
    msg.msg_namelen = 0;
    
    msg.msg_iov = iov;
    msg.msg_iovlen = 1;

    n = sendmsg(s, &msg, 0);

    if (n == -1) {
        err = errno;
        if (err == EAGAIN) {
            return GVFS_AGAIN;
        }

        gvfs_log(LOG_ERROR, "sendmsg() failed, error: %s",
        					gvfs_strerror(err));
        					
        return GVFS_ERROR;
    }

    return GVFS_OK;
}


gvfs_int_t
gvfs_read_channel(gvfs_socket_t s, gvfs_channel_t *ch, size_t size)
{
    ssize_t             n;
    int                 err;
    struct iovec        iov[1];
    struct msghdr       msg;

    union {
        struct cmsghdr  cm;
        char            space[CMSG_SPACE(sizeof(int))];
    } cmsg;

    iov[0].iov_base = (char *) ch;
    iov[0].iov_len = size;

    msg.msg_name = NULL;
    msg.msg_namelen = 0;
    msg.msg_iov = iov;
    msg.msg_iovlen = 1;

    msg.msg_control = (caddr_t) &cmsg;
    msg.msg_controllen = sizeof(cmsg);

    n = recvmsg(s, &msg, 0);

    if (n == -1) {
        err = errno;
        if (err == EAGAIN) {
            return GVFS_AGAIN;
        }

        gvfs_log(LOG_ERROR, "recvmsg() failed, error: %s",
        					gvfs_strerror(err));
        					
        return GVFS_ERROR;
    }

    if (n == 0) {
    	err = errno;
        gvfs_log(LOG_ERROR, "recvmsg() returned zero, errorinfo:%s",
        					gvfs_strerror(err));
        					
        return GVFS_ERROR;
    }

    if ((size_t) n < sizeof(gvfs_channel_t)) {
        gvfs_log(LOG_ERROR, "recvmsg() returned not enough data: %ld", n);
        return GVFS_ERROR;
    }


    if (ch->command == GVFS_CMD_OPEN_CHANNEL) {

        if (cmsg.cm.cmsg_len < (socklen_t) CMSG_LEN(sizeof(int))) {
            gvfs_log(LOG_ERROR, "%s", "recvmsg() returned too small ancillary data");
            return GVFS_ERROR;
        }

        if (cmsg.cm.cmsg_level != SOL_SOCKET || cmsg.cm.cmsg_type != SCM_RIGHTS)
        {
            gvfs_log(LOG_ERROR,
            
            		 "recvmsg() returned invalid ancillary data level %d or type %d",
                     cmsg.cm.cmsg_level, cmsg.cm.cmsg_type);
                     
            return GVFS_ERROR;
        }

        /* ch->fd = *(int *) CMSG_DATA(&cmsg.cm); */
		/* 将获取到的fd拷贝出去 */
        gvfs_memcpy(&ch->fd, CMSG_DATA(&cmsg.cm), sizeof(int));
    }

    if (msg.msg_flags & (MSG_TRUNC|MSG_CTRUNC)) {
        gvfs_log(LOG_ERROR, "%s", "recvmsg() truncated data");
    }


    return n;
}


gvfs_int_t
gvfs_add_channel_event(gvfs_cycle_t *cycle, gvfs_fd_t fd, gvfs_int_t event,
    gvfs_event_handler_pt handler)
{
    gvfs_event_t       *ev, *rev, *wev;
    gvfs_connection_t  *c;

    c = gvfs_get_connection(fd);

    if (c == NULL) {
    	gvfs_log(LOG_ERROR, "gvfs_get_connection(%d) failed", fd);
        return GVFS_ERROR;
    }

    c->pool = cycle->pool;

    rev = c->read;
    wev = c->write;

    rev->channel = 1;
    wev->channel = 1;

    ev = (event == GVFS_READ_EVENT) ? rev : wev;
    ev->handler = handler;

    if (GVFS_ERROR == gvfs_epoll_add_event(ev, GVFS_READ_EVENT, GVFS_CLEAR_EVENT)) {
    	return GVFS_ERROR;
    }

    return GVFS_OK;
}


void
gvfs_close_channel(gvfs_fd_t *fd)
{
    if (close(fd[0]) == -1) {
        gvfs_log(LOG_ERROR, "close() channel failed, error: %s",
        					gvfs_strerror(errno));
    }

    if (close(fd[1]) == -1) {
        gvfs_log(LOG_ERROR, "close() channel failed, error: %s",
        					gvfs_strerror(errno));
    }
}
