#include <pthread.h>
#include <assert.h>

#include "queue.h"
#include "common.h"

ia_queue_t* ia_queue_open( size_t size )
{
    ia_queue_t* q = ia_malloc( sizeof(ia_queue_t) );
    ia_memset( q, 0, sizeof(ia_queue_t) );
    q->list = ia_malloc( sizeof(ia_image_t*)*size );
    q->size = size;
    q->r = q->w = q->count = 0;
    return q;
}

void ia_queue_close( ia_queue_t* q )
{
    ia_free( q->list );
    ia_free( q );
}

/* adds image to queue */
void ia_queue_push( ia_queue_t* q, ia_image_t* iaf )
{
    int rc;

    // get lock on queue
    rc = ia_pthread_mutex_lock( &q->mutex );
    ia_pthread_error( rc, "ia_queue_push()", "ia_pthread_mutex_lock()" );
    if( q->w - q->r >= q->size )
    {
        rc = ia_pthread_cond_wait( &q->cond_rw, &q->mutex );
        ia_pthread_error( rc, "ia_queue_push()", "ia_pthread_cond_wait()" );
    }
    assert( q->w - q->r < q->size );

    // add image to queue
    //fprintf( stderr, "adding image %p to slot %d\n", (void*)iaf, (q->w % q->size) );
    q->list[q->w++ % q->size] = iaf;

    // if someone is waiting to pop, send signal
    rc = ia_pthread_cond_signal( &q->cond_ro );
    ia_pthread_error( rc, "ia_seq_push()", "ia_pthread_cond_signal()" );
    q->count++;

    // unlock queue
    rc = ia_pthread_mutex_unlock( &q->mutex );
    ia_pthread_error( rc, "ia_queue_push()", "ia_pthread_mutex_unlock()" );
}

/* returns unlocked image from queue */
ia_image_t* ia_queue_pop( ia_queue_t* q )
{
    int rc;
    ia_image_t* iaf;

    // get lock on queue
    rc = ia_pthread_mutex_lock( &q->mutex );
    ia_pthread_error( rc, "ia_queue_pop()", "ia_pthread_mutex_lock()" );
    if( q->w - q->r == 0 )
    {
        rc = ia_pthread_cond_wait( &q->cond_ro, &q->mutex );
        ia_pthread_error( rc, "ia_queue_pop()", "ia_pthread_cond_wait()" );
    }
    assert( q->w - q->r > 0 );

    // pop image off queue
    //fprintf( stderr, "popping image %p from slot %d\n", (void*)q->list[q->r % q->size], (q->r % q->size));
    iaf = q->list[q->r++ % q->size];

    // if someone is waiting to push, send signal
    rc = ia_pthread_cond_signal( &q->cond_rw );
    ia_pthread_error( rc, "ia_queue_pop()", "ia_pthread_cond_signal()" );

    q->count--;

    // unlock queue
    rc = ia_pthread_mutex_unlock( &q->mutex );
    ia_pthread_error( rc, "ia_queue_pop()", "ia_pthread_mutex_unlock()" );

    return iaf;
}
