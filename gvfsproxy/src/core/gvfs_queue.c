
#include <gvfs_config.h>
#include <gvfs_core.h>

gvfs_queue_t *gvfs_queue_middle(gvfs_queue_t *queue)
{
	gvfs_queue_t  *middle, *next;

    middle = gvfs_queue_head(queue);

    if (middle == gvfs_queue_last(queue)) {
        return middle;
    }

    next = gvfs_queue_head(queue);

    for ( ;; ) {
        middle = gvfs_queue_next(middle);

        next = gvfs_queue_next(next);

        if (next == gvfs_queue_last(queue)) {
            return middle;
        }

        next = gvfs_queue_next(next);

        if (next == gvfs_queue_last(queue)) {
            return middle;
        }
    }
}


void gvfs_queue_sort(gvfs_queue_t *queue,
    int (*cmp)(const gvfs_queue_t *, const gvfs_queue_t *))
{
    gvfs_queue_t  *q, *prev, *next;

    q = gvfs_queue_head(queue);

    if (q == gvfs_queue_last(queue)) {
        return;
    }

    for (q = gvfs_queue_next(q); q != gvfs_queue_sentinel(queue); q = next) {

        prev = gvfs_queue_prev(q);
        next = gvfs_queue_next(q);

        gvfs_queue_remove(q);

        do {
            if (cmp(prev, q) <= 0) {
                break;
            }

            prev = gvfs_queue_prev(prev);

        } while (prev != gvfs_queue_sentinel(queue));

        gvfs_queue_insert_after(prev, q);
    }
}

