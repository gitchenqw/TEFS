
#ifndef _GVFS_CORE_H_INCLUDED_
#define _GVFS_CORE_H_INCLUDED_

typedef struct gvfs_module_s      gvfs_module_t;
typedef struct gvfs_command_s     gvfs_command_t;
typedef struct gvfs_conf_s        gvfs_conf_t;
typedef struct gvfs_cycle_s       gvfs_cycle_t;
typedef struct gvfs_pool_s        gvfs_pool_t;
typedef struct gvfs_array_s       gvfs_array_t;
typedef struct gvfs_event_s       gvfs_event_t;
typedef struct gvfs_file_s        gvfs_file_t;
typedef struct gvfs_connection_s  gvfs_connection_t;

typedef void (*gvfs_event_handler_pt)(gvfs_event_t *ev);
typedef void (*gvfs_connection_handler_pt)(gvfs_connection_t *c);

#define  GVFS_OK          0
#define  GVFS_ERROR      -1
#define  GVFS_AGAIN      -2
#define  GVFS_BUSY       -3
#define  GVFS_DONE       -4
#define  GVFS_DECLINED   -5
#define  GVFS_ABORT      -6

#include <gvfs_base_type.h>
#include <gvfs_modules.h>
#include <gvfs_aes.h>
#include <gvfs_zlib.h>
#include <gvfs_dlist.h>
#include <gvfs_string.h>
#include <gvfs_alloc.h>
#include <gvfs_palloc.h>
#include <gvfs_array.h>
#include <gvfs_queue.h>
#include <gvfs_conf_file.h>
#include <gvfs_cycle.h>
#include <gvfs_socket.h>
#include <gvfs_os.h>
#include <gvfs_time.h>
#include <gvfs_times.h>
#include <gvfs_process.h>
#include <gvfs_process_cycle.h>
#include <gvfs_core.h>
#include <gvfs_errno.h>
#include <gvfs_log.h>
#include <gvfs_atomic.h>
#include <gvfs_shmtx.h>
#include <gvfs_shmem.h>
#include <gvfs_rbtree.h>
#include <gvfs_event.h>
#include <gvfs_event_timer.h>
#include <gvfs_file.h>
#include <gvfs_files.h>
#include <gvfs_utils.h>
#include <gvfs_connection.h>
#include <gvfs_event_posted.h>
#include <gvfs_event_connect.h>
#include <gvfs_http_config.h>
#include <gvfs_http_core_module.h>
#include <gvfs_http.h>
#include <gvfs_http_buf.h>
#include <gvfs_http_response.h>
#include <gvfs_http_parse.h>

#include <gvfs_private.h>
#include <gvfs_status.h>

#include <gvfs_cos.h>
#include <gvfs_cos_http_response.h>

#include <gvfs_http_request.h>
#include <gvfs_http_parse_cb.h>

#define LF     (u_char) 10
#define CR     (u_char) 13
#define CRLF   "\x0d\x0a"

#define gvfs_abs(value)       (((value) >= 0) ? (value) : - (value))
#define gvfs_max(val1, val2)  ((val1 < val2) ? (val2) : (val1))
#define gvfs_min(val1, val2)  ((val1 > val2) ? (val2) : (val1))

#define TRUE  1
#define FALSE 0

#endif /* _GVFS_CORE_H_INCLUDED_ */
