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

#include "asc16.h"

void asc16_free(asc16_t *asc16)
{
	if (asc16 != NULL) {
		free(asc16->data);
	}
}

static inline int file_size(FILE *pf)
{
	int pos, size;

	pos = ftell(pf);
	if (pos == -1)
		return -1;

	if (fseek(pf, 0, SEEK_END) != 0)
		return -1;

	size = ftell(pf);
	if (size == -1)
		return -1;

	if (fseek(pf, pos, SEEK_SET) != 0)
		return -1;

	return size;
}

static inline int file_readall(FILE *pf, char **pbuf)
{
	int size;
	char *buf;

	size = file_size(pf);
	if (size == -1)
		return -1;

	buf = malloc(size);
	if (buf == NULL)
		return -1;

	if (fread(buf, 1, size, pf) != size) {
		free(buf);
		return -1;
	}

	*pbuf = buf;

	return size;
}

int asc16_load(asc16_t *asc16, const char *filename)
{
	FILE *pf;
	int size;

	pf = fopen(filename, "rb");
	if (pf == NULL)
		return -1;

	size = file_readall(pf, &asc16->data);
	
	fclose(pf);

	if (size == -1)
		return -1;

	asc16->size = size;

	return 0;
}

void asc16_gylph_print(char *d)
{
	int i, j;
	for (j = 0; j < ASC16_GYLPH_HEIGHT; j++, d++) {
		for (i = 0; i < ASC16_GYLPH_WIDTH; i++) {
			if (asc16_is_setpixel(d, i))
				printf("*");
			else
				printf(" ");
		}
		printf("\n");
	}
}

