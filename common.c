#include "common.h"

ia_image_t* ia_image_create( size_t size )
{
    ia_image_t* iaf = malloc( sizeof(ia_image_t) );
    if( iaf == NULL )
        return NULL;
    ia_memset( iaf, 0, sizeof(ia_image_t) );

    iaf->pix = malloc( sizeof(ia_pixel_t)*size );
    if( iaf->pix == NULL )
        return NULL;
    ia_memset( iaf->pix, 0, sizeof(ia_pixel_t)*size );

    pthread_mutex_init( &iaf->mutex, NULL );
    ia_pthread_cond_init( &iaf->cond_ro, NULL );
    ia_pthread_cond_init( &iaf->cond_rw, NULL );

    return iaf;
}

void ia_image_free( ia_image_t* iaf )
{
    pthread_mutex_destroy( &iaf->mutex );
    pthread_cond_destroy( &iaf->cond_ro );
    pthread_cond_destroy( &iaf->cond_rw );

    ia_free( iaf->pix );
    ia_free( iaf );
}

