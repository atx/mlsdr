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

#ifndef UTILS_H
#define UTILS_H

#include <time.h>
#include <stdio.h>
#include <stdint.h>
#include <sys/stat.h>
#include <stdbool.h>

#define max(a, b) ({ \
	__typeof__(a) _a = (a); \
	__typeof__(b) _b = (b); \
	_a > _b ? _a : _b; })

#define min(a, b) ({ \
	__typeof__(a) _a = (a); \
	__typeof__(b) _b = (b); \
	_a < _b ? _a : _b; })

#define ARRAY_SIZE(x) (sizeof(x) / sizeof(x[0]))

#define UNUSED(x) ((void)(x))

#define round(x, t) ({ \
	__typeof__(x) _x = (x); \
	__typeof__(t) _t = (t); \
	((long)(_x / _t)) * _t;})

#define BYTE(x, b) (((x) >> ((b) * 8)) & 0xff)

#define BIT(b) (1 << (b))

inline float int16_to_float(int16_t i)
{
	return (float)i / INT16_MAX;
}

void timespec_canonicalize(struct timespec *t);
void timespec_add(struct timespec *a, struct timespec *b);
int clock_add_ms(clockid_t clk_id, struct timespec *tpret, unsigned int ms);

#endif
