#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 500
#endif

#include <unistd.h>
#include <pthread.h>
#include <assert.h>

#include "queue.h"
#include "common.h"

ia_queue_t* ia_queue_open( size_t size )
{
    ia_queue_t* q = ia_malloc( sizeof(ia_queue_t) );
    ia_memset( q, 0, sizeof(ia_queue_t) );
    q->size = size;
    q->count = 0;
    pthread_mutex_init( &q->mutex, NULL );
    pthread_cond_init( &q->cond_ro, NULL );
    pthread_cond_init( &q->cond_rw, NULL );
    return q;
}

void ia_queue_close( ia_queue_t* q )
{
    int rc = ia_pthread_mutex_lock( &q->mutex );
    ia_pthread_error( rc, "ia_queue_close()", "ia_pthread_mutex_lock()" );
    ia_image_t* iaf;
    while( q->count ) {
        rc = ia_pthread_mutex_unlock( &q->mutex );
        ia_pthread_error( rc, "ia_queue_close()", "ia_pthread_mutex_unlock()" );
        iaf = ia_queue_pop( q );
        ia_image_free( iaf );
    }
    pthread_mutex_unlock( &q->mutex );
    pthread_mutex_destroy( &q->mutex );
    pthread_cond_destroy( &q->cond_ro );
    pthread_cond_destroy( &q->cond_rw );
    ia_free( q );
}

/* adds image to queue */
void ia_queue_push( ia_queue_t* q, ia_image_t* iaf )
{
    int rc;

    // get lock on queue
    while( 1 ) {
        rc = ia_pthread_mutex_lock( &q->mutex );
        ia_pthread_error( rc, "ia_queue_push()", "ia_pthread_mutex_lock()" );
        if( q->count >= q->size )
        {
            rc = ia_pthread_cond_wait( &q->cond_rw, &q->mutex );
            ia_pthread_error( rc, "ia_queue_push()", "ia_pthread_cond_wait()" );
        }
        if( q->count < q->size )
            break;
        rc = ia_pthread_mutex_unlock( &q->mutex );
        ia_pthread_error( rc, "ia_queue_push()", "ia_pthread_mutex_unlock()" );
        usleep( 5 );
    }
    assert( q->count < q->size );

    // add image to queue
    if( !q->count ) {
        q->tail = iaf;
    } else if ( q->count == 1 ) {
        q->head = iaf;
        q->head->next = NULL;
        q->head->last = q->tail;
        q->tail->next = q->head;
    } else {
        iaf->next = NULL;
        iaf->last = q->head;
        q->head->next = iaf;
        q->head = iaf;
    }
    q->count++;

    // if someone is waiting to pop, send signal
    rc = ia_pthread_cond_signal( &q->cond_ro );
    ia_pthread_error( rc, "ia_seq_push()", "ia_pthread_cond_signal()" );

    // unlock queue
    rc = ia_pthread_mutex_unlock( &q->mutex );
    ia_pthread_error( rc, "ia_queue_push()", "ia_pthread_mutex_unlock()" );
}

void ia_queue_shove( ia_queue_t* q, ia_image_t* iaf )
{
    int rc;

    // get lock on queue
    rc = ia_pthread_mutex_lock( &q->mutex );
    ia_pthread_error( rc, "ia_queue_push()", "ia_pthread_mutex_lock()" );

    if( q->count >= q->size )
        q->size++;
    assert( q->count < q->size );

    // add image to queue
    if( !q->count ) {
        q->tail = iaf;
    } else if ( q->count == 1 ) {
        q->head = iaf;
        q->head->next = NULL;
        q->head->last = q->tail;
        q->tail->next = q->head;
    } else {
        iaf->next = NULL;
        iaf->last = q->head;
        q->head->next = iaf;
        q->head = iaf;
    }
    q->count++;

    // if someone is waiting to pop, send signal
    rc = ia_pthread_cond_signal( &q->cond_ro );
    ia_pthread_error( rc, "ia_seq_push()", "ia_pthread_cond_signal()" );

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
    while( 1 ) {
        rc = ia_pthread_mutex_lock( &q->mutex );
        ia_pthread_error( rc, "ia_queue_pop()", "ia_pthread_mutex_lock()" );
        if( q->count == 0 )
        {
            rc = ia_pthread_cond_wait( &q->cond_ro, &q->mutex );
            ia_pthread_error( rc, "ia_queue_pop()", "ia_pthread_cond_wait()" );
        }
        if( q->count > 0 )
            break;
        rc = ia_pthread_mutex_unlock( &q->mutex );
        ia_pthread_error( rc, "ia_queue_pop()", "ia_pthread_mutex_unlock()" );
        usleep( 5 );
    }
    assert( q->count > 0 );

    // pop image off queue
    //fprintf( stderr, "count %llu\niaf %p\ntail %p\ntail next p\n", q->count, (void*)iaf, (void*)q->tail ); // (void*)q->tail->next );
    iaf = q->tail;
    q->tail = q->tail->next;

    q->count--;

    // if someone is waiting to push, send signal
    rc = ia_pthread_cond_signal( &q->cond_rw );
    ia_pthread_error( rc, "ia_queue_pop()", "ia_pthread_cond_signal()" );

    // unlock queue
    rc = ia_pthread_mutex_unlock( &q->mutex );
    ia_pthread_error( rc, "ia_queue_pop()", "ia_pthread_mutex_unlock()" );

    return iaf;
}

int ia_queue_is_full( ia_queue_t* q )
{
    int rc, status;

    rc = ia_pthread_mutex_lock( &q->mutex );
    ia_pthread_error( rc, "ia_queue_pop()", "ia_pthread_mutex_lock()" );

    status = q->count >= q->size;

    rc = ia_pthread_mutex_unlock( &q->mutex );
    ia_pthread_error( rc, "ia_queue_pop()", "ia_pthread_mutex_unlock()" );

    return status;
}

int ia_queue_is_empty( ia_queue_t* q )
{
    int rc, status;

    rc = ia_pthread_mutex_lock( &q->mutex );
    ia_pthread_error( rc, "ia_queue_pop()", "ia_pthread_mutex_lock()" );

    status = q->count == 0;

    rc = ia_pthread_mutex_unlock( &q->mutex );
    ia_pthread_error( rc, "ia_queue_pop()", "ia_pthread_mutex_unlock()" );

    return status;
}

/*
void ia_queue_push_to( ia_queue_t* q, ia_image_t* iaf, uint64_t frameno )
{
    int rc;
    size_t pos = frameno % q->size;
    ia_image_t* iar;

    // get lock pos'th item in queue
    while( 1 ) {
        rc = ia_pthread_mutex_lock( &q->list[pos]->mutex );
        ia_pthread_error( rc, "ia_queue_push_to()", "ia_pthread_mutex_lock()" );
        if( q->list[pos]->users != 0 )
        {
            rc = ia_pthread_cond_wait( &q->list[pos]->cond_rw, &q->list[pos]->mutex );
            ia_pthread_error( rc, "ia_queue_push_to()", "ia_pthread_cond_wait()" );
        }
        if( q->list[pos]->users == 0 )
            break;
        rc = ia_pthread_mutex_unlock( &q->list[pos]->mutex );
        ia_pthread_error( rc, "ia_queue_push_to()", "ia_pthread_mutex_unlock()" );
        usleep( 5 );
    }
    assert( q->list[pos]->users == 0 );

    // add image to queue
    iar = q->list[pos];
    q->list[pos] = iaf;

    // increment count
    rc = ia_pthread_mutex_lock( &q->mutex );
    ia_pthread_error( rc, "ia_queue_push_to()", "ia_pthread_mutex_lock()" );
    q->count++;
    rc = ia_pthread_mutex_unlock( &q->mutex );
    ia_pthread_error( rc, "ia_queue_push_to()", "ia_pthread_mutex_unlock()" );

    // if someone is waiting to pop, send signal
    rc = ia_pthread_cond_signal( &iar->cond_ro );
    ia_pthread_error( rc, "ia_seq_push_to()", "ia_pthread_cond_signal()" );

    // unlock queue
    rc = ia_pthread_mutex_unlock( &iar->mutex );
    ia_pthread_error( rc, "ia_queue_push_to()", "ia_pthread_mutex_unlock()" );
}

ia_image_t* ia_queue_pop_from( ia_queue_t* q, uint64_t frameno )
{
    int rc;
    size_t pos = frameno % q->size;
    ia_image_t* iaf;

    // get lock on pos'th item of queue
    while( 1 ) {
        rc = ia_pthread_mutex_lock( &q->list[pos]->mutex );
        ia_pthread_error( rc, "ia_queue_pop_from()", "ia_pthread_mutex_lock()" );
        if( q->list[pos]->users == 0 || q->list[pos]->i_frame != frameno )
        {
            rc = ia_pthread_cond_wait( &q->list[pos]->cond_ro, &q->list[pos]->mutex );
            ia_pthread_error( rc, "ia_queue_pop_from()", "ia_pthread_cond_wait()" );
        }
        if( q->list[pos]->users != 0 && q->list[pos]->i_frame == frameno )
            break;
        rc = ia_pthread_mutex_unlock( &q->list[pos]->mutex );
        ia_pthread_error( rc, "ia_queue_pop_from()", "ia_pthread_mutex_unlock()" );
        usleep( 5 );
    }
    assert( q->list[pos]->users != 0 && q->list[pos]->i_frame == frameno );

    // pop image off queue
    iaf = q->list[pos];
    iaf->users++;

    // unlock item
    rc = ia_pthread_mutex_unlock( &iaf->mutex );
    ia_pthread_error( rc, "ia_queue_pop_from()", "ia_pthread_mutex_unlock()" );

    return iaf;
}

void ia_queue_release_from( ia_queue_t* q, uint64_t frameno )
{
    int rc;
    size_t pos = frameno % q->size;

    // get lock on pos'th item of queue
    rc = ia_pthread_mutex_lock( &q->list[pos]->mutex );
    ia_pthread_error( rc, "ia_queue_release_from()", "ia_pthread_mutex_lock()" );

    q->list[pos]->users--;

    assert( q->list[pos]->users >= 0 );

    // if someone is waiting to overwrite this entry -> send signal
    if( !q->list[pos]->users )
    {
        rc = ia_pthread_cond_signal( &q->list[pos]->cond_rw );
        ia_pthread_error( rc, "ia_queue_release_from()", "ia_pthread_cond_signal()" );
    }

    // unlock pos'th item
    rc = ia_pthread_mutex_unlock( &q->list[pos]->mutex );
    ia_pthread_error( rc, "ia_queue_release_from()", "ia_pthread_mutex_unlock()" );
}
*/
