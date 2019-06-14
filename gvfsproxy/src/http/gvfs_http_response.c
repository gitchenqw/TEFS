

#include <gvfs_config.h>
#include <gvfs_core.h>

#include <gvfs_proxy.h>

static const char *const http_responses[1000] = {
    [100] = "Continue",
    [200] = "OK",
    [201] = "Created",
    [202] = "Accepted",
    [400] = "Bad Request",
    [401] = "Unauthorized",
    [403] = "Forbidden",
    [404] = "Not Found",
    [406] = "Not Acceptable",
    [415] = "Unsupported Media Type",
    [500] = "Internal Server Error",
    [501] = "Not Implemented",
    [503] = "Service Unavailable",
    [505] = "HTTP Version Not Supported",
    [999] = NULL
};

const char *gvfs_http_reason_phrase(gvfs_http_response_code code)
{
    if (100 > code && 999 <= code) {
    	return NULL;
    }

    return http_responses[code];
}

VOID gvfs_http_response_headers(gvfs_buf_t *headers_out,
	gvfs_http_response_code code, size_t content_length)
{
	size_t    bsize, rsize;

	bsize = headers_out->end - headers_out->last;

	rsize = snprintf((CHAR *) headers_out->last, bsize, 
		    		 "HTTP/1.1 %d %s\r\n"
					 "Server: %s\r\n"
					 "Content-Type: text/html\r\n",
					 code, gvfs_http_reason_phrase(code), GVFS_VER);

	headers_out->last += rsize;
	bsize -= rsize;
	
	if (0 != content_length) {
		rsize = snprintf((CHAR *) headers_out->last, bsize,
						  "Content-Length: %lu\r\n",
						  content_length);
		headers_out->last += rsize;
		bsize -= rsize;
	}
	
	rsize = snprintf((CHAR *) headers_out->last, bsize,
					  "%s",
					  "Connection: keep-alive\r\n"
					  "\r\n");
	headers_out->last += rsize;
	
}
