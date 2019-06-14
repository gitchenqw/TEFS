#ifndef __GFS_MEM_H__
#define __GFS_MEM_H__

typedef struct gfs_mem_page_s    gfs_mem_page_t;
typedef struct gfs_mem_block_s   gfs_mem_block_t;
typedef struct gfs_mem_large_s   gfs_mem_large_t;
typedef struct gfs_mem_pool_s    gfs_mem_pool_t;

struct gfs_mem_page_s
{
    gfs_mem_page_t   *next;
    gfs_mem_page_t   *prev;
    char            *start;
    char            *end;
};

struct gfs_mem_block_s
{
    gfs_mem_block_t  *next;
    gfs_mem_block_t  *prev;
};

struct gfs_mem_large_s
{
    gfs_mem_large_t  *next;
    gfs_mem_large_t  *prev;
    char            *addr;
    unsigned long    size;
};

//block size = {32, 64, 128, 256, 512, 1024, 2048, 4096}
struct gfs_mem_pool_s
{
    gfs_mem_page_t    page;          //memory page list
    gfs_mem_block_t   block[8];      //different size block list
    gfs_mem_large_t   large;         //large size memory list
    unsigned long    size;          //memory pool all size
    unsigned long    used;          //memory pool used size
    unsigned long    msize;         //ugfs malloc used size
    unsigned long    init_pages;    //init page number
};

gfs_mem_pool_t*  gfs_mem_pool_create(unsigned int pages);
void*           gfs_mem_malloc(gfs_mem_pool_t *pool, unsigned int size);
void            gfs_mem_free(void *ptr);

void            gfs_mem_pool_reset(gfs_mem_pool_t *pool);
void            gfs_mem_pool_destory(gfs_mem_pool_t *pool);

extern unsigned long gfs_mem_allsize;
#endif
