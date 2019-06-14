#include <gfs_config.h>
#include <gfs_rbtree.h>
#include <gfs_mem.h>
#include <gfs_log.h>
#include <gfs_tools.h>
#include <gfs_global.h>
#include <gfs_queue.h>

typedef struct gfs_map_s
{
    gfs_rbtree_node_t   rbnode;
    gfs_queue_t         qnode;
    gfs_void           *data;
    struct gfs_map_s   *next;
}gfs_map_t;

static gfs_map_t            *maplist = NULL;
static gfs_rbtree_t          rbtree;
static gfs_rbtree_node_t     sentinel;
static gfs_queue_t           dslist;

gfs_int32 gfs_net_map_init()
{
    gfs_mem_pool_t      *pool;
    gfs_int32            size, i;
    gfs_map_t           *map, *next, *m;

	pool = gfs_global->pool;
	size = sizeof(gfs_map_t)*GFS_NET_DS_MAPS;
	next = NULL;
	
    map = gfs_mem_malloc(pool, size);
    if (!map)
    {
    	glog_error("gfs_mem_malloc(%p, %d) failed", pool, size);
    	return GFS_ERR;
    }

    for (i = 0; i < GFS_NET_DS_MAPS; i++)
    {
        m = &map[i];
        m->next = next;
        next = m;
    }

    maplist = m;
    gfs_rbtree_init(&rbtree, &sentinel, gfs_rbtree_insert_value);
    gfs_queue_init(&dslist);

    return GFS_OK;
}

gfs_int32 gfs_net_map_add(gfs_uint32 keyid, gfs_net_t *net)
{
    gfs_map_t *map;

    if (!maplist)
        return GFS_ERR;

    map = (gfs_map_t*)gfs_rbtree_lookup_value(&rbtree, &keyid);
	if(map != NULL) {
		glog_warn("key:%u for:%s fd:%d has exist", keyid, net->addr_text,
				  net->fd);
		map->data = net;
	}
    else
    {
        map = maplist;
        maplist = map->next;

        map->data = net;
        map->rbnode.key = keyid;
        gfs_rbtree_insert(&rbtree,  &map->rbnode);
        gfs_queue_insert_tail(&dslist, &map->qnode);
        glog_info("map add key:%u for:%s fd:%d success", keyid, net->addr_text,
        		  net->fd);
    }

    return GFS_OK;
}

gfs_void* gfs_net_map_get(gfs_uint32 keyid)
{
    gfs_map_t *map;

    map = (gfs_map_t*)gfs_rbtree_lookup_value(&rbtree, &keyid);

    if (map)
        return map->data;
    else
        return NULL;
}

gfs_int32  gfs_net_map_remove(gfs_uint32 keyid)
{
    gfs_map_t *map;

    if (!maplist)
        return GFS_ERR;

    map = (gfs_map_t*)gfs_rbtree_lookup_value(&rbtree, &keyid);
    if(map != NULL)
    {
        gfs_rbtree_delete(&rbtree, &map->rbnode);
        gfs_queue_remove(&map->qnode);
    }

    return GFS_OK;
}

gfs_int32  gfs_net_ds_cmp(const gfs_queue_t *qn1, const gfs_queue_t *qn2)
{
    gfs_net_t       *net1, *net2;
    gfs_uint32       weight1, weight2;

    net1 = (gfs_net_t*)gfs_queue_data(qn1, gfs_map_t, qnode);
    net2 = (gfs_net_t*)gfs_queue_data(qn2, gfs_map_t, qnode);

    weight1 = net1->user.weight + net1->user.cfft;
    weight2 = net2->user.weight + net2->user.cfft;

    return weight1 > weight2;
}

gfs_int32  gfs_net_map_gettopn(gfs_net_t **pnet, gfs_uint32 *num)
{
    gfs_int32       index;
    gfs_queue_t    *q;
    gfs_map_t      *map;
    time_t          now;

    static time_t sort_time = 0;

    if (*num < 1)
        return GFS_ERR;

    index = 0;
    now = time(NULL);
    if (now - sort_time >= 5*60)
    {
        gfs_queue_sort(&dslist, gfs_net_ds_cmp);
        sort_time = now;
    }
    

    for (q = gfs_queue_head(&dslist); q != gfs_queue_sentinel(&dslist); q = gfs_queue_next(q))
    {
        map = (gfs_map_t*)gfs_queue_data(q, gfs_map_t, qnode);
        pnet[index++] = (gfs_net_t*)map->data;
        if (index >= *num)
            break;
    }

    *num = index;

    return GFS_OK;
}

