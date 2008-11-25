#include <pthread.h>

#include "queue.h"
#include "common.h"

ia_queue_t* ia_queue_open( size_t size, int image_size )
{
    int i;
    ia_queue_t* q = ia_malloc( sizeof(ia_queue_t) );
    ia_memset( q, 0, sizeof(ia_queue_t) );
    q->list = ia_malloc( sizeof(ia_image_t*)*size );
    /*
    for( i = 0; i < size; i++ )
    {
        q->list[i] = ia_malloc( sizeof(ia_image_t) );
        if( q->list[i] == NULL )
            return NULL;
        ia_memset( q->list[i], 0, sizeof(ia_image_t) );

        q->list[i]->pix = ia_malloc( sizeof(ia_pixel_t)*image_size );
        if( q->list[i]->pix == NULL )
            return NULL;
        ia_memset( q->list[i]->pix, 0, sizeof(ia_pixel_t)*image_size );

        pthread_mutex_init( &q->list[i]->mutex, NULL );
        ia_pthread_cond_init( &q->list[i]->cond_ro, NULL );
        ia_pthread_cond_init( &q->list[i]->cond_rw, NULL );
    }
    */
    q->size = size;
    return q;
}

void ia_queue_close( ia_queue_t* q )
{
    int i;
    for( i = 0; i < q->size; i++ )
    {
        pthread_mutex_destroy( q->list[i]->mutex );
        pthread_cond_destroy( q->list[i]->cond_ro );
        pthread_cond_destroy( q->list[i]->cond_rw );
        ia_free( q->list[i]->pix );
        ia_free( q->list[i] );
    }
    ia_free( q->list );
    ia_free( q );
}

/* adds image to queue */
void ia_queue_push( ia_queue_t* q, ia_image_t* iaf )
{
    int rc;

    for( ;; )
    {
        rc = ia_pthread_lock( &q->mutex );
        ia_pthread_error( rc, "ia_queue_push()", "ia_pthread_lock()" );
        if( q->w - q->r != q->size )
        {
            q->list[q->w++ % q->size] = iaf;
            rc = ia_pthread_unlock( &q->mutex );
            ia_pthread_error( rc, "ia_queue_push()", "ia_pthread_unlock()" );
            return;
        }
        rc = ia_pthread_unlock( &q->mutex );
        ia_pthread_error( rc, "ia_queue_push()", "ia_pthread_unlock()" );
    }
}

/* returns unlocked image from queue */
ia_image_t* ia_queue_pop( void )
{
    int rc;
    ia_image_t* iaf;
    for( ;; )
    {
        rc = ia_pthread_lock( &q->mutex );
        ia_pthread_error( rc, "ia_queue_pop()", "ia_pthread_lock()" );
        if( q->w - q->r == 0 )
        {
            rc = ia_pthread_unlock( &q->mutex );
            ia_pthread_error( rc, "ia_queue_pop()", "ia_pthread_unlock()" );
            continue;
        }
        iaf = q->list[q->r++ % q->size];
        rc = ia_pthread_unlock( &q->mutex );
        ia_pthread_error( rc, "ia_queue_pop()", "ia_pthread_unlock()" );
        return iaf;
    }
}
