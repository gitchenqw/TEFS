#include <gfs_config.h>
#include <gfs_log.h>
#include <gfs_rbtree.h>
#include <gfs_tools.h>
#include <gfs_mem.h>
#include <gfs_global.h>
#include <gfs_timer.h>
#include <gfs_handle.h>
#include <gfs_busi.h>

static gfs_void gfs_rbtree_insert_timer_value(gfs_rbtree_node_t *temp, gfs_rbtree_node_t *node, gfs_rbtree_node_t *sen)
{
    gfs_rbtree_node_t  **p;

    for ( ;; ) 
    {
        if (node->key < temp->key)
            p = &temp->left;
        else if (node->key > temp->key)
            p = &temp->right;
        else
        {
            if ((gfs_uptr)node < (gfs_uptr)temp)
                p = &temp->left;
            else if ((gfs_uptr)node > (gfs_uptr)temp)
                p = &temp->right;
            else
                return;
        }

        if (*p == sen) 
            break;

        temp = *p;
    }

    *p = node;
    node->parent = temp;
    node->left = sen;
    node->right = sen;
    gfs_rbt_red(node);
}

static gfs_int32 gfs_rbtree_lookup_timer_value(gfs_rbtree_t *tree, gfs_timer_t *timer)
{
    gfs_rbtree_node_t *tmpnode;
    gfs_rbtree_node_t *sen;

    tmpnode = tree->root;
    sen = tree->sentinel;

    while(tmpnode != sen)
    {
        if (timer->node.key < tmpnode->key)
        {
            tmpnode = tmpnode->left;
            continue;
        }
        else if (timer->node.key > tmpnode->key)
        {
            tmpnode = tmpnode->right;
            continue;
        }
        else
        {
            if ((gfs_uptr)timer < (gfs_uptr)tmpnode)
            {
                tmpnode = tmpnode->left;
                continue;
            }
            else if ((gfs_uptr)timer > (gfs_uptr)tmpnode)
            {
                tmpnode = tmpnode->right;
                continue;
            }
            else
                return GFS_OK;
        }
    }

    return GFS_EVENT_NOTIMER_ERR;
}

gfs_timer_loop_t*   gfs_timer_loop_create()
{
    gfs_mem_pool_t      *pool;
    gfs_timer_loop_t    *loop;

    gfs_errno = 0;
    if (!gfs_global)
    {
        glog_error("global module has not init or init failed. create timer loop failed.");
        gfs_errno = GFS_GLOBAL_INIT_FAILED;
    }
    pool = gfs_global->pool;
    loop = gfs_mem_malloc(pool, sizeof(gfs_timer_loop_t));
    if (!loop)
    {
        glog_error("malloc<%lu> from pool:%p failed. create timer loop failed.", sizeof(gfs_timer_loop_t), pool);
        gfs_errno = GFS_MEMPOOL_MALLOC_ERR;
    }

    gfs_rbtree_init(&loop->gfsree, &loop->sentinel, gfs_rbtree_insert_timer_value);

    return loop;
}

gfs_void            gfs_timer_loop_destory(gfs_timer_loop_t* loop)
{
    if (!loop)
    {
        glog_info("timer loop destory. loop:%p\n", loop);
        gfs_mem_free(loop);
    }
}

gfs_void    gfs_timer_loop_run(gfs_timer_loop_t* loop)
{
    gfs_timer_t         *timer;
    gfs_uint32           now;

    now = gfs_mstime();
    while (1)
    {
        if (loop->gfsree.root == &loop->sentinel) {
            break;
        }

        timer = (gfs_timer_t *)gfs_rbtree_min(loop->gfsree.root, &loop->sentinel);
        if (now >= timer->node.key)
        {
			/* { modify by chenqianwu */
			if (timer->times == 0) { /* bug: 如果不运行,就没有必要添加到定时器中了*/
				continue;
			}
			/* } */
            
			/* { modify by chenqianwu */
			timer->handle(timer->param);
			/* } */

            // 执行定时器任务
			// gfs_busi_notice_handle(timer->param);
			// gfs_net_timer_handle(timer->param);
    
            gfs_rbtree_delete(&loop->gfsree, &timer->node);

            /* 定时器只执行一次,则任务不再添加到定时器中 */
            if (timer->times == 1) {
                continue;
            }

			/* 重新将定时器添加到红黑树中 */
            timer->times = (timer->times > 0) ? --timer->times : timer->times;
            timer->now = now;
            timer->node.key = timer->now + timer->secs;
            gfs_rbtree_insert(&loop->gfsree, &timer->node);
        }
        else {
            break;
        }
    }
}

gfs_void    gfs_timer_set(gfs_timer_t *timer, gfs_int32 msecs, gfs_int32 times, gfs_handle_fun_ptr handle, gfs_void *param)
{
    if (!timer || times == 0 || msecs < 10)
        return ;

    timer->secs = msecs/10;
    timer->times = times;
    timer->handle = handle;
    timer->param = param;
    timer->now = gfs_mstime();
    memset(&timer->node, 0, sizeof(gfs_rbtree_node_t));
    timer->node.key = timer->now + timer->secs;
}

gfs_int32   gfs_timer_add(gfs_timer_loop_t* loop, gfs_timer_t *timer)
{
    if (!loop || !timer || timer->times == 0)
        return GFS_PARAM_FUN_ERR;

    if (gfs_rbtree_lookup_timer_value(&loop->gfsree, timer) == GFS_EVENT_NOTIMER_ERR)
        gfs_rbtree_insert(&loop->gfsree, &timer->node);

    return GFS_OK;
}

gfs_void    gfs_timer_del(gfs_timer_loop_t* loop, gfs_timer_t *timer)
{
    if (!loop || !timer)
        return;

    if (gfs_rbtree_lookup_timer_value(&loop->gfsree, timer) == GFS_OK)
        gfs_rbtree_delete(&loop->gfsree, &timer->node);
    timer->times = 0;
}

