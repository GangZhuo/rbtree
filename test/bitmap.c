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
#include <errno.h>
#include <math.h>

#include "bitmap.h"

#define CIRCLE_PADDING		2	/* 圆周和数字的间隙 */
#define GOLDEN_RATIO		(1.618f) /* 黄金分割比 */
#define LINE_COLOR			COLOR_GRAY

#define NIL_KEY				(-1)

typedef struct rbnil_t {
	rbpoint_t base;
	dlitem_t dlentry;
} rbnil_t;

typedef struct draw_state_t {
	int minX;
	int maxValue;
	int radius;
	int advanceX, advanceY;

	int bitmapW, bitmapH;

	dllist_t *rows;
	int rowcount;

	int draw_nil;
	dllist_t nils;

	bitmap_t *bitmap;
	asc16_t *asc16;
} draw_state_t;

static rbnil_t *new_nil()
{
	rbnil_t *nil;

	nil = malloc(sizeof(rbnil_t));
	if (nil == NULL)
		return NULL;
	memset(nil, 0, sizeof(rbnil_t));
	nil->base.rbentry.key = itop(NIL_KEY);
	return nil;
}

void free_nil(rbnil_t *nil)
{
	free(nil);
}

static int add_row(draw_state_t *st, int rowindex, dlitem_t *item)
{
	if (st->rowcount <= rowindex) {
		dllist_t *rows;
		int i;
		rows = malloc((rowindex + 1) * sizeof(dllist_t));
		if (rows == NULL)
			return -1;
		for (i = 0; i < st->rowcount; i++) {
			dllist_init(rows + i);
			dllist_add_list_to_tail(rows + i, st->rows + i);
		}
		for (; i <= rowindex; i++) {
			dllist_init(rows + i);
		}
		free(st->rows);
		st->rows = rows;
		st->rowcount = rowindex + 1;
	}
	dllist_add(st->rows + rowindex, item);
	return 0;
}

static void free_draw_state(draw_state_t *st)
{
	dlitem_t *curr, *next;
	rbnil_t *nil;

	if (st) {
		free(st->rows);
		dllist_foreach(&st->nils, curr, next, rbnil_t, nil, dlentry) {
			dllist_remove(curr);
			if (rbnode_is_left(&nil->base.rbentry))
				nil->base.rbentry.parent->left = rbnode_nil;
			else if (rbnode_is_right(&nil->base.rbentry))
				nil->base.rbentry.parent->right = rbnode_nil;
			free_nil(nil);
		}
	}
}

static int predraw(rbtree_t *tree, rbnode_t *n, void *state)
{
	draw_state_t *st = state;
	rbpoint_t *a = container_of(n, rbpoint_t, rbentry);
	if (n->parent == NULL) {
		st->minX = a->x = a->y = 0;
		st->maxValue = ptoi(n->key);
	}
	else {
		rbpoint_t *p = container_of(n->parent, rbpoint_t, rbentry);

		if (rbnode_is_right(n)) {
			a->x = p->x + 1;
			if (st->maxValue < ptoi(n->key))
				st->maxValue = ptoi(n->key);
		}
		else {
			a->x = p->x - 1;
			if (st->minX > a->x)
				st->minX = a->x;
		}
		a->y = p->y + 1;
	}

	if (st->draw_nil && ptoi(n->key) != NIL_KEY) {
		if (rbnode_is_nil(n->left)) {
			rbnil_t *nil = new_nil();
			if (nil == NULL)
				return -1;
			nil->base.rbentry.parent = n;
			n->left = &nil->base.rbentry;
			dllist_add(&st->nils, &nil->dlentry);
		}

		if (rbnode_is_nil(n->right)) {
			rbnil_t *nil = new_nil();
			if (nil == NULL)
				return -1;
			nil->base.rbentry.parent = n;
			n->right = &nil->base.rbentry;
			dllist_add(&st->nils, &nil->dlentry);
		}
	}


	return add_row(st, a->y, &a->dlentry);
}

static inline int draw_lines(draw_state_t *st)
{
	int i;
	dllist_t *list;
	dlitem_t *curr, *next;
	rbpoint_t *point;

	for (i = 0; i < st->rowcount; i++) {
		list = st->rows + i;
		dllist_foreach(list, curr, next, rbpoint_t, point, dlentry) {
			if (!rbnode_is_root(&point->rbentry)) {
				rbpoint_t *parent = container_of(point->rbentry.parent, rbpoint_t, rbentry);
				if (bitmap_draw_line(st->bitmap,
					parent->x, parent->y,
					point->x, point->y, LINE_COLOR) != 0)
					return -1;
			}
		}
	}

	return 0;
}

static inline int draw_key(draw_state_t *st, rbpoint_t *point)
{
	int w;
	char *key;

	key = keytostr(point->rbentry.key);

	w = strlen(key) * ASC16_ADVANCE;

	if (bitmap_draw_string(st->bitmap, key,
		point->x - w / 2 + 1, point->y - ASC16_GYLPH_HEIGHT / 2 + 1,
		COLOR_WHITE, st->asc16) != 0)
		return -1;

	return 0;
}

static inline int draw_nil(bitmap_t *bitmap, int x, int y, asc16_t *asc16)
{
    int x0, y0, x1, y1;
    x0 = x - (3 * ASC16_GYLPH_WIDTH / 2) - CIRCLE_PADDING;
    y0 = y - (ASC16_GYLPH_HEIGHT / 2);
    x1 = x0 + 3 * ASC16_GYLPH_WIDTH + CIRCLE_PADDING + CIRCLE_PADDING;
    y1 = y0 + ASC16_GYLPH_HEIGHT;
    if (bitmap_draw_rect(bitmap, x0, y0, x1, y1, 1, COLOR_BLACK) != 0)
        return -1;

    if (bitmap_draw_string(bitmap, "NIL", x0 + CIRCLE_PADDING + 1, y0 + 1, COLOR_WHITE, asc16) != 0)
        return -1;

    return 0;
}

static inline int draw_rbnode(draw_state_t *st, rbpoint_t *point)
{
	if (ptoi(point->rbentry.key) == NIL_KEY) {
        return draw_nil(st->bitmap, point->x, point->y, st->asc16);
	}
	else {
		if (bitmap_draw_circle(st->bitmap,
			point->x, point->y,
			st->radius, 1,
			rbnode_is_red(&point->rbentry) ? COLOR_RED : COLOR_BLACK) != 0)
			return -1;

		if (draw_key(st, point) != 0)
			return -1;
	}

	return 0;
}

static inline int draw_rbnodes(draw_state_t *st)
{
	int i;
	dllist_t *list;
	dlitem_t *curr, *next;
	rbpoint_t *point;

	for (i = 0; i < st->rowcount; i++) {
		list = st->rows + i;
		dllist_foreach(list, curr, next, rbpoint_t, point, dlentry) {
			if (draw_rbnode(st, point) != 0)
				return -1;
		}
	}

	return 0;
}

static inline int draw_rbtree(draw_state_t *st)
{
	if (draw_lines(st) != 0)
		return -1;

	if (draw_rbnodes(st) != 0)
		return -1;

	return 0;
}

static int get_radius(int value)
{
	int r = 1;

	value /= 10;
	while (value != 0) {
		r++;
		value /= 10;
	}

	if (r < 3)
		r = 3;

	r *= ASC16_ADVANCE;
	r /= 2;

	r += CIRCLE_PADDING;

	return r;
}

static void move_x_fixup(draw_state_t *st, rbpoint_t *point)
{
	rbnode_t *n;
	rbpoint_t *m;
	dllist_t *list;
	int x;
	n = rbnode_is_nil(point->rbentry.left) ?
		point->rbentry.right :
		point->rbentry.left;
	if (!rbnode_is_nil(n)) {
		m = container_of(n, rbpoint_t, rbentry);
		if (m->x < point->x - 1) {
			x = point->x - 1 - m->x;
			m->x += x;
			list = st->rows + m->y;
			while (m->dlentry.next != &list->head) {
				m = container_of(m->dlentry.next, rbpoint_t, dlentry);
				m->x += x;
				move_x_fixup(st, m);
			}
		}
	}
}

static int calc_xy(draw_state_t *st)
{
	int i, w, h, x;
	dllist_t *list;
	dlitem_t *currItem, *nextItem;
	rbpoint_t *curr, *prev, *left, *right;

	st->radius = get_radius(st->maxValue);
	st->advanceX = (st->radius + CIRCLE_PADDING) * 2;
	st->advanceY = (int)((float)st->advanceX * GOLDEN_RATIO);

	w = 0;
	/* 重底往上扫描，使得节点不重叠 */
	for (i = st->rowcount - 1; i >= 0; i--) {
		list = st->rows + i;
		x = -st->minX;
		dllist_foreach(list, currItem, nextItem, rbpoint_t, curr, dlentry) {

			if (!dllist_is_start(list, currItem->prev)) {
				prev = container_of(currItem->prev, rbpoint_t, dlentry);
				if ((curr->x + x) <= prev->x)
					x += prev->x - curr->x - x + 1;
				if (!rbnode_is_nil(curr->rbentry.left)) {
					left = container_of(curr->rbentry.left, rbpoint_t, rbentry);
					if ((curr->x + x) <= left->x)
						x += left->x - curr->x - x + 1;
				}
			}

			curr->x += x;

			move_x_fixup(st, curr);
		}
	}

	w = 0;
	h = 0;
	for (i = st->rowcount - 1; i >= 0; i--) {
		list = st->rows + i;
		dllist_foreach(list, currItem, nextItem, rbpoint_t, curr, dlentry) {

			if (!rbnode_is_leaf(&curr->rbentry)) {
				left = container_of(curr->rbentry.left, rbpoint_t, rbentry);
				right = container_of(curr->rbentry.right, rbpoint_t, rbentry);
				curr->x = left->x + (right->x - left->x) / 2;
			}
			else {
				curr->x *= st->advanceX;
				curr->x += st->advanceX; /* left margin */
				if (st->draw_nil)
					curr->x += 3 * ASC16_GYLPH_WIDTH + CIRCLE_PADDING * 2;
			}

			curr->y *= st->advanceY;
			curr->y += st->advanceX; /* top margin */

			if (w < curr->x)
				w = curr->x;
			if (h < curr->y)
				h = curr->y;
		}
	}

	w += st->advanceX;  /* right margin */
	h += st->advanceX; /* bottom margin */

	if (st->draw_nil) {
		w += 3 * ASC16_GYLPH_WIDTH + CIRCLE_PADDING * 2;
		h += st->advanceY - (st->radius - ASC16_GYLPH_HEIGHT);
	}


	st->bitmapW = w;
	st->bitmapH = h;

	return 0;
}

bitmap_t *rbtree_save_as_bitmap(rbtree_t *tree, asc16_t *asc16, int nil)
{
	bitmap_t *bitmap;
	draw_state_t st = { 0 };

	st.draw_nil = nil;
	dllist_init(&st.nils);

	bitmap = malloc(sizeof(bitmap_t));
	if (bitmap == NULL)
		return NULL;

	memset(bitmap, 0, sizeof(bitmap_t));

	st.bitmap = bitmap;
	st.asc16 = asc16;

	if (rbtree_foreach_preorder(tree, predraw, &st) != 0) {
		free(bitmap);
		free_draw_state(&st);
		return NULL;
	}

	calc_xy(&st);

	if (bitmap_init(bitmap, st.bitmapW, st.bitmapH, COLOR_WHITE) != 0) {
		free(bitmap);
		free_draw_state(&st);
		return NULL;
	}

	if (draw_rbtree(&st) != 0) {
		free(bitmap);
		free_draw_state(&st);
		return NULL;
	}

	free_draw_state(&st);

	return bitmap;
}

int bitmap_init(bitmap_t *bitmap, int w, int h, color_t bg)
{
	int i;
	bitmap->w = w;
	bitmap->h = h;

	bitmap->colors = malloc(w * h * sizeof(color_t));
	if (bitmap->colors == NULL)
		return -1;
	for (i = 0; i < w * h; i++) {
		bitmap->colors[i] = bg;
	}
	return 0;
}

void bitmap_free(bitmap_t *bitmap)
{
	if (bitmap != NULL) {
		free(bitmap->colors);
	}
}

int bitmap_draw_point(bitmap_t *bitmap, int x, int y, color_t c)
{
	int i = x + y * bitmap->w;
	if (i >= 0 && i < (bitmap->w * bitmap->h)) {
		bitmap->colors[i] = c;
		return 0;
	}
	else {
		errno = ERANGE;
		return -1;
	}
}

static inline void swap_int(int *a, int *b)
{
	*a ^= *b;
	*b ^= *a;
	*a ^= *b;
}

/* Bresenham's line algorithm */
int bitmap_draw_line(bitmap_t *bitmap, int x1, int y1, int x2, int y2, color_t color)
{
	int ix, iy, cx, cy, n2dy, n2dydx, d;
	int dx, dy, yy;
	int r = 0;
	
	dx = abs(x2 - x1);
	dy = abs(y2 - y1);
	yy = 0;

	if (dx < dy) {
		yy = 1;
		swap_int(&x1, &y1);
		swap_int(&x2, &y2);
		swap_int(&dx, &dy);
	}

	ix = (x2 - x1) > 0 ? 1 : -1;
	iy = (y2 - y1) > 0 ? 1 : -1;
	cx = x1;
	cy = y1;
	n2dy = dy * 2;
	n2dydx = (dy - dx) * 2;
	d = dy * 2 - dx;

	if (yy) { /* 如果直线与 x 轴的夹角大于 45 度 */
		while (cx != x2) {
			if (d < 0) {
				d += n2dy;
			}
			else {
				cy += iy;
				d += n2dydx;
			}
			if (bitmap_draw_point(bitmap, cy, cx, color) != 0)
				r = -1;
			cx += ix;
		}
	}
	else { /* 如果直线与 x 轴的夹角小于 45 度 */
		while (cx != x2) {
			if (d < 0) {
				d += n2dy;
			}
			else {
				cy += iy;
				d += n2dydx;
			}
			if (bitmap_draw_point(bitmap, cx, cy, color) != 0)
				r = -1;
			cx += ix;
		}
	}

	return r;
}

/* 八对称性 */
static inline int draw_circle_8(bitmap_t *bitmap, int xc, int yc, int x, int y, color_t color)
{
	int r = 0;
	if (bitmap_draw_point(bitmap, xc + x, yc + y, color) != 0)
		r = -1;
	if (bitmap_draw_point(bitmap, xc - x, yc + y, color) != 0)
		r = -1;
	if (bitmap_draw_point(bitmap, xc + x, yc - y, color) != 0)
		r = -1;
	if (bitmap_draw_point(bitmap, xc - x, yc - y, color) != 0)
		r = -1;
	if (bitmap_draw_point(bitmap, xc + y, yc + x, color) != 0)
		r = -1;
	if (bitmap_draw_point(bitmap, xc - y, yc + x, color) != 0)
		r = -1;
	if (bitmap_draw_point(bitmap, xc + y, yc - x, color) != 0)
		r = -1;
	if (bitmap_draw_point(bitmap, xc - y, yc - x, color) != 0)
		r = -1;
	return r;
}

/* Bresenham's circle algorithm */
int bitmap_draw_circle(bitmap_t *bitmap, int xc, int yc, int r, int fill, color_t c)
{
	int x = 0, y = r, yi, d;
	int rc = 0;

	/* 如果圆在图片可见区域外，直接退出 */
	if (xc + r < 0 || xc - r >= bitmap->w ||
		yc + r < 0 || yc - r >= bitmap->h) {
		errno = ERANGE;
		return -1;
	}

	d = 3 - 2 * r;

	if (fill) {
		/* 如果填充（画实心圆）*/
		while (x <= y) {
			for (yi = x; yi <= y; yi++) {
				if (draw_circle_8(bitmap, xc, yc, x, yi, c) != 0)
					rc = -1;
			}

			if (d < 0) {
				d = d + 4 * x + 6;
			}
			else {
				d = d + 4 * (x - y) + 10;
				y--;
			}
			x++;
		}
	}
	else {
		/* 如果不填充（画空心圆）*/
		while (x <= y) {
			if (draw_circle_8(bitmap, xc, yc, x, y, c) != 0)
				rc = -1;

			if (d < 0) {
				d = d + 4 * x + 6;
			}
			else {
				d = d + 4 * (x - y) + 10;
				y--;
			}
			x++;
		}
	}

	return rc;
}

int bitmap_draw_rect(bitmap_t *bitmap, int x0, int y0, int x1, int y1, int fill, color_t color)
{
	int x, y;
	int r = 0;
	if (fill) {
		for (y = y0; y < y1; y++) {
			for (x = x0; x <= x1; x++) {
				if (bitmap_draw_point(bitmap, x, y, color) != 0)
					r = -1;
			}
		}
	}
	else {
		for (x = x0; x <= x1; x++) {
			if (bitmap_draw_point(bitmap, x, y0, color) != 0)
				r = -1;
			if (bitmap_draw_point(bitmap, x, y1, color) != 0)
				r = -1;
		}
		for (y = y0; y < y1; y++) {
			if (bitmap_draw_point(bitmap, x0, y, color) != 0)
				r = -1;
			if (bitmap_draw_point(bitmap, x1, y, color) != 0)
				r = -1;
		}
	}

	return r;
}

int bitmap_draw_char(bitmap_t *bitmap, int ch, int x, int y, color_t c, asc16_t *asc16)
{
	int i, j, r = 0;
	char *d;
	d = asc16_gylph_data(asc16, ch);
	for (j = 0; j < ASC16_GYLPH_HEIGHT; j++, d++) {
		for (i = 0; i < ASC16_GYLPH_WIDTH; i++) {
			if (asc16_is_setpixel(d, i))
				if (bitmap_draw_point(bitmap, x + i, y + j, c) != 0)
					r = -1;
		}
	}
	return r;
}

int bitmap_draw_string(bitmap_t *bitmap, char *str, int x, int y, color_t c, asc16_t *asc16)
{
	int r = 0;
	while (*str) {
		if (bitmap_draw_char(bitmap, *str, x, y, c, asc16) != 0)
			r = -1;
		x += ASC16_ADVANCE;
		str++;
	}
	return r;
}

static inline int fwritei32(int v, FILE *pf)
{
	char buf[4];
	int r;
	buf[0] = v & 0xff;
	buf[1] = (v >> 8) & 0xff;
	buf[2] = (v >> 16) & 0xff;
	buf[3] = (v >> 24) & 0xff;
	r = fwrite(buf, 1, 4, pf);
	if (r != 4)
		return -1;
	return 0;
}

static inline int fwritei24(int v, FILE *pf)
{
	char buf[4];
	int r;
	buf[0] = v & 0xff;
	buf[1] = (v >> 8) & 0xff;
	buf[2] = (v >> 16) & 0xff;
	r = fwrite(buf, 1, 3, pf);
	if (r != 3)
		return -1;
	return 0;
}

static inline int fwritei16(int v, FILE *pf)
{
	char buf[4];
	int r;
	buf[0] = v & 0xff;
	buf[1] = (v >> 8) & 0xff;
	r = fwrite(buf, 1, 2, pf);
	if (r != 2)
		return -1;
	return 0;
}

int bitmap_print(bitmap_t *bitmap, FILE *pf)
{
	char buf[1024];
	int r, i, j;

	buf[0] = 'B';
	buf[1] = 'M';

	r = fwrite(buf, 1, 2, pf);
	if (r != 2) return -1;

	r = fwritei32(bitmap->w * bitmap->h * 3 + 54, pf);
	if (r != 0) return -1;

	r = fwritei32(0, pf);
	if (r != 0) return -1;

	r = fwritei32(54, pf);
	if (r != 0) return -1;

	r = fwritei32(40, pf);
	if (r != 0) return -1;

	r = fwritei32(bitmap->w, pf);
	if (r != 0) return -1;

	r = fwritei32(bitmap->h, pf);
	if (r != 0) return -1;

	r = fwritei16(1, pf);
	if (r != 0) return -1;

	r = fwritei16(24, pf);
	if (r != 0) return -1;

	r = fwritei32(0, pf);
	if (r != 0) return -1;

	r = fwritei32(0, pf);
	if (r != 0) return -1;

	r = fwritei32(0, pf);
	if (r != 0) return -1;

	r = fwritei32(0, pf);
	if (r != 0) return -1;

	r = fwritei32(0, pf);
	if (r != 0) return -1;

	r = fwritei32(0, pf);
	if (r != 0) return -1;

	for (j = bitmap->h - 1; j >= 0; j--) {
		for (i = 0; i < bitmap->w; i++) {
			r = fwritei24(bitmap->colors[i + j * bitmap->w], pf);
			if (r != 0) return -1;
		}
	}

	return 0;
}

int bitmap_save(bitmap_t *bitmap, const char *filename)
{
	FILE *pf;
	int r;

	pf = fopen(filename, "wb");
	if (pf == NULL)
		return -1;

	r = bitmap_print(bitmap, pf);

	fclose(pf);

	return r;
}
