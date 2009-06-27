#include "swscale.h"
#include <FreeImage.h>
#include <pthread.h>

#ifdef HAVE_LIBSWSCALE

ia_swscale_t* ia_swscale_init( int32_t width, int32_t height,
                               int32_t s_fmt )
{
    ia_swscale_t* c = sws_getContext( width, height, s_fmt,
                                      width, height, PIX_FMT_BGR24,
                                      SWS_BICUBIC, NULL, NULL, NULL);
    if( c == NULL ) {
        return NULL;
    }
    return c;
}

int ia_swscale( ia_swscale_t* c, ia_image_t* iaf, int32_t width,
                int32_t height )
{
    ia_image_t* iar = ia_image_create( width, height );

    if( iar == NULL ) {
        return -1;
    }

    int s_stride[4] = {width*2, 0, 0, 0};
    int d_stride[4] = {width*3, 0, 0, 0};
    uint8_t* s_slice[4] = {iaf->pix, 0, 0, 0};
    uint8_t* d_slice[4] = {iar->pix, 0, 0, 0};

    if( sws_scale(c, s_slice, s_stride, 0, height, d_slice, d_stride)
        != height ) {
        return -1;
    }

    FreeImage_Unload( (FIBITMAP*)iaf->dib );
    iaf->dib = iar->dib;
    iaf->pix = iar->pix;

    pthread_mutex_destroy( &iar->mutex );
    pthread_cond_destroy( &iar->cond_ro );
    pthread_cond_destroy( &iar->cond_rw );

    ia_free( iar );

    return 0;
}

void ia_swscale_close( ia_swscale_t* c )
{
    ia_free( c );
}

#endif
