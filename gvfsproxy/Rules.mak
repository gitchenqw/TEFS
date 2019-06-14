CC = gcc
CFLAGS = -pipe -O -Wall -Wpointer-arith -Wno-unused-parameter -Wunused-function -Wunused-variable -Wunused-value -g
LINK = $(CC)

ALL_INCS = -I src/core \
	-I src/event \
	-I src/event/modules \
	-I src/os/unix \
	-I src/http \
	-I src/private \
	-I src/cos
	
CORE_SRCS = src/core/gvfs_array.c \
	src/core/gvfs_queue.c \
	src/core/gvfs_conf_file.c \
	src/core/gvfs_connection.c \
	src/core/gvfs_cycle.c \
	src/core/gvfs_modules.c \
	src/core/gvfs_palloc.c \
	src/core/gvfs_proxy.c \
	src/core/gvfs_shmtx.c \
	src/core/gvfs_string.c \
	src/core/gvfs_log.c \
	src/core/gvfs_file.c \
	src/core/gvfs_rbtree.c \
	src/core/gvfs_utils.c \
	src/core/gvfs_times.c \
	src/core/gvfs_zlib.c\
	src/core/gvfs_aes.c

EVENT_SRCS = src/event/gvfs_event.c \
	src/event/gvfs_event_accept.c \
	src/event/gvfs_event_posted.c \
	src/event/modules/gvfs_epoll_module.c \
	src/event/gvfs_event_timer.c \
	src/event/gvfs_event_connect.c

OS_SRCS = src/os/unix/gvfs_alloc.c \
	src/os/unix/gvfs_channel.c \
	src/os/unix/gvfs_os.c \
	src/os/unix/gvfs_process.c \
	src/os/unix/gvfs_process_cycle.c \
	src/os/unix/gvfs_recv.c \
	src/os/unix/gvfs_send.c \
	src/os/unix/gvfs_setproctitle.c \
	src/os/unix/gvfs_shmem.c \
	src/os/unix/gvfs_socket.c \
	src/os/unix/gvfs_errno.c \
	src/os/unix/gvfs_files.c \
	src/os/unix/gvfs_time.c
	
HTTP_SRCS = src/http/gvfs_http_core_module.c \
          src/http/gvfs_http.c \
          src/http/gvfs_http_parse.c \
          src/http/gvfs_http_parse_cb.c \
          src/http/gvfs_http_request.c \
		  src/http/gvfs_http_request_body.c \
          src/http/gvfs_http_response.c \
          src/http/gvfs_http_buf.c

PRIVATE_SRCS = src/private/gvfs_ds_interface.c \
             src/private/gvfs_ns_interface.c \
             src/private/gvfs_http_query.c \
             src/private/gvfs_http_upload.c \
             src/private/gvfs_http_download.c \
             src/private/gvfs_http_delete.c \
             src/private/gvfs_upstream.c \
             src/private/gvfs_ns_file_query.c \
             src/private/gvfs_ns_file_upload.c \
             src/private/gvfs_ns_file_download.c \
             src/private/gvfs_ns_file_delete.c \
             src/private/gvfs_ds_file_upload.c \
             src/private/gvfs_ds_file_download.c \
			 src/private/gvfs_status.c \
             src/private/gvfs_private.c

COS_SRCS = src/cos/gvfs_cos.c \
           src/cos/gvfs_cos_http_response.c \
		   src/cos/gvfs_cos_http_response_body.c
			