

#include <gvfs_config.h>
#include <gvfs_core.h>

#include <gvfs_ns_interface.h>
#include <gvfs_ns_file_upload.h>

INT32 gvfs_ns_file_upload_process(gvfs_http_request_t *r,
	gvfs_rsp_head_t *rsp_head)
{
	gvfs_ns_file_upload_rsp_t     *rspu;
	UINT8                         *p;
	UINT32                         n;
	size_t                         size;

	p = rsp_head->data;
	rspu = (gvfs_ns_file_upload_rsp_t *) rsp_head->rsh;

	size = sizeof(gvfs_ns_rsp_head_t);
	p += size;

	rspu->encrypt = gvfs_rb32(p);
	p += sizeof(UINT32);

	rspu->file_id = gvfs_rb64(p);
	r->fileid = rspu->file_id;
	p += sizeof(UINT64);

	rspu->ds_number = gvfs_rb32(p);
	r->ds_number = rspu->ds_number;
	p += sizeof(UINT32);

	if (0 < r->ds_number) {
		r->ds_addr = gvfs_palloc(r->pool,
							     r->ds_number * sizeof(gvfs_ipv4_addr_t));

		for (n = 0; n < r->ds_number; n++) {
			r->ds_addr[n].ip = *((UINT32 *)p);
			p += 16;

			r->ds_addr[n].port = *((UINT32 *)p);
			p += sizeof(UINT32);
		}
	}
	
	rspu->separate = gvfs_rb32(p);

	gvfs_log(LOG_INFO, "recv upload from ns id:%u file_id:%llu ds_number:%u",
			 			rsp_head->id, rspu->file_id, rspu->ds_number);
						
	return GVFS_NS_OK;
}

INT32 gvfs_ns_file_upload_completed(gvfs_http_request_t *r,
	gvfs_rsp_head_t *rsp_head)
{
	gvfs_buf_t                   *headers_out;
	CHAR                          request_body[128] = {0};
	INT32                         rc;
	size_t                        content_length, bsize, rsize;

	headers_out = &r->headers_out;

	content_length = snprintf(request_body, 128,
							 "rtn=%u&ds_number=%u",
							 rsp_head->rtn, r->ds_number);
						 
	gvfs_http_response_headers(headers_out, HTTP_Ok, content_length);

	bsize = headers_out->end - headers_out->last;
	rsize = snprintf((CHAR *) headers_out->last, bsize,
			 "%s",
			 request_body);
		 
	headers_out->last += rsize;

	gvfs_close_upstream(r);
	
	if (0 == rsp_head->rtn && 0 < r->ds_number) {
	
		rc = gvfs_ds_upload_request(r);
		if (GVFS_OK != rc) {
			headers_out->pos = headers_out->last = headers_out->start;
			gvfs_upstream_to_http_response(r, HTTP_NotAcceptable, NULL);
			return GVFS_NS_ERROR;
		}
	}
	
	return GVFS_NS_OK;
}

