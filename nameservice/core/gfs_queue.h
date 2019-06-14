#ifndef __GFS_QUEUE_H__
#define __GFS_QUEUE_H__
#include <gfs_config.h>

typedef struct gfs_queue_s  gfs_queue_t;

struct gfs_queue_s {
    gfs_queue_t  *prev;
    gfs_queue_t  *next;
};


#define gfs_queue_init(q)                                                     \
    (q)->prev = q;                                                            \
    (q)->next = q


#define gfs_queue_empty(h)                                                    \
    (h == (h)->prev)


#define gfs_queue_insert_head(h, x)                                           \
    (x)->next = (h)->next;                                                    \
    (x)->next->prev = x;                                                      \
    (x)->prev = h;                                                            \
    (h)->next = x


#define gfs_queue_insert_after   gfs_queue_insert_head


#define gfs_queue_insert_tail(h, x)                                           \
    (x)->prev = (h)->prev;                                                    \
    (x)->prev->next = x;                                                      \
    (x)->next = h;                                                            \
    (h)->prev = x


#define gfs_queue_head(h)                                                     \
    (h)->next


#define gfs_queue_last(h)                                                     \
    (h)->prev


#define gfs_queue_sentinel(h)                                                 \
    (h)


#define gfs_queue_next(q)                                                     \
    (q)->next


#define gfs_queue_prev(q)                                                     \
    (q)->prev


#if (GFS_DEBUG)

#define gfs_queue_remove(x)                                                   \
    (x)->next->prev = (x)->prev;                                              \
    (x)->prev->next = (x)->next;                                              \
    (x)->prev = NULL;                                                         \
    (x)->next = NULL

#else

#define gfs_queue_remove(x)                                                   \
    (x)->next->prev = (x)->prev;                                              \
    (x)->prev->next = (x)->next

#endif


#define gfs_queue_split(h, q, n)                                              \
    (n)->prev = (h)->prev;                                                    \
    (n)->prev->next = n;                                                      \
    (n)->next = q;                                                            \
    (h)->prev = (q)->prev;                                                    \
    (h)->prev->next = h;                                                      \
    (q)->prev = n;


#define gfs_queue_add(h, n)                                                   \
    (h)->prev->next = (n)->next;                                              \
    (n)->next->prev = (h)->prev;                                              \
    (h)->prev = (n)->prev;                                                    \
    (h)->prev->next = h;

#define gfs_offsetof(type, member) (size_t)&(((type *)0)->member)

#define gfs_queue_data(q, type, link)                                         \
    (type *) ((u_char *) q - gfs_offsetof(type, link))


gfs_queue_t *gfs_queue_middle(gfs_queue_t *queue);
gfs_void     gfs_queue_sort(gfs_queue_t *queue,
        gfs_int32 (*cmp)(const gfs_queue_t *, const gfs_queue_t *));


#endif /* _GFS_QUEUE_H_INCLUDED_ */

