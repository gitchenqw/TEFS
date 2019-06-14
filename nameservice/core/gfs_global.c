#include <gfs_config.h>
#include <gfs_global.h>
#include <gfs_log.h>
#include <gfs_mem.h>

gfs_global_t     *gfs_global = NULL;
gfs_int32         gfs_errno = 0;

#ifndef GFS_CONFIG
static gfs_char *conf_path = "../cfg/gfs.cfg";
#else
static gfs_char *conf_path = "/etc/gfs.cfg";
#endif


static gfs_int32 gfs_global_getcfg(gfs_global_t *global)
{
    FILE        *fp;
    gfs_char     buf[256];
    gfs_int32    isnewline;
    gfs_char    *pstr, *pch;

    fp = fopen(conf_path, "rt");
    if(fp == NULL)
    {
    	glog_error("fopen(\"%s\", \"rt\") failed, error:%s", conf_path,
    			   strerror(errno));
        return GFS_FOPEN_ERR;
    }

    isnewline = gfs_true;
    while(1)
    {
        if(fgets(buf, 256, fp) == NULL)
        {
            if(ferror(fp))
                glog_warn("read config file failed. errno code is %d\n", errno);
            break;
        }

        if(!isnewline)
        {
            if(buf[strlen(buf) - 1] == '\n')
                isnewline = gfs_true;
            continue;
        }
        else if (strlen(buf) == 255 && buf[254] != '\n')
        {
            isnewline = gfs_false;
            continue;
        }
        

        pstr = strtok(buf, " \t\n");

        if(!pstr || pstr[0] == '#')
            continue;
        else if(strcasecmp(pstr, "bind_ip") == 0)
        {
            pstr = strtok(NULL, " \t\n");
            pch = strchr(pstr, '#');
            if(pch)
                *pch = '\0';
            if(strlen(pstr) > 15)
            {
                glog_error("config remote_ip is error!");
                return GFS_PARAM_CFG_ERR;
            }
            strcpy(global->bind_ip, pstr);

            glog_debug("bind_ip: %s", global->bind_ip);
        }
        else if(strcasecmp(pstr, "tcp_port") == 0)
        {
            pstr = strtok(NULL, " \t\n"); 
            pch = strchr(pstr, '#');
            if(pch)
                *pch = '\0';
            global->tport = atoi(pstr);
            glog_debug("tcp_port: %u", global->tport);
        }
        else if(strcasecmp(pstr, "timeout") == 0)
        {
            pstr = strtok(NULL, " \t\n"); 
            pch = strchr(pstr, '#');
            if(pch)
                *pch = '\0';
            global->timeout = atoi(pstr);
            glog_debug("timeout: %u", global->timeout);
        }
        else if(strcasecmp(pstr, "max_num") == 0)
        {
            pstr = strtok(NULL, " \t\n"); 
            pch = strchr(pstr, '#');
            if(pch)
                *pch = '\0';
            global->max_num = atoi(pstr);
            glog_debug("max_num: %d", global->max_num);
        }
        else if(strcasecmp(pstr, "copies") == 0)
        {
            pstr = strtok(NULL, " \t\n"); 
            pch = strchr(pstr, '#');
            if(pch)
                *pch = '\0';
            global->copies = atoi(pstr);
            glog_debug("copies: %u", global->copies);
        }
        else if(strcasecmp(pstr, "block_size") == 0)
        {
            pstr = strtok(NULL, " \t\n"); 
            pch = strchr(pstr, '#');
            if(pch)
                *pch = '\0';
            global->block_size = atoi(pstr);
            glog_debug("copies: %u", global->block_size);
        }
        else if(strcasecmp(pstr, "log_level") == 0)
        {
            pstr = strtok(NULL, " \t\n"); 
            pch = strchr(pstr, '#');
            if(pch)
                *pch = '\0';
            if(strcasecmp(pstr, "DEBUG") == 0)
                global->log_level = DEBUG;
            else if(strcasecmp(pstr, "INFO") == 0)
                global->log_level = INFO;
            else if(strcasecmp(pstr, "WARN") == 0)
                global->log_level = WARN;
            else if(strcasecmp(pstr, "ERROR") == 0)
                global->log_level = ERROR;
            glog_debug("log_level: %d", global->log_level);
        }
        else if(strcasecmp(pstr, "mysql_conn") == 0)
        {
            pstr = strtok(NULL, " \t\n"); 
            pch = strchr(pstr, '#');
            if(pch)
                *pch = '\0';
            global->mysql_conn = atoi(pstr);
            glog_debug("mysql_conn: %d", global->mysql_conn);
        }
        else if(strcasecmp(pstr, "mysql_host") == 0)
        {
            pstr = strtok(NULL, " \t\n"); 
            pch = strchr(pstr, '#');
            if(pch)
                *pch = '\0';
            strcpy(global->mysql_host, pstr);
            glog_debug("mysql_host: %s", global->mysql_host);
        }
        else if(strcasecmp(pstr, "mysql_port") == 0)
        {
            pstr = strtok(NULL, " \t\n"); 
            pch = strchr(pstr, '#');
            if(pch)
                *pch = '\0';
            global->mysql_port = atoi(pstr);
            glog_debug("mysql_port: %u", global->mysql_port);
        }
        else if(strcasecmp(pstr, "mysql_db") == 0)
        {
            pstr = strtok(NULL, " \t\n"); 
            pch = strchr(pstr, '#');
            if(pch)
                *pch = '\0';
            strcpy(global->mysql_db_name, pstr);
            glog_debug("mysql_db: %s", global->mysql_db_name);
        }
        else if(strcasecmp(pstr, "mysql_user") == 0)
        {
            pstr = strtok(NULL, " \t\n"); 
            pch = strchr(pstr, '#');
            if(pch)
                *pch = '\0';
            strcpy(global->mysql_user, pstr);
            glog_debug("mysql_user: %s", global->mysql_user);
        }
        else if(strcasecmp(pstr, "mysql_pwd") == 0)
        {
            pstr = strtok(NULL, " \t\n"); 
            pch = strchr(pstr, '#');
            if(pch)
                *pch = '\0';
            strcpy(global->mysql_pwd, pstr);
            glog_debug("mysql_pwd: %s", global->mysql_pwd);
        }
    }

    fclose(fp);
    glog_debug("parse conf:%s success", conf_path);

    return GFS_OK;
}

gfs_int32   gfs_global_init()
{
    gfs_global_t    *global;
    gfs_mem_pool_t  *pool;
    gfs_int32        rtn;

    pool = gfs_mem_pool_create(10);
    if(!pool)
    { 
        glog_error("gfs_mem_pool_create(%d) failed", 10);
        exit(GFS_MEMPOOL_CREATE_ERR);
    }

    global = (gfs_global_t*)gfs_mem_malloc(pool, sizeof(gfs_global_t));
    if(!global)
    {
        glog_error("gfs_mem_malloc(%p, %lu) failed", pool,
        		   sizeof(gfs_global_t));
        exit(GFS_MEMPOOL_MALLOC_ERR);
    }

    gfs_global = global;

    global->pool = pool;
    strcpy(global->bind_ip, "127.0.0.1");
    global->tport = 8082;
    global->timeout = 120;
    global->max_num = 1000;
    global->client_num = 0;
    global->copies = 3;
    global->block_size = 67108864;
    
    global->mysql_conn = 0;
    global->mysql_host[0] = '\0';
    global->mysql_port = 0;
    global->mysql_db_name[0] = '\0';
    global->mysql_user[0] = '\0';
    global->mysql_pwd[0] = '\0';
    
    global->log_level = INFO;
    
    global->eloop = NULL;
    global->nloop = NULL;
    global->tcpser = NULL;

    rtn = gfs_global_getcfg(global);
    if (rtn != GFS_OK)
    {
    	glog_error("gfs_global_getcfg(%p) failed", global);	
        return GFS_GLOBAL_GETCFG_ERR;
    }

    glog_debug("%s", "gfs_global_init() success");

    return GFS_OK;
}

gfs_void    gfs_global_destory()
{
    gfs_mem_pool_t      *pool;

    pool = NULL;
    if (gfs_global)
    {
        pool = gfs_global->pool;
        gfs_mem_free(gfs_global);
        gfs_global = NULL;
    }

    if (pool)
    {
    	glog_info("gfs_global_destory(%p) success", pool);
        gfs_mem_pool_destory(pool);
    }
}

gfs_int32   gfs_global_reload_cfg()
{
    gfs_int32       rtn;

    rtn = gfs_global_getcfg(gfs_global);
    if (rtn != GFS_OK)
    {
        glog_warn("reload configuration file failed.");
        return GFS_GLOBAL_GETCFG_ERR;
    }

    glog_info("reload configuration file success.");
    return GFS_OK;
}
