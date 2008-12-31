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
#include <assert.h>

#include "queue.h"
#include "common.h"

#define ABS_MAX_SIZE 30

ia_queue_t* ia_queue_open( size_t size )
{
    ia_queue_t* q = ia_malloc( sizeof(ia_queue_t) );
    ia_memset( q, 0, sizeof(ia_queue_t) );
    q->size = size;
    q->count = 0;
    ia_pthread_mutex_init( &q->mutex, NULL );
    ia_pthread_cond_init( &q->cond_ro, NULL );
    ia_pthread_cond_init( &q->cond_rw, NULL );
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
    ia_pthread_mutex_unlock( &q->mutex );
    ia_pthread_mutex_destroy( &q->mutex );
    ia_pthread_cond_destroy( &q->cond_ro );
    ia_pthread_cond_destroy( &q->cond_rw );
    ia_free( q );
}

/* adds image to queue */
int _ia_queue_push( ia_queue_t* q, ia_image_t* iaf, ia_queue_pushtype_t pt )
{
    int rc;
    ia_image_t* iar;

    if( (pt == QUEUE_TAP && ia_queue_is_full(q))
        || (pt == QUEUE_TAP && q->size >= ABS_MAX_SIZE) )
        return 1;

    // get lock on queue
    rc = ia_pthread_mutex_lock( &q->mutex );
    ia_pthread_error( rc, "ia_queue_push()", "ia_pthread_mutex_lock()" );
    while( q->count >= q->size ) {
        if( pt == QUEUE_SHOVE || pt == QUEUE_SSORT ) {
            q->size++;
            break;
        }
        rc = ia_pthread_cond_wait( &q->cond_rw, &q->mutex );
        ia_pthread_error( rc, "ia_queue_push()", "ia_pthread_cond_wait()" );
    }
    assert( q->count < q->size );

    iaf->last =
    iaf->next = NULL;

    // add image to queue
    if( pt == QUEUE_SSORT || pt == QUEUE_PSORT ) {
        iar = q->tail;
        while( iar != NULL ) {
            if( iar->i_frame == iaf->i_frame-1 )
                break;
            iar = iar->next;
        }
        // empty list + add first node
        if( iar == NULL && !q->count ) {
            q->tail =
            q->head = iaf;
        // nonempty list + insert at beginning
        } else if( iar == NULL && q->count ) {
            iaf->next = q->tail;
            q->tail->last = iaf;
            q->tail = iaf;
        // nonempty + insert at end
        } else if( iar == q->head && q->count ) {
            iaf->last = iar;
            iar->next = iaf;
            q->head = iaf;
        // insert into middle
        } else {
            iaf->next = iar->next;
            iaf->last = iar;
            iaf->next->last = iaf;
            iaf->last->next = iaf;
        }
    } else {
        if( !q->count ) {
            q->tail =
            q->head = iaf;
        } else {
            iaf->next = NULL;
            iaf->last = q->head;
            q->head->next = iaf;
            q->head = iaf;
        }
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

int ia_queue_shove_sorted( ia_queue_t* q, ia_image_t* iaf )
{
    iaf->i_refcount = 0;
    return _ia_queue_push( q, iaf, QUEUE_SSORT );
}

int ia_queue_push_sorted( ia_queue_t* q, ia_image_t* iaf )
{
    iaf->i_refcount = 0;
    return _ia_queue_push( q, iaf, QUEUE_PSORT );
}

/* returns unlocked image from queue */
ia_image_t* ia_queue_pop( ia_queue_t* q )
{
    int rc;
    ia_image_t* iaf;

    // get lock on queue
    rc = ia_pthread_mutex_lock( &q->mutex );
    ia_pthread_error( rc, "ia_queue_pop()", "ia_pthread_mutex_lock()" );
    while( q->count == 0 ) {
        rc = ia_pthread_cond_wait( &q->cond_ro, &q->mutex );
        ia_pthread_error( rc, "ia_queue_pop()", "ia_pthread_cond_wait()" );
    }
    assert( q->count > 0 );

    // pop image off queue
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

/* returns image from queue without removing it from queue */
ia_image_t* ia_queue_pek( ia_queue_t* q, uint64_t frameno )
{
    int rc;
    ia_image_t* iaf;

    // get lock on queue
    rc = ia_pthread_mutex_lock( &q->mutex );
    ia_pthread_error( rc, "ia_queue_pek()", "ia_pthread_mutex_lock()" );
    while( q->count == 0 ) {
        rc = ia_pthread_cond_wait( &q->cond_ro, &q->mutex );
        ia_pthread_error( rc, "ia_queue_pek()", "ia_pthread_cond_wait()" );
    }
    foo:
    assert( q->count > 0 );

    // find frame
    iaf = q->tail;
    while( iaf != NULL ) {
        if( iaf->i_frame == frameno )
            break;
        iaf = iaf->next;
    }

    if( iaf != NULL ) {
        // increase reference count
        rc = ia_pthread_mutex_lock( &iaf->mutex );
        ia_pthread_error( rc, "ia_queue_pek()", "ia_pthread_mutex_lock()" );
        iaf->i_refcount++;
        rc = ia_pthread_mutex_unlock( &iaf->mutex );
        ia_pthread_error( rc, "ia_queue_pek()", "ia_pthread_mutex_unlock()" );
    } else {
        rc = ia_pthread_cond_wait( &q->cond_ro, &q->mutex );
        ia_pthread_error( rc, "ia_queue_pek()", "ia_pthread_cond_wait()" );
        goto foo;
    }

    // unlock queue
    rc = ia_pthread_mutex_unlock( &q->mutex );
    ia_pthread_error( rc, "ia_queue_pek()", "ia_pthread_mutex_unlock()" );

    return iaf;
}

void ia_queue_sht( ia_queue_t* q, ia_queue_t* f, ia_image_t* iaf, int i_maxrefs )
{
    int rc;
    ia_image_t *min, *max;
    
    // get lock on queue
    rc = ia_pthread_mutex_lock( &q->mutex );
    ia_pthread_error( rc, "ia_queue_sht()", "ia_pthread_mutex_lock()" );

    rc = ia_pthread_mutex_lock( &iaf->mutex );
    ia_pthread_error( rc, "ia_queue_sht()", "ia_pthread_mutex_lock()" );

    iaf->i_refcount--;

    rc = ia_pthread_mutex_unlock( &iaf->mutex );
    ia_pthread_error( rc, "ia_queue_sht()", "ia_pthread_mutex_unlock()" );

    // find min and max frameno
    iaf = min = max = q->tail;
    while( iaf != NULL ) {
        if( min->i_frame > iaf->i_frame )
            min = iaf;
        if( max->i_frame < iaf->i_frame )
            max = iaf;
        iaf = iaf->next;
    }

    if( max != NULL && min != NULL
        && (max->i_frame - min->i_frame > (uint32_t) i_maxrefs
            || i_maxrefs == 1)
        && min->i_refcount <= 0 ) {
        // unlock queue
        rc = ia_pthread_mutex_unlock( &q->mutex );
        ia_pthread_error( rc, "ia_queue_sht()", "ia_pthread_mutex_unlock()" );

        // pop min frame
        iaf = ia_queue_pop_frame( q, min->i_frame );
        if( iaf != NULL )
            ia_queue_shove( f, iaf );
    } else {
        // unlock queue
        rc = ia_pthread_mutex_unlock( &q->mutex );
        ia_pthread_error( rc, "ia_queue_sht()", "ia_pthread_mutex_unlock()" );
    }
}

/* returns unlocked image from queue */
ia_image_t* ia_queue_pop_frame( ia_queue_t* q, uint64_t frameno )
{
    int rc;
    ia_image_t* iaf;

    // get lock on queue
    rc = ia_pthread_mutex_lock( &q->mutex );
    ia_pthread_error( rc, "ia_queue_pop_frame()", "ia_pthread_mutex_lock()" );
    while( q->count == 0 ) {
        rc = ia_pthread_cond_wait( &q->cond_ro, &q->mutex );
        ia_pthread_error( rc, "ia_queue_pop_frame()", "ia_pthread_cond_wait()" );
    }
    assert( q->count > 0 );

    // pop image off queue
    iaf = q->tail;
    while( iaf != NULL ) {
        if( iaf->i_frame == frameno )
            break;
        iaf = iaf->next;
    }

    // if frame wasnt on list -> return null
    if( iaf == NULL ) {
        rc = ia_pthread_mutex_unlock( &q->mutex );
        ia_pthread_error( rc, "ia_queue_pop_frame()", "ia_pthread_mutex_unlock()" );

        return NULL;
    }

    // remove from list
    if( iaf == q->tail && iaf == q->head ) {
        q->tail =
        q->head = NULL;
    } else if( iaf == q->tail ) {
        q->tail = iaf->next;
        q->tail->last = NULL;
        if( q->tail == q->head )
            q->tail->next = NULL;
    } else if( iaf == q->head ) {
        q->head = iaf->last;
        q->head->next = NULL;
        if( q->tail == q->head )
            q->head->last = NULL;
    } else {
        iaf->last->next = iaf->next;
        iaf->next->last = iaf->last;
    }

    iaf->last =
    iaf->next = NULL;
    q->count--;

    // if someone is waiting to push, send signal
    rc = ia_pthread_cond_signal( &q->cond_rw );
    ia_pthread_error( rc, "ia_queue_pop_frame()", "ia_pthread_cond_signal()" );

    // unlock queue
    rc = ia_pthread_mutex_unlock( &q->mutex );
    ia_pthread_error( rc, "ia_queue_pop_frame()", "ia_pthread_mutex_unlock()" );

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
