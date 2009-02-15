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

/* Import filters */
#include "blur.h"
#include "copy.h"
#include "curvature.h"
#include "diff.h"
#include "draw_best_box.h"
#include "edges.h"
#include "flow.h"
#include "grayscale.h"
#include "monkey.h"
#include "normal.h"
#include "sad.h"
#include "ssd.h"

void init_filters( void )
{
    filters.init[BLUR]                 = NULL;
    filters.exec[BLUR]                 = &blur_exec;
    filters.clos[BLUR]                 = NULL;

    filters.init[COPY]                 = NULL;
    filters.exec[COPY]                 = &copy_exec;
    filters.clos[COPY]                 = NULL;

    filters.init[CURVATURE]            = NULL;
    filters.exec[CURVATURE]            = &curvature_exec;
    filters.clos[CURVATURE]            = NULL;

    filters.init[DIFF]                 = NULL;
    filters.exec[DIFF]                 = &diff_exec;
    filters.clos[DIFF]                 = NULL;

    filters.init[DRAW_BEST_BOX]        = NULL;
    filters.exec[DRAW_BEST_BOX]        = &draw_best_box_exec;
    filters.clos[DRAW_BEST_BOX]        = NULL;

    filters.init[EDGES]                = NULL;
    filters.exec[EDGES]                = &fstderiv_exec;
    filters.clos[EDGES]                = NULL;

    filters.init[FLOW]                 = NULL;
    filters.exec[FLOW]                 = &flow_exec;
    filters.clos[FLOW]                 = NULL;

    filters.init[GRAYSCALE]            = NULL;
    filters.exec[GRAYSCALE]            = &grayscale_exec;
    filters.clos[GRAYSCALE]            = NULL;

    filters.init[MONKEY]               = NULL;
    filters.exec[MONKEY]               = &monkey_exec;
    filters.clos[MONKEY]               = NULL;

    filters.init[NORMAL]               = NULL;
    filters.exec[NORMAL]               = &normal_exec;
    filters.clos[NORMAL]               = NULL;

    filters.init[SAD]                  = NULL;
    filters.exec[SAD]                  = &sad_exec;
    filters.clos[SAD]                  = NULL;

    filters.init[SSD]                  = NULL;
    filters.exec[SSD]                  = &ssd_exec;
    filters.clos[SSD]                  = NULL;

}
