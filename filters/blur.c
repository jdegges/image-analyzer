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

#include "blur.h"

static inline void gaussian( double* ptr, ssize_t size, double std )
{
    const double c = 1.0 / (2.0 * M_PI * pow(std, 2));
    const double k = -2.0 * std * std;
    const int s = sqrt(size);
    int i, j, p;

    for( p = 0; p < size; p++ )
    {
        i = (p % s) - (s / 2);
        j = (p / s) - (s / 2);
        ptr[p] = c * exp((pow(i,2) + pow(j,2)) / k);
    }
}

inline void blur( ia_seq_t* s, ia_filter_param_t* fp, ia_image_t** iaim, ia_image_t* iar )
{
    static const double std = 2.5;
    ia_image_t* iaf = iaim[0];
    const int bs = ceil(6*std);
    const int br = sqrt(bs);
    double kernel[bs*bs];
    int max = -INT_MAX;
    int min = INT_MAX;
    int i;
    ia_pixel_t color;

    gaussian( kernel, bs*bs, std );

    for( i = -br; i < s->param->i_height; i++ )
    {
        int j;
        for( j = -br; j < s->param->i_width; j++ )
        {
            int pix, sj, si;
            double sum;

            for( pix = 0; pix < 3; pix++ )
            {
                sum = 0;
                for( si = 0; si < bs; si++ )
                {
                    for( sj = 0; sj < bs; sj++ )
                    {
                        if( j+sj >= s->param->i_width || i+si >= s->param->i_height
                            || j+sj < 0 || i+si < 0 )
                            color = 0;
                        else
                            color = iaf->pix[offset(s->param->i_width,j+sj,i+si,pix)];
                        sum += kernel[bs*sj+si] * color;
                    }
                }
                if( j+br < s->param->i_width && i+br < s->param->i_height )
                {
                    min = sum < min ? sum : min;
                    max = sum > max ? sum : max;
                    iar->pix[offset(s->param->i_width,j+br,i+br,pix)] = sum;
                }
            }
        }
    }
    fp = fp;
}
