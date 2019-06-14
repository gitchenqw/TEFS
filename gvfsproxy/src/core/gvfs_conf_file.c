

#include <gvfs_config.h>
#include <gvfs_core.h>

#include <gvfs_proxy.h>

typedef struct gvfs_conf_buf_s gvfs_conf_buf_t;
struct gvfs_conf_buf_s {
    char        *start;
    char        *pos;
    char        *file;
    gvfs_pool_t  *pool;
    size_t       len;
    long         line;
};

static gvfs_conf_t *gvfs_conf_parse_file(char *file, gvfs_pool_t *pool, struct gvfs_list_head *parent);
void gvfs_dump_conf(gvfs_conf_t *conf);

#ifdef HAVE_DEBUG

void gvfs_dump_conf(gvfs_conf_t *conf);
static void gvfs_dump_conf_core(struct gvfs_list_head *link);

#endif

static long gvfs_read_n(FILE *fp,char *buf,long n)
{
    long r,rc;

    r = 0;
    while (n > 0) {
        rc = fread(buf+r,1,n,fp);

        if (rc <= 0) {
            break;
        }

        r += rc;
        n -= r;
    }

    return r;
}

static int gvfs_conf_read_token(gvfs_conf_buf_t *block, gvfs_conf_t* conf)
{
    char ch,*start,*word,*src,*dst,**cmd;
    int sharp_comment,need_space,last_space;
    int start_line,len;
    int quoted,d_quoted,s_quoted,found;

    found = 0;
    need_space = 0;
    last_space = 1;
    sharp_comment = 0;
    quoted = 0;
    s_quoted = 0;
    d_quoted = 0;

    for (;;) {

        if ((size_t)(block->pos - block->start) == block->len) {
            if (conf->stg.nelts > 0 || !last_space) {
                gvfs_log(LOG_ERROR, "file:%s line:%ld unexpected eof %c", block->file,block->line,ch);
                return CONF_CONFIG_ERROR;
            }

            return CONF_CONFIG_END;
        }

        ch = *block->pos++;

        if (ch == '\n') {
            block->line++;
            if (sharp_comment) { // 遇到换行符了,如果设置了评论标志,则标志置0   
                sharp_comment = 0;
            }
        }

        if (sharp_comment) {
            continue;
        }

        if (quoted) {
            quoted = 0;
            continue;
        }

        if (need_space) { // need_space初始化为0
            if (ch == ' ' || ch == '\t' || ch == CR || ch == LF) {
                last_space = 1;
                need_space = 0;
                continue;
            }

            if (ch == ';') {
                return CONF_CONFIG_NEXT;
            }

            if (ch == '{') {
                return CONF_CONFIG_BLOCK_START;
            }
            gvfs_log(LOG_ERROR, "file:%s line:%ld unexpected '%c'", block->file,block->line,ch);

            return CONF_CONFIG_ERROR;
        }

        if (last_space) { // last_space初始化为1
            if (ch == ' ' || ch == '\t' || ch == CR || ch == LF) {
                continue;
            }

            start = block->pos - 1;
            start_line = block->line;
            switch (ch) {

            case ';':
            case '{':
                if (conf->stg.nelts == 0) {

                    gvfs_log(LOG_ERROR, "file:%s line:%ld unexpected '%c'", block->file,block->line,ch);

                    return CONF_CONFIG_ERROR;
                }

                if (ch == '{') {
                    return CONF_CONFIG_BLOCK_START;
                }

                return CONF_CONFIG_NEXT;

            case '}':
                if (conf->stg.nelts != 0) {

                    gvfs_log(LOG_ERROR, "file:%s line:%ld unexpected '%c'", block->file,block->line,ch);

                    return CONF_CONFIG_ERROR;
                }

                return CONF_CONFIG_BLOCK_DONE;

            case '#': // 跳过注释
                sharp_comment = 1;
                continue;

            case '\\':
                quoted = 1;
                last_space = 0;
                continue;

            case '"':
                start++;
                d_quoted = 1;
                last_space = 0;
                continue;

            case '\'':
                start++;
                s_quoted = 1;
                last_space = 0;
                continue;

            default:
                last_space = 0; // 为何置为0
            }

        }
        
        else {
        
        
            if (ch == '{') {
                continue;
            }

            if (ch == '\\') {
                quoted = 1;
                continue;
            }

            if (d_quoted) {
                if (ch == '"') {
                    d_quoted = 0;
                    need_space = 1;
                    found = 1;
                }

            } else if (s_quoted) {
                if (ch == '\'') {
                    s_quoted = 0;
                    need_space = 1;
                    found = 1;
                }

            } else if (ch == ' ' || ch == '\t' || ch == CR || ch == LF
                || ch == ';' || ch == '{')
            {// 配置项名称已经扫描完成
                last_space = 1;
                found = 1;
            }

            if (found) {

                word = gvfs_palloc(block->pool,block->pos - start + 1);
                if (word == NULL) {
                    return CONF_CONFIG_ERROR;
                }

                for (dst = word, src = start, len = 0;
                    src < block->pos - 1;
                    len++)
                {
                    if (*src == '\\') {
                        switch (src[1]) {
                        case '"':
                        case '\'':
                        case '\\':
                            src++;
                            break;

                        case 't':
                            *dst++ = '\t';
                            src += 2;
                            continue;

                        case 'r':
                            *dst++ = '\r';
                            src += 2;
                            continue;

                        case 'n':
                            *dst++ = '\n';
                            src += 2;
                            continue;
                        }

                    }
                    *dst++ = *src++;
                }
                *dst = '\0'; // 注意与nginx不同是该处加了结束符

                cmd = gvfs_array_push(&conf->stg);
                if (cmd == NULL) {
                    return GVFS_ERROR;
                }

                *cmd = word;

                if (ch == ';') {
                    return CONF_CONFIG_NEXT;
                }

                if (ch == '{') {
                    return CONF_CONFIG_BLOCK_START;
                }

                found = 0;
            }
        }
    }
}

static 
gvfs_conf_t* gvfs_conf_parse_buf(gvfs_conf_buf_t *block, struct gvfs_list_head *parent)
{
    gvfs_pool_t *pool;
    int          rc;
    struct gvfs_list_head   *head,*prev,root;
    gvfs_conf_t *conf,*include,*child;
    char        **word,*file,*p,*start;

    INIT_LIST_HEAD(&root);
    pool = block->pool;

    for (;;) {
    
        conf = gvfs_pcalloc(pool, sizeof(gvfs_conf_t));
        if (conf == NULL) {
        	gvfs_log(LOG_ERROR, "gvfs_pcalloc:%lu failed", sizeof(gvfs_conf_t));
            return NULL;
        }

        INIT_LIST_HEAD(&conf->elts);

        gvfs_array_init(&conf->stg, pool, 10, sizeof(char*));
        rc = gvfs_conf_read_token(block, conf);

        switch (rc) {
        case CONF_CONFIG_NEXT: // 已经取完一行了

            conf->rship.prev = parent;
            gvfs_list_add_tail(&conf->elts, &root);

            word = conf->stg.elts;
            //printf("[%s](%s)\n", word[0], word[1]);
            if ((conf->stg.nelts > 1) && (strcmp(word[0],"include") == 0)) {

                p = start = block->file;
                while (*start) {
                    if ((*start == '/') || (*start == '\\')) {
                        p = start;
                    }
                    start++;
                }

                if ((*p == '/') || (*p == '\\')) {

                    file = gvfs_pcalloc(pool,strlen(block->file)+strlen(word[1])+1);
                    if (!file) {
                        return NULL;
                    }

                    memcpy(file,block->file,(p - block->file)+1);
                    strcat(file,word[1]);

                    include = gvfs_conf_parse_file(file,pool,parent);

                } else {

                    include = gvfs_conf_parse_file(word[1],pool,parent);
                }
                
                if (include) {
                    prev = include->elts.prev;
                    do {
                        head = prev->next;

                        list_del(head);
                        gvfs_list_add_tail(head, &root);

                    } while (head != prev);
                }
            }

            break;

        case CONF_CONFIG_BLOCK_START:

            conf->rship.prev = parent;

            gvfs_list_add_tail(&conf->elts, &root);
            child = gvfs_conf_parse_buf(block,&conf->rship);

            if (child) {
                conf->rship.next = &child->rship;
            }

            break;

        case CONF_CONFIG_BLOCK_DONE:

            if (&root != root.next) {

                conf = list_entry(root.next, gvfs_conf_t, elts);
                list_del(&root);

                return conf;
            }

            return NULL;

        case CONF_CONFIG_END:

            if (&root != root.next) {
                conf = list_entry(root.next,gvfs_conf_t,elts);
                list_del(&root);

                return conf;
            }

            return NULL;

        case CONF_CONFIG_ERROR:
            return NULL;

        default:
            break;
        }
    }

    return NULL;
}

gvfs_conf_t* gvfs_conf_parse_file(char *file, gvfs_pool_t *pool, struct gvfs_list_head *parent)
{
	FILE           *fp;
	size_t          size, r/*, dlen*/;
	CHAR           *buf;
	gvfs_conf_t    *conf;
    //UCHAR           dbuf[1024] = {0};
    //UCHAR           ucbuf[1024] = {0};
	gvfs_conf_buf_t block;
	//CHAR            last_value;

    conf = 0;
    fp = fopen(file,"rb");
    if (fp == NULL) {
        gvfs_log(LOG_ERROR, "fopen(\"%s\") failed, error: %s", file,
        	gvfs_strerror(errno));
        return conf;
    }

    fseek(fp,0,SEEK_END);
    size = ftell(fp);

    if (size <= 0) {
    	gvfs_log(LOG_ERROR, "file:%s size:%ld error", file, size);
        goto done_close;
    }

    buf = gvfs_alloc(size + 1);
    if (buf == NULL) {
    	gvfs_log(LOG_ERROR, "malloc:%ld failed", size);
        goto done_close;
    }

    rewind(fp);

    r = gvfs_read_n(fp, buf, size);
    if (r < size) {
    	gvfs_log(LOG_ERROR, "read:%ld return:%ld error", size, r);
        goto done_free;
    }
    buf[size] = '\0';


   	/* { 对配置文件进行解密
    if (GVFS_OK != gvfs_decode_base64(dbuf, &dlen, (UCHAR *) buf, size))
    {
    	gvfs_log(LOG_ERROR, "gvfs_decode_base64(\"%s\") failed",
    						GVFS_CONF_PATH);
    	goto done_free;
    }

	size = dlen;
	dlen = 1024;
	if (GVFS_OK != gvfs_uncompress(ucbuf, &dlen, dbuf, size)) {
    	gvfs_log(LOG_ERROR, "gvfs_uncompress(\"%s\") failed", GVFS_CONF_PATH);
    	goto done_free;
	}

	memset(dbuf, 0, sizeof(dbuf));
	size = gvfs_aes_ecb_decrypt(ucbuf, dlen, (CHAR *) dbuf, GVFS_AES_KEY);
	if (0 > size) {
    	gvfs_log(LOG_ERROR, "gvfs_aes_ecb_decrypt(\"%s\") failed",
    						GVFS_CONF_PATH);
    	goto done_free;
	}
	last_value = *(dbuf + size -1);
	dbuf[size - last_value] = 0;
    } */

    block.line = 0;
    block.pool = pool;
    block.file = file;
    //block.pos  = block.start = (CHAR *) dbuf;
    //block.len  = size - last_value;
    block.pos  = block.start = buf;
    block.len  = size;

    conf = gvfs_conf_parse_buf(&block,parent);

done_free:
    gvfs_free(buf);

done_close:
    fclose(fp);

    return conf;
}

#ifdef HAVE_DEBUG

static void gvfs_dump_conf_core(struct gvfs_list_head *link)
{
    struct gvfs_list_head *next,*prev;
    gvfs_conf_t *conf,*pconf,*c;
    char ** word,**pword;
    uint32_t n;

    next = link;

    do {
        conf = list_entry(next, gvfs_conf_t, elts);
        word = conf->stg.elts;

        n = 0;
        while (n < conf->stg.nelts) {
            printf("(%s)",word[n]);
            n++;
        }

        prev = conf->rship.prev;
        while (prev) {
            pconf = list_entry(prev,gvfs_conf_t, rship);
            pword = pconf->stg.elts;
            printf("->%s ",*pword);
            prev = pconf->rship.prev;
        }
        printf("\n");

        /*dump child*/
        if (conf->rship.next) {
            c = list_entry(conf->rship.next,gvfs_conf_t, rship);
            gvfs_dump_conf_core(&c->elts);
        }

        next = next->next;

    } while (next != link);
}

void gvfs_dump_conf(gvfs_conf_t *conf)
{
    gvfs_dump_conf_core(&conf->elts);
}

#endif

gvfs_int_t gvfs_conf_parse(gvfs_cycle_t *cycle)
{
    gvfs_conf_t *conf;

    cycle->conf = NULL;

    conf = gvfs_conf_parse_file(cycle->conf_file, cycle->pool, 0);

    if (conf) {
        cycle->conf = conf;

#ifdef HAVE_DEBUG
        gvfs_dump_conf(conf);
#endif

        return GVFS_OK;
    }

    return GVFS_ERROR;
}

gvfs_conf_t *gvfs_get_conf_specific(gvfs_conf_t *conf,char *name,int type)
{
    struct gvfs_list_head       *next,*head,*first;
    gvfs_conf_t  *pconf;
    char        **word;

    if (conf == NULL || name == NULL) {
        return NULL;
    }

    if ((type == GET_CONF_NEXT) || (type == GET_CONF_CURRENT)) {
        head = & conf->elts;
        first = head;

    } else if ((type == GET_CONF_CHILD) && (conf->rship.next)) {
        head = & (list_entry(conf->rship.next,gvfs_conf_t, rship))->elts;
        first = head;

    } else {
        return NULL;
    }

    next = head;
    if (type == GET_CONF_NEXT) {

        next = next->next;
        if (next == head) {
            return NULL;
        }
    }

    do {
        
        pconf = list_entry(next, gvfs_conf_t, elts);
        word = pconf->stg.elts;

        if (strcmp(*word,name) == 0) {
            return pconf;
        }

        next = next->next;
    } while (next != head);

    return NULL;
}

CHAR *gvfs_conf_set_flag_slot(gvfs_conf_t *cf, gvfs_command_t *cmd,
	void *conf)
{
    CHAR    *p = conf;
    CHAR    **value;
    UINT32  *fp;

    fp = (UINT32 *) (p + cmd->offset);

    value = cf->stg.elts;

    if (strcasecmp(value[1], "on") == 0) {
        *fp = TRUE;
    } else if (strcasecmp(value[1], "off") == 0) {
        *fp = FALSE;
    } else {
        return GVFS_CONF_ERROR;
    }

    gvfs_log(LOG_DEBUG, "command:%s value:%s", cmd->name, value[1]);
    
    return GVFS_CONF_OK;
}

CHAR *gvfs_conf_set_num_slot(gvfs_conf_t *cf, gvfs_command_t *cmd,
	void *conf)
{
    CHAR    *p = conf;
    CHAR    **value;
    UINT32  *fp;

    fp = (UINT32 *) (p + cmd->offset);
	
    value = cf->stg.elts;

    *fp = atoi(value[1]);
    
    gvfs_log(LOG_DEBUG, "command:%s value:%u", cmd->name, *fp);
    
    return GVFS_CONF_OK;
}

CHAR *gvfs_conf_set_str_slot(gvfs_conf_t *cf, gvfs_command_t *cmd,
	void *conf)
{
    CHAR    *p = conf;
    CHAR    **value;
    CHAR    **fp;

    fp = (CHAR **)(p + cmd->offset);
	
    value = cf->stg.elts;

    *fp = value[1];
    
    gvfs_log(LOG_DEBUG, "command:%s value:%s", cmd->name, *fp);

    return GVFS_CONF_OK;
}

