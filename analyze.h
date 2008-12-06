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

#ifndef _H_ANALYZE
#define _H_ANALYZE

#include "image_analyzer.h"

int     analyze( ia_param_t* p );

/*
// these functions do not yet jive with ia yet, so dont call them
// the implementations are also commented out in analyze.c
void zoom_in(ia_pixel_t* im, ia_pixel_t* out);
void k(ia_pixel_t* im, ia_pixel_t* out);
int* q(ia_pixel_t* im);
ia_pixel_t *get_gaussian(int ksize);
ia_pixel_t* get_reflectance_image(IplImage* im, ia_pixel_t* gaussian, int ksize);
ia_pixel_t* patch_diff(ia_pixel_t* ima, ia_pixel_t* imb, int ksize);
ia_pixel_t* Iplblur(IplImage* im, ia_pixel_t* gaussian, int ksize);
void dialate(ia_pixel_t* im, int ksize);


// these actually work but you wont ever need to call them outside of analyze.c
// so there is no real reason to have them in this interface
void fstderiv( ia_image_t* iai, ia_image_t* iar );
void diff( ia_image_t* iaa, ia_image_t* iab, ia_image_t* iar );
void bhatta( ia_seq_t* s );
void bhatta_init( void );
*/

#endif
