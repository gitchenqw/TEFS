

#ifndef _GVFS_CONNECTION_H_INCLUDE_
#define _GVFS_CONNECTION_H_INCLUDE_

typedef struct gvfs_listening_s gvfs_listening_t;

/* 该结构在gvfs_create_listening中进行初始化 */
struct gvfs_listening_s
{
	gvfs_socket_t       fd;              /* 服务器监听描述符          */

	struct sockaddr    *sockaddr;        /* 服务器监听的套接字地址     */
	socklen_t			socklen;         /* 服务器监听的套接字地址长度 */
	gvfs_str_t          addr_text;       /* 服务器监听IP的字符串形式   */

	int 				type;
	int 				backlog;         /* 监听队列的大小   */
	int 				rcvbuf;          /* 接收缓冲区的大小 */
	int 				sndbuf;          /* 发送缓冲区的大小 */

	gvfs_connection_handler_pt	handler; /* 定义accept之后的处理函数: gvfs_http_init_connection */

	size_t              pool_size;       /* accpet成功之后为每个连接分配的内存池大小 */

	gvfs_connection_t  *connection; 

    unsigned            listen:1;        /* 标志位: 已经调用过listen函数 */
};

struct gvfs_connection_s {
	gvfs_pool_t        *pool;        /* 预先为每个连接分配的内存池,accept成功后分配 */ 
    void               *data;        /* 两个用途:未分配之前指向自己,构成链表,分配后指向gvfs_http_request_t */
    gvfs_event_t       *read;
    gvfs_event_t       *write;

    gvfs_socket_t       fd;          /* accept返回的已连接fd,在gvfs_get_connection中赋值 */

    gvfs_recv_pt        recv;        /* 收包函数指针 */
    gvfs_send_pt        send;        /* 发包函数指针 */
    off_t               sent;        /* 已发送字节数 */
    gvfs_listening_t   *listening;   /* 该链接对应的监听地址结构 */
    
	gvfs_str_t          addr_text;   /* 客户端地址字符串形式 */
};

gvfs_listening_t *gvfs_create_listening(gvfs_cycle_t *cycle);
gvfs_int_t gvfs_open_listening_sockets(gvfs_cycle_t *cycle);
void gvfs_configure_listening_sockets(gvfs_cycle_t *cycle);
void gvfs_close_listening_sockets(gvfs_cycle_t *cycle);

gvfs_connection_t *gvfs_get_connection(gvfs_socket_t s);
void gvfs_free_connection(gvfs_connection_t *c);
void gvfs_close_connection(gvfs_connection_t *c);
gvfs_int_t gvfs_connection_error(gvfs_err_t err);


#endif
