#include <gfs_config.h>
#include <gfs_dbpool.h>

static gfs_mysql_pool_t *mysql_pool;

/* create a connection to the mysql server */
gfs_int32 gfs_connect_to(gfs_mysql_t* conn_node)
{
    MYSQL    *my_conn;
    gfs_char  value = 1;
    
    my_conn = mysql_init(0);
    if (NULL == my_conn) {
    	glog_error("mysql_init(%d) failed", 0);
    	return -1;
    }
    
    /* 为mysql连接设置再连接选项 */
    mysql_options(my_conn, MYSQL_OPT_RECONNECT, (char *) &value);
    
    if(!mysql_real_connect(my_conn, mysql_pool->host, mysql_pool->user,
    	mysql_pool->pwd, mysql_pool->db, (unsigned int)mysql_pool->port, 0,
    	CLIENT_MULTI_RESULTS))
    {
    	glog_error("mysql_real_connect(host:%s, user:%s, pwd:%s, db:%s, port:%u) failed",
    			   mysql_pool->host, mysql_pool->user, mysql_pool->pwd,
    			   mysql_pool->db, mysql_pool->port);
        return -1;
    }
    
    mysql_query(my_conn,"SET NAMES utf8");
    
    conn_node->conn = my_conn;
    conn_node->res = NULL;
    
	glog_debug("mysql_real_connect(host:%s, user:%s, pwd:%s, db:%s, port:%u) success",
			   mysql_pool->host, mysql_pool->user, mysql_pool->pwd,
			   mysql_pool->db, mysql_pool->port);

    
    return 0;
}

gfs_mysql_pool_t* gfs_mysql_pool_create()
{
	gfs_int32           conn = gfs_global->mysql_conn;
	gfs_mysql_t         *conn_node;
	gfs_mysql_pool_t    *sql_pool;
	gfs_mem_pool_t      *pool;
	
    mysql_library_init(0,0,0);

    sql_pool = (gfs_mysql_pool_t*)malloc(sizeof(gfs_mysql_pool_t));
    if(NULL == sql_pool) {
    	glog_error("malloc(%lu) failed, error:%s", sizeof(gfs_mysql_pool_t),
    			   strerror(errno));
    			   
        return NULL;
    }
        
    sql_pool->conn = NULL;
    
    sql_pool->conn_num = 0;
    sql_pool->used_num = 0;
    
    sql_pool->port = gfs_global->mysql_port;
    strcpy(sql_pool->host,gfs_global->mysql_host);
    strcpy(sql_pool->db,  gfs_global->mysql_db_name);
    strcpy(sql_pool->user,gfs_global->mysql_user);
    strcpy(sql_pool->pwd, gfs_global->mysql_pwd);
    
    pool = NULL;
    sql_pool->pool = pool;
    
    mysql_pool =sql_pool;
    
    while(sql_pool->conn_num != conn)
    {

        conn_node = (gfs_mysql_t *)malloc(sizeof(gfs_mysql_t));
        if (NULL == conn_node) {
        	glog_error("malloc(%lu) failed, error:%s", sizeof(gfs_mysql_t),
        			   strerror(errno));
        	return NULL;
        }
        
        if (0 != gfs_connect_to(conn_node)) {
        	return NULL;
        }
        
        conn_node->used = 0; /* unused */
        conn_node->next = sql_pool->conn;
        sql_pool->conn  = conn_node;
        
        sql_pool->conn_num++;
    }
    
    return mysql_pool;
}

gfs_void    gfs_mysql_pool_free()
{
    gfs_mysql_t   *buf;
    while(mysql_pool->conn_num != 0 && mysql_pool->conn != NULL)
    {
        mysql_close(mysql_pool->conn->conn);
        buf = mysql_pool->conn;
        mysql_pool->conn = mysql_pool->conn->next;
        mysql_pool->conn_num--;
        free(buf);
    }
    free(mysql_pool);

    mysql_library_end();
}

gfs_mysql_t* gfs_mysql_get()
{
    gfs_mysql_t* conn_node = NULL;
    
    if(mysql_pool->conn_num != 0 && mysql_pool->conn != NULL) {
        conn_node = mysql_pool->conn;
        mysql_pool->conn = conn_node->next;
        mysql_pool->conn_num--;
        mysql_pool->used_num++;
        mysql_ping(conn_node->conn); /* 检查与服务器连接是否正常,是否有必要重连 */
    }
    else {
    	glog_error("sql connection:%d error", mysql_pool->conn_num);
    }
    
    return conn_node;
}

gfs_int32   gfs_mysql_put(gfs_mysql_t *mysql)
{
    gfs_mysql_t     *conn_node;
    
    if(mysql->used == 1)
    {
        free(mysql);
        conn_node = (gfs_mysql_t*)malloc(sizeof(gfs_mysql_t));
        if(!conn_node)
        {
            glog_error("alloc the memory failed  \n");
            return -1;
        }
        if(gfs_connect_to(conn_node))
        {
            free(conn_node);
            return -1;
        }
    }
    else {
        conn_node = mysql;
    }
    conn_node->res  = NULL;
    conn_node->next = mysql_pool->conn;
    conn_node->used =0;
    mysql_pool->conn  = conn_node;
    mysql_pool->conn_num++;
    mysql_pool->used_num--;
    mysql = NULL;
    return 0;
}

gfs_int32   gfs_mysql_pool_current_num()
{
    return mysql_pool->conn_num;
}
/*
 * 
 * 开CLIENT_MULTI_STATEMENTS:
 *              insert加 ok，delete加 FALSE  (调用顺序错误)
                insert不加ok，delete不加 false (语法错误）

 * 开CLIENT_MULTI_RESULTS：  
                delete加 ok ，不加 false

 * 没有开
 *              insert的不加ok，delete不加false，都加上都ok
 * 
 * 
 * PS:insert: proc_insert_app()
 *    delete: proc_del_app()
 * 
 * */
