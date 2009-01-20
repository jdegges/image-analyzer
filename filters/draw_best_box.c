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

#include "draw_best_box.h"

void draw_best_box( ia_seq_t* s, ia_filter_param_t* fp, ia_image_t** iaim, ia_image_t* iar )
{
    int i, j;
    int top, bottom, left, right;
    ia_image_t* iaf = iaim[0];

    top    = left  =  INT_MAX;
    bottom = right = -INT_MAX;

    for( i = 0; i < s->param->i_height; i++ )
    {
        for( j = 0; j < s->param->i_width; j++ )
        {
            if( iaf->pix[O(i,j)] > 128 )
            {
                top    = (   top > i) ? i : top;
                left   = (  left > j) ? j : left;
                bottom = (bottom < i) ? i : bottom;
                right  = ( right < j) ? j : right;
            }

            iar->pix[O(i,j)+0] = 0;
            iar->pix[O(i,j)+1] = 0;
            iar->pix[O(i,j)+2] = 0;
        }
    }

    for( i = top; i <= bottom ; i++ )
    {
        for( j = left; j <= right; j++ )
        {
            iar->pix[O(i,j)+0] = 255;
            iar->pix[O(i,j)+1] = 255;
            iar->pix[O(i,j)+2] = 255;
        }
    }
    fp = fp;
}
