#include <gfs_config.h>
#include <gfs_log.h>
#include <gfs_mem.h>
#include <gfs_global.h>
#include <gfs_net.h>
#include <gfs_tools.h>
#include <gfs_net_map.h>
#include <gfs_handle.h>
#include <gfs_event.h>
#include <gfs_timer.h>
#include <gfs_busi.h>
#include <gfs_busi_types.h>

gfs_busi_t*     gfs_busi_create(gfs_net_t *c, gfs_void *data, gfs_uint32 len);
gfs_int32       gfs_busi_destory(gfs_busi_t *busi);

typedef struct gfs_busi_data_s
{
    gfs_int32            type;
    gfs_busi_handle_ptr  parse;
    gfs_busi_handle_ptr  handle;
    gfs_busi_handle_ptr  tobuf;
    gfs_int32            req_size;
    gfs_int32            rsp_size;
}gfs_busi_data_t;

static gfs_busi_data_t gfs_busi_data[] = {
    {0, 0, 0, 0, 0, 0},
    {1,  gfs_busi_login_parse,       gfs_busi_login_handle,        gfs_busi_login_tobuf,            sizeof(gfs_busi_req_login_t),        sizeof(gfs_busi_rsp_login_t)},
    {2,  gfs_busi_upload_parse,      gfs_busi_upload_handle,       gfs_busi_upload_tobuf,           sizeof(gfs_busi_req_upload_t),       sizeof(gfs_busi_rsp_upload_t)},
    {3,  gfs_busi_download_parse,    gfs_busi_download_handle,     gfs_busi_download_tobuf,         sizeof(gfs_busi_req_download_t),     sizeof(gfs_busi_rsp_download_t)},
    {4,  gfs_busi_delete_file_parse, gfs_busi_delete_file_handle,  gfs_busi_delete_file_tobuf,      sizeof(gfs_busi_req_delfile_t),      sizeof(gfs_busi_rsp_delfile_t)},
    {5,  0, 0, 0, 0, 0},
    {6,  gfs_busi_delete_block_parse,gfs_busi_delete_block_handle, gfs_busi_delete_block_tobuf,     sizeof(gfs_busi_nte_delblock_t),     sizeof(gfs_busi_nte_delblock_t)},
    {7,  0, 0, 0, 0, 0},
    {8,  gfs_busi_copy_block_parse,  gfs_busi_copy_block_handle,   gfs_busi_copy_block_tobuf,       sizeof(gfs_busi_nte_cpy_block_t),    sizeof(gfs_busi_nte_delblock_t)},
    {9,  gfs_busi_heartbeat_parse,   gfs_busi_heartbeat_handle,    gfs_busi_heartbeat_tobuf,        sizeof(gfs_busi_heartbeat_t),        sizeof(gfs_busi_nte_delblock_t)},
    {10, gfs_busi_getfsize_parse,    gfs_busi_getfsize_handle,     gfs_busi_getfsize_tobuf,         sizeof(gfs_busi_req_getfsize_t),     sizeof(gfs_busi_rsp_getfsize_t)}

};

gfs_busi_t*     gfs_busi_create(gfs_net_t *c, gfs_void *data, gfs_uint32 len)
{
    gfs_void            *pbuf;
    gfs_mem_pool_t      *pool;
    gfs_busi_t          *busi;
    gfs_uint32           id, type;

    pool = c->pool;
    
    busi = gfs_mem_malloc(pool, sizeof(gfs_busi_t));
    if (!busi)
    {
    	glog_error("gfs_mem_malloc(%p, %lu) failed", pool, sizeof(gfs_busi_t));
        return NULL;
    }

    /* { busi的recv buf的空间和收到的实际的数据包长度相等 */
    pbuf = gfs_mem_malloc(pool, len);
    if (!pbuf)
    {
        gfs_mem_free(busi);
    	glog_error("gfs_mem_malloc(%p, %u) failed", pool, len);
        return NULL;
    }
    memcpy(pbuf, data, len);
    busi->rbuf.data = pbuf;
    busi->rbuf.len = busi->rbuf.pos = len;
    busi->rbuf.hd = 0;
    /* } */

    /* { 获取id字段,只用于打印日志方便定位问题 */
    memcpy(&id, data + 4, 4);
    id = ntohl(id);
    /* } */

	/* { 获取type类型字段 */
    memcpy(&type, data + 8, 4);
    type = ntohl(type);
    if (type < 1 || type > 10)
    {
    	glog_error("busi type:%u not in[1,10] error", type);
        return NULL;
    }
    /* } */

    pbuf = gfs_mem_malloc(pool, gfs_busi_data[type].rsp_size);
    if (!pbuf)
    {
        gfs_mem_free(busi->rbuf.data);
        gfs_mem_free(busi);
    	glog_error("gfs_mem_malloc(%p, %d) failed", pool,
    			   gfs_busi_data[type].rsp_size);
        return NULL;
    }
    
	/* { 该处并未分配存储空间 */
    busi->sbuf.data = NULL;
    busi->sbuf.len = 0;
    busi->sbuf.pos = busi->sbuf.hd = 0;
    /* */

    busi->reh = busi->rbuf.data;
    busi->rsh = pbuf;
    busi->type = type;
    busi->pool = pool;
    busi->c = c;
    busi->data = NULL;

	glog_debug("create busi for:%s fd:%d type:%u id:%u success",
			   c->addr_text, c->fd, type, id);
	
    return busi;
}

gfs_int32 gfs_busi_destory(gfs_busi_t *busi)
{
    gfs_mem_free(busi->rsh);
    gfs_mem_free(busi->rbuf.data);
    gfs_mem_free(busi->sbuf.data);
    gfs_mem_free(busi);
    return GFS_OK;
}

gfs_int32       gfs_busi_handle(gfs_net_t *c, gfs_void *data, gfs_uint32 len)
{
    gfs_int32    ret, type;
    gfs_event_t *ev;
    gfs_busi_t  *busi;

    ev = &c->ev;
    busi = gfs_busi_create(c, data, len);

    if (NULL == busi) {
    	return GFS_ERR;
    }
    
    type = busi->type;

    switch (type) {
    	case 1: // end
    		ret = gfs_busi_login_parse(busi);
    		if (GFS_OK != ret) {
    			gfs_busi_login_tobuf(busi);
    		} else {
    			gfs_busi_login_handle(busi);
    			gfs_busi_login_tobuf(busi);
    		}
    		break;
    		
    	case 2: // end
    		ret = gfs_busi_upload_parse(busi);
    		if (GFS_OK != ret) {
    			gfs_busi_upload_tobuf(busi);
    		} else {
    			gfs_busi_upload_handle(busi);
    			gfs_busi_upload_tobuf(busi);
    		}
    		break;

    	case 3: // end
    		ret = gfs_busi_download_parse(busi);
    		if (GFS_OK != ret) {
    			gfs_busi_download_tobuf(busi);
    		} else {
    			gfs_busi_download_handle(busi);
    			gfs_busi_download_tobuf(busi);
    		}
    		break;

    	case 4: // end
    		ret = gfs_busi_delete_file_parse(busi);
    		if (GFS_OK != ret) {
    			gfs_busi_delete_file_tobuf(busi);
    		} else {
    			gfs_busi_delete_file_handle(busi);
    			gfs_busi_delete_file_tobuf(busi);
    		}
    		break;

   		case 6: // end
    		ret = gfs_busi_delete_block_parse(busi);
    		if (GFS_OK != ret) {
    			gfs_busi_delete_block_tobuf(busi);
    		} else {
    			gfs_busi_delete_block_handle(busi);
    			gfs_busi_delete_block_tobuf(busi);
    		}
   			break;

   		case 8: // end
    		ret = gfs_busi_copy_block_parse(busi);
    		if (GFS_OK != ret) {
    			gfs_busi_copy_block_tobuf(busi);
    		} else {
    			gfs_busi_copy_block_handle(busi);
    			gfs_busi_copy_block_tobuf(busi);
    		}
   			break;

   		case 9: // end
    		ret = gfs_busi_heartbeat_parse(busi);
    		if (GFS_OK != ret) {
    			gfs_busi_heartbeat_tobuf(busi);
    		} else {
    			gfs_busi_heartbeat_handle(busi);
    			gfs_busi_heartbeat_tobuf(busi);
    		}
   			break;

   		case 10: // end
    		ret = gfs_busi_getfsize_parse(busi);
    		if (GFS_OK != ret) {
    			gfs_busi_getfsize_tobuf(busi);
    		} else {
    			gfs_busi_getfsize_handle(busi);
    			gfs_busi_getfsize_tobuf(busi);
    		}
   			break;
    		
    	default:
    	 	gfs_busi_destory(busi);
    	 	return GFS_OK;
    		
    }

    /* { modify by chenqianwu */
    // ret = gfs_busi_data[type].parse(busi);
    // if (ret != GFS_OK)
    // {
    //     gfs_busi_data[type].tobuf(busi);
    // }
    // else
    // {
    //     gfs_busi_data[type].handle(busi);
    //     gfs_busi_data[type].tobuf(busi);
    // }
    /* } */

    while (busi->sbuf.hd < busi->sbuf.pos)
    {
        gfs_net_memcopy(c->sbuf, &busi->sbuf);
		gfs_handle_tcp_send(ev);
		/* { modify by chenqianwu */
		// ev->write(ev);
		/* } */
    }
    
    gfs_busi_destory(busi);
    return GFS_OK;
}

gfs_int32 gfs_busi_init()
{
    static gfs_timer_t timer;

	/* 该任务3000000 / 10 = 300000 = 300秒执行一次 */
    gfs_timer_set(&timer, 3000000, -1, gfs_busi_notice_handle, NULL);
    gfs_timer_add(gfs_global->eloop->tloop, &timer);

    return GFS_OK;
}
