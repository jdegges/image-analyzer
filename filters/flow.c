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

#include "flow.h"

inline void flow( ia_seq_t* s, ia_filter_param_t* fp, ia_image_t** iaim, ia_image_t* iar )
{
    int i;
    double lmin, op;

    lmin = -255*9;
    op = 255.0 / (255*9*2);  //  = max / (lmax - lmin)

    assert( s->param->i_maxrefs > 2 );

    for ( i = 0; i < s->param->i_height; i++ )
    {
        int j;

        for ( j = 0; j < s->param->i_width; j++)
        {
            int h, k;
            double dzr, dzg, dzb;

            if ( i == 0 || j == 0 || j == s->param->i_width-1 || i == s->param->i_height-1 )
            {
                memset( &iar->pix[offset(s->param->i_width,j,i,0)], 0, sizeof(ia_pixel_t)*3 );
                continue;
            }

            dzr = dzg = dzb = 0;
            for( h = i-1; h < i+1; h++ )
            {
                for( k = j-1; k < j+1; k++ )
                {
                    dzr += (iaim[2]->pix[offset(s->param->i_width,k,h,0)]
                         - iaim[0]->pix[offset(s->param->i_width,k,h,0)]);
                    dzg += (iaim[2]->pix[offset(s->param->i_width,k,h,1)]
                         - iaim[0]->pix[offset(s->param->i_width,k,h,1)]);
                    dzb += (iaim[2]->pix[offset(s->param->i_width,k,h,2)]
                         - iaim[0]->pix[offset(s->param->i_width,k,h,2)]);
                }
            }

            iar->pix[offset(s->param->i_width,j,i,0)] = ( dzr - lmin ) * op;
            iar->pix[offset(s->param->i_width,j,i,1)] = ( dzg - lmin ) * op;
            iar->pix[offset(s->param->i_width,j,i,2)] = ( dzb - lmin ) * op;
        }
    }
    fp = fp;
}
