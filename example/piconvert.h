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

#ifndef EXAMPLE_PICONVERT_H_
#define EXAMPLE_PICONVERT_H_

#ifdef __cplusplus
extern "C" {
#endif

union piconvert_t {
	int i;
	void *p;
};

/* convert pointer to integer. */
static inline int ptoi(void *p)
{
	union piconvert_t c;
	c.p = p;
	return c.i;
}

/* convert integer to pointer. */
static inline void *itop(int i)
{
	union piconvert_t c;
	c.i = i;
	return c.p;
}

#ifdef __cplusplus
}
#endif

#endif
