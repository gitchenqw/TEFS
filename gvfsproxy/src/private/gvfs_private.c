

#include <gvfs_config.h>
#include <gvfs_core.h>

VOID gvfs_upstream_to_http_response(gvfs_http_request_t *r,
	gvfs_http_response_code code, CHAR *http_body)
{
	gvfs_buf_t                   *headers_out;
	CHAR                          request_body[HTTP_MAIN_CHUNK_SIZE] = {0};
	size_t                        content_length, bsize, rsize;

	headers_out = &r->headers_out;

	if (NULL == http_body) {
		content_length = snprintf(request_body, HTTP_MAIN_CHUNK_SIZE, 
						 "<html>\r\n"
						     "<head>\r\n"
						     "<title>Welcome to gvfs</title>\r\n"
						     "</head>\r\n"
						    
						     "<body bgcolor=\"white\" text=\"black\">\r\n"
						        "<center><h1>%s!</h1></center>\r\n"
						     "</body>\r\n"
						 "</html>\r\n",
						 gvfs_http_reason_phrase(code));
	}
	else {
		content_length = snprintf(request_body, 128,
								 "%s",
								 http_body);
	}
	
	gvfs_http_response_headers(headers_out, code, content_length);

	bsize = headers_out->end - headers_out->last;
	rsize = snprintf((CHAR *) headers_out->last, bsize,
			 "%s",
			 request_body);
	
	headers_out->last += rsize;
	
}

