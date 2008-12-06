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

#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 500
#endif

#include <unistd.h>
#include <pthread.h>
#include <assert.h>

#include "queue.h"
#include "common.h"

#define ABS_MAX_SIZE 60

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
int _ia_queue_push( ia_queue_t* q, ia_image_t* iaf, ia_queue_pushtype_t pt )
{
    int rc;

    if( (pt == QUEUE_TAP && ia_queue_is_full(q))
        || (pt == QUEUE_TAP && q->size >= ABS_MAX_SIZE) )
        return 1;

    // get lock on queue
    while( 1 ) {
        rc = ia_pthread_mutex_lock( &q->mutex );
        ia_pthread_error( rc, "ia_queue_push()", "ia_pthread_mutex_lock()" );
        if( q->count >= q->size )
        {
            if( pt == QUEUE_SHOVE ) {
                q->size++;
                break;
            }
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

    iaf->last =
    iaf->next = NULL;

    // add image to queue
    if( !q->count ) {
        q->tail =
        q->head = iaf;
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

    return 0;
}

int ia_queue_tap( ia_queue_t* q, ia_image_t* iaf )
{
    return _ia_queue_push( q, iaf, QUEUE_TAP );
}

void ia_queue_push( ia_queue_t* q, ia_image_t* iaf )
{
    _ia_queue_push( q, iaf, QUEUE_PUSH );
}

int ia_queue_shove( ia_queue_t* q, ia_image_t* iaf )
{
    return _ia_queue_push( q, iaf, QUEUE_SHOVE );
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
