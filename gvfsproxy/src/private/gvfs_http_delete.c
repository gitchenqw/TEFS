

#include <gvfs_config.h>
#include <gvfs_core.h>

#include <gvfs_upstream.h>
#include <gvfs_ns_file_delete.h>


static INT32 gvfs_delete_querystr_parse(CHAR *input, CHAR *filehash,
	INT32 filehash_len)
{
	INT32 icnt               = 0;
	CHAR  *pfilehash         = NULL;
	static CHAR *sfilehash   = "filehash=";

	pfilehash   = strstr(input, sfilehash);
	
	if (NULL == pfilehash) {
		return GVFS_ERROR;
	}

	icnt = 0;
	pfilehash += strlen(sfilehash);
	while ('\0' != *pfilehash && icnt < filehash_len) {
		snprintf(filehash + icnt, 2, "%s", pfilehash);
		pfilehash++;
		icnt++;
	}

	return GVFS_OK;
}

INT32 gvfs_ns_delete_request(gvfs_http_request_t *r)
{
	gvfs_event_t              *rev;
	gvfs_connection_t         *c;
	gvfs_upstream_t           *nu;
	gvfs_buf_t                *buf;
	
	CHAR                       query_str[512] = {0};
	CHAR                       filehash[64]   = {0};
	UINT8                      request[52]    = {0};

	
	gvfs_ns_req_head_t         req_head;
	INT32                      rc;
	UINT32                     sid;
	size_t                     size;
	UINT8                      file_hash[16];
	
	snprintf(query_str, r->query_string_len + 1, "%s", r->query_string);

	rc = gvfs_delete_querystr_parse(query_str, filehash, 64);
	if (GVFS_OK != rc) {
		gvfs_log(LOG_ERROR, "gvfs_query_querystr_parse(\"%s\") failed",
			query_str);
			
		return GVFS_ERROR;
	}
	
	if (GVFS_OK != gvfs_upstream_init(r, ns_ip_addr, ns_port)) {
		return GVFS_ERROR;
	}

	sid = gvfs_get_randid();
	req_head.len = htonl(36);
	req_head.id = htonl(sid);
	req_head.type = htonl(0x04);
	size = 0;
	memcpy(request + size, &req_head, sizeof(gvfs_ns_req_head_t));
	size += sizeof(gvfs_ns_req_head_t);
	
	size += sizeof(UINT32); /* skip encrypt field */

	gvfs_hextoi((u_char *) file_hash, 16, (u_char *) filehash, strlen(filehash));
	memcpy(request + size, &file_hash, 16);
	size += 16;

	size += sizeof(UINT32); /* skip seperate field */
	
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

	gvfs_log(LOG_DEBUG, "send delete to ns len:%u id:%u type:%02x size:%lu success",
			             36, sid, 0x04, size);

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


