

#include <gvfs_config.h>
#include <gvfs_core.h>
#include <gvfs_event.h>


ssize_t
gvfs_unix_send(gvfs_connection_t *c, u_char *buf, size_t size)
{
	ssize_t 	   n;
	gvfs_err_t	   err;
	gvfs_event_t  *wev;

	wev = c->write;

	for ( ;; ) {
		n = send(c->fd, buf, size, 0);

		gvfs_log(LOG_DEBUG, "send: fd:%d %ld of %lu", c->fd, n, size);

		if (n > 0) {
			if (n < (ssize_t) size) {
				wev->ready = 0;
			}

			c->sent += n;

			return n;
		}

		err = errno;

		if (n == 0) {
			gvfs_log(LOG_ERROR, "send(%d) returned zero", c->fd);
			wev->ready = 0;
			return n;
		}

		if (err == EAGAIN || err == EINTR) {
			wev->ready = 0;

			gvfs_log(LOG_DEBUG, "send(%d) not ready", c->fd);

			if (err == EAGAIN) {
				return GVFS_AGAIN;
			}

		} else {
			wev->error = 1;
			gvfs_log(LOG_ERROR, "send(%d) failed, error:%s", c->fd,
								gvfs_strerror(err));
			return GVFS_ERROR;
		}
	}
}

INT32 gvfs_completed_send(gvfs_connection_t *c, UINT8 *data,
	size_t size)
{
	size_t n = 0;
	
	for ( ; ; ) {
		n = c->send(c, data, size);

		/* 返回EINTR c->send内部已经作了继续发送处理 */
		if (GVFS_AGAIN == n || 0 == n) {
			gvfs_msleep(100);
			continue;
		}
		else if (GVFS_ERROR == n) {
			return GVFS_ERROR;
		}

		data += n;
		size -= n;

		if (0 == size) {
			break;
		}
	}
	
	return GVFS_OK;
}

