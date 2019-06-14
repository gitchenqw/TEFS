#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <gfs_mem.h>

#ifdef __x86_64__
#define WSIZE       8       // Word and header/footer size (bytes)
#define DSIZE       16      // Doubleword size (bytes)
#elif __i386__
#define WSIZE       4       // Word and header/footer size (bytes)
#define DSIZE       8       // Doubleword size (bytes)
#endif
//#define CHUNKSIZE  (1<<12)  // Extend heap by this amount (bytes)
#define PAGESIZE        4096
#define UNALLOCATED     0
#define ALLOCATED       1
#define INITIAL         2
#define LARGE           4
#define BLOCK_MIN       (3*DSIZE)   // #(size|flag)#(<pool><prev><next>[...])#(size|flag)#
#define MIN_LARGE_SIZE  3072        // 1024*3

#define MAX(x, y) ((x) > (y)? (x) : (y))

// Pack a size and allocated bit into a word
#define PACK(size, alloc)  ((size) | (alloc))

// Read and write a word at address p 
#define GET(p)       (*(unsigned int *)(p))
#define PUT(p, val)  (*(unsigned int *)(p) = (val))

// Read the size and allocated fields from address p
#define GET_SIZE(p)  (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)

#define GET_INIT(p)  (GET(p) & 0x2)         //Get page init tag
#define GET_LARGE(p) (GET(p) & 0x4)         //Get large block tag

// Given block ptr bp, compute address of its header and footer
#define HDRP(bp)       ((char *)(bp) - WSIZE)
#define FTRP(bp)       ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

// Given block ptr bp, compute address of next and previous blocks
#define NEXT_BLKP(bp)  ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))
#define PREV_BLKP(bp)  ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))

#define gfs_mem_align(d, a)     (((d) + (a - 1)) & ~(a - 1))

#define gfs_mem_queue_init(q)                \
    (q)->prev = q;                          \
    (q)->next = q
#define gfs_mem_queue_empty(q)               \
    (h == (h)->prev)
#define gfs_mem_queue_insert_head(h, x)      \
    (x)->next = (h)->next;                  \
    (x)->next->prev = x;                    \
    (x)->prev = h;                          \
    (h)->next = x
#define gfs_mem_queue_insert_tail(h, x)      \
    (x)->prev = (h)->prev;                  \
    (x)->prev->next = x;                    \
    (x)->next = h;                          \
    (h)->prev = x
#define gfs_mem_queue_next(q)                \
    (q)->next
#define gfs_mem_queue_head(h)                \
    (h)->next
#define gfs_mem_queue_prev(q)                \
    (q)->prev
#define gfs_mem_queue_remove(x)              \
    (x)->next->prev = (x)->prev;            \
    (x)->prev->next = (x)->next

unsigned long gfs_mem_allsize = 0;

static void  gfs_mem_place(gfs_mem_pool_t *pool, void *bp, size_t asize);
static void* gfs_mem_find_fit(void *bp, size_t asize);
static void* gfs_mem_coalesce(gfs_mem_pool_t *pool, void *bp);
static int   gfs_mem_getindex(unsigned int num);
static void* gfs_mem_malloc_internal(size_t size);
static void  gfs_mem_free_internal(void *bp, size_t size);
static void  gfs_mem_extend_pool(gfs_mem_pool_t *pool);

gfs_mem_pool_t*  gfs_mem_pool_create(unsigned int pages)
{
    gfs_mem_pool_t   *pool;
    gfs_mem_page_t   *page;
    gfs_mem_block_t  *block;
    char            *ptr, *bp;
    unsigned long    size;
    int              i;

    if (!pages)
        return NULL;

    ptr = gfs_mem_malloc_internal(PAGESIZE);

    PUT(ptr, PACK(0, (ALLOCATED | INITIAL)));       //head [0/1]
    PUT(ptr + PAGESIZE - WSIZE, PACK(0, (ALLOCATED | INITIAL)));    //tail [0/1]

    size = sizeof(gfs_mem_page_t) + sizeof(gfs_mem_pool_t) + DSIZE;
    bp = ptr + DSIZE;
    PUT(HDRP(bp), PACK(size, ALLOCATED));
    PUT(FTRP(bp), PACK(size, ALLOCATED));
    page = (gfs_mem_page_t*)bp;
    page->start = ptr;
    page->end = ptr + PAGESIZE;

    pool = (gfs_mem_pool_t*)(bp + sizeof(gfs_mem_page_t));
    pool->size = PAGESIZE*pages;
    pool->used = GET_SIZE(ptr + WSIZE) + DSIZE;
    pool->msize = 0;
    pool->init_pages = pages;
    gfs_mem_queue_init(&pool->large);
    gfs_mem_queue_init(&pool->page);
    gfs_mem_queue_insert_tail(&pool->page, page);
    for (i = 0; i < 8; i++)
    {
        gfs_mem_queue_init(&pool->block[i]);
    }

    bp = NEXT_BLKP(bp);
    size = PAGESIZE - size - DSIZE;
    PUT(HDRP(bp), PACK(size, UNALLOCATED));
    PUT(FTRP(bp), PACK(size, UNALLOCATED));

    block = (gfs_mem_block_t*)bp;
    gfs_mem_queue_insert_head(&pool->block[gfs_mem_getindex(size)], block);

    size = sizeof(gfs_mem_page_t) + DSIZE;
    for (i = 1; i < pages; i++)
    {
        ptr = gfs_mem_malloc_internal(PAGESIZE);
        PUT(ptr, PACK(0, (ALLOCATED | INITIAL)));     //head [0/1]
        PUT(ptr + PAGESIZE - WSIZE, PACK(0, (ALLOCATED | INITIAL)));      //tail [0/1]
        pool->used += DSIZE;
        bp = ptr + DSIZE;
        PUT(HDRP(bp), PACK(PAGESIZE - DSIZE, UNALLOCATED));
        PUT(FTRP(bp), PACK(PAGESIZE - DSIZE, UNALLOCATED));
        block = (gfs_mem_block_t*)bp;
        gfs_mem_queue_insert_head(&pool->block[gfs_mem_getindex(PAGESIZE - DSIZE)], block);

        bp = ptr + DSIZE;
        gfs_mem_place(pool, bp, size);     //divide large block memory

        page = (gfs_mem_page_t*)bp;
        page->start = ptr;
        page->end = ptr + PAGESIZE;
        gfs_mem_queue_insert_head(&pool->page, page);
    }

    return pool;
}

void*           gfs_mem_malloc(gfs_mem_pool_t *pool, unsigned int size)
{
    gfs_mem_large_t  *large;
    size_t           asize;
    int              i;
    void            *bp;

    if (!pool || !size)
        return NULL;

    if (size <= DSIZE)
        asize = BLOCK_MIN;
    else if (size > MIN_LARGE_SIZE)
    {
        size = gfs_mem_align(size, 8);
        size += (sizeof(gfs_mem_large_t) + DSIZE);
        bp = gfs_mem_malloc_internal(size);
        large = (gfs_mem_large_t*)bp;
        large->addr = bp;
        large->size = size;
        pool->used += size;
        pool->msize += size;
        gfs_mem_queue_insert_tail(&pool->large, large);
        bp += sizeof(gfs_mem_large_t);
        PUT(bp, PACK(size, (ALLOCATED | LARGE)));
        bp += WSIZE;
        *(gfs_mem_pool_t**)bp = pool;
        return bp + WSIZE;
    }
    else
        asize = gfs_mem_align(size, 8) + 2*DSIZE;

    i = gfs_mem_getindex(asize);
    i = (asize > (1<<(i+4)) && i < 7) ? (i+1) : i;
    for (; i < 8; i++)
    {
        if ((bp = gfs_mem_find_fit((void*)&pool->block[i], asize)) != NULL)
        {
            gfs_mem_place(pool, bp, asize);
            pool->msize += GET_SIZE(HDRP(bp));
            *(gfs_mem_pool_t**)bp = pool;
            return bp + WSIZE;
        }
    }

    gfs_mem_extend_pool(pool);
    if (!(bp = gfs_mem_find_fit((void*)&pool->block[7], asize)))
        return NULL;

    gfs_mem_place(pool, bp, asize);
    pool->msize += GET_SIZE(HDRP(bp));
    *(gfs_mem_pool_t**)bp = pool;
    return bp + WSIZE;
}

void            gfs_mem_free(void *ptr)
{
    gfs_mem_pool_t  *pool;
    gfs_mem_large_t *large;
    char           *bp;
    size_t          size;

    if (!ptr)
        return;

    bp = (char*)ptr - WSIZE;
    pool = *(gfs_mem_pool_t**)bp;
    size = GET_SIZE(HDRP(bp));
    pool->used -= size;
    pool->msize-= size;

    if (GET_LARGE(HDRP(bp)))
    {
        large = (gfs_mem_large_t*)(bp - WSIZE - sizeof(gfs_mem_large_t));
        gfs_mem_queue_remove(large);
        gfs_mem_free_internal(large->addr, size);
    }
    else
    {
        if (GET(HDRP(bp)) != GET(FTRP(bp)))
        {
            printf("WARN: the memory of crossing the line. or other error!\n");
            exit(-1);
        }
        PUT(HDRP(bp), PACK(size, UNALLOCATED));
        PUT(FTRP(bp), PACK(size, UNALLOCATED));
        gfs_mem_coalesce(pool, bp);
    }
}

void            gfs_mem_pool_reset(gfs_mem_pool_t *pool)
{
    gfs_mem_large_t      *lhead, *large, *lnext;
    gfs_mem_page_t       *phead, *page, *pnext;
    void                 *bp;
    gfs_mem_block_t      *block;
    size_t               i, size;

    if (!pool)
        return;

    lhead = &pool->large;
    for(large = gfs_mem_queue_head(lhead); large != lhead; large = lnext)
    {
        lnext = gfs_mem_queue_next(large);
        gfs_mem_queue_remove(large);
        gfs_mem_free_internal(large->addr, large->size);
    }

    pool->size = pool->init_pages * PAGESIZE;
    pool->used = 0;
    pool->msize = 0;
    gfs_mem_queue_init(&pool->large);
    for (i = 0; i < 8; i++)
    {
        gfs_mem_queue_init(&pool->block[i]);
    }

    phead = &pool->page;
    for(page = gfs_mem_queue_head(phead); page != phead; page = pnext)
    {
        pnext = gfs_mem_queue_next(page);
        if (GET_INIT(page->start))
        {
            size = PAGESIZE - GET_SIZE(HDRP(page)) - DSIZE;
            bp = NEXT_BLKP(page);
            PUT(HDRP(bp), PACK(size, UNALLOCATED));
            PUT(FTRP(bp), PACK(size, UNALLOCATED));
            block = (gfs_mem_block_t*)bp;
            gfs_mem_queue_insert_head(&pool->block[gfs_mem_getindex(size)], block);
            pool->used += PAGESIZE - size;

        }
        else
        {
            gfs_mem_queue_remove(page);
            gfs_mem_free_internal(page->start, PAGESIZE);
        }
    }
}

void            gfs_mem_pool_destory(gfs_mem_pool_t *pool)
{
    gfs_mem_large_t      *lhead, *large, *lnext;
    gfs_mem_page_t       *phead, *page, *pnext;

    if (!pool)
        return;

    phead = &pool->page;
    lhead = &pool->large;

    for(large = gfs_mem_queue_head(lhead); large != lhead; large = lnext)
    {
        lnext = gfs_mem_queue_next(large);
        gfs_mem_queue_remove(large);
        gfs_mem_free_internal(large->addr, large->size);
    }

    for(page = gfs_mem_queue_head(phead); page != phead; page = pnext)
    {
        pnext = gfs_mem_queue_next(page);
        gfs_mem_queue_remove(page);
        gfs_mem_free_internal(page->start, PAGESIZE);
    }
}

static inline int gfs_mem_getindex(unsigned int num)
{
    int index;

    if(num<= 32)
        return 0;
    else if (num > 2048)
        return 7;

    num--;
    num >>= 4;
    for (index = 0; num >>= 1; index++){}

    return index;
}

//divide large block memory
static void  gfs_mem_place(gfs_mem_pool_t *pool, void *bp, size_t asize)
{
    gfs_mem_block_t *block;
    size_t          csize;

    block = (gfs_mem_block_t*)bp;
    gfs_mem_queue_remove(block);
    csize = GET_SIZE(HDRP(bp));

    if ((csize - asize) >= BLOCK_MIN)
    {
        PUT(HDRP(bp), PACK(asize, ALLOCATED));
        PUT(FTRP(bp), PACK(asize, ALLOCATED));
        bp = NEXT_BLKP(bp);
        PUT(HDRP(bp), PACK(csize-asize, UNALLOCATED));
        PUT(FTRP(bp), PACK(csize-asize, UNALLOCATED));

        block = (gfs_mem_block_t*)bp;
        gfs_mem_queue_insert_head(&pool->block[gfs_mem_getindex(csize - asize)], block);
        pool->used += asize;
    }
    else
    {
        PUT(HDRP(bp), PACK(csize, ALLOCATED));
        PUT(FTRP(bp), PACK(csize, ALLOCATED));
        pool->used += csize;
    }
}

//Find a fit for a block with asize bytes (first fit)
static void* gfs_mem_find_fit(void *bp, size_t asize)
{
    gfs_mem_block_t      *block, *q;

    block = (gfs_mem_block_t*)bp;
    for(q = gfs_mem_queue_head(block); q != block; q = gfs_mem_queue_next(q))
    {
        if (asize <= GET_SIZE(HDRP(q)))
            return q;
    }

    return NULL;
}

//Boundary tag coalescing. Return ptr to coalesced block
static void* gfs_mem_coalesce(gfs_mem_pool_t *pool, void *bp)
{
    gfs_mem_block_t  *block;
    size_t           prev_alloc;
    size_t           next_alloc;
    size_t           size;

    prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size = GET_SIZE(HDRP(bp));

    if (prev_alloc && next_alloc)              /* Case 1 */
    {
        block = (gfs_mem_block_t*)bp;
        size = GET_SIZE(HDRP(bp));
        gfs_mem_queue_insert_head(&pool->block[gfs_mem_getindex(size)], block);

        return bp;
    }
    else if (prev_alloc && !next_alloc)        /* Case 2 */
    {
        block = (gfs_mem_block_t*)NEXT_BLKP(bp);
        gfs_mem_queue_remove(block);

        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size,0));

        block = (gfs_mem_block_t*)bp;
        gfs_mem_queue_insert_head(&pool->block[gfs_mem_getindex(size)], block);
    }
    else if (!prev_alloc && next_alloc)        /* Case 3 */
    {
        block = (gfs_mem_block_t*)PREV_BLKP(bp);
        gfs_mem_queue_remove(block);

        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        PUT(FTRP(bp), PACK(size, 0));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);

        gfs_mem_queue_insert_head(&pool->block[gfs_mem_getindex(size)], block);
    }
    else                                       /* Case 4 */
    {
        block = (gfs_mem_block_t*)NEXT_BLKP(bp);
        gfs_mem_queue_remove(block);
        block = (gfs_mem_block_t*)PREV_BLKP(bp);
        gfs_mem_queue_remove(block);

        size += GET_SIZE(HDRP(PREV_BLKP(bp))) + 
            GET_SIZE(FTRP(NEXT_BLKP(bp)));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);

        gfs_mem_queue_insert_head(&pool->block[gfs_mem_getindex(size)], block);
    }

    return bp;
}

static inline void* gfs_mem_malloc_internal(size_t size)
{
    void    *ptr;

    ptr = malloc(size);
    if (!ptr)
    {
        printf("malloc failed! errno is %d. %s\n", errno, strerror(errno));
        exit(-1);
    }
    gfs_mem_allsize += size;

    return ptr;
}

static inline void  gfs_mem_free_internal(void *bp, size_t size)
{
    if (!bp || size == 0)
        return;

    gfs_mem_allsize -= size;
    free(bp);
}

static void  gfs_mem_extend_pool(gfs_mem_pool_t *pool)
{
    gfs_mem_page_t   *page;
    gfs_mem_block_t  *block;
    char            *ptr, *bp;
    size_t           size, i;

    size = sizeof(gfs_mem_page_t) + DSIZE;

    for (i = 0; i < pool->init_pages; i++)
    {
        ptr = gfs_mem_malloc_internal(PAGESIZE);
        pool->size += PAGESIZE;
        PUT(ptr, PACK(0, ALLOCATED));     //head [0/1]
        PUT(ptr + PAGESIZE - WSIZE, PACK(0, ALLOCATED));      //tail [0/1]
        pool->used += DSIZE;
        bp = ptr + DSIZE;
        PUT(HDRP(bp), PACK(PAGESIZE - DSIZE, UNALLOCATED));
        PUT(FTRP(bp), PACK(PAGESIZE - DSIZE, UNALLOCATED));
        block = (gfs_mem_block_t*)bp;
        gfs_mem_queue_insert_head(&pool->block[gfs_mem_getindex(PAGESIZE - DSIZE)], block);

        bp = ptr + DSIZE;
        gfs_mem_place(pool, bp, size);     //divide large block memory

        page = (gfs_mem_page_t*)bp;
        page->start = ptr;
        page->end = ptr + PAGESIZE;
        gfs_mem_queue_insert_head(&pool->page, page);
    }
}
