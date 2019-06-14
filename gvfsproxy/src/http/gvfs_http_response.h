

#ifndef _GVFS_HTTP_RESPONSE_H_INCLUDE_
#define _GVFS_HTTP_RESPONSE_H_INCLUDE_


#include <gvfs_config.h>
#include <gvfs_core.h>


enum gvfs_http_response_code_e{
    HTTP_Continue               = 100,
    HTTP_Ok                     = 200,
    HTTP_Created                = 201,
    HTTP_Accepted               = 202,
    HTTP_BadRequest             = 400,
	HTTP_Unauthorized           = 401,
    HTTP_Forbidden              = 403,
    HTTP_NotFound               = 404,
    HTTP_NotAcceptable          = 406,
    HTTP_InternalServerError    = 500,
    HTTP_NotImplemented         = 501,
    HTTP_ServiceUnavailable     = 503,
    HTTP_VersionNotSupported    = 505,
};


const char *gvfs_http_reason_phrase(gvfs_http_response_code code);
VOID gvfs_http_response_headers(gvfs_buf_t *headers_out,
	gvfs_http_response_code code, size_t content_length);


#endif
