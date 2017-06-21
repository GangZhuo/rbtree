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
#include <string.h>
#include <stddef.h>
#include <ctype.h>
#include <assert.h>
#include <errno.h>

#include "../rbtree.h"
#include "asc16.h"
#include "bitmap.h"

#ifdef _WIN32
#define strcasecmp stricmp
#endif

#define COMMAND_MAX		8
#define OPTION_MAX		8

typedef struct mesh_t {
	int w, h;
	int margin;
	int r;
	rbpoint_t *points;
	int value;
} mesh_t;

typedef struct array_t {
	int entries;
	int cap;
	int *array;
} array_t;

static int keycmp(void *a, void *b);

static int entries = 0;
static rbtree_t tree = RBTREE_INIT(keycmp);
static asc16_t asc16 = { 0 };

#define free_rbtree(rb) rbtree_foreach_postorder((rb), (free_node), NULL)

static void help()
{
	printf(
		"Red-Black-Tree\n"
		"  commands:\n"
		"    insert <integer>       insert integer value(s),\n"
		"                           the value should be positive.\n"
		"    delete <integer>       delete value(s).\n"
		"    find   <integer>       find node(s).\n"
		"    clear                  clear red-black-tree.\n"
		"    print  [pre|in|post]   print red-black-tree.\n"
		"                             options:\n"
		"                               pre  - preorder.\n"
		"                               in   - inorder.\n"
		"                               post - postorder.\n"
		"    load   <path>          load from file.\n"
		"    save   <path>          save to file.\n"
		"    bmp    [nonil] <path>  save as bitmap.\n"
		"    help                   print help.\n"
		"    quit                   quit.\n"
	);

}

static void clean_input_buffer()
{
	int ch;
	while ((ch = getchar()) != '\n' && ch != EOF);
}

static void skip_blank()
{
	int ch;
	while ((ch = getchar()) != '\n' && ch != EOF && isblank(ch));
	if (ch == EOF)
		ch = '\n';
	ungetc(ch, stdin);
}

static int next_arg(char *buf, int bufsize)
{
	int i, ch;
	skip_blank();
	for (i = 0, ch = getchar();
		ch != '\n' && ch != EOF && i < bufsize;
		i++, ch = getchar()) {
		if (isblank(ch))
			break;
		buf[i] = (char)ch;
	}
	if (i < bufsize)
		buf[i] = '\0';
	if (i > 0) {
		if (ch == '\n' || ch == EOF)
			ungetc('\n', stdin);
	}
	return i;
}

static int next_value(int *pv)
{
	int nread = 0, negative = 0, ch, v = 0;
	while ((ch = getchar()) != '\n' && ch != EOF) {
		if (isblank(ch)) {
			if (nread == 0)
				continue;
			else
				break;
		}
		else if (ch == '-' || ch == '+') {
			if (negative || nread > 0) {
				clean_input_buffer();
				errno = EINVAL;
				return -1;
			}
			negative = ((ch == '-') ? -1 : 1);
		}
		else if (isdigit(ch)) {
			v *= 10;
			v += ch - '0';
			nread++;
		}
		else {
			clean_input_buffer();
			errno = EINVAL;
			return -1;
		}
	}
	if (nread > 0) {
		if (negative == -1)
			v = -v;
		*pv = v;
		if (ch == '\n' || ch == EOF)
			ungetc('\n', stdin);
		return 0;
	}
	errno = EOF;
	return -1;
}

static int read_values(array_t *arr)
{
	int r, v;
	while ((r = next_value(&v)) == 0) {
		if (arr->cap <= arr->entries) {
			int cap = arr->cap << 1, *p;
			if (cap == 0)
				cap = 8;
			p = realloc(arr->array, cap * sizeof(int));
			if (p == NULL)
				return -1;
			arr->cap = cap;
			arr->array = p;
		}
		arr->array[arr->entries++] = v;
	}
	if (r && errno == EOF)
		return 0;
	return r;
}

static int keycmp(void *a, void *b)
{
	int x = ptoi(a), y = ptoi(b);
	return (x - y);
}

static int print_node(rbtree_t *tree, rbnode_t *n, void *state)
{
	rbpoint_t *point = container_of(n, rbpoint_t, rbentry);
	if (n->parent == NULL) {
		printf("       %4d %5s       %4d %4d\n", ptoi(n->key),
			n->color == rbnode_red ? "r" : "b",
			point->x, point->y);
	}
	else {
		printf("%6d %4d %5s %5s %4d %4d\n",
			ptoi(n->parent->key),
			ptoi(n->key),
			n->color == rbnode_red ? "r" : "b",
			n->parent->right == n ? "r" : "l",
			point->x, point->y);
	}
	return 0;
}

static void free_point(rbpoint_t *point)
{
	free(point);
}

static int free_node(rbtree_t *tree, rbnode_t *n, void *state)
{
	rbpoint_t *point = container_of(n, rbpoint_t, rbentry);
	free_point(point);
	return 0;
}

static void insert(int v)
{
	rbpoint_t *point;

	if (v < 0) {
		printf("insert %d error: require positive integer.\n", v);
		return;
	}

	point = malloc(sizeof(rbpoint_t));
	if (point == NULL) {
		printf("insert %d error: alloc.\n", v);
		return;
	}
	memset(point, 0, sizeof(rbpoint_t));
	point->rbentry.key = itop(v);
	if (rbtree_insert(&tree, &point->rbentry) == 0) {
		printf("insert %d.\n", v);
		entries++;
	}
	else {
		printf("insert %d error: exist.\n", v);
	}
}

static void delete(int v)
{
	rbnode_t *n;
	rbpoint_t *point;
	n = rbtree_lookup(&tree, itop(v));
	if (n == NULL) {
		printf("%d not exist.\n", v);
	}
	else {
		point = container_of(n, rbpoint_t, rbentry);
		rbtree_delete(&tree, n);
		printf("delete %d\n", ptoi(n->key));
		free_point(point);
		entries--;
	}
}

static void lookup(int v)
{
	rbnode_t *n;
	n = rbtree_lookup(&tree, itop(v));
	if (n == NULL) {
		printf("%d not exist.\n", v);
	}
	else {
		print_node(&tree, n, NULL);
	}
}

static void do_insert(array_t *values)
{
	int i;
	for (i = 0; i < values->entries; i++) {
		insert(values->array[i]);
	}
	printf("%d entries.\n", entries);
}

static void do_delete(array_t *values)
{
	int i;
	for (i = 0; i < values->entries; i++) {
		delete(values->array[i]);
	}
	printf("%d entries.\n", entries);
}

static void do_lookup(array_t *values)
{
	int i;
	printf("parent node color   dir    x    y\n");
	for (i = 0; i < values->entries; i++) {
		lookup(values->array[i]);
	}
}

static void do_print(const char *opt)
{
	printf("parent node color   dir    x    y\n");
	if (opt == NULL)
		rbtree_foreach_print(&tree, print_node, NULL);
	else if (strcmp("pre", opt) == 0)
		rbtree_foreach_preorder(&tree, print_node, NULL);
	else if (strcmp("in", opt) == 0)
		rbtree_foreach_inorder(&tree, print_node, NULL);
	else if (strcmp("post", opt) == 0)
		rbtree_foreach_postorder(&tree, print_node, NULL);
	else {
		if (strlen(opt) != 0) {
			printf("warning: unknow print option \"%s\", print in default mode.\n", opt);
		}
		rbtree_foreach_print(&tree, print_node, NULL);
	}

	printf("%d entries.\n", entries);
}

static void do_load(const char *path)
{
	FILE *pf;
	char buf[1024];
	int r, i, ch, v;

	printf("Loading from %s ...\n", path);

	pf = fopen(path, "rb");
	if (pf == NULL) {
		printf("Can't open the file %s.\n", path);
		return;
	}

	v = -1;

	while ((r = fread(buf, 1, sizeof(buf), pf)) > 0) {
		for (i = 0; i < r; i++) {
			ch = buf[i];
			if (isspace(ch)) {
				if (v >= 0) {
					insert(v);
					v = -1;
				}
			}
			else if (!isdigit(ch)) {
				printf("Invalid file format %s.\n", path);
				fclose(pf);
				return;
			}
			else {
				if (v == -1)
					v = ch - '0';
				else {
					v *= 10;
					v += ch - '0';
				}
			}
		}
	}

	if (v >= 0) {
		insert(v);
		v = -1;
	}

	fclose(pf);

	printf("%d entries.\n", entries);
}

struct save_state {
	FILE *pf;
	int i;
};

static int save_node(rbtree_t *tree, rbnode_t *n, void *state)
{
	struct save_state *ss = state;
	char *key;
	int r;

	if (ss->i > 0) {
		if ((ss->i % 8) == 0) {
			if (fwrite("\n", 1, 1, ss->pf) != 1)
				return -1;
		}
		else {
			if (fwrite("\t", 1, 1, ss->pf) != 1)
				return -1;
		}
	}

    key = keytostr(n->key);

	r = strlen(key);

	if (fwrite(key, 1, r, ss->pf) != r)
		return -1;

	ss->i++;

	return 0;
}

static void do_save(const char *path)
{
	FILE *pf;
	int r;
	struct save_state ss = { 0 };

	printf("Saving to %s ...\n", path);

	pf = fopen(path, "wb");
	if (pf == NULL) {
		printf("Can't open the file %s.\n", path);
		return;
	}

	ss.pf = pf;
	ss.i = 0;

	r = rbtree_foreach_inorder(&tree, save_node, &ss);
	if (r != 0)
		printf("Failed to write file %s.\n", path);

	fclose(pf);

	printf("%d entries.\n", entries);
}

static void do_bmp(const char *path, int nil)
{
	bitmap_t *bitmap;
	printf("Creating bitmap...\n");

	bitmap = rbtree_save_as_bitmap(&tree, &asc16, nil);

	if (bitmap == NULL) {
		printf("Failed to create bitmap.\n");
		return;
	}

	if (path && strlen(path) > 0) {
		if (bitmap_save(bitmap, path) != 0) {
			bitmap_free(bitmap);
			printf("Failed to save bitmap to file %s.\n", path);
			return;
		}
	}
	else {
		if (bitmap_print(bitmap, stdout) != 0) {
			bitmap_free(bitmap);
			printf("Failed to print bitmap to \"stdout\".\n");
			return;
		}
		printf("\n");
	}

	bitmap_free(bitmap);

	printf("Done. %d entries.\n", entries);
}

static void do_clean()
{
	printf("Cleaning...\n");
	free_rbtree(&tree);
	rbtree_init(&tree, keycmp);
	entries = 0;
	printf("%d entries.\n", entries);
}

static void run()
{
	char cmd[COMMAND_MAX];
	int r, suc;
	array_t values = { 0 };

	while (1) {
		printf("> ");
		r = next_arg(cmd, COMMAND_MAX);
		if (r == COMMAND_MAX)
			cmd[COMMAND_MAX - 1] = '\0';
		if (r == 0) {
			continue;
		}
		else if (strcmp("insert", cmd) == 0 || strcmp("i", cmd) == 0) {
			values.entries = 0;
			r = read_values(&values);
			if (r == 0) {
				if (values.entries == 0) {
					printf("No value(s).\n");
				}
				else {
					do_insert(&values);
				}
			}
			else {
				printf("Invalid value(s), the value(s) should be a positive integer.\n");
			}
		}
		else if (strcmp("delete", cmd) == 0 || strcmp("d", cmd) == 0) {
			values.entries = 0;
			if (read_values(&values) == 0) {
				if (values.entries == 0) {
					printf("No value(s).\n");
				}
				else {
					do_delete(&values);
				}
			}
			else {
				printf("Invalid value(s), the value(s) should be a positive integer.\n");
			}
		}
		else if (strcmp("find", cmd) == 0 || strcmp("f", cmd) == 0) {
			values.entries = 0;
			if (read_values(&values) == 0) {
				if (values.entries == 0) {
					printf("No value(s).\n");
				}
				else {
					do_lookup(&values);
				}
			}
			else {
				printf("Invalid value(s), the value(s) should be a positive integer.\n");
			}
		}
		else if (strcmp("clear", cmd) == 0 || strcmp("c", cmd) == 0) {
			clean_input_buffer();
			do_clean();
		}
		else if (strcmp("print", cmd) == 0 || strcmp("p", cmd) == 0) {
			char opt[OPTION_MAX];
			while ((r = next_arg(opt, OPTION_MAX)) != 0) {
				if (r == OPTION_MAX)
					opt[OPTION_MAX - 1] = '\0';
				do_print(opt);
			}
		}
		else if (strcmp("load", cmd) == 0) {
			char path[FILENAME_MAX];
			suc = 0;
			while ((r = next_arg(path, FILENAME_MAX)) != 0) {
				if (r == FILENAME_MAX)
					path[FILENAME_MAX - 1] = '\0';
				do_load(path);
				suc++;
			}
			if (suc == 0)
				printf("Invalid argument: require path.\n");
		}
		else if (strcmp("save", cmd) == 0) {
			char path[FILENAME_MAX];
			suc = 0;
			while ((r = next_arg(path, FILENAME_MAX)) != 0) {
				if (r == FILENAME_MAX)
					path[FILENAME_MAX - 1] = '\0';
				do_save(path);
				suc++;
			}
			if (suc == 0)
				printf("Invalid argument: require path.\n");
		}
		else if (strcmp("bmp", cmd) == 0) {
			char path[FILENAME_MAX], *p;
			int nil = 0;
			suc = 0;
			while ((r = next_arg(path, FILENAME_MAX)) != 0) {
				if (r == FILENAME_MAX)
					path[FILENAME_MAX - 1] = '\0';

				if (nil == 0) {
					if (strcmp("nonil", path) == 0) {
						nil = 1;
						continue;
					}
					else
						nil = -1;
				}
				
				p = path;
				while (*p && (*p) != '.')
					p++;
				if (*p == '.' && strcasecmp(".bmp", p)) {
					printf("Invalid path \"%s\": require \".bmp\" file.\n", path);
					clean_input_buffer();
					break;
				}
				if (*p == '\0') {
					if (r >= FILENAME_MAX - 4) {
						printf("Invalid path \"%s\": too long.\n", path);
						clean_input_buffer();
						break;
					}
					strcat(path, ".bmp");
				}

				do_bmp(path, nil == 1 ? 0 : 1);
				suc++;
			}
			if (suc == 0)
				do_bmp(NULL, nil == 1 ? 0 : 1);
		}
		else if (strcmp("help", cmd) == 0 || strcmp("?", cmd) == 0 || strcmp("h", cmd) == 0) {
			help();
			clean_input_buffer();
		}
		else if (strcmp("quit", cmd) == 0 || strcmp("exit", cmd) == 0 || strcmp("q", cmd) == 0) {
			clean_input_buffer();
			break;
		}
		else {
			clean_input_buffer();
			printf("wrong command. use \"help\" to print help.\n");
		}
	}

	free(values.array);

	free_rbtree(&tree);

}

int main(int argc, char **argv)
{
	if (asc16_load(&asc16, "ASC16")) {
		printf("Failed to read \"ASC16\"\n");
		return EXIT_FAILURE;
	}

	if (argc > 1) {
		int i;
		char *cmd;
		for (i = 1; i < argc; i++) {
			cmd = argv[i];
			if (strcmp("insert", cmd) == 0 || strcmp("i", cmd) == 0) {
				i++;
				if (i < argc) {
					int v = atoi(argv[i]);
					if (v < 1) {
						printf("Invalid value, the value(s) should be a positive integer.\n");
						return EXIT_FAILURE;
					}
					else {
						insert(v);
					}
				}
				else {
					printf("Invalid arguments, usage: rbtree [insert 1] [insert 2]...\n");
					return EXIT_FAILURE;
				}
			}
			else if (strcmp("delete", cmd) == 0 || strcmp("d", cmd) == 0) {
				i++;
				if (i < argc) {
					int v = atoi(argv[i]);
					if (v < 1) {
						printf("Invalid value, the value(s) should be a positive integer.\n");
						return EXIT_FAILURE;
					}
					else {
						delete(v);
					}
				}
				else {
					printf("Invalid arguments, usage: rbtree [delete 1] [delete 2]...\n");
					return EXIT_FAILURE;
				}
			}
			else if (strcmp("find", cmd) == 0 || strcmp("f", cmd) == 0) {
				i++;
				if (i < argc) {
					int v = atoi(argv[i]);
					if (v < 1) {
						printf("Invalid value, the value(s) should be a positive integer.\n");
						return EXIT_FAILURE;
					}
					else {
						lookup(v);
					}
				}
				else {
					printf("Invalid arguments, usage: rbtree [find 1] [find 2]...\n");
					return EXIT_FAILURE;
				}
			}
			else if (strcmp("clear", cmd) == 0 || strcmp("c", cmd) == 0) {
				do_clean();
			}
			else if (strcmp("print", cmd) == 0 || strcmp("p", cmd) == 0) {
				const char *opt = "";
				if ((i + 1) < argc) {
					if (strcmp("pre", argv[i + 1]) == 0 ||
						strcmp("in", argv[i + 1]) == 0 ||
						strcmp("post", argv[i + 1]) == 0) {
						opt = argv[++i];
					}
				}
				do_print(opt);
			}
			else if (strcmp("load", cmd) == 0) {
				i++;
				if (i < argc) {
					do_load(argv[i]);
				}
				else {
					printf("Invalid arguments, usage: rbtree [load <path>] [load <path>]...\n");
					return EXIT_FAILURE;
				}
			}
			else if (strcmp("save", cmd) == 0) {
				i++;
				if (i < argc) {
					do_save(argv[i]);
				}
				else {
					printf("Invalid arguments, usage: rbtree [save <path>] [save <path>]...\n");
					return EXIT_FAILURE;
				}
			}
			else if (strcmp("bmp", cmd) == 0) {
				const char *path = NULL, *p;
				int nil = 1;

				if ((i + 1) < argc) {
					if (nil == 0) {
						if (strcmp("nonil", argv[i + 1]) == 0) {
							nil = 0;
							i++;
						}
					}
					if ((i + 1) < argc) {
						path = argv[i + 1];
						p = path;
						while (*p && (*p) != '.')
							p++;
						if (*p == '.' && strcasecmp(".bmp", p) == 0)
							i++;
						else
							path = NULL;
					}
					do_bmp(path, nil);
				}
			}
			else if (strcmp("help", cmd) == 0 || strcmp("?", cmd) == 0 || strcmp("h", cmd) == 0) {
				help();
			}
			else if (strcmp("quit", cmd) == 0 || strcmp("exit", cmd) == 0 || strcmp("q", cmd) == 0) {
				return EXIT_SUCCESS;
			}
			else {
				printf("wrong command \"%s\". use \"help\" to print help.\n", cmd);
				return EXIT_FAILURE;
			}
		}
		printf("\n");
	}

	help();
	printf("\n");

	run();

	asc16_free(&asc16);

	return EXIT_SUCCESS;
}

