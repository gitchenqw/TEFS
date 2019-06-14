

#include <gvfs_config.h>
#include <gvfs_core.h>

#include <gvfs_upstream.h>
#include <gvfs_ns_file_download.h>

#include <gvfs_upstream.h>
#include <gvfs_ds_file_download.h>

#if 0
static INT32 gvfs_download_querystr_parse(CHAR *input,
	CHAR *filehash, INT32 filehash_len, CHAR *blockindex,
	INT32 blockindex_len, CHAR*blocksize, INT32 blocksize_len)
{
	INT32        icnt = 0;
	CHAR        *pfilehash = NULL;
	CHAR        *pblockindex = NULL;
	CHAR        *pblocksize = NULL;
	static CHAR *sfilehash = "filehash=";
	static CHAR *sblocksize = "&blocksize=";
	static CHAR *sblockindex = "&blockindex=";

	pfilehash   = strstr(input, sfilehash);
	pblocksize  = strstr(input, sblocksize);
	pblockindex = strstr(input, sblockindex);
	
	if (NULL == pfilehash || NULL == pblocksize || NULL == pblockindex) {
		return GVFS_ERROR;
	}

	icnt = 0;
	pfilehash += strlen(sfilehash);
	while ('&' != *pfilehash && icnt < filehash_len) {
		snprintf(filehash + icnt, 2, "%s", pfilehash);
		pfilehash++;
		icnt++;
	}

	icnt = 0;
	pblocksize += strlen(sblocksize);
	while ('&' != *pblocksize && icnt < blocksize_len) {
		snprintf(blocksize + icnt, 2, "%s", pblocksize);
		pblocksize++;
		icnt++;
	}

	icnt = 0;
	pblockindex += strlen(sblockindex);
	while ('\0' != *pblockindex && icnt < blockindex_len) {
		snprintf(blockindex + icnt, 2, "%s", pblockindex);
		pblockindex++;
		icnt++;
	}
	
	return GVFS_OK;
}
#endif

INT32 gvfs_ns_download_request(gvfs_http_request_t *r)
{
	gvfs_event_t              *rev;
	gvfs_connection_t         *c;
	gvfs_upstream_t           *nu;
	gvfs_buf_t                *buf;
	
	CHAR                       blocksize[16];
	CHAR                       query_str[512] = {0};
	CHAR                       filehash[64]   = {0};
	CHAR                       blockindex[16] = {0};
	UINT8                      request[52]    = {0};
	
	gvfs_ns_req_head_t         req_head;
	// INT32                      rc;
	UINT32                     sid;
	size_t                     size;
	UINT8                      file_hash[16];
	UINT32                     block_index;

	snprintf(query_str, r->query_string_len + 1, "%s", r->query_string);

#if 0
	rc = gvfs_download_querystr_parse(query_str, filehash, 64, blockindex, 16,
									  blocksize, 16);
	if (GVFS_OK != rc) {
		gvfs_log(LOG_ERROR, "gvfs_upload_query_str_parse(\"%s\") failed",
			query_str);
			
		return GVFS_ERROR;
	}
#endif
	
	if (GVFS_OK != gvfs_string_value(query_str, "blocksize", blocksize, 16, "&"))
	{
		return GVFS_ERROR;
	}
	
	if (GVFS_OK != gvfs_string_value(query_str, "filehash", filehash, 64, "&"))
	{
		return GVFS_ERROR;
	}
	
	if (GVFS_OK != gvfs_string_value(query_str, "blockindex", blockindex, 16, "&"))
	{
		return GVFS_ERROR;
	}

	if (GVFS_OK != gvfs_upstream_init(r, ns_ip_addr, ns_port)) {
		return GVFS_ERROR;
	}

	r->end = atoll(blocksize);
	
	req_head.len = htonl(40);
	sid = gvfs_get_randid();
	req_head.id = htonl(sid);
	req_head.type = htonl(0x03);
	size = 0;
	memcpy(request + size, &req_head, sizeof(gvfs_ns_req_head_t));
	size += sizeof(gvfs_ns_req_head_t);
	
	size += sizeof(UINT32);
	
	gvfs_hextoi((u_char *) file_hash, 16, (u_char *) filehash, strlen(filehash));
	memcpy(request + size, &file_hash, 16);
	size += 16;
	
	block_index = atoll(blockindex);
	block_index = ntohl(block_index);
	memcpy(request + size, &block_index, sizeof(UINT32));
	size += sizeof(UINT32);

	size += sizeof(UINT32);
	
	nu = r->upstream;
	nu->sid = sid;
	c = nu->peer.connection;
	rev = c->read;
	rev->handler = gvfs_ns_process_response_header;
	if (!rev->active) {
		if (GVFS_OK != gvfs_epoll_add_event(rev, GVFS_READ_EVENT,
			GVFS_CLEAR_EVENT))
		{
			goto failed;
		}
	}
	
	if (GVFS_OK != gvfs_completed_send(c, request, size)) {
		goto failed;
	}

	gvfs_log(LOG_INFO, "send download to ns len:%u id:%u type:%02x size:%lu success",
			 			40, sid, 0x03, size);

	c->pool = gvfs_create_pool(NS_MEM_POOL_SIZE);
	if (NULL == c->pool) {
		goto failed;
	}
	
	nu->pool = c->pool;

	buf = &nu->buf;
	buf->size = 3072;
	buf->start = gvfs_palloc(nu->pool, buf->size);
	buf->pos = buf->last = buf->start;
	buf->end = buf->start + buf->size;
	
	nu->peer.data = r;
	
	gvfs_event_add_timer(rev, 30000);

	return GVFS_OK;

failed:
	gvfs_close_connection(c);
	r->upstream = NULL;

	return GVFS_ERROR;
}


INT32 gvfs_ds_download_request(gvfs_http_request_t *r)
{
	gvfs_event_t      *rev;
	gvfs_connection_t *c;
	gvfs_upstream_t   *du;
	CHAR              *ip_addr = NULL;
	gvfs_buf_t        *buf;
	INT32              port = 0;
	
	UINT8              request[52]    = {0};

	gvfs_ds_req_head_t req_head;
	struct in_addr     in;
	size_t             size = 0;
	UINT32             cnt = 0;
	UINT32             sid;
	UINT64             fileid     = 0;
	UINT32             blockindex = 0;
	UINT32             start      = 0;
	UINT32             end        = 0;
	UINT32             block_number;

	block_number = r->block_number;
	for (cnt = 0; cnt < block_number; cnt++) {
		in.s_addr =  r->ds_addr[cnt].ip;

		ip_addr = inet_ntoa(in);
		port = ntohl(r->ds_addr[cnt].port);

		blockindex = r->block_index[cnt];
		if (GVFS_OK != gvfs_upstream_init(r, ip_addr, port)) {
			return GVFS_ERROR;
		}

		break;
	}
	
	if (cnt == block_number) {
		return GVFS_ERROR;
	}
	
	req_head.len = 	htonl(36);
	sid = gvfs_get_randid();
	req_head.id = htonl(sid);
	req_head.type = htonl(0x0102);
	size = 0;
	gvfs_memcpy(request, &req_head, sizeof(gvfs_ds_req_head_t));
	size += sizeof(gvfs_ds_req_head_t);
	
	/* { blockid¹²12¸ö×Ö½Ú */
	fileid = gvfs_rb64((UINT8 *) &r->fileid);
	memcpy(request + size, &fileid, 8);
	size += 8;

	blockindex = htonl(blockindex);
	memcpy(request + size, &blockindex, 4);
	size += 4;
	/* } */

	start = 0;
	memcpy(request + size, &start, 4);
	size += 4;

	end = htonl(r->end - 1);
	memcpy(request + size, &end, 4);
	size += 4;

	du = r->upstream;
	du->sid = sid;
	c = du->peer.connection;
	rev = c->read;
	rev->handler = gvfs_ds_process_respodse_header;
	if (!rev->active) {
		if (GVFS_OK != gvfs_epoll_add_event(rev, GVFS_READ_EVENT,
			GVFS_CLEAR_EVENT))
		{
			goto failed;
		}
	}
	
	if (GVFS_OK != gvfs_completed_send(c, request, size)) {
		goto failed;
	}
	
	gvfs_log(LOG_INFO, "send download to ds:%s:%d len:%u id:%u type:%04x "
					   "size:%lu success", ip_addr, port,
					   36, sid, 0x0102, size);
	
	c->pool = gvfs_create_pool(NS_MEM_POOL_SIZE);
	if (NULL == c->pool) {
		goto failed;
	}
	
	du->pool = c->pool;

	buf = &du->buf;
	buf->size = 1024;
	buf->start = gvfs_palloc(du->pool, buf->size);
	buf->pos = buf->last = buf->start;
	buf->end = buf->start + buf->size;
	
	du->peer.data = r;

	gvfs_event_add_timer(rev, 30000);
	
	return GVFS_OK;
	
failed:
	gvfs_close_connection(c);
	r->upstream = NULL;
	
	return GVFS_ERROR;
}

