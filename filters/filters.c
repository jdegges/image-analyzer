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

#include "filters.h"

void init_filters( void )
{
    filters.init[SAD]       = NULL;
    filters.init[DERIV]     = NULL;
    filters.init[FLOW]      = NULL;
    filters.init[CURV]      = NULL;
    filters.init[SSD]       = NULL;
    filters.init[SSD]       = NULL;
    filters.init[ME]        = NULL;
    filters.init[BLOBS]     = NULL;
    filters.init[DIFF]      = NULL;
    filters.init[BHATTA]    = NULL;
    filters.init[COPY]      = NULL;
    filters.init[MBOX]      = NULL;
    filters.init[MONKEY]    = NULL;
    filters.init[NORMAL]    = NULL;
    filters.init[GRAYSCALE] = NULL;

    filters.exec[SAD]       = &sad;
    filters.exec[DERIV]     = &fstderiv;
    filters.exec[FLOW]      = &flow;
    filters.exec[CURV]      = &curvature;
    filters.exec[SSD]       = &ssd;
    filters.exec[ME]        = NULL;
    filters.exec[BLOBS]     = NULL;
    filters.exec[DIFF]      = &diff;
    filters.exec[BHATTA]    = NULL;
    filters.exec[COPY]      = &copy;
    filters.exec[MBOX]      = &draw_best_box;
    filters.exec[MONKEY]    = &monkey;
    filters.exec[NORMAL]    = &normal;
    filters.exec[GRAYSCALE] = &grayscale;

    filters.clos[SAD]       = NULL;
    filters.clos[DERIV]     = NULL;
    filters.clos[FLOW]      = NULL;
    filters.clos[CURV]      = NULL;
    filters.clos[SSD]       = NULL;
    filters.clos[SSD]       = NULL;
    filters.clos[ME]        = NULL;
    filters.clos[BLOBS]     = NULL;
    filters.clos[DIFF]      = NULL;
    filters.clos[BHATTA]    = NULL;
    filters.clos[COPY]      = NULL;
    filters.clos[MBOX]      = NULL;
    filters.clos[MONKEY]    = NULL;
    filters.clos[NORMAL]    = NULL;
    filters.clos[GRAYSCALE] = NULL;
}
