/*
 * 1)对于Linux本身可以通过sys_errlist[errno]获取错误描述,但编译告警
 * 2)GVFS_SYS_NERR通过sys_nerr值获取
 * 3)为何不直接使用strerror()和strerror_r()的原因是非异步信号安全,strerror_r()
 *   仅仅是线程安全
 * 4)该实现的最大价值在于在信号处理函数中安全使用errno值
 */


#include <gvfs_config.h>
#include <gvfs_core.h>


static gvfs_str_t  *gvfs_sys_errlist;
static gvfs_str_t   gvfs_unknown_error = gvfs_string("Unknown error");


u_char *
gvfs_strerror(gvfs_err_t err)
{
    gvfs_str_t  *msg;

    msg = ((gvfs_uint_t) err < GVFS_SYS_NERR) ? &gvfs_sys_errlist[err]:
                                                &gvfs_unknown_error;

    return msg->data;
}


gvfs_uint_t
gvfs_strerror_init(void)
{
	char       *msg;
	u_char     *p;
	size_t      len;
	gvfs_err_t  err;

	len = GVFS_SYS_NERR * sizeof(gvfs_str_t);

	gvfs_sys_errlist = malloc(len);
	if (gvfs_sys_errlist == NULL) {
		goto failed;
	}

    for (err = 0; err < GVFS_SYS_NERR; err++) {
        msg = strerror(err);
        len = gvfs_strlen(msg) + 1; /* 确保错误描述字符串是以\0结尾的 */

        p = malloc(len);
        if (p == NULL) {
            goto failed;
        }

        gvfs_cpystrn((u_char *) p, (u_char *) msg, len);
        gvfs_sys_errlist[err].len = len;
        gvfs_sys_errlist[err].data = p;
    }

	return GVFS_OK;

failed:

	err = errno;
	gvfs_log(LOG_ERROR, "malloc(%lu) failed (%d: %s)", len, err, strerror(err));

	return GVFS_ERROR;
}
