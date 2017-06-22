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

#include "piconvert.h"
#include "../rbtree.h"

/* free rbtree. */
#define free_rbtree(rb) rbtree_foreach_postorder((rb), free_node, NULL)

/* compare two key. */
static int keycmp(void *a, void *b)
{
	int x = ptoi(a), y = ptoi(b);
	return (x - y);
}

static int print_node(rbtree_t *tree, rbnode_t *n, void *state)
{
	if (rbnode_is_root(n)) {
		printf("       %4d %5s\n",
			ptoi(n->key),
			rbnode_is_red(n) ? "red" : "black");
	}
	else {
		printf("%6d %4d %5s %5s\n",
			ptoi(n->parent->key),
			ptoi(n->key),
			rbnode_is_red(n) ? "red" : "black",
			rbnode_is_right(n) ? "right" : "left");
	}
	return 0;
}

static int free_node(rbtree_t *tree, rbnode_t *n, void *state)
{
	free(n);
	return 0;
}

int main(int argc, char **argv)
{
	rbtree_t tree = RBTREE_INIT(keycmp);
	int i;
	rbnode_t *n;
	
	/* insert */
	for(i = 0; i < 10; i++) {
		n = malloc(sizeof(rbnode_t));
		n->key = itop(i);
		rbtree_insert(&tree, n);
	}
	
	/* lookup */
	n = rbtree_lookup(&tree, itop(5));
	if (n)
		printf("Find %d.\n", ptoi(n->key));
	else
		printf("5 not exist.\n");
	
	/* delete */
	rbtree_delete(&tree, n);
	
	/* print */
	printf("parent node color   dir\n");
	rbtree_foreach_inorder(&tree, print_node, NULL);
	
	free_rbtree(&tree);
	
	return 0;
}

