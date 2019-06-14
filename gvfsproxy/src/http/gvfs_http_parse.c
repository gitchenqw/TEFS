

#include <gvfs_config.h>
#include <gvfs_core.h>


#define HTTP_MAX_HEADER_SIZE (80*1024)


#ifndef ULLONG_MAX
#define ULLONG_MAX ((uint64_t) -1) /* 2^64-1 */
#endif

#ifndef MIN
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#endif

#ifndef BIT_AT
# define BIT_AT(a, i)                                                \
  (!!((unsigned int) (a)[(unsigned int) (i) >> 3] &                  \
   (1 << ((unsigned int) (i) & 7))))
#endif


#define SET_ERRNO(e)                                                 \
do {                                                                 \
	parser->http_errno = (e);                                        \
} while(0)

#define CALLBACK_NOTIFY_(FOR, ER)                                    \
do {                                                                 \
	if (settings->on_##FOR) {                                        \
		if (0 != settings->on_##FOR(parser)) {                       \
			SET_ERRNO(HPE_CB_##FOR);                                 \
		}                                                            \
                                                                     \
		if (HTTP_PARSER_ERRNO(parser) != HPE_OK) {                   \
			return (ER);                                             \
		}                                                            \
	}                                                                \
} while (0)

#define CALLBACK_NOTIFY(FOR)            CALLBACK_NOTIFY_(FOR, p - data + 1)
#define CALLBACK_NOTIFY_NOADVANCE(FOR)  CALLBACK_NOTIFY_(FOR, p - data)


#define CALLBACK_DATA_(FOR, LEN, ER)                                         \
do {                                                                         \
	if (FOR##_mark) {                                                        \
		if (settings->on_##FOR) {                                            \
			if (0 != settings->on_##FOR(parser, FOR##_mark, (LEN))) {       \
				SET_ERRNO(HPE_CB_##FOR);                                     \
			}                                                                \
                                                                             \
			if (HTTP_PARSER_ERRNO(parser) != HPE_OK) {                       \
				return (ER);                                                 \
			}                                                                \
		}                                                                    \
	    FOR##_mark = NULL;                                                   \
	}                                                                        \
} while (0)
  
#define CALLBACK_DATA(FOR)                                           \
    CALLBACK_DATA_(FOR, p - FOR##_mark, p - data + 1)
#define CALLBACK_DATA_NOADVANCE(FOR)                                 \
    CALLBACK_DATA_(FOR, p - FOR##_mark, p - data)


#define MARK(FOR)                                                    \
do {                                                                 \
	if (!FOR##_mark) {                                               \
		FOR##_mark = p;                                              \
	}                                                                \
} while (0)


#define PROXY_CONNECTION  "proxy-connection"
#define CONNECTION        "connection"
#define CONTENT_LENGTH    "content-length"
#define TRANSFER_ENCODING "transfer-encoding"
#define UPGRADE           "upgrade"
#define CHUNKED           "chunked"
#define KEEP_ALIVE        "keep-alive"
#define CLOSE             "close"


static const char *gvfs_http_method_strings[] = {
	"DELETE",
	"GET",
	"HEAD",
	"POST",
	"PUT",
            
	"CONNECT",
	"OPTIONS",
	"TRACE",
                  
	"COPY",
	"LOCK",
	"MKCOL",
	"MOVE",
	"PROPFIND",
	"PROPPATCH",
	"SEARCH",
	"UNLOCK",
               
	"REPORT",
    "MKACTIVITY",
	"CHECKOUT", 
	"MERGE", 
 
	"M-SEARCH", 
    "NOTIFY", 
	"SUBSCRIBE", 
	"UNSUBSCRIBE", 

	"PATCH", 
	"PURGE"
};


/* Tokens as defined by rfc 2616. Also lowercases them.
 *        token       = 1*<any CHAR except CTLs or separators>
 *     separators     = "(" | ")" | "<" | ">" | "@"
 *                    | "," | ";" | ":" | "\" | <">
 *                    | "/" | "[" | "]" | "?" | "="
 *                    | "{" | "}" | SP | HT
 */
static const char tokens[256] = {
/*   0 nul    1 soh    2 stx    3 etx    4 eot    5 enq    6 ack    7 bel  */
        0,       0,       0,       0,       0,       0,       0,       0,
/*   8 bs     9 ht    10 nl    11 vt    12 np    13 cr    14 so    15 si   */
        0,       0,       0,       0,       0,       0,       0,       0,
/*  16 dle   17 dc1   18 dc2   19 dc3   20 dc4   21 nak   22 syn   23 etb */
        0,       0,       0,       0,       0,       0,       0,       0,
/*  24 can   25 em    26 sub   27 esc   28 fs    29 gs    30 rs    31 us  */
        0,       0,       0,       0,       0,       0,       0,       0,
/*  32 sp    33  !    34  "    35  #    36  $    37  %    38  &    39  '  */
        0,      '!',      0,      '#',     '$',     '%',     '&',    '\'',
/*  40  (    41  )    42  *    43  +    44  ,    45  -    46  .    47  /  */
        0,       0,      '*',     '+',      0,      '-',     '.',      0,
/*  48  0    49  1    50  2    51  3    52  4    53  5    54  6    55  7  */
       '0',     '1',     '2',     '3',     '4',     '5',     '6',     '7',
/*  56  8    57  9    58  :    59  ;    60  <    61  =    62  >    63  ?  */
       '8',     '9',      0,       0,       0,       0,       0,       0,
/*  64  @    65  A    66  B    67  C    68  D    69  E    70  F    71  G  */
        0,      'a',     'b',     'c',     'd',     'e',     'f',     'g',
/*  72  H    73  I    74  J    75  K    76  L    77  M    78  N    79  O  */
       'h',     'i',     'j',     'k',     'l',     'm',     'n',     'o',
/*  80  P    81  Q    82  R    83  S    84  T    85  U    86  V    87  W  */
       'p',     'q',     'r',     's',     't',     'u',     'v',     'w',
/*  88  X    89  Y    90  Z    91  [    92  \    93  ]    94  ^    95  _  */
       'x',     'y',     'z',      0,       0,       0,      '^',     '_',
/*  96  `    97  a    98  b    99  c   100  d   101  e   102  f   103  g  */
       '`',     'a',     'b',     'c',     'd',     'e',     'f',     'g',
/* 104  h   105  i   106  j   107  k   108  l   109  m   110  n   111  o  */
       'h',     'i',     'j',     'k',     'l',     'm',     'n',     'o',
/* 112  p   113  q   114  r   115  s   116  t   117  u   118  v   119  w  */
       'p',     'q',     'r',     's',     't',     'u',     'v',     'w',
/* 120  x   121  y   122  z   123  {   124  |   125  }   126  ~   127 del */
       'x',     'y',     'z',      0,      '|',      0,      '~',       0 };


static const int8_t unhex[256] = {
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,-1,-1,-1,-1,-1,-1,
	-1,10,11,12,13,14,15,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,10,11,12,13,14,15,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1
};

#if HTTP_PARSER_STRICT
#define T(v) 0
#else
#define T(v) v
#endif

static const uint8_t normal_url_char[32] = {
/*   0 nul    1 soh    2 stx    3 etx    4 eot    5 enq    6 ack    7 bel  */
        0    |   0    |   0    |   0    |   0    |   0    |   0    |   0,
/*   8 bs     9 ht    10 nl    11 vt    12 np    13 cr    14 so    15 si   */
        0    | T(2)   |   0    |   0    | T(16)  |   0    |   0    |   0,
/*  16 dle   17 dc1   18 dc2   19 dc3   20 dc4   21 nak   22 syn   23 etb */
        0    |   0    |   0    |   0    |   0    |   0    |   0    |   0,
/*  24 can   25 em    26 sub   27 esc   28 fs    29 gs    30 rs    31 us  */
        0    |   0    |   0    |   0    |   0    |   0    |   0    |   0,
/*  32 sp    33  !    34  "    35  #    36  $    37  %    38  &    39  '  */
        0    |   2    |   4    |   0    |   16   |   32   |   64   |  128,
/*  40  (    41  )    42  *    43  +    44  ,    45  -    46  .    47  /  */
        1    |   2    |   4    |   8    |   16   |   32   |   64   |  128,
/*  48  0    49  1    50  2    51  3    52  4    53  5    54  6    55  7  */
        1    |   2    |   4    |   8    |   16   |   32   |   64   |  128,
/*  56  8    57  9    58  :    59  ;    60  <    61  =    62  >    63  ?  */
        1    |   2    |   4    |   8    |   16   |   32   |   64   |   0,
/*  64  @    65  A    66  B    67  C    68  D    69  E    70  F    71  G  */
        1    |   2    |   4    |   8    |   16   |   32   |   64   |  128,
/*  72  H    73  I    74  J    75  K    76  L    77  M    78  N    79  O  */
        1    |   2    |   4    |   8    |   16   |   32   |   64   |  128,
/*  80  P    81  Q    82  R    83  S    84  T    85  U    86  V    87  W  */
        1    |   2    |   4    |   8    |   16   |   32   |   64   |  128,
/*  88  X    89  Y    90  Z    91  [    92  \    93  ]    94  ^    95  _  */
        1    |   2    |   4    |   8    |   16   |   32   |   64   |  128,
/*  96  `    97  a    98  b    99  c   100  d   101  e   102  f   103  g  */
        1    |   2    |   4    |   8    |   16   |   32   |   64   |  128,
/* 104  h   105  i   106  j   107  k   108  l   109  m   110  n   111  o  */
        1    |   2    |   4    |   8    |   16   |   32   |   64   |  128,
/* 112  p   113  q   114  r   115  s   116  t   117  u   118  v   119  w  */
        1    |   2    |   4    |   8    |   16   |   32   |   64   |  128,
/* 120  x   121  y   122  z   123  {   124  |   125  }   126  ~   127 del */
        1    |   2    |   4    |   8    |   16   |   32   |   64   |   0, };
        
#undef T

enum state {
	s_dead = 1, /* important that this is > 0 */

	s_start_req_or_res,
	s_res_or_resp_H,
	s_start_res,
	s_res_H,
	s_res_HT,
	s_res_HTT,
	s_res_HTTP,
	s_res_first_http_major,
	s_res_http_major,
	s_res_first_http_minor,
	s_res_http_minor,
	s_res_first_status_code,
	s_res_status_code,
	s_res_status_start,
	s_res_status,
	s_res_line_almost_done,

	s_start_req, // step1

	s_req_method, // step2
	s_req_spaces_before_url, // step3
	s_req_schema,
	s_req_schema_slash,
	s_req_schema_slash_slash,
	s_req_server_start,
	s_req_server,
	s_req_server_with_at,
	s_req_path, // step4
	s_req_query_string_start,
	s_req_query_string,
	s_req_fragment_start,
	s_req_fragment,
	s_req_http_start, // step5
	s_req_http_H, // step6
	s_req_http_HT, // step7
	s_req_http_HTT, // step8
	s_req_http_HTTP, // step9
	s_req_first_http_major, // step10
	s_req_http_major, // step11
	s_req_first_http_minor, // step12
	s_req_http_minor, // step13
	s_req_line_almost_done, // step14

	s_header_field_start, // step15
	s_header_field, // step16
	s_header_value_discard_ws,
	s_header_value_discard_ws_almost_done,
	s_header_value_discard_lws,
	s_header_value_start,
	s_header_value,
	s_header_value_lws,

	s_header_almost_done,

	s_chunk_size_start,
	s_chunk_size,
	s_chunk_parameters,
	s_chunk_size_almost_done,

	s_headers_almost_done,
	s_headers_done,

	/* Important: 's_headers_done' must be the last 'header' state. All
	* states beyond this must be 'body' states. It is used for overflow
	* checking. See the PARSING_HEADER() macro.
	*/

	s_chunk_data,
	s_chunk_data_almost_done,
	s_chunk_data_done,

	s_body_identity,
	s_body_identity_eof,

	s_message_done,
};


#define PARSING_HEADER(state) (state <= s_headers_done)


enum header_states {
	h_general = 0,
	h_C,
	h_CO,
	h_CON,

    h_matching_connection,
    h_matching_proxy_connection,
    h_matching_content_length,
    h_matching_transfer_encoding,
    h_matching_upgrade,

    h_connection,
    h_content_length,
    h_transfer_encoding,
    h_upgrade,

    h_matching_transfer_encoding_chunked,
    h_matching_connection_keep_alive,
    h_matching_connection_close,

    h_transfer_encoding_chunked,
    h_connection_keep_alive,
    h_connection_close,
};

enum http_host_state {
    s_http_host_dead = 1,
    s_http_userinfo_start,
    s_http_userinfo,
    s_http_host_start,
    s_http_host_v6_start,
    s_http_host,
    s_http_host_v6,
    s_http_host_v6_end,
    s_http_host_port_start,
    s_http_host_port
};


//#define CR                  '\r'
//#define LF                  '\n'

/* A=0x01000001 a=0x01100001*/
#define LOWER(c)            (unsigned char)(c | 0x20)
#define IS_ALPHA(c)         (LOWER(c) >= 'a' && LOWER(c) <= 'z')
#define IS_NUM(c)           ((c) >= '0' && (c) <= '9')
#define IS_ALPHANUM(c)      (IS_ALPHA(c) || IS_NUM(c))
#define IS_HEX(c)           (IS_NUM(c) || (LOWER(c) >= 'a' && LOWER(c) <= 'f'))

#define IS_MARK(c)          ((c) == '-' || (c) == '_' || (c) == '.' || \
  (c) == '!' || (c) == '~' || (c) == '*' || (c) == '\'' || (c) == '(' || \
  (c) == ')')
  
#define IS_USERINFO_CHAR(c) (IS_ALPHANUM(c) || IS_MARK(c) || (c) == '%' || \
  (c) == ';' || (c) == ':' || (c) == '&' || (c) == '=' || (c) == '+' || \
  (c) == '$' || (c) == ',')


#if HTTP_PARSER_STRICT

#define TOKEN(c)            (tokens[(unsigned char)c])

#define IS_URL_CHAR(c)      (BIT_AT(normal_url_char, (unsigned char)c))

#define IS_HOST_CHAR(c)     (IS_ALPHANUM(c) || (c) == '.' || (c) == '-')

#else

#define TOKEN(c)            ((c == ' ') ? ' ' : tokens[(unsigned char)c])

#define IS_URL_CHAR(c)                                                         \
	(BIT_AT(normal_url_char, (unsigned char)c) || ((c) & 0x80))
	
#define IS_HOST_CHAR(c)                                                        \
	(IS_ALPHANUM(c) || (c) == '.' || (c) == '-' || (c) == '_')
	
#endif


#define start_state (parser->type == HTTP_REQUEST ? s_start_req : s_start_res)


#if HTTP_PARSER_STRICT
#define STRICT_CHECK(cond)                                           \
do {                                                                 \
	if (cond) {                                                      \
		SET_ERRNO(HPE_STRICT);                                       \
		goto error;                                                  \
	}                                                                \
} while (0)

#define NEW_MESSAGE() (gvfs_http_should_keep_alive(parser) ? start_state : s_dead)
#else
#define STRICT_CHECK(cond)
#define NEW_MESSAGE() start_state
#endif


static struct {
	const char *name;
	const char *description;
} gvfs_http_strerror_tab[] = {
	{"HPE_OK", "success"},
	
	{"HPE_CB_message_begin", "the on_message_begin callback failed"},
	{"HPE_CB_url", "the on_url callback failed"},
	{"HPE_CB_header_field", "the on_header_field callback failed"},
	{"HPE_CB_header_value", "the on_header_value callback failed"},
	{"HPE_CB_headers_complete", "the on_headers_complete callback failed"},
	{"HPE_CB_body", "the on_body callback failed"},
	{"HPE_CB_message_complete", "the on_message_complete callback failed"},
	{"HPE_CB_status", "the on_status callback failed"},
	

	{"HPE_INVALID_EOF_STATE", "stream ended at an unexpected time"},
	
	{"HPE_HEADER_OVERFLOW", "too many header bytes seen; overflow detected"},
	{"HPE_CLOSED_CONNECTION", "data received after completed connection: close message"},
	{"HPE_INVALID_VERSION", "invalid HTTP version"},
	{"HPE_INVALID_STATUS", "invalid HTTP status code"},
	{"HPE_INVALID_METHOD", "invalid HTTP method"},
	{"HPE_INVALID_URL", "invalid URL"},
	{"HPE_INVALID_HOST", "invalid host"},
	{"HPE_INVALID_PORT", "invalid port"},
	{"HPE_INVALID_PATH", "invalid path"},
	{"HPE_INVALID_QUERY_STRING", "invalid query string"},
	{"HPE_INVALID_FRAGMENT", "invalid fragment"},
    {"HPE_LF_EXPECTED", "LF character expected"},
	{"HPE_INVALID_HEADER_TOKEN", "invalid character in header"},
	{"HPE_INVALID_CONTENT_LENGTH","invalid character in content-length header"},
	{"HPE_INVALID_CHUNK_SIZE","invalid character in chunk size header"},
	{"HPE_INVALID_CONSTANT", "invalid constant string"},
	{"HPE_INVALID_INTERNAL_STATE", "encountered unexpected internal state"},
	{"HPE_STRICT", "strict mode assertion failed"},
	{"HPE_PAUSED", "parser is paused"},
	{"HPE_UNKNOWN", "an unknown error occurred"}
};


static int http_message_needs_eof(const gvfs_http_parse_t *parser);

/* Our URL parser.
 *
 * This is designed to be shared by gvfs_http_parse_t_execute() for URL validation,
 * hence it has a state transition + byte-for-byte interface. In addition, it
 * is meant to be embedded in gvfs_http_parse_t_parse_url(), which does the dirty
 * work of turning state transitions URL components for its API.
 *
 * This function should only be invoked with non-space characters. It is
 * assumed that the caller cares about (and can detect) the transition between
 * URL and non-URL states by looking for these.
 */
static enum state
parse_url_char(enum state s, const char ch)
{
	if (ch == ' ' || ch == '\r' || ch == '\n') {
		return s_dead;
	}

#if HTTP_PARSER_STRICT
	if (ch == '\t' || ch == '\f') {
		return s_dead;
	}
#endif

	switch (s) {
	case s_req_spaces_before_url:
	
		/* 
		 * Proxied requests are followed by scheme of an absolute URI (alpha).
		 * All methods except CONNECT are followed by '/' or '*'.
		 */
		if (ch == '/' || ch == '*') {
			return s_req_path;
		}
		
		if (IS_ALPHA(ch)) {
			return s_req_schema;
		}

		break;

	case s_req_schema:
		if (IS_ALPHA(ch)) {
			return s;
		}

		if (ch == ':') {
			return s_req_schema_slash;
		}

		break;

	case s_req_schema_slash:
		if (ch == '/') {
			return s_req_schema_slash_slash;
		}

		break;

	case s_req_schema_slash_slash:
		if (ch == '/') {
			return s_req_server_start;
		}

		break;

	case s_req_server_with_at:
		if (ch == '@') {
			return s_dead;
		}

	/* FALLTHROUGH */
	case s_req_server_start:
	case s_req_server:
	{
		if (ch == '/') {
			return s_req_path;
		}

		if (ch == '?') {
			return s_req_query_string_start;
		}

		if (ch == '@') {
			return s_req_server_with_at;
		}

		if (IS_USERINFO_CHAR(ch) || ch == '[' || ch == ']') {
			return s_req_server;
		}

	      break;
	}
	
	case s_req_path:
	{
		if (IS_URL_CHAR(ch)) { 
			return s;
		}

		switch (ch) {
		case '?':
			return s_req_query_string_start;

		case '#':
			return s_req_fragment_start;
		}

		break;
	}
	
	case s_req_query_string_start:
	case s_req_query_string:
	{
		if (IS_URL_CHAR(ch)) {
			return s_req_query_string;
		}

		switch (ch) {
        case '?':
			/* allow extra '?' in query string */
			return s_req_query_string;

		case '#':
			return s_req_fragment_start;
		}

		break;
	}
	
	case s_req_fragment_start:
	{
		if (IS_URL_CHAR(ch)) {
			return s_req_fragment;
		}

		switch (ch) {
		case '?':
			return s_req_fragment;

		case '#':
			return s;
		}

		break;
	}
	
	case s_req_fragment:
	{
		if (IS_URL_CHAR(ch)) {
			return s;
		}

		switch (ch) {
			case '?':
			case '#':
				return s;
			}

	      break;
	}
	
    default:
		break;
  }

	/* We should never fall out of the switch above unless there's an error */
	return s_dead;
}


size_t
gvfs_http_parse(gvfs_http_parse_t *parser, const gvfs_http_parse_cb_t *settings,
	const char *data, size_t len)
{
	char         c, ch;
	int8_t       unhex_val;
	const char  *p = data;
	const char  *header_field_mark = 0;
	const char  *header_value_mark = 0;
	const char  *url_mark = 0;
	const char  *body_mark = 0;
	const char  *status_mark = 0;

	if (HTTP_PARSER_ERRNO(parser) != HPE_OK) {
		return 0;
	}

	//gvfs_log(LOG_DEBUG, "parser->state: %u", parser->state);
	if (len == 0) {
		switch (parser->state) {
		case s_body_identity_eof:
			CALLBACK_NOTIFY_NOADVANCE(message_complete);
			return 0;

		case s_dead:
		case s_start_req_or_res:
		case s_start_res:
		case s_start_req:
			return 0;

		default:
			SET_ERRNO(HPE_INVALID_EOF_STATE);
			return 1;
		}
	}


	if (parser->state == s_header_field)
		header_field_mark = data;
	if (parser->state == s_header_value)
		header_value_mark = data;
		
	switch (parser->state) {
	case s_req_path:
	case s_req_schema:
	case s_req_schema_slash:
	case s_req_schema_slash_slash:
	case s_req_server_start:
	case s_req_server:
	case s_req_server_with_at:
	case s_req_query_string_start:
	case s_req_query_string:
	case s_req_fragment_start:
	case s_req_fragment:
		url_mark = data;
		break;
	case s_res_status:
		status_mark = data;
		break;
	}

	for (p = data; p != data + len; p++) {
		ch = *p;

		if (PARSING_HEADER(parser->state)) {
			++parser->nread;
			
			if (parser->nread > HTTP_MAX_HEADER_SIZE) {
				SET_ERRNO(HPE_HEADER_OVERFLOW);
				goto error;
			}
	    }

		reexecute_byte:
		switch (parser->state) {

		case s_dead:
		{
			if (ch == CR || ch == LF)
				break;

			SET_ERRNO(HPE_CLOSED_CONNECTION);
			goto error;
		}

		case s_start_req_or_res:
		{
			if (ch == CR || ch == LF)
				break;
				
			parser->flags = 0;
			parser->content_length = ULLONG_MAX;

			if (ch == 'H') {
				parser->state = s_res_or_resp_H;
				CALLBACK_NOTIFY(message_begin);
			} 
			else {
				parser->type = HTTP_REQUEST;
				parser->state = s_start_req;
				
				goto reexecute_byte;
			}

			break;
		}

		case s_res_or_resp_H:
		{
			if (ch == 'T') {
				parser->type = HTTP_RESPONSE;
				parser->state = s_res_HT;
			}
			else {
				if (ch != 'E') {
					SET_ERRNO(HPE_INVALID_CONSTANT);
					goto error;
				}

				parser->type = HTTP_REQUEST;
				parser->method = HTTP_HEAD;
				parser->index = 2;
				parser->state = s_req_method;
			}
			break;
		}
		
		case s_start_res:
		{
			parser->flags = 0;
			parser->content_length = ULLONG_MAX;

			switch (ch) {
			case 'H':
				parser->state = s_res_H;
				break;

			case CR:
			case LF:
				break;

			default:
				SET_ERRNO(HPE_INVALID_CONSTANT);
				goto error;
			}

			CALLBACK_NOTIFY(message_begin);
			break;
		}

		case s_res_H:
			STRICT_CHECK(ch != 'T');
			parser->state = s_res_HT;
			break;

		case s_res_HT:
			STRICT_CHECK(ch != 'T');
			parser->state = s_res_HTT;
			break;

		case s_res_HTT:
			STRICT_CHECK(ch != 'P');
			parser->state = s_res_HTTP;
			break;

		case s_res_HTTP:
			STRICT_CHECK(ch != '/');
			parser->state = s_res_first_http_major;
			break;

		case s_res_first_http_major:
			if (ch < '0' || ch > '9') {
				SET_ERRNO(HPE_INVALID_VERSION);
				goto error;
			}

	        parser->http_major = ch - '0';
	        parser->state = s_res_http_major;
	        break;

		case s_res_http_major:
		{
			if (ch == '.') {
				parser->state = s_res_first_http_minor;
				break;
			}

			if (!IS_NUM(ch)) {
				SET_ERRNO(HPE_INVALID_VERSION);
				goto error;
			}

			parser->http_major *= 10;
			parser->http_major += ch - '0';

			if (parser->http_major > 999) {
				SET_ERRNO(HPE_INVALID_VERSION);
				goto error;
			}

			break;
		}

		case s_res_first_http_minor:
			if (!IS_NUM(ch)) {
				SET_ERRNO(HPE_INVALID_VERSION);
				goto error;
			}

			parser->http_minor = ch - '0';
			parser->state = s_res_http_minor;
			break;

		case s_res_http_minor:
		{
			if (ch == ' ') {
				parser->state = s_res_first_status_code;
				break;
			}

			if (!IS_NUM(ch)) {
				SET_ERRNO(HPE_INVALID_VERSION);
				goto error;
			}

			parser->http_minor *= 10;
			parser->http_minor += ch - '0';

			if (parser->http_minor > 999) {
				SET_ERRNO(HPE_INVALID_VERSION);
				goto error;
			}

			break;
		}

		case s_res_first_status_code:
		{
			if (!IS_NUM(ch)) {
				if (ch == ' ') {
					break;
				}

				SET_ERRNO(HPE_INVALID_STATUS);
				goto error;
			}
			
			parser->status_code = ch - '0';
			parser->state = s_res_status_code;
			break;
		}

		case s_res_status_code:
		{
			if (!IS_NUM(ch)) {
				switch (ch) {
				case ' ':
					parser->state = s_res_status_start;
					break;
				case CR:
					parser->state = s_res_line_almost_done;
					break;
				case LF:
					parser->state = s_header_field_start;
					break;
				default:
					SET_ERRNO(HPE_INVALID_STATUS);
					goto error;
				}
				break;
			}

			parser->status_code *= 10;
			parser->status_code += ch - '0';

			if (parser->status_code > 999) {
				SET_ERRNO(HPE_INVALID_STATUS);
				goto error;
			}

			break;
		}

		case s_res_status_start:
		{
			if (ch == CR) {
				parser->state = s_res_line_almost_done;
				break;
			}

			if (ch == LF) {
				parser->state = s_header_field_start;
				break;
			}

			MARK(status);
			parser->state = s_res_status;
			parser->index = 0;
			break;
		}

		case s_res_status:
			if (ch == CR) {
				parser->state = s_res_line_almost_done;
				CALLBACK_DATA(status);
				break;
			}

			if (ch == LF) {
				parser->state = s_header_field_start;
				CALLBACK_DATA(status);
				break;
			}

			break;

		case s_res_line_almost_done:
			STRICT_CHECK(ch != LF);
			parser->state = s_header_field_start;
			break;

		case s_start_req:
		{
			if (ch == CR || ch == LF)
				break;

			if (!IS_ALPHA(ch)) {
				SET_ERRNO(HPE_INVALID_METHOD);
				goto error;
			}

			parser->flags = 0;
			parser->content_length = ULLONG_MAX;
			parser->method = (gvfs_http_method_t) 0;
			parser->index = 1;

			switch (ch) 
			{
			case 'C': /* or COPY, CHECKOUT */
				parser->method = HTTP_CONNECT; 
				break;
			case 'D':
				parser->method = HTTP_DELETE;
				break;
			case 'G':
				parser->method = HTTP_GET;
				break;



			case 'H':
				parser->method = HTTP_HEAD;
				break;
			case 'L':
				parser->method = HTTP_LOCK;
				break;
			case 'M': /* or MOVE, MKACTIVITY, MERGE, M-SEARCH */
				parser->method = HTTP_MKCOL; 
				break;
			case 'N':
				parser->method = HTTP_NOTIFY;
				break;
			case 'O': 
				parser->method = HTTP_OPTIONS; 
				break;
			case 'P': /* or PROPFIND|PROPPATCH|PUT|PATCH|PURGE */
				parser->method = HTTP_POST; 
				break;
				
			case 'R': 
				parser->method = HTTP_REPORT;
				break;
			case 'S': /* or SEARCH */
				parser->method = HTTP_SUBSCRIBE; 
				break;
			case 'T': 
				parser->method = HTTP_TRACE;
				break;
			case 'U': /* or UNSUBSCRIBE */
				parser->method = HTTP_UNLOCK; 
				break;
			
			default:
				SET_ERRNO(HPE_INVALID_METHOD);
				goto error;
			}

			parser->state = s_req_method;

			CALLBACK_NOTIFY(message_begin);

			break;
		}

		case s_req_method:
		{
			const char *matcher;
			
			if (ch == '\0') {
				SET_ERRNO(HPE_INVALID_METHOD);
				goto error;
			}

			matcher = gvfs_http_method_strings[parser->method];

			if (ch == ' ' && matcher[parser->index] == '\0') { 
				parser->state = s_req_spaces_before_url;
			}
			else if (ch == matcher[parser->index]) {
				;
			}
			else if (parser->method == HTTP_CONNECT) {
				if (parser->index == 1 && ch == 'H') {
					parser->method = HTTP_CHECKOUT;
				}
				else if (parser->index == 2  && ch == 'P') {
					parser->method = HTTP_COPY;
				}
				else {
					SET_ERRNO(HPE_INVALID_METHOD);
					goto error;
				}
			} 
			else if (parser->method == HTTP_MKCOL) {
				if (parser->index == 1 && ch == 'O') {
					parser->method = HTTP_MOVE;
				}
				else if (parser->index == 1 && ch == 'E') {
					parser->method = HTTP_MERGE;
				} 
				else if (parser->index == 1 && ch == '-') {
					parser->method = HTTP_MSEARCH;
				} 
				else if (parser->index == 2 && ch == 'A') {
					parser->method = HTTP_MKACTIVITY;
				} 
				else {
					SET_ERRNO(HPE_INVALID_METHOD);
					goto error;
				}
	        } 
			else if (parser->method == HTTP_SUBSCRIBE) {
				if (parser->index == 1 && ch == 'E') {
					parser->method = HTTP_SEARCH;
				} 
				else {
					SET_ERRNO(HPE_INVALID_METHOD);
					goto error;
				}
			} 
			else if (parser->index == 1 && parser->method == HTTP_POST) {
				if (ch == 'R') { /* or HTTP_PROPPATCH */
					parser->method = HTTP_PROPFIND; 
				} 
				else if (ch == 'U') { /* or HTTP_PURGE */
					parser->method = HTTP_PUT; 
				} 
				else if (ch == 'A') {
					parser->method = HTTP_PATCH;
				} 
				else {
					SET_ERRNO(HPE_INVALID_METHOD);
					goto error;
				}
	        } 
			else if (parser->index == 2) {
			
				if (parser->method == HTTP_PUT) {
					if (ch == 'R') {
						parser->method = HTTP_PURGE;
					}
					else {
						SET_ERRNO(HPE_INVALID_METHOD);
						goto error;
					}
				}
				
				else if (parser->method == HTTP_UNLOCK) {
					if (ch == 'S') {
						parser->method = HTTP_UNSUBSCRIBE;
					} else {
						SET_ERRNO(HPE_INVALID_METHOD);
						goto error;
					}
				} 
				
				else {
					SET_ERRNO(HPE_INVALID_METHOD);
					goto error;
				}
				
			} 
			else if (parser->index == 4 && parser->method == HTTP_PROPFIND
				&& ch == 'P') {
				parser->method = HTTP_PROPPATCH;
	        } 
	        else {
				SET_ERRNO(HPE_INVALID_METHOD);
				goto error;
	        }

			++parser->index;
			break;
		}

		case s_req_spaces_before_url:
		{
			/* 如果url前面有多个空格也做兼容性处理 */
			if (ch == ' ') {
				break;
			}

			MARK(url);
			
			if (parser->method == HTTP_CONNECT) {
				parser->state = s_req_server_start;
			}

			/* 返回下一阶段处理状态 */
	        parser->state = parse_url_char((enum state)parser->state, ch);
			if (parser->state == s_dead) {
				SET_ERRNO(HPE_INVALID_URL);
				goto error;
			}

			break;
		}

		case s_req_schema:
		case s_req_schema_slash:
		case s_req_schema_slash_slash:
		case s_req_server_start:
		{
			switch (ch) {
			/* No whitespace allowed here */
			case ' ':
			case CR:
			case LF:
				SET_ERRNO(HPE_INVALID_URL);
				goto error;
			default:
				parser->state = parse_url_char((enum state)parser->state, ch);
				if (parser->state == s_dead) {
					SET_ERRNO(HPE_INVALID_URL);
					goto error;
				}
			}

			break;
		}

		case s_req_server:
		case s_req_server_with_at:
		case s_req_path:
		case s_req_query_string_start:
		case s_req_query_string:
		case s_req_fragment_start:
		case s_req_fragment:
		{
			switch (ch) {
			case ' ': /* 到了url和HTTP协议版本之间的空格,回调on_url*/
				parser->state = s_req_http_start;
				CALLBACK_DATA(url);
				break;
			case CR:
			case LF:
				parser->http_major = 0;
				parser->http_minor = 9;
				parser->state = (ch == CR) ?
				s_req_line_almost_done :
				s_header_field_start;
				CALLBACK_DATA(url);
				break;
			default:
				parser->state = parse_url_char((enum state)parser->state, ch);
				if (parser->state == s_dead) {
					SET_ERRNO(HPE_INVALID_URL);
					goto error;
				}
			}
		
			break;
		}

		case s_req_http_start:
		{
			switch (ch) {
			case 'H':
				parser->state = s_req_http_H;
				break;
			case ' ':
				break;
			default:
				SET_ERRNO(HPE_INVALID_CONSTANT);
				goto error;
			}
			break;
		}

		case s_req_http_H:
			STRICT_CHECK(ch != 'T');
			parser->state = s_req_http_HT;
			break;

		case s_req_http_HT:
			STRICT_CHECK(ch != 'T');
			parser->state = s_req_http_HTT;
			break;

		case s_req_http_HTT:
			STRICT_CHECK(ch != 'P');
			parser->state = s_req_http_HTTP;
			break;

		case s_req_http_HTTP:
			STRICT_CHECK(ch != '/');
			parser->state = s_req_first_http_major;
			break;

		case s_req_first_http_major:
			if (ch < '1' || ch > '9') {
				SET_ERRNO(HPE_INVALID_VERSION);
				goto error;
			}

			parser->http_major = ch - '0';
			parser->state = s_req_http_major;
			break;

		case s_req_http_major:
		{
			if (ch == '.') {
				parser->state = s_req_first_http_minor;
				break;
			}

			if (!IS_NUM(ch)) {
				SET_ERRNO(HPE_INVALID_VERSION);
				goto error;
			}

			parser->http_major *= 10;
			parser->http_major += ch - '0';

			if (parser->http_major > 999) {
				SET_ERRNO(HPE_INVALID_VERSION);
				goto error;
			}

			break;
		}

		case s_req_first_http_minor:
			if (!IS_NUM(ch)) {
				SET_ERRNO(HPE_INVALID_VERSION);
				goto error;
			}

			parser->http_minor = ch - '0';
			parser->state = s_req_http_minor;
			break;

		case s_req_http_minor:
		{
			if (ch == CR) {
				parser->state = s_req_line_almost_done;
				break;
			}

			if (ch == LF) {
				parser->state = s_header_field_start;
				break;
			}

			if (!IS_NUM(ch)) {
				SET_ERRNO(HPE_INVALID_VERSION);
				goto error;
			}

			parser->http_minor *= 10;
			parser->http_minor += ch - '0';

			if (parser->http_minor > 999) {
				SET_ERRNO(HPE_INVALID_VERSION);
				goto error;
			}

			break;
		}

		case s_req_line_almost_done:
		{
			if (ch != LF) {
				SET_ERRNO(HPE_LF_EXPECTED);
				goto error;
			}

			parser->state = s_header_field_start;
			break;
		}

		case s_header_field_start:
		{
	        if (ch == CR) {
				parser->state = s_headers_almost_done;
				break;
	        }

	        if (ch == LF) {
				parser->state = s_headers_almost_done;
				goto reexecute_byte;
	        }

	        c = TOKEN(ch); /* TOKEN中存放ASCII表,大写字母也当小写字母处理*/

	        if (!c) {
				SET_ERRNO(HPE_INVALID_HEADER_TOKEN);
				goto error;
	        }

	        MARK(header_field);

	        parser->index = 0;
	        parser->state = s_header_field;

	        switch (c)
	        {
			case 'c':
				parser->header_state = h_C;
				break;

			case 'p':
				parser->header_state = h_matching_proxy_connection;
				break;

			case 't':
				parser->header_state = h_matching_transfer_encoding;
				break;

			case 'u':
				parser->header_state = h_matching_upgrade;
				break;

			default:
				parser->header_state = h_general;
				break;
	        }
	        
			break;
		}

		case s_header_field:
		{
			c = TOKEN(ch);

			if (c) {
				switch (parser->header_state)
				{
				case h_general:
					break;

	            case h_C: 
					parser->index++;
					parser->header_state = (c == 'o' ? h_CO : h_general);
					break;

	            case h_CO:
					parser->index++;
					parser->header_state = (c == 'n' ? h_CON : h_general);
					break;

	            case h_CON:
					parser->index++;
					switch (c)
					{
	                case 'n':
						parser->header_state = h_matching_connection;
						break;
	                case 't':
						parser->header_state = h_matching_content_length;
						break;
	                default:
						parser->header_state = h_general;
						break;
					}
					break;

				/* connection */
				case h_matching_connection:
					parser->index++;
					if (parser->index > sizeof(CONNECTION)-1
						|| c != CONNECTION[parser->index]) {
						parser->header_state = h_general;
					}
					else if (parser->index == sizeof(CONNECTION)-2) {
						/* connection已经扫描完成 */
						parser->header_state = h_connection;
					}
					break;

				/* proxy-connection */
				case h_matching_proxy_connection:
					parser->index++;
					if (parser->index > sizeof(PROXY_CONNECTION)-1
						|| c != PROXY_CONNECTION[parser->index]) {
						parser->header_state = h_general;
					}
					else if (parser->index == sizeof(PROXY_CONNECTION)-2) {
						parser->header_state = h_connection;
					}
					break;

				/* content-length */
				case h_matching_content_length:
					parser->index++;
					if (parser->index > sizeof(CONTENT_LENGTH)-1
						|| c != CONTENT_LENGTH[parser->index]) {
						parser->header_state = h_general;
					} 
					else if (parser->index == sizeof(CONTENT_LENGTH)-2) {
						parser->header_state = h_content_length;
					}
					break;

				/* transfer-encoding */
				case h_matching_transfer_encoding:
					parser->index++;
					if (parser->index > sizeof(TRANSFER_ENCODING)-1
						|| c != TRANSFER_ENCODING[parser->index]) {
						parser->header_state = h_general;
					} 
					else if (parser->index == sizeof(TRANSFER_ENCODING)-2) {
						parser->header_state = h_transfer_encoding;
					}
					break;

				/* upgrade */
				case h_matching_upgrade:
					parser->index++;
					if (parser->index > sizeof(UPGRADE)-1
						|| c != UPGRADE[parser->index]) {
						parser->header_state = h_general;
					} 
					else if (parser->index == sizeof(UPGRADE)-2) {
						parser->header_state = h_upgrade;
					}
					break;

				case h_connection:
				case h_content_length:
				case h_transfer_encoding:
				case h_upgrade:
					if (ch != ' ')
						parser->header_state = h_general;
					break;

				default:
					break;
				}
				
				break;
	        }

	        if (ch == ':') {
				parser->state = s_header_value_discard_ws;
				CALLBACK_DATA(header_field);
				break;
	        }

	        if (ch == CR) {
				parser->state = s_header_almost_done;
				CALLBACK_DATA(header_field);
				break;
	        }

			if (ch == LF) {
				parser->state = s_header_field_start;
				CALLBACK_DATA(header_field);
				break;
			}

			SET_ERRNO(HPE_INVALID_HEADER_TOKEN);
			goto error;
		}

		case s_header_value_discard_ws:
		{
			if (ch == ' ' || ch == '\t') break;

			if (ch == CR) {
				parser->state = s_header_value_discard_ws_almost_done;
				break;
			}

			if (ch == LF) {
				parser->state = s_header_value_discard_lws;
				break;
			}

			// 这里没有break,解析的时候直接走到s_header_value_start
		}

		/* FALLTHROUGH */
		case s_header_value_start:
		{
			MARK(header_value);

			parser->state = s_header_value;
			parser->index = 0;

			c = LOWER(ch);

			switch (parser->header_state) {
			case h_upgrade:
				parser->flags |= F_UPGRADE;
				parser->header_state = h_general;
				break;

			case h_transfer_encoding:
				/* looking for 'Transfer-Encoding: chunked' */
				if ('c' == c) {
					parser->header_state = h_matching_transfer_encoding_chunked;
				}
				else {
					parser->header_state = h_general;
				}
				break;

			case h_content_length:
				if (!IS_NUM(ch)) {
					SET_ERRNO(HPE_INVALID_CONTENT_LENGTH);
					goto error;
				}

				parser->content_length = ch - '0';
				parser->body_size = parser->content_length;
				break;

			case h_connection:
				if (c == 'k') { /* looking for 'Connection: keep-alive' */
					parser->header_state = h_matching_connection_keep_alive;
					
				}
				else if (c == 'c') { /* looking for 'Connection: close' */
					parser->header_state = h_matching_connection_close;
				}
				else {
					parser->header_state = h_general;
				}
				break;

			default:
				parser->header_state = h_general;
				break;
			}
			
			break;
			
		}

		case s_header_value:
		{

			if (ch == CR) {
				parser->state = s_header_almost_done;
				CALLBACK_DATA(header_value);
				break;
			}

			if (ch == LF) {
				parser->state = s_header_almost_done;
				CALLBACK_DATA_NOADVANCE(header_value);
				goto reexecute_byte;
			}

			c = LOWER(ch);

			switch (parser->header_state) {
			case h_general:
				break;

			case h_connection:
			case h_transfer_encoding:
				break;

			case h_content_length:
			{
				uint64_t t;

				if (ch == ' ') break;

				if (!IS_NUM(ch)) {
					SET_ERRNO(HPE_INVALID_CONTENT_LENGTH);
					goto error;
				}

				t = parser->content_length;
				t *= 10;
				t += ch - '0';

				if ((ULLONG_MAX - 10) / 10 < parser->content_length) {
					SET_ERRNO(HPE_INVALID_CONTENT_LENGTH);
					goto error;
				}

				parser->content_length = t;
				parser->body_size = t;
				break;
			}

			/* Transfer-Encoding: chunked */
			case h_matching_transfer_encoding_chunked:
				parser->index++;
				if (parser->index > sizeof(CHUNKED)-1
					|| c != CHUNKED[parser->index]) {
					parser->header_state = h_general;
				} 
				else if (parser->index == sizeof(CHUNKED)-2) {
					parser->header_state = h_transfer_encoding_chunked;
				}
				break;

			/* looking for 'Connection: keep-alive' */
			case h_matching_connection_keep_alive:
				parser->index++;
				if (parser->index > sizeof(KEEP_ALIVE)-1
					|| c != KEEP_ALIVE[parser->index]) {
					parser->header_state = h_general;
				} 
				else if (parser->index == sizeof(KEEP_ALIVE)-2) {
					/* keep-alive字段已经匹配完成 */
					parser->header_state = h_connection_keep_alive;
				}
				break;

			/* looking for 'Connection: close' */
			case h_matching_connection_close:
				parser->index++;
				if (parser->index > sizeof(CLOSE)-1 || c != CLOSE[parser->index]) {
					parser->header_state = h_general;
				} 
				else if (parser->index == sizeof(CLOSE)-2) {
					parser->header_state = h_connection_close;
				}
				break;

			case h_transfer_encoding_chunked:
			case h_connection_keep_alive:
			case h_connection_close:
				if (ch != ' ') parser->header_state = h_general;
				break;

			default:
				parser->state = s_header_value;
				parser->header_state = h_general;
				break;
	        }
			break;
		}

		case s_header_almost_done:
		{
			STRICT_CHECK(ch != LF);

			parser->state = s_header_value_lws;
			break;
		}

		case s_header_value_lws:
		{
			if (ch == ' ' || ch == '\t') {
				parser->state = s_header_value_start;
				goto reexecute_byte;
			}

			/* finished the header */
			switch (parser->header_state) {
			case h_connection_keep_alive:
				parser->flags |= F_CONNECTION_KEEP_ALIVE;
				break;
			case h_connection_close:
				parser->flags |= F_CONNECTION_CLOSE;
				break;
			case h_transfer_encoding_chunked:
				parser->flags |= F_CHUNKED;
				break;
			default:
				break;
			}

			parser->state = s_header_field_start;
			goto reexecute_byte;
		}

		case s_header_value_discard_ws_almost_done:
		{
			STRICT_CHECK(ch != LF);
			parser->state = s_header_value_discard_lws;
			break;
		}

		case s_header_value_discard_lws:
		{
			if (ch == ' ' || ch == '\t') {
				parser->state = s_header_value_discard_ws;
				break;
			}
			else {
				/* header value was empty */
				MARK(header_value);
				parser->state = s_header_field_start;
				CALLBACK_DATA_NOADVANCE(header_value);
				goto reexecute_byte;
			}
		}

		case s_headers_almost_done:
		{
			STRICT_CHECK(ch != LF);

			if (parser->flags & F_TRAILING) {
				/* End of a chunked request */
				parser->state = NEW_MESSAGE();
				CALLBACK_NOTIFY(message_complete);
				break;
			}

			parser->state = s_headers_done;

			parser->upgrade =
				(parser->flags & F_UPGRADE || parser->method == HTTP_CONNECT);

			if (settings->on_headers_complete) {
				switch (settings->on_headers_complete(parser)) {
				case 0:
					break;

				case 1:
					parser->flags |= F_SKIPBODY;
					break;

				default:
					SET_ERRNO(HPE_CB_headers_complete);
					return p - data; /* Error */
				}
			}

			if (HTTP_PARSER_ERRNO(parser) != HPE_OK) {
				return p - data;
			}

			/* 注意goto与break的区别,goto不会计算到下一个字节 */
			goto reexecute_byte;
		}

		case s_headers_done:
		{
			STRICT_CHECK(ch != LF);

			parser->nread = 0;

			/* Exit, the rest of the connect is in a different protocol. */
			if (parser->upgrade) {
				parser->state = NEW_MESSAGE();
				CALLBACK_NOTIFY(message_complete);
				return (p - data) + 1;
			}

			if (parser->flags & F_SKIPBODY) {
				parser->state = NEW_MESSAGE();
				CALLBACK_NOTIFY(message_complete);
			} else if (parser->flags & F_CHUNKED) {
				/* chunked encoding - ignore Content-Length header */
				parser->state = s_chunk_size_start;
			} else {
				if (parser->content_length == 0) {
					/* Content-Length header given but zero: Content-Length: 0\r\n */
					parser->state = NEW_MESSAGE();
					CALLBACK_NOTIFY(message_complete);
				} else if (parser->content_length != ULLONG_MAX) {
					/* Content-Length header given and non-zero */
					parser->state = s_body_identity;
				} else {
					if (parser->type == HTTP_REQUEST ||
						!http_message_needs_eof(parser)) {
						/* Assume content-length 0 - read the next */
						parser->state = NEW_MESSAGE();
						CALLBACK_NOTIFY(message_complete);
					} else {
						/* Read body until EOF */
						parser->state = s_body_identity_eof;
					}
				}
			}

			break;
		}

		case s_body_identity:
		{
			uint64_t to_read = MIN(parser->content_length,
			(uint64_t) ((data + len) - p));

			MARK(body);
			parser->content_length -= to_read;
			p += to_read - 1;

			if (parser->content_length == 0) {
				parser->state = s_message_done;

				CALLBACK_DATA_(body, p - body_mark + 1, p - data);
				goto reexecute_byte;
			}

			break;
		}

		case s_body_identity_eof:
			MARK(body);
			p = data + len - 1;

			break;

		case s_message_done:
			parser->state = NEW_MESSAGE();
			CALLBACK_NOTIFY(message_complete);
			break;

		case s_chunk_size_start:
		{
			unhex_val = unhex[(unsigned char)ch];
			if (unhex_val == -1) {
				SET_ERRNO(HPE_INVALID_CHUNK_SIZE);
				goto error;
			}

			parser->content_length = unhex_val;
			parser->state = s_chunk_size;
			break;
		}

		case s_chunk_size:
		{
			uint64_t t;

			if (ch == CR) {
				parser->state = s_chunk_size_almost_done;
				break;
			}

			unhex_val = unhex[(unsigned char)ch];

			if (unhex_val == -1) {
				if (ch == ';' || ch == ' ') {
					parser->state = s_chunk_parameters;
					break;
				}

				SET_ERRNO(HPE_INVALID_CHUNK_SIZE);
				goto error;
			}

			t = parser->content_length;
			t *= 16;
			t += unhex_val;

			/* Overflow? Test against a conservative limit for simplicity. */
			if ((ULLONG_MAX - 16) / 16 < parser->content_length) {
				SET_ERRNO(HPE_INVALID_CONTENT_LENGTH);
				goto error;
			}

			parser->content_length = t;
			break;
		}

		case s_chunk_parameters:
		{
			/* just ignore this shit. TODO check for overflow */
			if (ch == CR) {
				parser->state = s_chunk_size_almost_done;
				break;
			}
			break;
		}

		case s_chunk_size_almost_done:
		{
			STRICT_CHECK(ch != LF);

			parser->nread = 0;

			if (parser->content_length == 0) {
				parser->flags |= F_TRAILING;
				parser->state = s_header_field_start;
			} else {
				parser->state = s_chunk_data;
			}
			break;
		}

		case s_chunk_data:
		{
			uint64_t to_read = MIN(parser->content_length,
			(uint64_t) ((data + len) - p));

			/* See the explanation in s_body_identity for why the content
			* length and data pointers are managed this way.
			*/
			MARK(body);
			parser->content_length -= to_read;
			p += to_read - 1;

			if (parser->content_length == 0) {
				parser->state = s_chunk_data_almost_done;
			}

			break;
		}

		case s_chunk_data_almost_done:
			STRICT_CHECK(ch != CR);
			parser->state = s_chunk_data_done;
			CALLBACK_DATA(body);
			break;

		case s_chunk_data_done:
			STRICT_CHECK(ch != LF);
			parser->nread = 0;
			parser->state = s_chunk_size_start;
			break;

		default:
			SET_ERRNO(HPE_INVALID_INTERNAL_STATE);
			goto error;
		}
	}

	CALLBACK_DATA_NOADVANCE(header_field);
	CALLBACK_DATA_NOADVANCE(header_value);
	CALLBACK_DATA_NOADVANCE(url);
	CALLBACK_DATA_NOADVANCE(body);
	CALLBACK_DATA_NOADVANCE(status);

	return len;

error:
	if (HTTP_PARSER_ERRNO(parser) == HPE_OK) {
		SET_ERRNO(HPE_UNKNOWN);
	}

	return (p - data);
}


/* Does the parser need to see an EOF to find the end of the message? */
static int
http_message_needs_eof (const gvfs_http_parse_t *parser)
{
	if (parser->type == HTTP_REQUEST) {
		return 0;
	}

	/* See RFC 2616 section 4.4 */
	if (parser->status_code / 100 == 1 || /* 1xx e.g. Continue */
		parser->status_code == 204 ||     /* No Content */
		parser->status_code == 304 ||     /* Not Modified */
		parser->flags & F_SKIPBODY) {     /* response to a HEAD request */
		
		return 0;
	}

	if ((parser->flags & F_CHUNKED) || parser->content_length != ULLONG_MAX) {
		return 0;
	}

	return 1;
}


int
gvfs_http_should_keep_alive(const gvfs_http_parse_t *parser)
{
	if (parser->http_major > 0 && parser->http_minor > 0) {
		/* HTTP/1.1 */
		if (parser->flags & F_CONNECTION_CLOSE) {
			return 0;
		}
	}
	else {
		/* HTTP/1.0 or earlier */
		if (!(parser->flags & F_CONNECTION_KEEP_ALIVE)) {
			return 0;
		}
	}

	return !http_message_needs_eof(parser);
}


const char *
gvfs_http_method_str(gvfs_http_method_t m)
{
	if (m >= sizeof(gvfs_http_method_strings) / sizeof(gvfs_http_method_strings[0])) {
		return NULL;
	}

	return gvfs_http_method_strings[m];
}


void
gvfs_http_parse_init (gvfs_http_parse_t *parser, gvfs_http_parse_type_t t)
{
	void *data = parser->data; /* preserve application data */
	memset(parser, 0, sizeof(*parser));
	parser->data = data;
	parser->type = t;
	parser->state = (t == HTTP_REQUEST ? s_start_req : (t == HTTP_RESPONSE ? s_start_res : s_start_req_or_res));
	parser->http_errno = HPE_OK;
}


const char *
gvfs_http_errno_name(gvfs_http_errno_t err)
{
	if (err >= sizeof(gvfs_http_strerror_tab)/sizeof(gvfs_http_strerror_tab[0])) {
		return NULL;
	}
	
	return gvfs_http_strerror_tab[err].name;
}

const char *
gvfs_http_errno_description(gvfs_http_errno_t err)
{
	if (err >= sizeof(gvfs_http_strerror_tab)/sizeof(gvfs_http_strerror_tab[0])) {
		return NULL;
	}
	
	return gvfs_http_strerror_tab[err].description;
}

static enum http_host_state
http_parse_host_char(enum http_host_state s, const char ch)
{
	switch(s) {
	case s_http_userinfo:
	case s_http_userinfo_start:
		if (ch == '@') {
			return s_http_host_start;
		}

		if (IS_USERINFO_CHAR(ch)) {
			return s_http_userinfo;
		}
		break;

	case s_http_host_start:
		if (ch == '[') {
			return s_http_host_v6_start;
		}

		if (IS_HOST_CHAR(ch)) {
			return s_http_host;
		}

		break;

	case s_http_host:
		if (IS_HOST_CHAR(ch)) {
			return s_http_host;
		}

	/* FALLTHROUGH */
	case s_http_host_v6_end:
		if (ch == ':') {
			return s_http_host_port_start;
		}

		break;

	case s_http_host_v6:
		if (ch == ']') {
			return s_http_host_v6_end;
		}

	/* FALLTHROUGH */
	case s_http_host_v6_start:
		if (IS_HEX(ch) || ch == ':' || ch == '.') {
			return s_http_host_v6;
		}

		break;

	case s_http_host_port:
	case s_http_host_port_start:
		if (IS_NUM(ch)) {
			return s_http_host_port;
		}

		break;

	default:
	
		break;
		
	}
	
	return s_http_host_dead;
	
}

static int
http_parse_host(const char * buf, gvfs_http_parse_url_t *u, int found_at)
{
	enum http_host_state s;

	const char *p;
	size_t buflen = u->field_data[UF_HOST].off + u->field_data[UF_HOST].len;

	u->field_data[UF_HOST].len = 0;

	s = found_at ? s_http_userinfo_start : s_http_host_start;

	for (p = buf + u->field_data[UF_HOST].off; p < buf + buflen; p++) {
		enum http_host_state new_s = http_parse_host_char(s, *p);

		if (new_s == s_http_host_dead) {
			return 1;
		}

		switch(new_s) {
		case s_http_host:
			if (s != s_http_host) {
				u->field_data[UF_HOST].off = p - buf;
			}
			u->field_data[UF_HOST].len++;
			break;

		case s_http_host_v6:
			if (s != s_http_host_v6) {
				u->field_data[UF_HOST].off = p - buf;
			}
			u->field_data[UF_HOST].len++;
			break;

		case s_http_host_port:
			if (s != s_http_host_port) {
				u->field_data[UF_PORT].off = p - buf;
				u->field_data[UF_PORT].len = 0;
				u->field_set |= (1 << UF_PORT);
			}
			u->field_data[UF_PORT].len++;
			break;

		case s_http_userinfo:
			if (s != s_http_userinfo) {
				u->field_data[UF_USERINFO].off = p - buf ;
				u->field_data[UF_USERINFO].len = 0;
				u->field_set |= (1 << UF_USERINFO);
			}
			u->field_data[UF_USERINFO].len++;
			break;

		default:
			break;
	    }
		s = new_s;
	}

	/* Make sure we don't end somewhere unexpected */
	switch (s) {
	case s_http_host_start:
	case s_http_host_v6_start:
	case s_http_host_v6:
	case s_http_host_port_start:
	case s_http_userinfo:
	case s_http_userinfo_start:
		return 1;
	default:
		break;
	}

	return 0;
}

int
gvfs_http_parse_url(const char *request_url, unsigned int request_url_len,
	int is_connect, gvfs_http_parse_url_t *u)
{
	enum state s;
	const char *p;
	const char *buf;
	size_t buflen;
	gvfs_http_parse_url_fields_t uf, old_uf;
	int found_at = 0;

	u->port = u->field_set = 0;
	s = is_connect ? s_req_server_start : s_req_spaces_before_url;
	uf = old_uf = UF_MAX;

	buf = request_url;
	buflen = request_url_len;
	
	for (p = buf; p < buf + buflen; p++) {
		s = parse_url_char(s, *p);

		/* Figure out the next field that we're operating on */
		switch (s) {
		case s_dead:
			return 1;

		/* Skip delimeters */
		case s_req_schema_slash:
		case s_req_schema_slash_slash:
		case s_req_server_start:
		case s_req_query_string_start:
		case s_req_fragment_start:
			continue;

		case s_req_schema:
			uf = UF_SCHEMA;
			break;

		case s_req_server_with_at:
			found_at = 1;

		/* FALLTROUGH */
		case s_req_server:
			uf = UF_HOST;
			break;

		case s_req_path:
			uf = UF_PATH;
			break;

		case s_req_query_string:
			uf = UF_QUERY;
			break;

		case s_req_fragment:
			uf = UF_FRAGMENT;
			break;

		default:
			return 1;
		}

		/* Nothing's changed; soldier on */
		if (uf == old_uf) {
			u->field_data[uf].len++;
			continue;
		}

		u->field_data[uf].off = p - buf;
		u->field_data[uf].len = 1;

		u->field_set |= (1 << uf);
		old_uf = uf;
	}

	/* host must be present if there is a schema */
	/* parsing http:///toto will fail */
	if ((u->field_set & ((1 << UF_SCHEMA) | (1 << UF_HOST))) != 0) {
		if (http_parse_host(buf, u, found_at) != 0) {
			return 1;
		}
	}

	/* CONNECT requests can only contain "hostname:port" */
	if (is_connect && u->field_set != ((1 << UF_HOST)|(1 << UF_PORT))) {
		return 1;
	}

	if (u->field_set & (1 << UF_PORT)) {
		/* Don't bother with endp; we've already validated the string */
		unsigned long v = strtoul(buf + u->field_data[UF_PORT].off, NULL, 10);

		/* Ports have a max value of 2^16 */
		if (v > 0xffff) {
			return 1;
		}

		u->port = (uint16_t) v;
	}

	return 0;
}

void gvfs_http_url_dump (char *request_url, const gvfs_http_parse_url_t *u,
	char **out, unsigned int *olen, gvfs_http_parse_url_fields_t type)
{
	unsigned short i;
	unsigned short len;
	unsigned short off;
	
	for (i = 0; i < UF_MAX; i++)
	{
		if (!(u->field_set & (1 << i)))
			continue;

		len = u->field_data[i].len;
		off = u->field_data[i].off;
		if (i == type) {
			*olen = len;
			*out =  request_url + off;
		}
	}
}

int
gvfs_http_body_is_final(const gvfs_http_parse_t *parser) {
	return parser->state == s_message_done;
}
