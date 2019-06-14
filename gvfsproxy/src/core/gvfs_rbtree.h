
#ifndef _GVFS_RBTREE_H_INCLUDED_
#define _GVFS_RBTREE_H_INCLUDED_


#include <gvfs_config.h>
#include <gvfs_core.h>


typedef gvfs_uint_t  gvfs_rbtree_key_t;
typedef gvfs_int_t   gvfs_rbtree_key_int_t;


typedef struct gvfs_rbtree_node_s  gvfs_rbtree_node_t;

struct gvfs_rbtree_node_s {
    gvfs_rbtree_key_t       key;       /* 关键字节点 */
    gvfs_rbtree_node_t     *left;      /* 左孩子节点指针 */
    gvfs_rbtree_node_t     *right;     /* 右孩子节点指针 */
    gvfs_rbtree_node_t     *parent;    /* 双亲节点指针 */
    u_char                  color;     /* 节点颜色 */
};


typedef struct gvfs_rbtree_s  gvfs_rbtree_t;

typedef void (*gvfs_rbtree_insert_pt) (gvfs_rbtree_node_t *root,
    gvfs_rbtree_node_t *node, gvfs_rbtree_node_t *sentinel);

struct gvfs_rbtree_s {
    gvfs_rbtree_node_t     *root;      /* 指向根节点,根节点也是一个数据节点 */
    gvfs_rbtree_node_t     *sentinel;  /* 指向NIL哨兵节点 */
    gvfs_rbtree_insert_pt   insert;    /* 添加节点时决定是新增还是替换节点 */
};


#define gvfs_rbtree_init(tree, s, i)                                           \
    gvfs_rbtree_sentinel_init(s);                                              \
    (tree)->root = s;                                                         \
    (tree)->sentinel = s;                                                     \
    (tree)->insert = i


void gvfs_rbtree_insert(gvfs_rbtree_t *tree, gvfs_rbtree_node_t *node);
void gvfs_rbtree_delete(gvfs_rbtree_t *tree, gvfs_rbtree_node_t *node);

/* 预提供的2个插入方式,也可自定义插入方法 */
void gvfs_rbtree_insert_value(gvfs_rbtree_node_t *root, gvfs_rbtree_node_t *node,
    gvfs_rbtree_node_t *sentinel);
void gvfs_rbtree_insert_timer_value(gvfs_rbtree_node_t *root,
    gvfs_rbtree_node_t *node, gvfs_rbtree_node_t *sentinel);


#define gvfs_rbt_red(node)               ((node)->color = 1)
#define gvfs_rbt_black(node)             ((node)->color = 0)
#define gvfs_rbt_is_red(node)            ((node)->color)
#define gvfs_rbt_is_black(node)          (!gvfs_rbt_is_red(node))
#define gvfs_rbt_copy_color(n1, n2)      (n1->color = n2->color)


/* a sentinel must be black */

#define gvfs_rbtree_sentinel_init(node)  gvfs_rbt_black(node)


static inline gvfs_rbtree_node_t *
gvfs_rbtree_min(gvfs_rbtree_node_t *node, gvfs_rbtree_node_t *sentinel)
{
    while (node->left != sentinel) {
        node = node->left;
    }

    return node;
}


#endif /* _GVFS_RBTREE_H_INCLUDED_ */
