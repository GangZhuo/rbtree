/*
* MIT License
* 
* Copyright (c) 2017 Gang Zhuo <gang.zhuo@gmail.com>
* 
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
* 
* The above copyright notice and this permission notice shall be included in all
* copies or substantial portions of the Software.
* 
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*/

#ifndef RBTREE_H_
#define RBTREE_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef struct rbtree_t rbtree_t;
typedef struct rbnode_t rbnode_t;

typedef enum rbnode_color_t {
	rbnode_black,
	rbnode_red
} rbnode_color_t;

struct rbnode_t {
	void *key;
	rbnode_color_t color;
	rbnode_t *left;
	rbnode_t *right;
	rbnode_t *parent;
};

typedef int (*rbtree_keycmp_func_t)(void *a, void *b);
typedef int (*rbtree_iterate_func_t)(rbtree_t *tree, rbnode_t *n, void *state);

struct rbtree_t {
	rbnode_t *root;
	rbtree_keycmp_func_t keycmp;
};

#define RBTREE_INIT(keycmp) { .root = NULL, .keycmp = keycmp }

#define rbtree_init(tree, _keycmp) \
	do { \
		(tree)->root = NULL; \
		(tree)->keycmp = (_keycmp); \
	} while (0)

#define rbnode_nil			(NULL)
#define rbnode_is_nil(n)	((n) == rbnode_nil)
#define rbnode_is_root(n)	(rbnode_is_nil((n)->parent))
#define rbnode_is_black(n)	(rbnode_is_nil(n) || (n)->color == rbnode_black)
#define rbnode_is_red(n)	(!rbnode_is_nil(n) && (n)->color == rbnode_red)
#define rbnode_is_left(n)	((n)->parent->left == (n))
#define rbnode_is_right(n)	((n)->parent->right == (n))
#define rbnode_is_leaf(n)	(rbnode_is_nil((n)->left) || rbnode_is_nil((n)->right))
#define rbnode_set_black(n) ((n)->color = rbnode_black)
#define rbnode_set_red(n)	((n)->color = rbnode_red)

/* Insert node.
If successful, returns 0, otherwise returns -1,
and errno set to EEXIST when failure.*/
int rbtree_insert(rbtree_t *tree, rbnode_t *n);

/* Lookup node by key.
If found, returns node that found, otherwise returns NULL. */
rbnode_t *rbtree_lookup(rbtree_t *tree, void *key);

/* Delete a node from the tree.
Make sure that the 'n' is a element in the tree node link. */
void rbtree_delete(rbtree_t *tree, rbnode_t *n);

/* Iterate nodes.
If successful, returns 0, otherwise returns error code,
which is return by iteration function. */
int rbtree_foreach_preorder(rbtree_t *tree,
	rbtree_iterate_func_t iteration, void *state);

/* Same as rbtree_foreach_preorder, but inorder. */
int rbtree_foreach_inorder(rbtree_t *tree,
	rbtree_iterate_func_t iteration, void *state);

/* Same as rbtree_foreach_preorder, but postorder. */
int rbtree_foreach_postorder(rbtree_t *tree,
	rbtree_iterate_func_t iteration, void *state);

int rbtree_foreach_print(rbtree_t *tree,
	rbtree_iterate_func_t iteration, void *state);

#define rbtree_offsetof(s,m) ((size_t)&(((s*)0)->m))

#define rbtree_container_of(field, struct_type, field_name) \
	((struct_type *)(((char *)(field)) - rbtree_offsetof(struct_type, field_name)))

#ifdef __cplusplus
}
#endif

#endif
