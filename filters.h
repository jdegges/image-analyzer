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

#include "common.h"
#include "image_analyzer.h"
#include "ia_sequence.h"

/* Filter Indexes */
#define SAD     1
#define DERIV   2
#define FLOW    3
#define CURV    4
#define SSD     5
#define ME      6
#define BLOBS   7
#define DIFF    8
#define BHATTA  9
#define COPY    10
#define MBOX    11
#define MONKEY  12


static inline int offset( int w, int x, int y, int p )
{
    return w*3*y + x*3+p;
}

#define O( y,x ) (s->param->i_width*3*y + x*3)


/* Import filters */
#include "filters/copy.h"
#include "filters/curvature.h"
#include "filters/draw_best_box.h"
#include "filters/first_derivative.h"


/* Set up the filter function pointers */
typedef void (*init_funcs)(ia_seq_t*);
typedef void (*exec_funcs)(ia_seq_t*, ia_image_t**, ia_image_t*);
typedef void (*clos_funcs)(void);

typedef struct ia_filters_t
{
    init_funcs filters_init[15];
    exec_funcs filters_exec[15];
    clos_funcs filters_clos[15];
} ia_filters_t;

ia_filters_t filters;

void init_filters( void )
{
    filters.filters_init[SAD]       = NULL;
    filters.filters_init[DERIV]     = NULL;
    filters.filters_init[FLOW]      = NULL;
    filters.filters_init[CURV]      = NULL;
    filters.filters_init[SSD]       = NULL;
    filters.filters_init[SSD]       = NULL;
    filters.filters_init[ME]        = NULL;
    filters.filters_init[BLOBS]     = NULL;
    filters.filters_init[DIFF]      = NULL;
    filters.filters_init[BHATTA]    = NULL;
    filters.filters_init[COPY]      = NULL;
    filters.filters_init[MBOX]      = NULL;
    filters.filters_init[MONKEY]    = NULL;

    filters.filters_exec[SAD]       = NULL;
    filters.filters_exec[DERIV]     = &fstderiv;
    filters.filters_exec[FLOW]      = NULL;
    filters.filters_exec[CURV]      = &curvature;
    filters.filters_exec[SSD]       = NULL;
    filters.filters_exec[ME]        = NULL;
    filters.filters_exec[BLOBS]     = NULL;
    filters.filters_exec[DIFF]      = NULL;
    filters.filters_exec[BHATTA]    = NULL;
    filters.filters_exec[COPY]      = &copy;
    filters.filters_exec[MBOX]      = &draw_best_box;
    filters.filters_exec[MONKEY]    = NULL;

    filters.filters_clos[SAD]       = NULL;
    filters.filters_clos[DERIV]     = NULL;
    filters.filters_clos[FLOW]      = NULL;
    filters.filters_clos[CURV]      = NULL;
    filters.filters_clos[SSD]       = NULL;
    filters.filters_clos[SSD]       = NULL;
    filters.filters_clos[ME]        = NULL;
    filters.filters_clos[BLOBS]     = NULL;
    filters.filters_clos[DIFF]      = NULL;
    filters.filters_clos[BHATTA]    = NULL;
    filters.filters_clos[COPY]      = NULL;
    filters.filters_clos[MBOX]      = NULL;
    filters.filters_clos[MONKEY]    = NULL;
}

#endif
