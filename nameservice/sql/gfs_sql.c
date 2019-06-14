#include <gfs_sql.h>

static gfs_int32 cpy_pages = 0;
static gfs_int32 del0_pages = 0;
static gfs_int32 del1_pages = 0;

gfs_int32 gfs_mysql_init()
{
	if(NULL == gfs_mysql_pool_create()) {
		glog_error("%s", "gfs_mysql_pool_create() failed");
		return SQL_POOL_ERR;
	}

	glog_debug("%s", "gfs_mysql_init() success");
	return SQL_OK;
}

gfs_int32   gfs_mysql_free()
{
    gfs_mysql_pool_free();
    return SQL_OK;
}

gfs_void    gfs_mysql_free_result(gfs_mysql_t* sql_conn)
{
    mysql_free_result(sql_conn->res);
    while(!mysql_next_result(sql_conn->conn));
    {
        sql_conn->res = mysql_store_result(sql_conn->conn);
        mysql_free_result(sql_conn->res); 
    }
}

gfs_int32 gfs_mysql_login(gfs_uint32 dsid, gfs_uint64 dsize,
	gfs_uint64 remain)
{
    gfs_mysql_t*    sql_conn;
    gfs_char        query[64];
    gfs_int32       rtn = SQL_OK;
    my_ulonglong rc;

    sql_conn = gfs_mysql_get();
    if(NULL == sql_conn) {
        glog_error("%s", "gfs_mysql_get() failed");
        return SQL_POOL_ERR;
    }

    sprintf(query, "call ds_login(%u, %llu, %llu)", dsid, dsize, remain);

    /* mysql_query 返回0代表成功 */
    if(mysql_query(sql_conn->conn, query))
    {
    	glog_error("mysql_query(\"%s\") failed, error:%s", query,
    			   mysql_error(sql_conn->conn));
    			   
        if(mysql_errno(sql_conn->conn) == CR_SERVER_LOST) {
            sql_conn->used = 1;
        }
        
		gfs_mysql_put(sql_conn);
		return SQL_QUERY_ERR;
    }

	rc = mysql_affected_rows(sql_conn->conn);
    if(0 == rc) {
    	glog_info("sql:%s no affected rows", query);
        rtn = SQL_INSERT_EXISTS;
    } else if (-1 == rc) {
    	glog_info("mysql_affected_rows(%s) failed, error:%s",
    			  query, mysql_error(sql_conn->conn));
    	rtn = SQL_QUERY_ERR;
    }

	glog_info("sql:%s success", query);
	
    gfs_mysql_put(sql_conn);

    return rtn;
}

gfs_int32   gfs_mysql_upload(gfs_char *fhash, gfs_uint64 fsize, gfs_uint32 bnum, gfs_int32 bindex, gfs_int32 *status, gfs_uint64 *fid)
{
    gfs_mysql_t*    sql_conn;
    gfs_char        query[128];
    MYSQL_ROW       row;
    gfs_int32       rtn;

    *status = -1;

    sql_conn = gfs_mysql_get();
    if(NULL == sql_conn) {
    	glog_error("%s", "gfs_mysql_get() failed");
        return SQL_POOL_ERR;
    }

    sprintf(query, "call upload_file('%s', %llu, %u, %d)", fhash, fsize, bnum, bindex);
    
    if(mysql_query(sql_conn->conn,query))
    {
    	glog_error("mysql_query(\"%s\") failed, error:%s", query,
    			   mysql_error(sql_conn->conn));
    			   
        if(mysql_errno(sql_conn->conn) == CR_SERVER_LOST) {
            sql_conn->used = 1;
        }

		gfs_mysql_put(sql_conn);
        return SQL_ARGS_ERR;
    }

    sql_conn->res = mysql_store_result(sql_conn->conn);
    if(sql_conn->res) { // there are rows
        row = mysql_fetch_row(sql_conn->res);
        if (row)
        {
            *status = atoi(row[0]);
            *fid = atoll(row[1]);

			glog_info("sql:%s success, status:%d, fid:%llu", query, *status, *fid);
            rtn = SQL_OK;
        }
        else if(!mysql_errno(sql_conn->conn))
        {
        	glog_info("sql:%s empty", query);
            rtn = SQL_EMPTY;
        }
    }
    else { // returned nothing; should it hava?
    	
    	glog_error("sql:%s failed, error:%s", query, mysql_error(sql_conn->conn));
        rtn = SQL_FETCH_ERR;
    }
    
    gfs_mysql_free_result(sql_conn);
    gfs_mysql_put(sql_conn);

    return rtn;
}

gfs_int32 gfs_mysql_delfile(gfs_char *fhash)
{
    gfs_mysql_t*    sql_conn;
    gfs_char        query[64];
    gfs_int32       rtn = SQL_OK;

    sql_conn = gfs_mysql_get();
    if(NULL == sql_conn) {
    	glog_error("%s", "gfs_mysql_get() failed");
        return SQL_POOL_ERR;
    }

    sprintf(query, "call delete_file('%s')", fhash);
    
    if(mysql_query(sql_conn->conn,query))
    {
    	glog_error("mysql_query(\"%s\") failed, error:%s", query,
    			   mysql_error(sql_conn->conn));
    
        if(mysql_errno(sql_conn->conn) == CR_SERVER_LOST) {
            sql_conn->used = 1;
        }

        gfs_mysql_put(sql_conn);
        return SQL_ARGS_ERR;
    }

    if(!mysql_affected_rows(sql_conn->conn))
    {
    	glog_info("sql:%s no affected rows", query);
        rtn = SQL_FILE_NOT_EXIST;
    }

	glog_info("sql:%s success", query);
	
    gfs_mysql_put(sql_conn);

    return rtn;
}

gfs_int32 gfs_mysql_delblock(gfs_uint64 fid, gfs_uint32 bindex, gfs_uint32 dsid)
{
    gfs_mysql_t*    sql_conn;
    gfs_char        query[64];
    gfs_int32       rtn = SQL_OK;

    sql_conn = gfs_mysql_get();
    if(NULL == sql_conn) {
    	glog_error("%s", "gfs_mysql_get() failed");
        return SQL_POOL_ERR;
    }

    sprintf(query, "call delete_block(%llu, %d, %u)", fid, bindex, dsid);
    
    if(mysql_query(sql_conn->conn,query))
    {
    	glog_error("mysql_query(\"%s\") failed, error:%s", query,
    			   mysql_error(sql_conn->conn));
    			   
        if(mysql_errno(sql_conn->conn) == CR_SERVER_LOST) {
            sql_conn->used = 1;
        }

        gfs_mysql_put(sql_conn);
        return SQL_ARGS_ERR;
    }

	/* 由于存储过程中有多条SQL,所以这里的返回值不准确 */
    if(!mysql_affected_rows(sql_conn->conn)) {
    	glog_info("sql:%s no affected rows", query);

    	/* { */
    	// rtn = xxxx;
    	/* } */
    }
    
	glog_info("sql:%s success", query);

    gfs_mysql_put(sql_conn);

    return rtn;
}

gfs_int32   gfs_mysql_cpyblock(gfs_uint64 fid, gfs_uint32 bindex, gfs_uint32 dsid)
{
    gfs_mysql_t*    sql_conn;
    gfs_char        query[64];
    gfs_int32       rtn = SQL_OK;

    sql_conn = gfs_mysql_get();
    if(NULL == sql_conn) {
    	glog_error("%s", "gfs_mysql_get() failed");
        return SQL_POOL_ERR;
    }

    sprintf(query, "call copy_block(%llu, %d, %u)", fid, bindex, dsid);
    
    if(mysql_query(sql_conn->conn,query))
    {
    	glog_error("mysql_query(\"%s\") failed, error:%s", query,
    			   mysql_error(sql_conn->conn));
    			   
        if(mysql_errno(sql_conn->conn) == CR_SERVER_LOST) {
            sql_conn->used = 1;
        }

        gfs_mysql_put(sql_conn);
        return SQL_ARGS_ERR;
    }

    if(!mysql_affected_rows(sql_conn->conn))
    {
    	glog_info("sql:%s no affected rows", query);
        rtn = SQL_FILE_NOT_EXIST;
    }
    
	glog_info("sql:%s success", query);

    gfs_mysql_put(sql_conn);

    return rtn;
}

gfs_int32   gfs_mysql_download(gfs_mem_pool_t *pool, gfs_char* fhash, gfs_uint32 bindex, gfs_uint64 *fid, gfs_sql_block_t **blist, gfs_uint32 *bnum)
{
    gfs_mysql_t*    sql_conn;
    gfs_char        query[64];
    MYSQL_ROW       row;
    gfs_int32       res_index, num, i;
    gfs_int32       status, rtn = SQL_ERROR;
    gfs_sql_block_t *pdata;

    res_index = 0;
    i = num = 0;
    *bnum = 0;
    *blist = NULL;

    sql_conn = gfs_mysql_get();
    if(NULL == sql_conn) {
    	glog_error("%s", "gfs_mysql_get() failed");
        return SQL_POOL_ERR;
    }

    sprintf(query, "call download_file('%s', %d)", fhash, bindex);

	glog_info("download exec proc: call download_file('%s', %d)",
			  fhash, bindex);
    
    if(mysql_query(sql_conn->conn,query))
    {
    	glog_error("mysql_query(\"%s\") failed, error:%s", query,
    			   mysql_error(sql_conn->conn));
    			   
        if(mysql_errno(sql_conn->conn) == CR_SERVER_LOST) {
            sql_conn->used = 1;
        }

        gfs_mysql_put(sql_conn);
        return SQL_ARGS_ERR;
    }

    do
    { /* did current statement return data? */
        sql_conn->res = mysql_store_result(sql_conn->conn);
        if (sql_conn->res)
        { /* yes; process rows and free the result set */
            if (res_index < 1)
            {
                res_index++;
                row = mysql_fetch_row(sql_conn->res);
                if (row)
                {
                    *fid = atoll(row[0]);
                    rtn = SQL_OK;
                }
                else if(!mysql_errno(sql_conn->conn))
                {
                    rtn = SQL_EMPTY;
                    glog_info("<mysql download> file %s not exist.", fhash);
                    //mysql_free_result(sql_conn->res);
                    break;
                }
            }
            else if (res_index >= 1)
            {
                res_index++;
                num = mysql_num_rows(sql_conn->res);
                if (num == 0)
                {
                    //mysql_free_result(sql_conn->res);
                    break;
                }
                else
                {
                    pdata = (gfs_sql_block_t*)gfs_mem_malloc(pool, num * sizeof(gfs_sql_block_t));
                    if (pdata == NULL)
                    {
                    	glog_error("gfs_mem_malloc(%p, %lu) failed", pool,
                    			   num * sizeof(gfs_sql_block_t));
                        //mysql_free_result(sql_conn->res);
                        break;
                    }
                    *bnum = num;
                    *blist = pdata;
                }
                while(i < num)
                {
                    row = mysql_fetch_row(sql_conn->res);
                    if (row)
                    {
                        pdata[i].bindex = atoi(row[0]);
                        pdata[i].dsid = atoi(row[1]);
                        i++;
                        continue;
                    }
                    else if(!mysql_errno(sql_conn->conn))
                    {
                    	glog_info("sql:%s no fetch row", query);
                        rtn = SQL_EMPTY;
                        break;
                    }
                }
            }
            //mysql_free_result(sql_conn->res);
        }
        else /* no result set or error */
        {
            if (mysql_field_count(sql_conn->conn) == 0)
                glog_warn("<mysql download> %lld rows affected.", mysql_affected_rows(sql_conn->conn));
            else /* some error occurred */
            {
                glog_warn("<mysql download> Could not retrieve result set.");
                //mysql_free_result(sql_conn->res);
                break;
            }
            //mysql_free_result(sql_conn->res);
        }
        /* more results? -1 = no, >0 = error, 0 = yes (keep looping) */
        if ((status = mysql_next_result(sql_conn->conn)) > 0) 
            glog_error("<mysql download> status [%d] error no [%d] error[%s] info[%s]\n",status,mysql_errno(sql_conn->conn),mysql_error(sql_conn->conn),mysql_info(sql_conn->conn));
    } while (status == 0); 

    gfs_mysql_free_result(sql_conn);
    gfs_mysql_put(sql_conn);

    return rtn;
}

gfs_int32   gfs_mysql_get_delblock(gfs_int32 opt, gfs_uint32 copies, gfs_sql_block_t *blist, gfs_uint32 *bnum)
{
    gfs_mysql_t*    sql_conn;
    gfs_char        query[64];
    MYSQL_ROW       row;
    gfs_int32       res_index, num, i;
    gfs_int32       rtn = SQL_OK;
    gfs_int32      *pages;

    res_index = 0;
    i = num = 0;
    *bnum = 0;

    sql_conn = gfs_mysql_get();
    if(NULL == sql_conn) {
    	glog_notice("%s", "gfs_mysql_get() failed");
        return SQL_POOL_ERR;
    }

    if (opt == 0) {
        pages = &del0_pages; /* 查询状态标记为删除的块 */
    }
    else if (opt == 1) {
        pages = &del1_pages; /* 查询备份数大于copies的块 */
    }

    sprintf(query, "call get_del_block(%d, %d, %d)", opt, copies, *pages);

	glog_notice("sql:%s opt:%d copies:%u pages:%d", query, opt, copies, *pages);
    
    if(0 != mysql_query(sql_conn->conn,query)) {
    
    	glog_notice("mysql_query(\"%s\") failed, error:%s", query,
    			   mysql_error(sql_conn->conn));

		/* 到服务器的连接在查询期间丢失 */
        if(mysql_errno(sql_conn->conn) == CR_SERVER_LOST) {
            sql_conn->used = 1; /* 该标志用于重新创建到MySQL服务器的连接 */
        }

        gfs_mysql_put(sql_conn);
        
        return SQL_ARGS_ERR;
    }

    /* 没有行返回,返回一个空集合, 空集合 != NULL, 返回NULL表示出现了错误 */
    sql_conn->res = mysql_store_result(sql_conn->conn);
    if(sql_conn->res) {
    
        num = mysql_num_rows(sql_conn->res); /* 返回一个结果集中行数 */
        if (num == 0) { /* pages用于实现类似分页查询的功能 */
            *pages = 0;
        }
        else {
            *bnum = num;
            (*pages)++;
        }

        while(i < num)
        {
            row = mysql_fetch_row(sql_conn->res); /* 从结果集合中取下一行 */
            if (row) {
                blist[i].fid = atoi(row[0]);
                blist[i].bindex = atoi(row[1]);
                blist[i].dsid = atoi(row[2]);
                i++;
                continue;
            }
            /* 返回NULL说明已经结果集的尾部 */
            else if(0 == mysql_errno(sql_conn->conn)) {
				glog_notice("sql:%s empty", query);
                rtn = SQL_EMPTY;
                break;
            }
        }

    }
    else { /* 1:malloc失败,如结果集太大;2:数据不能被读取;3:查询没有返回数据 */
    	glog_notice("sql:%s failed, error:%s", query, mysql_error(sql_conn->conn));
        rtn = SQL_FETCH_ERR;
    }

    glog_notice("sql:%s success", query);

    gfs_mysql_free_result(sql_conn);
    gfs_mysql_put(sql_conn);

    return rtn;
}

gfs_int32   gfs_mysql_get_cpyblock(gfs_uint32 copies, gfs_sql_block_t *blist, gfs_uint32 *bnum)
{
    gfs_mysql_t*    sql_conn;
    gfs_char        query[64];
    MYSQL_ROW       row;
    gfs_int32       res_index, num, i;
    gfs_int32       rtn = SQL_OK;

    res_index = 0;
    i = num = 0;
    *bnum = 0;

    sql_conn = gfs_mysql_get();
    if(NULL == sql_conn) {
    	glog_notice("%s", "gfs_mysql_get() failed");
        return SQL_POOL_ERR;
    }

    sprintf(query, "call get_cpy_block(%d, %d)", copies, cpy_pages);

    glog_notice("sql:%s, copies:%u", query, copies);
    
    if(mysql_query(sql_conn->conn,query))
    {
    	glog_notice("mysql_query(\"%s\") failed, error:%s", query,
    			    mysql_error(sql_conn->conn));
    			   
        if(mysql_errno(sql_conn->conn) == CR_SERVER_LOST) {
            sql_conn->used = 1;
        }

        gfs_mysql_put(sql_conn);
        return SQL_ARGS_ERR;
    }
    
    sql_conn->res = mysql_store_result(sql_conn->conn);
    if(sql_conn->res)
    {
        num = mysql_num_rows(sql_conn->res);
        if (num == 0) {
            cpy_pages = 0;
        }
        else {
            *bnum = num;
            cpy_pages++;
        }

        while(i < num)
        {
            row = mysql_fetch_row(sql_conn->res);
            if (row) {
                blist[i].fid = atoi(row[0]);
                blist[i].bindex = atoi(row[1]);
                blist[i].dsid = atoi(row[2]);
                i++;
                continue;
            }
            else if(!mysql_errno(sql_conn->conn)) {
				glog_notice("sql:%s empty", query);
                rtn = SQL_EMPTY;
                break;
            }
        }
    }
    else {
    	glog_notice("sql:%s failed, error:%s", query, mysql_error(sql_conn->conn));
        rtn = SQL_FETCH_ERR;
    }

	glog_notice("sql:%s success", query);
	
    gfs_mysql_free_result(sql_conn);
    gfs_mysql_put(sql_conn);

    return rtn;
}

gfs_int32   gfs_mysql_getfsize(gfs_char *fhash, gfs_uint64 *fsize)
{
    gfs_mysql_t*    sql_conn;
    gfs_char        query[128];
    MYSQL_ROW       row;
    gfs_int32       rtn;

    sql_conn = gfs_mysql_get();
    if(NULL == sql_conn) {
    	glog_error("%s", "gfs_mysql_get() failed");
        return SQL_POOL_ERR;
    }

    sprintf(query, "call get_fsize('%s')", fhash);
    
    if(mysql_query(sql_conn->conn,query))
    {
    	glog_error("mysql_query(\"%s\") failed, error:%s", query,
    			   mysql_error(sql_conn->conn));
    			   
        if(mysql_errno(sql_conn->conn) == CR_SERVER_LOST) {
            sql_conn->used = 1;
        }

        gfs_mysql_put(sql_conn);
        return SQL_ARGS_ERR;
    }

    sql_conn->res = mysql_store_result(sql_conn->conn);
    if(sql_conn->res)
    {
        row = mysql_fetch_row(sql_conn->res);
        if (row)
        {
            *fsize = atoi(row[0]);

			glog_info("sql:%s success", query);
            rtn = SQL_OK;
        }
        else if(!mysql_errno(sql_conn->conn))
        {
        	glog_info("sql:%s empty", query);
            rtn = SQL_EMPTY;
        }
    }
    else
    {
    	glog_error("sql:%s failed, error:%s", query, mysql_error(sql_conn->conn));
        rtn = SQL_FETCH_ERR;
    }
    
    gfs_mysql_free_result(sql_conn);
    gfs_mysql_put(sql_conn);

    return rtn;
}

gfs_int32   gfs_mysql_set_deling(gfs_uint64 fid, gfs_uint32 bindex, gfs_uint32 dsid)
{
    gfs_mysql_t*    sql_conn;
    gfs_char        query[64];
    gfs_int32       rtn = SQL_OK;

    sql_conn = gfs_mysql_get();
    if(NULL == sql_conn) {
    	glog_error("%s", "gfs_mysql_get() failed");
        return SQL_POOL_ERR;
    }

    sprintf(query, "call set_del_ing(%llu, %d, %u)", fid, bindex, dsid);
    
    if(mysql_query(sql_conn->conn,query))
    {
    	glog_error("mysql_query(\"%s\") failed, error:%s", query,
    			   mysql_error(sql_conn->conn));
    			   
        if(mysql_errno(sql_conn->conn) == CR_SERVER_LOST) {
            sql_conn->used = 1;
        }

        gfs_mysql_put(sql_conn);
        return SQL_ARGS_ERR;
    }

    if(!mysql_affected_rows(sql_conn->conn))
    {
    	glog_info("sql:%s no affected rows", query);
        rtn = SQL_BLOCK_NOT_EXIST;
    }
    
	glog_info("sql:%s success", query);

    gfs_mysql_put(sql_conn);

    return rtn;
}
