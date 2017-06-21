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

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include "rbtree.h"

static void rbtree_left_rotate(rbtree_t *tree, rbnode_t *x)
{
	rbnode_t *y;

	y = x->right;
	x->right = y->left;

	if (!rbnode_is_nil(y->left))
		y->left->parent = x;

	y->parent = x->parent;

	if (rbnode_is_root(x))
		tree->root = y;
	else if (rbnode_is_left(x))
		x->parent->left = y;
	else
		x->parent->right = y;

	y->left = x;
	x->parent = y;
}

static void rbtree_right_rotate(rbtree_t *tree, rbnode_t *y)
{
	rbnode_t *x;

	x = y->left;
	y->left = x->right;

	if (!rbnode_is_nil(x->right))
		x->right->parent = y;

	x->parent = y->parent;

	if (rbnode_is_root(y))
		tree->root = x;
	else if (rbnode_is_left(y))
		y->parent->left = x;
	else
		y->parent->right = x;

	x->right = y;
	y->parent = x;
}

static void rbtree_insert_fixup(rbtree_t *tree, rbnode_t *n)
{
	while (!rbnode_is_nil(n) && rbnode_is_red(n->parent)) {
		rbnode_t *uncle;
		if (rbnode_is_left(n->parent)) {
			uncle = n->parent->parent->right;
			if (rbnode_is_red(uncle)) {
				rbnode_set_black(n->parent);
				rbnode_set_black(uncle);
				rbnode_set_red(n->parent->parent);
				n = n->parent->parent;
			}
			else {
				if (rbnode_is_right(n)) {
					n = n->parent;
					rbtree_left_rotate(tree, n);
				}
				rbnode_set_black(n->parent);
				rbnode_set_red(n->parent->parent);
				rbtree_right_rotate(tree, n->parent->parent);
			}
		}
		else {
			uncle = n->parent->parent->left;
			if (rbnode_is_red(uncle)) {
				rbnode_set_black(n->parent);
				rbnode_set_black(uncle);
				rbnode_set_red(n->parent->parent);
				n = n->parent->parent;
			}
			else {
				if (rbnode_is_left(n)) {
					n = n->parent;
					rbtree_right_rotate(tree, n);
				}
				rbnode_set_black(n->parent);
				rbnode_set_red(n->parent->parent);
				rbtree_left_rotate(tree, n->parent->parent);
			}
		}
	}
	tree->root->color = rbnode_black;
}

/* binary tree insert */
static int btree_insert(rbtree_t *tree, rbnode_t *n)
{
	int cmp;
	rbnode_t *x, *y;

	n->left = n->right = rbnode_nil;

	y = x = tree->root;
	while (!rbnode_is_nil(x)) {
		y = x;
		cmp = tree->keycmp(n->key, x->key);
		if (cmp < 0)
			x = x->left;
		else if (cmp > 0)
			x = x->right;
		else {
			errno = EEXIST;
			return -1;
		}
	}

	n->parent = y;
	if (rbnode_is_nil(y))
		tree->root = n;
	else if (cmp < 0)
		y->left = n;
	else
		y->right = n;

	return 0;
}

int rbtree_insert(rbtree_t *tree, rbnode_t *n)
{

	if (btree_insert(tree, n) != 0)
		return -1;

	n->color = rbnode_red;

	rbtree_insert_fixup(tree, n);

	return 0;
}

rbnode_t *rbtree_lookup(rbtree_t *tree, void *key)
{
	rbnode_t *n = tree->root;
	int cmp;
	while (!rbnode_is_nil(n) && (cmp = tree->keycmp(n->key, key)) != 0) {
		if (cmp > 0)
			n = n->left;
		else
			n = n->right;
	}
	return n;
}

static rbnode_t *rbtree_successor(rbtree_t *tree, rbnode_t *n)
{
	if (!rbnode_is_nil(n->right)) {
		n = n->right;
		while (!rbnode_is_nil(n->left))
			n = n->left;
	}
	else {
		while (!rbnode_is_root(n) && rbnode_is_right(n))
			n = n->parent;
		n = n->parent;
	}
	return n;
}

static rbnode_t *rbtree_predecessor(rbtree_t *tree, rbnode_t *n)
{
	if (!rbnode_is_nil(n->left)) {
		n = n->left;
		while (!rbnode_is_nil(n->right))
			n = n->right;
	}
	else {
		while (!rbnode_is_root(n) && rbnode_is_left(n))
			n = n->parent;
		n = n->parent;
	}
	return n;
}

static void rbtree_delete_fixup(rbtree_t *tree, rbnode_t *x, rbnode_t *p)
{
	rbnode_t *b;

	while (!rbnode_is_nil(p) && rbnode_is_black(x)) {
		if (rbnode_is_nil(x) || rbnode_is_left(x)) {
			b = p->right;
			if (rbnode_is_red(b)) {
				rbnode_set_black(b);
				rbnode_set_red(p);
				rbtree_left_rotate(tree, p);
				b = p->right;
			}
			if (rbnode_is_nil(b) || (rbnode_is_black(b->right) && rbnode_is_black(b->left))) {
				if (!rbnode_is_nil(b))
					rbnode_set_red(b);
				x = p;
                p = p->parent;
			}
			else {
				if (rbnode_is_black(b->right)) {
					rbnode_set_black(b->left);
					rbnode_set_red(b);
					rbtree_right_rotate(tree, b);
					b = p->right;
				}
				else
					rbnode_set_black(b->right);
				b->color = p->color;
				rbnode_set_black(p);
				rbtree_left_rotate(tree, p);
				x = tree->root;
                p = rbnode_nil;
			}
		}
		else {
			b = p->left;
			if (rbnode_is_red(b)) {
				rbnode_set_black(b);
				rbnode_set_red(p);
				rbtree_right_rotate(tree, p);
				b = p->left;
			}
			if (rbnode_is_nil(b) || (rbnode_is_black(b->right) && rbnode_is_black(b->left))) {
				if (!rbnode_is_nil(b))
					rbnode_set_red(b);
				x = p;
                p = p->parent;
			}
			else {
				if (rbnode_is_black(b->left)) {
					rbnode_set_black(b->right);
					rbnode_set_red(b);
					rbtree_left_rotate(tree, b);
					b = p->left;
				}
				else
					rbnode_set_black(b->left);
				b->color = p->color;
				rbnode_set_black(p);
				rbtree_right_rotate(tree, p);
				x = tree->root;
                p = rbnode_nil;
			}
		}
	}
    if (!rbnode_is_nil(x))
        rbnode_set_black(x);
}

void rbtree_delete(rbtree_t *tree, rbnode_t *n)
{
	rbnode_t *x, *y;

	y = rbnode_is_leaf(n) ? n : rbtree_successor(tree, n);
	x = rbnode_is_nil(y->left) ? y->right : y->left;

	if (!rbnode_is_nil(x))
		x->parent = y->parent;

	if (rbnode_is_root(y))
		tree->root = x;
	else if (rbnode_is_left(y))
		y->parent->left = x;
	else
		y->parent->right = x;

	if (y != n) {

        if (rbnode_is_black(y))
            rbtree_delete_fixup(tree, x, y->parent);

		y->left = n->left;
		y->right = n->right;
		y->parent = n->parent;
		y->color = n->color;

		n->left->parent = y;
		n->right->parent = y;

		if (rbnode_is_root(n))
			tree->root = y;
		else if (rbnode_is_left(n))
			n->parent->left = y;
		else
			n->parent->right = y;
	}
    else if (rbnode_is_black(y))
        rbtree_delete_fixup(tree, x, y->parent);
}

static int preorder(rbtree_t *tree, rbnode_t *n,
	rbtree_iterate_func_t iteration, void *state)
{
	if (!rbnode_is_nil(n)) {
		int r;
		if ((r = (*iteration)(tree, n, state)) != 0)
			return r;
		if ((r = preorder(tree, n->left, iteration, state)) != 0)
			return r;
		if ((r = preorder(tree, n->right, iteration, state)) != 0)
			return r;
	}
	return 0;
}

int rbtree_foreach_preorder(rbtree_t *tree,
	rbtree_iterate_func_t iteration, void *state)
{
	return preorder(tree, tree->root, iteration, state);
}

static int inorder(rbtree_t *tree, rbnode_t *n,
	rbtree_iterate_func_t iteration, void *state)
{
	if (!rbnode_is_nil(n)) {
		int r;
		if ((r = inorder(tree, n->left, iteration, state)) != 0)
			return r;
		if ((r = (*iteration)(tree, n, state)) != 0)
			return r;
		if ((r = inorder(tree, n->right, iteration, state)) != 0)
			return r;
	}
	return 0;
}

int rbtree_foreach_inorder(rbtree_t *tree,
	rbtree_iterate_func_t iteration, void *state)
{
	return inorder(tree, tree->root, iteration, state);
}

static int postorder(rbtree_t *tree, rbnode_t *n,
	rbtree_iterate_func_t iteration, void *state)
{
	if (!rbnode_is_nil(n)) {
		int r;
		if ((r = postorder(tree, n->left, iteration, state)) != 0)
			return r;
		if ((r = postorder(tree, n->right, iteration, state)) != 0)
			return r;
		if ((r = (*iteration)(tree, n, state)) != 0)
			return r;
	}
	return 0;
}

int rbtree_foreach_postorder(rbtree_t *tree,
	rbtree_iterate_func_t iteration, void *state)
{
	return postorder(tree, tree->root, iteration, state);
}

static int printorder(rbtree_t *tree, rbnode_t *n,
	rbtree_iterate_func_t iteration, void *state)
{
	int r;
	if (!rbnode_is_nil(n->left)) {
		if ((r = (*iteration)(tree, n->left, state)) != 0)
			return r;
	}
	if (!rbnode_is_nil(n->right)) {
		if ((r = (*iteration)(tree, n->right, state)) != 0)
			return r;
	}
	if (!rbnode_is_nil(n->left)) {
		if ((r = printorder(tree, n->left, iteration, state)) != 0)
			return r;
	}
	if (!rbnode_is_nil(n->right)) {
		if ((r = printorder(tree, n->right, iteration, state)) != 0)
			return r;
	}
	return 0;
}

int rbtree_foreach_print(rbtree_t *tree,
	rbtree_iterate_func_t iteration, void *state)
{
	if (!rbnode_is_nil(tree->root)) {
		int r;
		if ((r = (*iteration)(tree, tree->root, state)) != 0)
			return r;
		return printorder(tree, tree->root, iteration, state);
	}
	return 0;
}

