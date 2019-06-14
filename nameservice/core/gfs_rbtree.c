#include <stdlib.h>
#include <stdio.h>
#include <gfs_rbtree.h>


static inline void gfs_rbtree_left_rotate(gfs_rbtree_node_t **root, gfs_rbtree_node_t *sentinel, gfs_rbtree_node_t *node);
static inline void gfs_rbtree_right_rotate(gfs_rbtree_node_t **root, gfs_rbtree_node_t *sentinel, gfs_rbtree_node_t *node);


void gfs_rbtree_insert(gfs_rbtree_t *tree, gfs_rbtree_node_t *node)
{
    gfs_rbtree_node_t  **root, *temp, *sentinel;

    /* a binary tree insert */
    root = (gfs_rbtree_node_t **) &tree->root;
    sentinel = tree->sentinel;

    if (*root == sentinel) {
        node->parent = NULL;
        node->left = sentinel;
        node->right = sentinel;
        gfs_rbt_black(node);
        *root = node;

        return;
    }

    tree->insert(*root, node, sentinel);

    /* re-balance tree */

    while (node != *root && gfs_rbt_is_red(node->parent)) {

        if (node->parent == node->parent->parent->left) {
            temp = node->parent->parent->right;

            if (gfs_rbt_is_red(temp)) {
                gfs_rbt_black(node->parent);
                gfs_rbt_black(temp);
                gfs_rbt_red(node->parent->parent);
                node = node->parent->parent;

            } else {
                if (node == node->parent->right) {
                    node = node->parent;
                    gfs_rbtree_left_rotate(root, sentinel, node);
                }

                gfs_rbt_black(node->parent);
                gfs_rbt_red(node->parent->parent);
                gfs_rbtree_right_rotate(root, sentinel, node->parent->parent);
            }

        } else {
            temp = node->parent->parent->left;

            if (gfs_rbt_is_red(temp)) {
                gfs_rbt_black(node->parent);
                gfs_rbt_black(temp);
                gfs_rbt_red(node->parent->parent);
                node = node->parent->parent;

            } else {
                if (node == node->parent->left) {
                    node = node->parent;
                    gfs_rbtree_right_rotate(root, sentinel, node);
                }

                gfs_rbt_black(node->parent);
                gfs_rbt_red(node->parent->parent);
                gfs_rbtree_left_rotate(root, sentinel, node->parent->parent);
            }
        }
    }

    gfs_rbt_black(*root);
}


void gfs_rbtree_insert_value(gfs_rbtree_node_t *temp, gfs_rbtree_node_t *node, gfs_rbtree_node_t *sentinel)
{
    gfs_rbtree_node_t  **p;

    for ( ;; ) {

        p = (node->key < temp->key) ? &temp->left : &temp->right;

        if (*p == sentinel) {
            break;
        }

        temp = *p;
    }

    *p = node;
    node->parent = temp;
    node->left = sentinel;
    node->right = sentinel;
    gfs_rbt_red(node);
}


void gfs_rbtree_delete(gfs_rbtree_t *tree, gfs_rbtree_node_t *node)
{
    gfs_uint_t           red;
    gfs_rbtree_node_t  **root, *sentinel, *subst, *temp, *w;

    /* a binary tree delete */
    root = (gfs_rbtree_node_t **) &tree->root;
    sentinel = tree->sentinel;

    if (node->left == sentinel) {
        temp = node->right;
        subst = node;

    } else if (node->right == sentinel) {
        temp = node->left;
        subst = node;

    } else {
        subst = gfs_rbtree_min(node->right, sentinel);

        if (subst->left != sentinel) {
            temp = subst->left;
        } else {
            temp = subst->right;
        }
    }

    if (subst == *root) {
        *root = temp;
        gfs_rbt_black(temp);

        /* DEBUG stuff */
        node->left = NULL;
        node->right = NULL;
        node->parent = NULL;
        node->key = 0;

        return;
    }

    red = gfs_rbt_is_red(subst);

    if (subst == subst->parent->left) {
        subst->parent->left = temp;

    } else {
        subst->parent->right = temp;
    }

    if (subst == node) {

        temp->parent = subst->parent;

    } else {

        if (subst->parent == node) {
            temp->parent = subst;

        } else {
            temp->parent = subst->parent;
        }

        subst->left = node->left;
        subst->right = node->right;
        subst->parent = node->parent;
        gfs_rbt_copy_color(subst, node);

        if (node == *root) {
            *root = subst;

        } else {
            if (node == node->parent->left) {
                node->parent->left = subst;
            } else {
                node->parent->right = subst;
            }
        }

        if (subst->left != sentinel) {
            subst->left->parent = subst;
        }

        if (subst->right != sentinel) {
            subst->right->parent = subst;
        }
    }

    /* DEBUG stuff */
    node->left = NULL;
    node->right = NULL;
    node->parent = NULL;
    node->key = 0;

    if (red) {
        return;
    }

    /* a delete fixup */

    while (temp != *root && gfs_rbt_is_black(temp)) {

        if (temp == temp->parent->left) {
            w = temp->parent->right;

            if (gfs_rbt_is_red(w)) {
                gfs_rbt_black(w);
                gfs_rbt_red(temp->parent);
                gfs_rbtree_left_rotate(root, sentinel, temp->parent);
                w = temp->parent->right;
            }

            if (gfs_rbt_is_black(w->left) && gfs_rbt_is_black(w->right)) {
                gfs_rbt_red(w);
                temp = temp->parent;

            } else {
                if (gfs_rbt_is_black(w->right)) {
                    gfs_rbt_black(w->left);
                    gfs_rbt_red(w);
                    gfs_rbtree_right_rotate(root, sentinel, w);
                    w = temp->parent->right;
                }

                gfs_rbt_copy_color(w, temp->parent);
                gfs_rbt_black(temp->parent);
                gfs_rbt_black(w->right);
                gfs_rbtree_left_rotate(root, sentinel, temp->parent);
                temp = *root;
            }

        } else {
            w = temp->parent->left;

            if (gfs_rbt_is_red(w)) {
                gfs_rbt_black(w);
                gfs_rbt_red(temp->parent);
                gfs_rbtree_right_rotate(root, sentinel, temp->parent);
                w = temp->parent->left;
            }

            if (gfs_rbt_is_black(w->left) && gfs_rbt_is_black(w->right)) {
                gfs_rbt_red(w);
                temp = temp->parent;

            } else {
                if (gfs_rbt_is_black(w->left)) {
                    gfs_rbt_black(w->right);
                    gfs_rbt_red(w);
                    gfs_rbtree_left_rotate(root, sentinel, w);
                    w = temp->parent->left;
                }

                gfs_rbt_copy_color(w, temp->parent);
                gfs_rbt_black(temp->parent);
                gfs_rbt_black(w->left);
                gfs_rbtree_right_rotate(root, sentinel, temp->parent);
                temp = *root;
            }
        }
    }

    gfs_rbt_black(temp);
}

gfs_rbtree_node_t* gfs_rbtree_lookup_value(gfs_rbtree_t *rbtree, gfs_rbtree_key_t *key)
{
    gfs_rbtree_node_t *tmpnode;
    gfs_rbtree_node_t *lookup_node;
    gfs_rbtree_node_t *sentinel;

    tmpnode = rbtree->root;
    sentinel = rbtree->sentinel;
    lookup_node = NULL;

    while(tmpnode != sentinel)
    {
        if (*key != tmpnode->key)
        {
            tmpnode = (*key < tmpnode->key) ? tmpnode->left : tmpnode->right;
            continue;
        }

        lookup_node = tmpnode;
        break;
    }

    return lookup_node;
}

static inline void gfs_rbtree_left_rotate(gfs_rbtree_node_t **root, gfs_rbtree_node_t *sentinel, gfs_rbtree_node_t *node)
{
    gfs_rbtree_node_t  *temp;

    temp = node->right;
    node->right = temp->left;

    if (temp->left != sentinel) {
        temp->left->parent = node;
    }

    temp->parent = node->parent;

    if (node == *root) {
        *root = temp;

    } else if (node == node->parent->left) {
        node->parent->left = temp;

    } else {
        node->parent->right = temp;
    }

    temp->left = node;
    node->parent = temp;
}


static inline void gfs_rbtree_right_rotate(gfs_rbtree_node_t **root, gfs_rbtree_node_t *sentinel, gfs_rbtree_node_t *node)
{
    gfs_rbtree_node_t  *temp;

    temp = node->left;
    node->left = temp->right;

    if (temp->right != sentinel) {
        temp->right->parent = node;
    }

    temp->parent = node->parent;

    if (node == *root) {
        *root = temp;

    } else if (node == node->parent->right) {
        node->parent->right = temp;

    } else {
        node->parent->left = temp;
    }

    temp->right = node;
    node->parent = temp;
}
