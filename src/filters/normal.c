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

#include "normal.h"

void normal_exec( ia_seq_t* s, ia_filter_param_t* fp, ia_image_t** iaim, ia_image_t* iar )
{
    ia_image_t* iaf = iaim[0];
    int i;

    for( i = s->param->i_height-1; i > 0; i-- )
    {
        int j;

        for( j = s->param->i_width-1; j > 0; j-- )
        {
            double n[3]; // {r, g, b}
            int ci, cj, pix;

            if( i == 0 || j == 0 || i == s->param->i_height-1 || j == s->param->i_width-1 )
            {
                memset( &iar->pix[offset(iar->i_pitch,j,i,0)], 0, sizeof(ia_pixel_t)*3 );
                continue;
            }

            for( pix = 3; pix--; )
                n[pix] = 1;

            for( ci = i-1; ci < i+1; ci++ )
            {
                for( cj = j-1; cj < j+1; cj++ )
                {
                    if( ci == i && cj == j )
                        continue;
                    for( pix = 3; pix--; )
                    {
                        n[pix] *= iaf->pix[offset(iar->i_pitch,j,i,pix)]
                                  - iaf->pix[offset(iar->i_pitch,cj,ci,pix)];
                    }
                }
            }

            for( pix = 3; pix--; )
                iar->pix[offset(iar->i_pitch,j,i,pix)] = clip_uint8( sqrt(fabs(n[pix])) );
        }
    }
    fp = fp;
}
