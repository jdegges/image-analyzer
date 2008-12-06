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

#ifndef _H_ZOOM_IN
#define _H_ZOOM_IN

void zoom_in(ia_pixel_t* im, ia_pixel_t* out)
{
    int i, j, height, width;
    height = HEIGHT*2;
    width = WIDTH*2;
    for(i = 0; i < HEIGHT; i++)
    {
        for(j = 0; j < WIDTH; j++)
        {
            out[width*(i*2)+(j*2)] = im[WIDTH*(i)+(j)];
        }
    }
    
    for(i = 0; i < height; i++)
    {
        for(j = 0; j < width; j++)
        {
            if(j%2 != 0 && i%2 != 0)
                out[width*i+j] = (((out[width*(i-1)+(j-1)] + out[width*(i+1)+(j+1)])/2) + ((out[width*(i+1)+(j-1)] + out[width*(i-1)+(j+1)])/2))/2;
            else if(j%2 == 0 && i%2 != 0)
                out[width*i+j] = (out[width*(i-1)+j]+out[width*(i+1)+j])/2;
            else if(i%2 == 0 && j%2 != 0)
                out[width*i+j] = (out[width*i+(j-1)]+out[width*i+(j+1)])/2;
        }
    }
}

#endif
