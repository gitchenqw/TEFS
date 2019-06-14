

#include <gvfs_config.h>
#include <gvfs_core.h>


static struct gvfs_list_head gvfs_http_request_body_buf_list;


INT32
gvfs_http_create_request_body_buf_list(UINT32 size, UINT32 nelts)
{
	gvfs_buf_t *buf;
	UINT32      n;

	INIT_LIST_HEAD(&gvfs_http_request_body_buf_list);
	
	for (n = 0; n < nelts; n++) {
		buf = gvfs_alloc(sizeof(gvfs_buf_t));
		if (NULL == buf) {
			return GVFS_ERROR;
		}

		buf->data = gvfs_alloc(size);
		if (NULL == buf->data) {
			return GVFS_ERROR;
		}

		buf->size = size;
		buf->start = buf->pos = buf->last = buf->data;
		buf->end = buf->start + size;
		INIT_LIST_HEAD(&buf->lh);

		gvfs_list_add_tail(&buf->lh, &gvfs_http_request_body_buf_list);
	}

	return GVFS_OK;
}

gvfs_buf_t *gvfs_http_get_request_buf_body(VOID)
{
	gvfs_buf_t            *request_body_buf;
	struct gvfs_list_head *lh, *tmp;

	request_body_buf = NULL;
	list_for_each_safe(lh, tmp, &gvfs_http_request_body_buf_list) {

		request_body_buf = list_entry(lh, gvfs_buf_t, lh);

		list_del(&request_body_buf->lh);
		INIT_LIST_HEAD(&request_body_buf->lh);
		request_body_buf->pos = request_body_buf->last = request_body_buf->start;
		break;
	}

	return request_body_buf;
}

VOID gvfs_http_free_request_body_buf(gvfs_buf_t *request_body_buf)
{
	gvfs_list_add_tail(&request_body_buf->lh, &gvfs_http_request_body_buf_list);
}
