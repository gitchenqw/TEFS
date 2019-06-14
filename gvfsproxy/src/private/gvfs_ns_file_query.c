

#include <gvfs_config.h>
#include <gvfs_core.h>

#include <gvfs_ns_interface.h>
#include <gvfs_ns_file_upload.h>

INT32 gvfs_ns_file_query_process(gvfs_http_request_t *r,
	gvfs_rsp_head_t *rsp_head)
{
	gvfs_ns_file_query_rsp_t     *rspq;
	UINT8                        *p;
	size_t                        size;

	rspq = (gvfs_ns_file_query_rsp_t *) rsp_head->rsh;
	p = rsp_head->data;

	size = sizeof(gvfs_ns_rsp_head_t);
	p += size;

	rspq->encrypt = gvfs_rb32(p);
	p += sizeof(UINT32);
	
	rspq->file_size  = gvfs_rb64(p);
	p += sizeof(UINT64);
	
	rspq->block_size = gvfs_rb32(p);
	p += sizeof(UINT32);

	rspq->separate = gvfs_rb32(p);
	
	gvfs_log(LOG_INFO, "recv query from ns id:%u file_size:%llu block_size:%u",
						rsp_head->id, rspq->file_size, rspq->block_size);
						
	return GVFS_NS_OK;
}


INT32 gvfs_ns_file_query_completed(gvfs_http_request_t *r,
	gvfs_rsp_head_t *rsp_head)
{
	gvfs_buf_t                   *headers_out;
	gvfs_ns_file_query_rsp_t     *rspq;
	CHAR                          request_body[128] = {0};
	size_t                        content_length, bsize, rsize;

	headers_out = &r->headers_out;

	if (0 == rsp_head->rtn || 20 == rsp_head->rtn) {
		rspq = (gvfs_ns_file_query_rsp_t *) rsp_head->rsh;
		content_length = snprintf(request_body, 128,
								 "file_size=%llu&block_size=%u",
								 rspq->file_size, rspq->block_size);
	} else {
		content_length = snprintf(request_body, 128, "rtn=%u", rsp_head->rtn);
	}
	
	gvfs_http_response_headers(headers_out, HTTP_Ok, content_length);

	bsize = headers_out->end - headers_out->last;
	rsize = snprintf((CHAR *) headers_out->last, bsize,
			 "%s",
			 request_body);
			 
	headers_out->last += rsize;

	return GVFS_NS_OK;
}
