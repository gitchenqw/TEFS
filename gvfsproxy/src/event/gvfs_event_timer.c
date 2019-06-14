

#include <gvfs_config.h>
#include <gvfs_core.h>


gvfs_rbtree_t             gvfs_event_timer_rbtree;   /* 超时管理的红黑树结构 */
static gvfs_rbtree_node_t gvfs_event_timer_sentinel; /* 红黑树的哨兵节点     */


VOID
gvfs_event_timer_init()
{
    gvfs_rbtree_init(&gvfs_event_timer_rbtree, &gvfs_event_timer_sentinel,
                    gvfs_rbtree_insert_timer_value);
}


VOID
gvfs_event_expire_timers(VOID)
{
	gvfs_event_t        *ev;
	gvfs_rbtree_node_t  *node, *root, *sentinel;
	gvfs_connection_t   *c;

	sentinel = gvfs_event_timer_rbtree.sentinel;

    for ( ;; ) {

        root = gvfs_event_timer_rbtree.root;

        if (root == sentinel) {
            return;
        }

        node = gvfs_rbtree_min(root, sentinel);

        if ((gvfs_msec_int_t) node->key - (gvfs_msec_int_t) gvfs_current_msec <= 0)
        {
            ev = (gvfs_event_t *) ((char *) node - offsetof(gvfs_event_t, timer));
			c = (gvfs_connection_t *)ev->data;
			
            gvfs_log(LOG_DEBUG, "event timer del for:%s fd:%d: key:%lu",
                           		c->addr_text.data, c->fd, ev->timer.key);

            gvfs_rbtree_delete(&gvfs_event_timer_rbtree, &ev->timer);
            ev->timer_set = 0;
            ev->timedout = 1;
            ev->handler(ev);
            
            continue;
        }

        break;
    }

}


void
gvfs_event_del_timer(gvfs_event_t *ev)
{
	gvfs_connection_t *c;

	c = (gvfs_connection_t *) ev->data;
	
	gvfs_log(LOG_DEBUG, "event timer del for:%s fd:%d key:%lu",
						c->addr_text.data, c->fd, ev->timer.key);
						
    gvfs_rbtree_delete(&gvfs_event_timer_rbtree, &ev->timer);

    ev->timer_set = 0;
    
}


void
gvfs_event_add_timer(gvfs_event_t *ev, gvfs_msec_t timer)
{
    gvfs_msec_t        key;
    gvfs_msec_int_t    diff;
    gvfs_connection_t *c;

    key = gvfs_current_msec + timer;

	c = (gvfs_connection_t *) ev->data;
	
    if (ev->timer_set) {

		/*
		 * 如果之前已经添加到定时器中,当客户端快速连接时,如果间隔小于300ms,
		 * 则使用先前的值,这样可以节省删除和插入红黑树的时间,提高效率
		 */
        diff = (gvfs_msec_int_t) (key - ev->timer.key);

        if (gvfs_abs(diff) < GVFS_TIMER_LAZY_DELAY) {
        	
            gvfs_log(LOG_DEBUG, "event timer for:%s fd:%d, old:%lu, new:%lu",
                            	c->addr_text.data, c->fd, ev->timer.key, key);
            return;
        }

        gvfs_event_del_timer(ev);
    }

    ev->timer.key = key;
    gvfs_rbtree_insert(&gvfs_event_timer_rbtree, &ev->timer);
    ev->timer_set = 1;

    gvfs_log(LOG_DEBUG, "event timer add for:%s fd:%d: timer:%lu:%lu",
                   		c->addr_text.data, c->fd, timer, ev->timer.key);
}

