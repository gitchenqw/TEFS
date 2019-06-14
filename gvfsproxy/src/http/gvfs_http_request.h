

#ifndef _GVFS_HTTP_REQUEST_H_INCLUDE_
#define _GVFS_HTTP_REQUEST_H_INCLUDE_


#include <gvfs_config.h>
#include <gvfs_core.h>

struct gvfs_ipv4_addr_s {
	UINT32 ip;
	UINT32 port;
};

struct gvfs_http_request_s {
	gvfs_pool_t       *pool;
	gvfs_http_parse_t *parser;
	
	UINT32             should_keep_alive:1;       /* 是否长连接 */
	UINT32             begin_parsed:1;            /* HTTP已经开始解析 */
	UINT32             headers_parse_completed:1; /* HTTP消息头已经解析完成 */
	UINT32             message_parse_completed:1; /* HTTP消息头和消息体已经解析完成 */
	UINT32             body_is_final:1;           /* HTTP消息体已经解析完成 */
	UINT32             body_with_headers:1;       /* 消息中同时包含消息头和体*/
	UINT32             upstream_busy:1;
	
	gvfs_http_method_t method;
	
	CHAR              *request_path;
	UINT32             request_path_len;
	CHAR              *request_url;
	UINT32             request_url_len;
	CHAR              *query_string;
	UINT32             query_string_len;
	CHAR               file_name[256];
	CHAR              *body_in_headers;
	UINT32             body_in_headers_len;
	
	size_t             body_size;
	size_t             recv_size;
	size_t             send_size;

	gvfs_buf_t         headers_in;
	gvfs_buf_t         headers_out;
	gvfs_buf_t        *request_body;
	
	gvfs_connection_t *connection;
	gvfs_upstream_t   *upstream;

	INT32              ffd;
	UINT32             blockindex;
	UINT64             fileid;
	UINT32             ds_number;
	UINT32             start;
	UINT32             end;
	UINT32             block_number;
	UINT32            *block_index;
	gvfs_ipv4_addr_t  *ds_addr;
} ;


#endif
