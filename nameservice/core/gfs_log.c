#include <gfs_config.h>
#include <gfs_global.h>
#include <gfs_mem.h>
#include <gfs_log.h>

/* { add by chenqianwu */

zlog_category_t *zc;

int gfs_init_log(const char *zlog_conf_file, const char *name)
{
	int              zr;
	zr = zlog_init(zlog_conf_file);
	if (zr) {
		printf("zlog_init(\"%s\") failed\n", zlog_conf_file);
		return GFS_OK;
	}

	zc = zlog_get_category(name);
	if (!zc) {
		printf("zlog_get_category(\"%s\") failed\n", name);
		zlog_fini();
		return GFS_ERR;
	}

	return GFS_OK;
}

/* } */

static gfs_char     buffer[2048];
static FILE*        logfp = NULL;
static gfs_mem_pool_t   *pool = NULL;

gfs_int32 gfs_log_init()
{
    FILE*       fp;

    fp = fopen("run.log", "at");
    if(fp == NULL)
    {
        printf("fopen log file failed! error code is %d\n", errno);
        return GFS_FOPEN_ERR;
    }
    logfp = fp;

    return GFS_OK;
}

gfs_void  gfs_log_destory()
{
    if(logfp)
    {
        fclose(logfp);
        logfp = NULL;
    }
}

static gfs_char* getstimeval()
{
    struct timeval  tv;
    struct tm       tm;
    gfs_int32       len;
    static gfs_char buf[32];

    gettimeofday(&tv, NULL);
    localtime_r(&tv.tv_sec, &tm);
    len = strftime(buf, 32, "%Y-%m-%d %H:%M:%S", &tm);
    sprintf(buf + len, ".%03d", (int)(tv.tv_usec/1000));

    return buf;
}

gfs_int32 gfs_log_real(gfs_int32 level, const gfs_char* format, ...)
{
    gfs_char        *pbuf;
    gfs_int32        len, res;
    va_list          arg_list;
    gfs_uptr         pas, pus;  //test by zmy

    if(logfp == NULL)
    {
        printf("log module failed to initialize or not initialized\n");
        return GFS_LOG_INIT_ERR;
    }

    if (gfs_global && level <gfs_global->log_level)
        return GFS_OK;

    pas = pus = 0;
    if (gfs_global)
        pool = gfs_global->pool;

    pas = pool ? pool->size : 0;        //test by zmy
    pus = pool ? pool->used : 0;        //test by zmy

    pbuf = buffer;
    switch(level)
    {
        case DEBUG:
            len = sprintf(pbuf, "[DEBUG][%s] <%d> <ps:%lu:%lu mem:%lu> ", getstimeval(), getpid(), pas, pus, gfs_mem_allsize);
            break;
        case INFO:
            len = sprintf(pbuf, "[INFO ][%s] <%d> <ps:%lu:%lu mem:%lu> ", getstimeval(), getpid(), pas, pus, gfs_mem_allsize);
            break;
        case WARN:
            len = sprintf(pbuf, "[WARN ][%s] <%d> <ps:%lu:%lu mem:%lu> ", getstimeval(), getpid(), pas, pus, gfs_mem_allsize);
            break;
        case ERROR:
            len = sprintf(pbuf, "[ERROR][%s] <%d> <ps:%lu:%lu mem:%lu> ", getstimeval(), getpid(), pas, pus, gfs_mem_allsize);
            break;
    }


    pbuf += len;
    va_start(arg_list, format);
    len += vsprintf(pbuf, format, arg_list);
    va_end(arg_list);
    
    if(buffer[len - 1] !='\n')
    {
        buffer[len] = '\n';
        buffer[++len] = '\0';
    }
    
    res = fwrite(buffer, len, 1, logfp);
    if(res <= 0)
    {
        //printf("write log file error. errno code is %d\n", errno);
        return GFS_FIO_ERR;
    }

    fflush(logfp);
    //printf(buffer);

    return GFS_OK;
}

