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

#ifndef _H_MONKEY
#define _H_MONKEY

static inline void monkey( ia_seq_t* s, ia_image_t** iaim, ia_image_t* iar )
{
    int i,j;
    double dev, avg;

    i = s->param->i_size*3-1;
    while( i-- )
    {
        avg = 0;
        j = s->param->i_maxrefs;
        while( j-- )
            avg += iaim[j]->pix[i];

        avg /= s->param->i_maxrefs;

        dev = 0;
        iar->pix[i] = iaim[s->param->i_maxrefs-1]->pix[i];
        j = s->param->i_maxrefs;
        while( j-- )
        {
            if( ia_abs(avg-iaim[j]->pix[i]) > dev )
            {
                dev = ia_abs(avg-iaim[j]->pix[i]);
                iar->pix[i] = iaim[j]->pix[i];
            }
        }
    }
}

#endif
