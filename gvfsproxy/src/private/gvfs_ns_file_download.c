

#include <gvfs_config.h>
#include <gvfs_core.h>
#include <gvfs_ns_interface.h>

INT32 gvfs_ns_file_download_process(gvfs_http_request_t *r,
	gvfs_rsp_head_t *rsp_head)
{
	gvfs_ns_file_download_rsp_t     *rspd;
	UINT8                           *p;
	UINT32                           n;
	size_t                           size;

	rspd = (gvfs_ns_file_download_rsp_t *) rsp_head->rsh;
	p = rsp_head->data;

	size = sizeof(gvfs_ns_rsp_head_t);
	p += size;

	rspd->encrypt = gvfs_rb32(p);
	p += sizeof(UINT32);

	rspd->file_id = gvfs_rb64(p);
	p += sizeof(UINT64);

	r->fileid = rspd->file_id;

	rspd->block_number = gvfs_rb32(p);
	r->block_number = rspd->block_number;
	p += sizeof(UINT32);

	if (0 < r->block_number) {
		r->block_index = gvfs_palloc(r->pool, r->block_number * sizeof(UINT32));
		r->ds_addr = gvfs_palloc(r->pool,
							     r->ds_number * sizeof(gvfs_ipv4_addr_t));

		for (n = 0; n < r->block_number; n++) {
			r->block_index[n] = gvfs_rb32(p);
			p += 4;
			
			r->ds_addr[n].port = *((UINT32 *)p);
			p += sizeof(UINT32);
			
			r->ds_addr[n].ip = *((UINT32 *)p);
			p += 16;

		}
	}
	
	rspd->separate = gvfs_rb32(p);
	
	gvfs_log(LOG_INFO, "recv download from ns id:%u file_id:%llu block_number:%u",
			 			rsp_head->id, rspd->file_id, rspd->block_number);
			 
	return GVFS_NS_OK;
}

INT32 gvfs_ns_file_download_completed(gvfs_http_request_t *r,
	gvfs_rsp_head_t *rsp_head)
{
	gvfs_buf_t                   *headers_out;
	CHAR                          request_body[128] = {0};
	INT32                         rc;
	size_t                        content_length, bsize, rsize;

	headers_out = &r->headers_out;

	content_length = snprintf(request_body, 128,
							 "rtn=%u&block_number=%u",
							 rsp_head->rtn, r->block_number);
	if (0 == rsp_head->rtn) {					 
		gvfs_http_response_headers(headers_out, HTTP_Ok, content_length);
	}
	else {
		gvfs_http_response_headers(headers_out, HTTP_NotFound, content_length);
	}

	bsize = headers_out->end - headers_out->last;
	rsize = snprintf((CHAR *) headers_out->last, bsize,
			 "%s",
			 request_body);
		 
	headers_out->last += rsize;

	gvfs_close_upstream(r);

	if (0 == rsp_head->rtn && 0 < r->block_number) {
		headers_out->pos = headers_out->last = headers_out->start;
		
		rc = gvfs_ds_download_request(r);
		if (GVFS_OK != rc) {
			gvfs_upstream_to_http_response(r, HTTP_NotAcceptable, NULL);
			return GVFS_NS_ERROR;
		}
	} else {
		gvfs_http_finalize_request(r, 200);
	}
	
	return GVFS_NS_OK;
}

