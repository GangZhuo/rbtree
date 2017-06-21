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

#ifndef ASC16_H_
#define ASC16_H_

#ifdef __cplusplus
extern "C" {
#endif

#define ASC16_GYLPH_WIDTH	8	/* 字符的宽度 */
#define ASC16_GYLPH_HEIGHT	16	/* 字符的高度 */
#define ASC16_GYLPH_SIZE	16	/* 单个字符占用的点阵数据字节数 */
#define ASC16_ADVANCE		8	/* 单个字符前进宽度。例如，画出 A 后，后移 10 像素后，画下一个字符 B */

typedef struct asc16_t {
	char *data;
	int size;
} asc16_t;

#define ASC16_INIT() {0}

#define asc16_init(asc16) \
	do { \
		(asc16)->data = NULL; \
		(asc16)->size = 0; \
	} while (0)

#define asc16_is_setpixel(d, i) ((*d) & (0x80 >> i))

#define asc16_gylph_count(asc16) ((asc16)->size / ASC16_GYLPH_SIZE)

#define asc16_gylph_data(asc16, ch) ((asc16)->data + (ASC16_GYLPH_SIZE * (ch)))

void asc16_free(asc16_t *asc16);

int asc16_load(asc16_t *asc16, const char *filename);

void asc16_gylph_print(char *d);

#ifdef __cplusplus
}
#endif

#endif
