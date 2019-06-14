#include <gfs_queue.h>

gfs_queue_t* gfs_queue_middle(gfs_queue_t *queue)
{
    gfs_queue_t  *middle, *next;

    middle = gfs_queue_head(queue);

    if (middle == gfs_queue_last(queue)) {
        return middle;
    }

    next = gfs_queue_head(queue);

    for ( ;; ) {
        middle = gfs_queue_next(middle);

        next = gfs_queue_next(next);

        if (next == gfs_queue_last(queue)) {
            return middle;
        }

        next = gfs_queue_next(next);

        if (next == gfs_queue_last(queue)) {
            return middle;
        }
    }
}


/* the stable insertion sort */

void gfs_queue_sort(gfs_queue_t *queue, gfs_int32 (*cmp)(const gfs_queue_t *, const gfs_queue_t *))
{
    gfs_queue_t  *q, *prev, *next;

    q = gfs_queue_head(queue);

    if (q == gfs_queue_last(queue)) {
        return;
    }

    for (q = gfs_queue_next(q); q != gfs_queue_sentinel(queue); q = next) {

        prev = gfs_queue_prev(q);
        next = gfs_queue_next(q);

        gfs_queue_remove(q);

        do {
            if (cmp(prev, q) <= 0) {
                break;
            }

            prev = gfs_queue_prev(prev);

        } while (prev != gfs_queue_sentinel(queue));

        gfs_queue_insert_after(prev, q);
    }
}

