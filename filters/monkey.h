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

void monkey( ia_seq_t* s )
{
    int i,j;
    ia_pixel_t dev, avg;

    if( s->i_nrefs < s->param->i_maxrefs )
    {
        ia_memset( s->iar->pix,0,sizeof(ia_pixel_t)*s->param->i_size*3 );
        return;
    }

    i = s->param->i_size*3-1;
    while( i-- )
    {
        avg = s->iaf->pix[i];
        j = s->i_nrefs-1;
        while( j-- )
            avg += s->ref[j]->pix[i];

        avg /= ( s->i_nrefs + 1 );

        dev = ia_abs( avg - s->iaf->pix[i] );
        s->iar->pix[i] = s->iaf->pix[i];
        j = s->i_nrefs-1;
        while( j-- )
        {
            if( ia_abs(avg-s->ref[j]->pix[i]) > dev )
            {
                dev = ia_abs(avg-s->ref[j]->pix[i]);
                s->iar->pix[i] = s->ref[j]->pix[i];
            }
        }
    }
}

#endif
