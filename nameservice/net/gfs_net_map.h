#ifndef __GFS_NET_MAP_H__
#define __GFS_NET_MAP_H__
#include <gfs_config.h>
#include <gfs_net.h>

gfs_int32  gfs_net_map_init();
gfs_int32  gfs_net_map_add(gfs_uint32 keyid, gfs_net_t *net);
gfs_net_t* gfs_net_map_get(gfs_uint32 keyid);
gfs_int32  gfs_net_map_remove(gfs_uint32 keyid);
gfs_int32  gfs_net_map_gettopn(gfs_net_t **pnet, gfs_uint32 *num);
#endif