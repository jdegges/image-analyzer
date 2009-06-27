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

#include "common.h"
#include <FreeImage.h>

ia_image_t* ia_image_create( size_t width, size_t height )
{
    ia_image_t* iaf = calloc( 1, sizeof(ia_image_t) );

    if( iaf == NULL )
        return NULL;

    iaf->dib = FreeImage_Allocate( width, height, 24,
                              0, 0, 0 );
    if( !iaf->dib )
        return NULL;
    iaf->pix = FreeImage_GetBits( (FIBITMAP*)iaf->dib );
    if( iaf->pix == NULL )
        return NULL;

    pthread_mutex_init( &iaf->mutex, NULL );
    ia_pthread_cond_init( &iaf->cond_ro, NULL );
    ia_pthread_cond_init( &iaf->cond_rw, NULL );

    iaf->i_size = width*height*3;

    return iaf;
}

void ia_image_free( ia_image_t* iaf )
{
    pthread_mutex_destroy( &iaf->mutex );
    pthread_cond_destroy( &iaf->cond_ro );
    pthread_cond_destroy( &iaf->cond_rw );

    if( iaf->dib )
        FreeImage_Unload( (FIBITMAP*)iaf->dib );
    ia_free( iaf );
}

