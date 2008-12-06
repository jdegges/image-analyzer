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

#ifndef _H_CURVATURE
#define _H_CURVATURE

static inline void curvature( ia_seq_t* s, ia_image_t** iaim, ia_image_t* iar )
{
    int i, j;
    double kr, kg, kb, op;
    ia_image_t* iaf = iaim[0];

    op = 255.0 / (255 * 8);

    for( i = 0; i < s->param->i_height; i++ )
    {
        for( j = 0; j < s->param->i_width; j++ )
        {
            if( i == 0 || j == 0 || j == s->param->i_width-1 || i == s->param->i_height-1 )
            {
                iar->pix[offset(s->param->i_width,j,i,0)] =
                iar->pix[offset(s->param->i_width,j,i,1)] =
                iar->pix[offset(s->param->i_width,j,i,2)] = 0;
                continue;
            }

            kr = (fabs( iaf->pix[offset(s->param->i_width,j-1,i-1,0)] - iaf->pix[offset(s->param->i_width,j-1,i  ,0)] )
                + fabs( iaf->pix[offset(s->param->i_width,j-1,i  ,0)] - iaf->pix[offset(s->param->i_width,j-1,i+1,0)] )
                + fabs( iaf->pix[offset(s->param->i_width,j-1,i+1,0)] - iaf->pix[offset(s->param->i_width,j  ,i+1,0)] )
                + fabs( iaf->pix[offset(s->param->i_width,j  ,i+1,0)] - iaf->pix[offset(s->param->i_width,j+1,i+1,0)] )
                + fabs( iaf->pix[offset(s->param->i_width,j+1,i+1,0)] - iaf->pix[offset(s->param->i_width,j+1,i  ,0)] )
                + fabs( iaf->pix[offset(s->param->i_width,j+1,i  ,0)] - iaf->pix[offset(s->param->i_width,j+1,i-1,0)] )
                + fabs( iaf->pix[offset(s->param->i_width,j+1,i-1,0)] - iaf->pix[offset(s->param->i_width,j  ,i-1,0)] )
                + fabs( iaf->pix[offset(s->param->i_width,j  ,i-1,0)] - iaf->pix[offset(s->param->i_width,j-1,i-1,0)] )) * op;

            kg = (fabs( iaf->pix[offset(s->param->i_width,j-1,i-1,1)] - iaf->pix[offset(s->param->i_width,j-1,i  ,1)] )
                + fabs( iaf->pix[offset(s->param->i_width,j-1,i  ,1)] - iaf->pix[offset(s->param->i_width,j-1,i+1,1)] )
                + fabs( iaf->pix[offset(s->param->i_width,j-1,i+1,1)] - iaf->pix[offset(s->param->i_width,j  ,i+1,1)] )
                + fabs( iaf->pix[offset(s->param->i_width,j  ,i+1,1)] - iaf->pix[offset(s->param->i_width,j+1,i+1,1)] )
                + fabs( iaf->pix[offset(s->param->i_width,j+1,i+1,1)] - iaf->pix[offset(s->param->i_width,j+1,i  ,1)] )
                + fabs( iaf->pix[offset(s->param->i_width,j+1,i  ,1)] - iaf->pix[offset(s->param->i_width,j+1,i-1,1)] )
                + fabs( iaf->pix[offset(s->param->i_width,j+1,i-1,1)] - iaf->pix[offset(s->param->i_width,j  ,i-1,1)] )
                + fabs( iaf->pix[offset(s->param->i_width,j  ,i-1,1)] - iaf->pix[offset(s->param->i_width,j-1,i-1,1)] )) * op;


            kb = (fabs( iaf->pix[offset(s->param->i_width,j-1,i-1,2)] - iaf->pix[offset(s->param->i_width,j-1,i  ,2)] )
                + fabs( iaf->pix[offset(s->param->i_width,j-1,i  ,2)] - iaf->pix[offset(s->param->i_width,j-1,i+1,2)] )
                + fabs( iaf->pix[offset(s->param->i_width,j-1,i+1,2)] - iaf->pix[offset(s->param->i_width,j  ,i+1,2)] )
                + fabs( iaf->pix[offset(s->param->i_width,j  ,i+1,2)] - iaf->pix[offset(s->param->i_width,j+1,i+1,2)] )
                + fabs( iaf->pix[offset(s->param->i_width,j+1,i+1,2)] - iaf->pix[offset(s->param->i_width,j+1,i  ,2)] )
                + fabs( iaf->pix[offset(s->param->i_width,j+1,i  ,2)] - iaf->pix[offset(s->param->i_width,j+1,i-1,2)] )
                + fabs( iaf->pix[offset(s->param->i_width,j+1,i-1,2)] - iaf->pix[offset(s->param->i_width,j  ,i-1,2)] )
                + fabs( iaf->pix[offset(s->param->i_width,j  ,i-1,2)] - iaf->pix[offset(s->param->i_width,j-1,i-1,2)] )) * op;

            iar->pix[offset(s->param->i_width,j,i,0)] = kr;
            iar->pix[offset(s->param->i_width,j,i,1)] = kg;
            iar->pix[offset(s->param->i_width,j,i,2)] = kb;
        }
    }
}

#endif
