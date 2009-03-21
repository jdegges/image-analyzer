/******************************************************************************
 * Copyright (c) 2008 Joey Degges
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *****************************************************************************/

#ifndef _H_FILTERS
#define _H_FILTERS

#include <math.h>
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <assert.h>
#include <unistd.h>

#include "../common.h"
#include "../image_analyzer.h"
#include "../ia_sequence.h"

/* Filters Indexes */
#define BLUR            1
#define COPY            2
#define CURVATURE       3
#define DIFF            4
#define DRAW_BEST_BOX   5
#define EDGES           6
#define FLOW            7
#define GRAYSCALE       8
#define MONKEY          9
#define NORMAL          10
#define SAD             11
#define SSD             12

static const char FILTERS[][30] = {
    {"BLUR"},
    {"COPY"},
    {"CURVATURE"},
    {"DIFF"},
    {"DRAW_BEST_BOX"},
    {"EDGES"},
    {"FLOW"},
    {"GRAYSCALE"},
    {"MONKEY"},
    {"NORMAL"},
    {"SAD"},
    {"SSD"},
    {0}
};

static inline int offset( int w, int x, int y, int p )
{
    return w*3*y + x*3+p;
}

#define O( y,x ) (s->param->i_width*3*y + x*3)


/* Set up the filter function pointers */
typedef void (*init_funcs)(ia_seq_t*, ia_filter_param_t*);
typedef void (*exec_funcs)(ia_seq_t*, ia_filter_param_t*, ia_image_t**, ia_image_t*);
typedef void (*clos_funcs)(ia_filter_param_t*);

typedef struct ia_filters_t
{
    init_funcs      init[20];
    exec_funcs      exec[20];
    clos_funcs      clos[20];
} ia_filters_t;

ia_filters_t filters;

void init_filters( void );

#endif
