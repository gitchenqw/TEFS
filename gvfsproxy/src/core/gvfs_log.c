
#include <gvfs_config.h>
#include <gvfs_core.h>
#include <zlog.h>

zlog_category_t *zc;

gvfs_int_t gvfs_init_log(const char *zlog_conf_file, const char *name)
{
	int              zr;
	zr = zlog_init(zlog_conf_file);
	if (zr) {
		printf("zlog_init(\"%s\") failed\n", zlog_conf_file);
		return GVFS_ERROR;
	}

	zc = zlog_get_category(name);
	if (!zc) {
		printf("zlog_get_category(\"%s\") failed\n", name);
		zlog_fini();
		return GVFS_ERROR;
	}

	return GVFS_OK;
}
