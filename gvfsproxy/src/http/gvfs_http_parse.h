

#ifndef _GVFS_HTTP_PARSE_H_INCLUDE_
#define _GVFS_HTTP_PARSE_H_INCLUDE_


#include <gvfs_config.h>
#include <gvfs_core.h>


/* 
 * Compile with -DHTTP_PARSER_STRICT=0 to make less checks, but run
 * faster
 */
#ifndef HTTP_PARSER_STRICT
#define HTTP_PARSER_STRICT 1
#endif

typedef enum gvfs_http_method_e {
	HTTP_DELETE      = 0,
	HTTP_GET         = 1,
	HTTP_HEAD        = 2,
	HTTP_POST        = 3,
	HTTP_PUT         = 4,

	HTTP_CONNECT     = 5,
	HTTP_OPTIONS     = 6,
	HTTP_TRACE       = 7,

	HTTP_COPY        = 8,
	HTTP_LOCK        = 9,
	HTTP_MKCOL       = 10,
	HTTP_MOVE        = 11,
	HTTP_PROPFIND    = 12,
	HTTP_PROPPATCH   = 13,
	HTTP_SEARCH      = 14,
	HTTP_UNLOCK      = 15,
	
	HTTP_REPORT      = 16,
	HTTP_MKACTIVITY  = 17,
	HTTP_CHECKOUT    = 18,
	HTTP_MERGE       = 19,

	HTTP_MSEARCH     = 20,
	HTTP_NOTIFY      = 21,
	HTTP_SUBSCRIBE   = 22,
	HTTP_UNSUBSCRIBE = 23,

	HTTP_PATCH       = 24,
	HTTP_PURGE       = 25
} gvfs_http_method_t;


typedef enum gvfs_http_errno_e {
	HPE_OK,

	HPE_CB_message_begin,
	HPE_CB_url,
	HPE_CB_header_field,
	HPE_CB_header_value,
	HPE_CB_headers_complete,
	HPE_CB_body,
	HPE_CB_message_complete,
	HPE_CB_status,

	HPE_INVALID_EOF_STATE,
	HPE_HEADER_OVERFLOW,
	HPE_CLOSED_CONNECTION,

	HPE_INVALID_VERSION, 
	HPE_INVALID_STATUS, 
	HPE_INVALID_METHOD, 
	HPE_INVALID_URL,
	HPE_INVALID_HOST, 
	HPE_INVALID_PORT, 
	HPE_INVALID_PATH, 
	HPE_INVALID_QUERY_STRING,
	HPE_INVALID_FRAGMENT, 
	HPE_LF_EXPECTED, 
	HPE_INVALID_HEADER_TOKEN, 
	HPE_INVALID_CONTENT_LENGTH,               
		
	HPE_INVALID_CHUNK_SIZE,  
		
	HPE_INVALID_CONSTANT,
	HPE_INVALID_INTERNAL_STATE,
	HPE_STRICT, 
	HPE_PAUSED,
	HPE_UNKNOWN,
	
} gvfs_http_errno_t;


typedef enum gvfs_http_parse_type_s {
	HTTP_REQUEST,
	HTTP_RESPONSE,
	HTTP_BOTH
} gvfs_http_parse_type_t;


/* Flag values for gvfs_http_parse_t.flags field */
enum flags
  { F_CHUNKED               = 1 << 0
  , F_CONNECTION_KEEP_ALIVE = 1 << 1
  , F_CONNECTION_CLOSE      = 1 << 2
  , F_TRAILING              = 1 << 3
  , F_UPGRADE               = 1 << 4
  , F_SKIPBODY              = 1 << 5
  };


/* Get an http_errno value from an gvfs_http_parse_t */
#define HTTP_PARSER_ERRNO(p)            ((gvfs_http_errno_t) (p)->http_errno)


typedef struct gvfs_http_parse_s{
  /* PRIVATE */
  unsigned int    type:2;             /* enum gvfs_http_parse_t_type */
  unsigned int    flags:6;            /* F_* values from 'flags' enum; semi-public */
  unsigned int    state:8;            /* enum state from gvfs_http_parse_t.c */
  unsigned int    header_state:8;     /* enum header_state from gvfs_http_parse_t.c */
  unsigned int    index:8;            /* index into current matcher */

  uint32_t        nread;              /* bytes read in various scenarios */
  uint64_t        content_length;     /* bytes in body (0 if no Content-Length header) */

  /* READ-ONLY */
  unsigned short  http_major;
  unsigned short  http_minor;
  unsigned int    status_code:16;     /* responses only */
  unsigned int    method:8;           /* requests only */
  unsigned int    http_errno:7;
  unsigned int    body_size;

  /* 1 = Upgrade header was present and the parser has exited because of that.
   * 0 = No upgrade header present.
   * Should be checked when gvfs_http_parse_t_execute() returns in addition to
   * error checking.
   */
  unsigned int    upgrade:1;

  /* PUBLIC */
  void           *data;                /* A pointer to get hook to "user" object */
} gvfs_http_parse_t;

typedef int (*gvfs_http_data_cb) (gvfs_http_parse_t*, const char *at, size_t length);
typedef int (*gvfs_http_cb) (gvfs_http_parse_t*);

typedef struct gvfs_http_parse_cb_s{
	gvfs_http_cb      on_message_begin;     /* 置标志位 */
	gvfs_http_data_cb on_url;               /* 拷贝URL  */
	gvfs_http_data_cb on_status;
	gvfs_http_data_cb on_header_field;      /* 拷贝消息头字段名称 */
	gvfs_http_data_cb on_header_value;      /* 拷贝消息头值 */
	gvfs_http_cb      on_headers_complete;  /* 消息头解析完了进行调用 */
	gvfs_http_data_cb on_body;              /* 消息体解析调用 */
	gvfs_http_cb      on_message_complete;  /* HTTP消息解析完调用 */
} gvfs_http_parse_cb_t;


typedef enum gvfs_http_parse_url_fields_s {
	UF_SCHEMA           = 0,
	UF_HOST             = 1, 
	UF_PORT             = 2, 
	UF_PATH             = 3,
	UF_QUERY            = 4,
	UF_FRAGMENT         = 5,
	UF_USERINFO         = 6,
	UF_MAX              = 7
} gvfs_http_parse_url_fields_t;


/* Result structure for gvfs_http_parse_t_parse_url().
 *
 * Callers should index into field_data[] with UF_* values iff field_set
 * has the relevant (1 << UF_*) bit set. As a courtesy to clients (and
 * because we probably have padding left over), we convert any port to
 * a uint16_t.
 */
typedef struct gvfs_http_parse_url_s {
	uint16_t field_set;           /* Bitmask of (1 << UF_*) values */
	uint16_t port;                /* Converted UF_PORT string */

	struct {
		uint16_t off;             /* Offset into buffer in which field starts */
		uint16_t len;             /* Length of run in buffer */
	} field_data[UF_MAX];
} gvfs_http_parse_url_t;

void gvfs_http_parse_init(gvfs_http_parse_t *parser, gvfs_http_parse_type_t type);


size_t gvfs_http_parse(gvfs_http_parse_t *parser, const gvfs_http_parse_cb_t *settings,
	const char *data, size_t len);


/* 
 * If http_should_keep_alive() in the on_headers_complete or
 * on_message_complete callback returns 0, then this should be
 * the last message on the connection.
 * If you are the server, respond with the "Connection: close" header.
 * If you are the client, close the connection.
 */
int gvfs_http_should_keep_alive(const gvfs_http_parse_t *parser);

/* 获取HTTP请求方法字符串 */
const char *gvfs_http_method_str(gvfs_http_method_t m);

/* 获取解析错误名称 */
const char *gvfs_http_errno_name(gvfs_http_errno_t err);

/* 获取解析错误描述 */
const char *gvfs_http_errno_description(gvfs_http_errno_t err);

/* 解析URL,失败返回非0,成功返回0  */
int gvfs_http_parse_url(const char *request_url, unsigned int request_url_len,
	int is_connect, gvfs_http_parse_url_t *u);
	
void gvfs_http_url_dump (char *request_url, const gvfs_http_parse_url_t *u,
	char **out, unsigned int *olen, gvfs_http_parse_url_fields_t type);

/* 检查消息体是否已经处理完成 */
int gvfs_http_body_is_final(const gvfs_http_parse_t *parser);


#endif /* _GVFS_HTTP_PARSE_H_INCLUDE_ */
