

#ifndef _GVFS_CONF_FILE_H_INCLUDED_
#define _GVFS_CONF_FILE_H_INCLUDED_


#include <gvfs_config.h>
#include <gvfs_core.h>


#define GET_CONF_CURRENT           0
#define GET_CONF_NEXT              1
#define GET_CONF_CHILD             2

#define CONF_CONFIG_BLOCK_START    0
#define CONF_CONFIG_BLOCK_DONE     1
#define CONF_CONFIG_NEXT           2
#define CONF_CONFIG_ERROR          3
#define CONF_CONFIG_END            4

#define CONF_ON                    1
#define CONF_OFF                   0

/* gvfs_core_conf_t */
#define CONF_WORKER_PROCESSES      4  /* 默认工作进程数 */

#define GVFS_CONF_OK          NULL
#define GVFS_CONF_ERROR       (void *) -1


struct gvfs_conf_s {
    gvfs_array_t stg;
    struct gvfs_list_head    elts;
    struct gvfs_list_head    rship;
};

struct gvfs_command_s {
	CHAR        *name;
	INT32        type;
    CHAR        *(*set)(gvfs_conf_t *cf, gvfs_command_t *cmd, VOID *conf);
    gvfs_uint_t  offset;
};

gvfs_int_t gvfs_conf_parse(gvfs_cycle_t *cycle);
gvfs_conf_t *gvfs_get_conf_specific(gvfs_conf_t *conf, char *name, int type);

CHAR *gvfs_conf_set_flag_slot(gvfs_conf_t *cf, gvfs_command_t *cmd,
	void *conf);
CHAR *gvfs_conf_set_num_slot(gvfs_conf_t *cf, gvfs_command_t *cmd,
	void *conf);
CHAR *gvfs_conf_set_str_slot(gvfs_conf_t *cf, gvfs_command_t *cmd,
	void *conf);

#endif
