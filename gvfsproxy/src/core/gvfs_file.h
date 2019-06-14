

#ifndef _GVFS_FILE_H_INCLUDE_
#define _GVFS_FILE_H_INCLUDE_


#include <gvfs_config.h>
#include <gvfs_core.h>
#include <gvfs_channel.h>


struct gvfs_file_s {
	gvfs_fd_t                  fd;
	gvfs_str_t                 name;
	struct stat                info;
    off_t                      offset;
    off_t                      sys_offset;
};


#endif
