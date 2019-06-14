#ifndef __GFS_RBTREE_H__
#define __GFS_RBTREE_H__

typedef unsigned int gfs_uint_t;
typedef int          gfs_int_t;

typedef gfs_uint_t    gfs_rbtree_key_t;
typedef gfs_int_t     gfs_rbtree_key_int_t;


typedef struct gfs_rbtree_node_s  gfs_rbtree_node_t;

struct gfs_rbtree_node_s
{
    gfs_rbtree_key_t       key;
    gfs_rbtree_node_t     *left;
    gfs_rbtree_node_t     *right;
    gfs_rbtree_node_t     *parent;
    u_char                 color;
    u_char                 data;
};


typedef struct gfs_rbtree_s  gfs_rbtree_t;

typedef void (*gfs_rbtree_insert_pt) (gfs_rbtree_node_t *root, gfs_rbtree_node_t *node, gfs_rbtree_node_t *sentinel);

struct gfs_rbtree_s
{
    gfs_rbtree_node_t     *root;
    gfs_rbtree_node_t     *sentinel;
    gfs_rbtree_insert_pt   insert;
};


#define gfs_rbtree_init(tree, s, i)                                          \
    gfs_rbtree_sentinel_init(s);                                               \
    (tree)->root = s;                                                          \
    (tree)->sentinel = s;                                                      \
    (tree)->insert = i



void gfs_rbtree_insert(gfs_rbtree_t *tree, gfs_rbtree_node_t *node);
void gfs_rbtree_delete(gfs_rbtree_t *tree, gfs_rbtree_node_t *node);
gfs_rbtree_node_t* gfs_rbtree_lookup_value(gfs_rbtree_t *tree, gfs_rbtree_key_t *key);             //addd by zrbt
void gfs_rbtree_insert_value(gfs_rbtree_node_t *root, gfs_rbtree_node_t *node, gfs_rbtree_node_t *sentinel);
//void gfs_rbtree_insert_timer_value(gfs_rbtree_node_t *root, gfs_rbtree_node_t *node, gfs_rbtree_node_t *sentinel);

#define gfs_rbt_red(node)               ((node)->color = 1)
#define gfs_rbt_black(node)             ((node)->color = 0)
#define gfs_rbt_is_red(node)            ((node)->color)
#define gfs_rbt_is_black(node)          (!gfs_rbt_is_red(node))
#define gfs_rbt_copy_color(n1, n2)      (n1->color = n2->color)


/* a sentinel must be black */

#define gfs_rbtree_sentinel_init(node)  gfs_rbt_black(node)


static inline gfs_rbtree_node_t* gfs_rbtree_min(gfs_rbtree_node_t *node, gfs_rbtree_node_t *sentinel)
{
    while (node->left != sentinel) {
        node = node->left;
    }

    return node;
}


#endif
