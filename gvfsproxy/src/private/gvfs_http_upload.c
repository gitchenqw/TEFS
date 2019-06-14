

#include <gvfs_config.h>
#include <gvfs_core.h>

#include <gvfs_upstream.h>
#include <gvfs_ns_file_upload.h>

#include <gvfs_upstream.h>
#include <gvfs_ds_file_upload.h>

#if 0
static INT32 gvfs_upload_querystr_parse(CHAR *input, CHAR *filename,
	INT32 filename_len, CHAR *filesize, INT32 filesize_len, CHAR*blocksize, 
	INT32 blocksize_len,CHAR *filehash, INT32 filehash_len, CHAR *blocknum,
	INT32 blocknum_len, CHAR *blockindex, INT32 blockindex_len)
{
	INT32        icnt         = 0;
	CHAR        *pfilename    = NULL;
	CHAR        *pfilesize    = NULL;
	CHAR        *pblocksize   = NULL;
	CHAR        *pfilehash    = NULL;
	CHAR        *pblocknum    = NULL;
	CHAR        *pblockindex  = NULL;
	static CHAR *sfilename    = "filename=";
	static CHAR *sfilesize    = "&filesize=";
	static CHAR *sblocksize   = "&blocksize=";
	static CHAR *sfilehash    = "&filehash=";
	static CHAR *sblocknum    = "&blocknumber=";
	static CHAR *sblockindex  = "&blockindex=";

	pfilename   = strstr(input, sfilename);
	pfilesize   = strstr(input, sfilesize);
	pblocksize  = strstr(input, sblocksize);
	pfilehash   = strstr(input, sfilehash);
	pblocknum   = strstr(input, sblocknum);
	pblockindex = strstr(input, sblockindex);
	
	if (NULL == pfilename || NULL == pfilesize || NULL == pblocksize
		|| NULL == pfilehash || NULL == pblockindex || NULL == pblocknum) {
		return GVFS_ERROR;
	}

	pfilename += strlen(sfilename);
	while ('&' != *pfilename && icnt < filename_len) {
		snprintf(filename + icnt, 2, "%s", pfilename);
		pfilename++;
		icnt++;
	}

	icnt = 0;
	pfilesize += strlen(sfilesize);
	while ('&' != *pfilesize && icnt < filesize_len) {
		snprintf(filesize + icnt, 2, "%s", pfilesize);
		pfilesize++;
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
	pfilehash += strlen(sfilehash);
	while ('&' != *pfilehash && icnt < filehash_len) {
		snprintf(filehash + icnt, 2, "%s", pfilehash);
		pfilehash++;
		icnt++;
	}

	icnt = 0;
	pblocknum += strlen(sblocknum);
	while ('&' != *pblocknum && icnt < blocknum_len) {
		snprintf(blocknum + icnt, 2, "%s", pblocknum);
		pblocknum++;
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

INT32 gvfs_ns_upload_request(gvfs_http_request_t *r)
{
	gvfs_event_t      *rev;
	gvfs_connection_t *c;
	gvfs_upstream_t   *nu;
	gvfs_buf_t        *buf;

	CHAR               query_str[512];
	CHAR               filename[128];
	CHAR               filesize[16];
	CHAR               blocksize[16];
	CHAR               filehash[64];
	CHAR               blockindex[16];
	CHAR               blocknum[16];
	UINT8              request[52] = {0};
	
	gvfs_ns_req_head_t req_head;
	// INT32              rc;
	UINT32             sid;
	size_t             size;
	UINT64             file_size;     
	UINT8              file_hash[16];
	UINT32             block_number;
	UINT32             block_index;

	snprintf(query_str, r->query_string_len + 1, "%s", r->query_string);

#if 0
	rc = gvfs_upload_querystr_parse(query_str, filename, 128, filesize, 16,
									blocksize, 16, filehash, 64, blocknum, 16,
									blockindex, 16);
	if (GVFS_OK != rc) {
		gvfs_log(LOG_ERROR, "gvfs_upload_query_str_parse(\"%s\") failed",
						    query_str);
			
		return GVFS_ERROR;
	}
#endif

	if (GVFS_OK != gvfs_string_value(query_str, "filename", filename, 128, "&"))
	{
		return GVFS_ERROR;
	}
	
	if (GVFS_OK != gvfs_string_value(query_str, "filesize", filesize, 16, "&"))
	{
		return GVFS_ERROR;
	}

	if (GVFS_OK != gvfs_string_value(query_str, "blocksize", blocksize, 16, "&"))
	{
		return GVFS_ERROR;
	}
	
	if (GVFS_OK != gvfs_string_value(query_str, "filehash", filehash, 64, "&"))
	{
		return GVFS_ERROR;
	}
	
	if (GVFS_OK != gvfs_string_value(query_str, "blocknumber", blocknum, 16, "&"))
	{
		return GVFS_ERROR;
	}
	
	if (GVFS_OK != gvfs_string_value(query_str, "blockindex", blockindex, 16, "&"))
	{
		return GVFS_ERROR;
	}

	snprintf(r->file_name, 128, "%s", filename);
	
	if (GVFS_OK != gvfs_upstream_init(r, ns_ip_addr, ns_port)) {
		return GVFS_ERROR;
	}

	r->end = atoll(blocksize);
	
	sid = gvfs_get_randid();
	req_head.len = htonl(52);
	req_head.id = htonl(sid);
	req_head.type = htonl(0x02);
	size = 0;
	memcpy(request + size, &req_head, sizeof(gvfs_ns_req_head_t));
	size += sizeof(gvfs_ns_req_head_t);
	
	size += sizeof(UINT32);

	
	gvfs_hextoi((u_char *) file_hash, 16, (u_char *) filehash, strlen(filehash));
	memcpy(request + size, &file_hash, 16);
	size += 16;

	file_size = atoll(filesize);
	file_size = gvfs_rb64((UINT8 *) &file_size);
	memcpy(request + size, &file_size, sizeof(UINT64));
	size += sizeof(UINT64);

	block_number = atol(blocknum);
	block_number = htonl(block_number);
	memcpy(request + size, &block_number, sizeof(UINT32));
	size += sizeof(UINT32);
	
	block_index = atoll(blockindex);
	r->blockindex = block_index;
	
	block_index = ntohl(block_index);
	memcpy(request + size, &block_index, sizeof(UINT32));
	size += sizeof(UINT32);

	size += sizeof(UINT32);
	
	nu = r->upstream;
	nu->sid = sid;           /* 保存消息ID,打印日志,用于根据消息ID定位问题 */
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

	gvfs_log(LOG_INFO, "send upload to ns len:%u id:%u type:%02x size:%lu success",
						52, sid, 0x02, size);
	
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

INT32 gvfs_ds_upload_request(gvfs_http_request_t *r)
{
	gvfs_event_t      *rev;
	gvfs_connection_t *c;
	gvfs_upstream_t   *du;
	CHAR              *ip_addr = NULL;
	gvfs_buf_t        *buf;
	INT32              port = 0;
	
	UINT8              request[1460] = {0};
	UINT32             sid;
	gvfs_ds_req_head_t req_head;
	struct in_addr     in;
	
	/* 12个字节共同构成block id */
	UINT32 cnt        = 0;
	UINT32 cur_ds     = 0;
	size_t size       = 0;
	UINT64 fileid     = 0;
	UINT32 blockindex = 0;
	UINT32 start      = 0;
	UINT32 end        = 0;
	UINT32 ds_number  = 0;

	ds_number = r->ds_number;

	/* { 遍历DS地址,连接成功其中一个即可 */
	for (cnt = 0; cnt < ds_number; cnt++) {
	
		in.s_addr =  r->ds_addr[cnt].ip;
		ip_addr = inet_ntoa(in);
		
		port = ntohl(r->ds_addr[cnt].port);

		if (GVFS_OK != gvfs_upstream_init(r, ip_addr, port)) {
			continue;
		}

		cur_ds = cnt;
		break;
	}

	if (cnt == ds_number) {
		return GVFS_ERROR;
	}
	/* } */

	ds_number -= 1;

	/* { 请求消息封包 */
	req_head.len = 40 + ds_number * 20;
	req_head.len = htonl(req_head.len);

	sid = gvfs_get_randid();
	req_head.id  = htonl(sid);
	req_head.type = htonl(0x0101);
	req_head.encrypt = 0;
	memcpy(request, &req_head, 16);
	size += sizeof(gvfs_ds_req_head_t);

	/* { blockid共12个字节 */
	fileid = gvfs_rb64((UINT8 *) &r->fileid);
	memcpy(request + size, &fileid, 8);
	size += 8;

	blockindex = htonl(r->blockindex);
	memcpy(request + size, &blockindex, 4);
	size += 4;
	/* } */

	start = 0;
	memcpy(request + size, &start, 4);
	size += 4;

	end = htonl(r->end - 1);
	memcpy(request + size, &end, 4);
	size += 4;

	ds_number = htonl(ds_number);
	memcpy(request + size, &ds_number, 4);
	size += 4;
	
	for (cnt = 0; cnt < r->ds_number; cnt++) {

		if (cur_ds == cnt) {
			continue;
		}
		
		memcpy(request + size, &r->ds_addr[cnt].ip, 4);
		size += 16;

		memcpy(request + size, &r->ds_addr[cnt].port, 4);
		size += 4;
	}
	/* } */

	/* Dataserver no seperate field */
	
	/* 发送数据包 */
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

	gvfs_log(LOG_INFO, "send upload to ds:%s:%d len:%u id:%u type:%04x "
						"size:%lu success", ip_addr, port,
						ntohl(req_head.len), sid, 0x0101, size);
	
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
	
	return GVFS_OK;
	
failed:
	gvfs_close_connection(c);
	r->upstream = NULL;

	return GVFS_ERROR;
}
