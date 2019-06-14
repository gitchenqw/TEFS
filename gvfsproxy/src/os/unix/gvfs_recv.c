

#include <gvfs_config.h>
#include <gvfs_core.h>
#include <gvfs_event.h>


ssize_t
gvfs_unix_recv(gvfs_connection_t *c, u_char *buf, size_t size)
{
	ssize_t 	   n;
	gvfs_err_t	   err;
	gvfs_event_t  *rev;

	rev = c->read;

	do {
		n = recv(c->fd, buf, size, 0);

		gvfs_log(LOG_DEBUG, "recv: fd:%d %ld of %lu", c->fd, n, size);

		if (n == 0) {
			rev->ready = 0;
			rev->eof = 1;
			return n;

		} else if (n > 0) {
			return n;
		}

		err = errno;

		if (err == EAGAIN || err == EINTR) {
			gvfs_log(LOG_DEBUG, "recv(%d) not ready", c->fd);
			n = GVFS_AGAIN;

		} else {
			gvfs_log(LOG_ERROR, "recv(%d) failed, error:%s", c->fd,
								gvfs_strerror(err));
			n = gvfs_connection_error(err);
			break;
		}

	} while (err == EINTR);

	rev->ready = 0;

	if (n == GVFS_ERROR) {
		rev->error = 1;
	}

	return n;
}

