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

#ifndef BITMAP_H_
#define BITMAP_H_

#include <stddef.h>
#include "../rbtree.h"
#include "asc16.h"
#include "dllist.h"

#ifdef __cplusplus
extern "C" {
#endif

#define COLOR_GRAY			0xff333333
#define COLOR_RED			0xffff3333
#define COLOR_BLACK			0xff333333
#define COLOR_WHITE			0xffffffff

typedef struct rbpoint_t {
	rbnode_t rbentry;
	dlitem_t dlentry;
	int x, y;
} rbpoint_t;

typedef int color_t;

typedef struct bitmap_t {
	int w, h;
	color_t *colors;
} bitmap_t;

union piconvert_t {
	int i;
	void *p;
};

static inline int ptoi(void *p)
{
	union piconvert_t c;
	c.p = p;
	return c.i;
}

static inline void *itop(int i)
{
	union piconvert_t c;
	c.i = i;
	return c.p;
}

bitmap_t *rbtree_save_as_bitmap(rbtree_t *tree, asc16_t *asc16, int nil);

int bitmap_init(bitmap_t *bitmap, int w, int h, color_t bg);

void bitmap_free(bitmap_t *bitmap);

/* convert key to string, it's thread-unsafe. */
static inline char *keytostr(void *key)
{
	static char buf[128];
	snprintf(buf, sizeof(buf), "%d", ptoi(key));
	return buf;
}

int bitmap_draw_point(bitmap_t *bitmap, int x, int y, color_t color);

int bitmap_draw_line(bitmap_t *bitmap, int x0, int y0, int x1, int y1, color_t color);

int bitmap_draw_circle(bitmap_t *bitmap, int x, int y, int r, int fill, color_t color);

int bitmap_draw_rect(bitmap_t *bitmap, int x0, int y0, int x1, int y1, int fill, color_t color);

int bitmap_draw_char(bitmap_t *bitmap, int ch, int x, int y, color_t color, asc16_t *asc16);

int bitmap_draw_string(bitmap_t *bitmap, char *str, int x, int y, color_t color, asc16_t *asc16);

int bitmap_print(bitmap_t *bitmap, FILE *pf);

int bitmap_save(bitmap_t *bitmap, const char *filename);

#ifndef offsetof
#define offsetof(s,m) ((size_t)&(((s*)0)->m))
#endif

#ifndef container_of
#define container_of(field, struct_type, field_name) \
	((struct_type *)(((char *)(field)) - offsetof(struct_type, field_name)))
#endif

#ifdef __cplusplus
}
#endif

#endif
