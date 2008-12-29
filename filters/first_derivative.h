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

#ifndef _H_FIRST_DERIVATIVE
#define _H_FIRST_DERIVATIVE

void fstderiv( ia_seq_t* s, ia_image_t** iaim, ia_image_t* iar )
{
    ia_image_t* iaf = iaim[0];
    //const double op = 255/sqrt(pow(255*3,2)*2); //  = max / (lmax - lmin)
    int i;

    for( i = s->param->i_height; i--; )
    {
        int j;
        for( j = s->param->i_width; j--; )
        {
            int ci, cj, pix;
            double dx[3]; // {r, g, b}
            double dy[3]; // {r, g, b}

            if( i == 0 || j == 0 || i == s->param->i_height-1 || j == s->param->i_width-1 )
            {
                memset( &iar->pix[offset(s->param->i_width,j,i,0)], 0, sizeof(ia_pixel_t)*3 );
                continue;
            }

            for( pix = 3; pix--; ) {
                dx[pix] =
                dy[pix] = 0;
            }

            // left and right
            for( ci = i-1; ci <= i+1; ci++ ) {
                for( pix = 3; pix--; ) {
                    dx[pix] -= iaf->pix[offset(s->param->i_width,j-1,ci,pix)]; // left
                    dx[pix] += iaf->pix[offset(s->param->i_width,j+1,ci,pix)]; // right
                }
            }

            // top and bottom
            for( cj = j-1; cj <= j+1; cj++ ) {
                for( pix = 3; pix--; ) {
                    dy[pix] += iaf->pix[offset(s->param->i_width,cj,i-1,pix)];  // top
                    dy[pix] -= iaf->pix[offset(s->param->i_width,cj,i+1,pix)];  // bottom
                }
            }

            // to scale the resultant pixel down to its appropriate value it
            // should be multiplied by op, but visually, it looks better to
            // clip it... this may change in the future.
            for( pix = 3; pix--; ) {
                iar->pix[offset(s->param->i_width,j,i,pix)] = clip_uint8( sqrt(pow(dx[pix],2)+pow(dy[pix],2)) );
            }
        }
    }
}

#endif
