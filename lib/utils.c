/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2016 Josef Gajdusek
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
 * */

#include <errno.h>
#include <stdlib.h>
#include <stdbool.h>
#include <limits.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <bsd/string.h>
#include <string.h>
#include <time.h>

#include "utils.h"

void timespec_canonicalize(struct timespec *t)
{
	t->tv_sec += t->tv_nsec / 1000000000;
	t->tv_nsec = t->tv_nsec % 1000000000;
}

void timespec_add(struct timespec *a, struct timespec *b)
{
	a->tv_sec += b->tv_sec;
	a->tv_nsec += b->tv_nsec;
	timespec_canonicalize(a);
}

int clock_add_ms(clockid_t clk_id, struct timespec *tpret, unsigned int ms)
{
	int ret = clock_gettime(clk_id, tpret);
	if (ret != 0) {
		return ret;
	}

	struct timespec toadd = {
		.tv_sec = ms / 1000,
		.tv_nsec = (ms % 1000) * 1000 * 1000,
	};

	timespec_add(tpret, &toadd);

	return 0;
}
