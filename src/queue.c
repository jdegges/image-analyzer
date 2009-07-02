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

ia_queue_t* ia_queue_open( size_t size, int life )
{
    ia_queue_t* q = ia_malloc( sizeof(ia_queue_t) );
    ia_memset( q, 0, sizeof(ia_queue_t) );
    q->size = size;
    q->life = life;
    ia_pthread_mutex_init( &q->mutex, NULL );
    ia_pthread_cond_init( &q->cond_nonempty, NULL );
    ia_pthread_cond_init( &q->cond_nonfull, NULL );
    return q;
}

void ia_queue_close( ia_queue_t* q )
{
    int rc;
    if( 0 != (rc = ia_pthread_mutex_lock( &q->mutex )) )
        ia_pthread_error( rc, "ia_queue_close()", "ia_pthread_mutex_lock()" );
    while( q->count ) {
        if( 0 != (rc = ia_pthread_mutex_unlock( &q->mutex )) )
            ia_pthread_error( rc, "ia_queue_close()", "ia_pthread_mutex_unlock()" );
        ia_image_free( ia_queue_pop( q ) );
    }
    ia_pthread_mutex_unlock( &q->mutex );
    ia_pthread_mutex_destroy( &q->mutex );
    ia_pthread_cond_destroy( &q->cond_nonempty );
    ia_pthread_cond_destroy( &q->cond_nonfull );
    ia_free( q );
}

ia_queue_obj_t* ia_queue_create_obj( void* data )
{
    ia_queue_obj_t* obj = malloc( sizeof(ia_queue_obj_t) );
    if( !obj ) {
        fprintf( stderr, "Out of memory!\n" );
        ia_pthread_exit( NULL );
    }

    obj->data = data;
    return obj;
}

void ia_queue_destroy_obj( ia_queue_obj_t* obj )
{
    ia_free( obj );
}

/* adds image to queue */
int _ia_queue_push( ia_queue_t* q, void* data, uint32_t pos, ia_queue_pushtype_t pt )
{
    int rc;
    ia_queue_obj_t* obj;
    ia_queue_obj_t* tmp;

    if( (pt == QUEUE_TAP && ia_queue_is_full(q))
        || (pt == QUEUE_TAP && q->size >= ABS_MAX_SIZE) )
        return 1;

    // get lock on queue
    if( 0 != (rc = ia_pthread_mutex_lock( &q->mutex )) )
        ia_pthread_error( rc, "ia_queue_push()", "ia_pthread_mutex_lock()" );
    while( q->count >= q->size ) {
        if( pos == q->waiter_pos
            || pt == QUEUE_SHOVE || pt == QUEUE_SSORT ) {
            break;
        }
        if( 0 != (rc = ia_pthread_cond_wait( &q->cond_nonfull, &q->mutex )) )
            ia_pthread_error( rc, "ia_queue_push()", "ia_pthread_cond_wait()" );
    }

    obj = ia_queue_create_obj( data );
    if( !obj ) {
        return 1;
    }

    obj->last =
    obj->next = NULL;
    obj->i_pos = pos;
    obj->life = q->life;

    // add image to queue
    if( pt == QUEUE_SSORT || pt == QUEUE_PSORT ) {
        tmp = q->tail;
        while( tmp != NULL ) {
            if( tmp->i_pos == obj->i_pos-1 )
                break;
            tmp = tmp->next;
        }
        // empty list + add first node
        if( tmp == NULL && !q->count ) {
            q->tail =
            q->head = obj;
        // nonempty list + insert at beginning
        } else if( tmp == NULL && q->count ) {
            obj->next = q->tail;
            q->tail->last = obj;
            q->tail = obj;
        // nonempty + insert at end
        } else if( tmp == q->head && q->count ) {
            obj->last = tmp;
            tmp->next = obj;
            q->head = obj;
        // insert into middle
        } else {
            obj->next = tmp->next;
            obj->last = tmp;
            obj->next->last = obj;
            obj->last->next = obj;
        }
    } else {
        if( !q->count ) {
            q->tail =
            q->head = obj;
        } else {
            obj->next = NULL;
            obj->last = q->head;
            q->head->next = obj;
            q->head = obj;
        }
    }
    q->count++;

    // if someone is waiting to pop, send signal
    if( 0 != (rc = ia_pthread_cond_signal( &q->cond_nonempty )) )
        ia_pthread_error( rc, "ia_seq_push()", "ia_pthread_cond_signal()" );

    // unlock queue
    if( 0 != (rc = ia_pthread_mutex_unlock( &q->mutex )) )
        ia_pthread_error( rc, "ia_queue_push()", "ia_pthread_mutex_unlock()" );

    return 0;
}

int ia_queue_tap( ia_queue_t* q, void* data, uint32_t pos )
{
    return _ia_queue_push( q, data, pos, QUEUE_TAP );
}

void ia_queue_push( ia_queue_t* q, void* data, uint32_t pos )
{
    _ia_queue_push( q, data, pos, QUEUE_PUSH );
}

int ia_queue_shove( ia_queue_t* q, void* data, uint32_t pos )
{
    return _ia_queue_push( q, data, pos, QUEUE_SHOVE );
}

int ia_queue_shove_sorted( ia_queue_t* q, void* data, uint32_t pos )
{
    return _ia_queue_push( q, data, pos, QUEUE_SSORT );
}

int ia_queue_push_sorted( ia_queue_t* q, void* data, uint32_t pos )
{
    return _ia_queue_push( q, data, pos, QUEUE_PSORT );
}

/* returns unlocked image from queue */
void* ia_queue_pop( ia_queue_t* q )
{
    int rc;
    ia_queue_obj_t* obj;
    void* data;

    // get lock on queue
    if( 0 != (rc = ia_pthread_mutex_lock( &q->mutex )) )
        ia_pthread_error( rc, "ia_queue_pop()", "ia_pthread_mutex_lock()" );
    while( q->count == 0 ) {
        if( 0 != (rc = ia_pthread_cond_wait( &q->cond_nonempty, &q->mutex )) )
            ia_pthread_error( rc, "ia_queue_pop()", "ia_pthread_cond_wait()" );
    }
    assert( q->count > 0 );

    // pop image off queue
    obj = q->tail;
    q->tail = q->tail->next;
    data = obj->data;
    q->count--;

    // if someone is waiting to push, send signal
    if( 0 != (rc = ia_pthread_cond_signal( &q->cond_nonfull )) )
        ia_pthread_error( rc, "ia_queue_pop()", "ia_pthread_cond_signal()" );

    // unlock queue
    if( 0 != (rc = ia_pthread_mutex_unlock( &q->mutex )) )
        ia_pthread_error( rc, "ia_queue_pop()", "ia_pthread_mutex_unlock()" );

    // clean up queue object
    ia_queue_destroy_obj( obj );

    return data;
}

/* returns image from queue without removing it from queue */
void* ia_queue_pek( ia_queue_t* q, uint32_t pos )
{
    int rc;
    ia_queue_obj_t* obj;

    // get lock on queue
    if( 0 != (rc = ia_pthread_mutex_lock( &q->mutex )) )
        ia_pthread_error( rc, "ia_queue_pek()", "ia_pthread_mutex_lock()" );
    while( q->count == 0 ) {
        if( 0 != (rc = ia_pthread_cond_wait( &q->cond_nonempty, &q->mutex )) )
            ia_pthread_error( rc, "ia_queue_pek()", "ia_pthread_cond_wait()" );
    }

    for( ;; ) {
        assert( q->count > 0 );

        // find obj at position pos
        obj = q->tail;
        while( obj != NULL ) {
            if( obj->i_pos == pos )
                break;
            obj = obj->next;
        }

        if( obj != NULL ) {
            break;
        } else {
            if( 0 != (rc = ia_pthread_cond_wait( &q->cond_nonempty, &q->mutex )) )
                ia_pthread_error( rc, "ia_queue_pek()", "ia_pthread_cond_wait()" );
        }
    }

    // unlock queue
    if( 0 != (rc = ia_pthread_mutex_unlock( &q->mutex )) )
        ia_pthread_error( rc, "ia_queue_pek()", "ia_pthread_mutex_unlock()" );

    return obj->data;
}

void ia_queue_sht( ia_queue_t* q, void* data, uint8_t count )
{
    int rc;
    ia_queue_obj_t* obj;

    // get lock on queue
    if( 0 != (rc = ia_pthread_mutex_lock( &q->mutex )) )
        ia_pthread_error( rc, "ia_queue_sht()", "ia_pthread_mutex_lock()" );


    // find associated object
    obj = q->tail;
    while( obj != NULL ) {
        if( obj->data == data )
            break;
        obj = obj->next;
    }

    if( obj == NULL ) {
        // wake up the threads waiting to push to this queue
        if( 0 != (rc = ia_pthread_cond_signal( &q->cond_nonfull )) )
            ia_pthread_error( rc, "ia_queue_sht()", "ia_pthread_cond_signal()" );

        // unlock queue
        if( 0 != (rc = ia_pthread_mutex_unlock( &q->mutex )) )
            ia_pthread_error( rc, "ia_queue_sht()", "ia_pthread_mutex_unlock()" );
        return;
    }
    obj->life--;

    // remove dead frames on the list
    obj = q->tail;
    while( obj != NULL ) {
        assert( obj->life >= 0 );
        if( obj->life == 0 ) {
            void* data = ia_queue_pop_item_unlocked( q, obj->i_pos );
            if( data != NULL )
                ia_image_free( (ia_image_t*)data );
            obj = q->tail;
            continue;
        }
        obj = obj->next;
    }

    // wake up the threads waiting to push to this queue
    if( 0 != (rc = ia_pthread_cond_signal( &q->cond_nonfull )) )
        ia_pthread_error( rc, "ia_queue_sht()", "ia_pthread_cond_signal()" );

    // unlock queue
    if( 0 != (rc = ia_pthread_mutex_unlock( &q->mutex )) )
        ia_pthread_error( rc, "ia_queue_sht()", "ia_pthread_mutex_unlock()" );
}

/* returns unlocked image from queue */
void* ia_queue_pop_item( ia_queue_t* q, uint32_t pos )
{
    int rc;
    ia_queue_obj_t* obj = NULL;
    void* data;

    // get lock on queue
    if( 0 != (rc = ia_pthread_mutex_lock( &q->mutex )) )
        ia_pthread_error( rc, "ia_queue_pop_frame()", "ia_pthread_mutex_lock()" );

    q->waiter_pos = pos;

    while( q->count == 0 ) {
        if( 0 != (rc = ia_pthread_cond_wait( &q->cond_nonempty, &q->mutex )) )
            ia_pthread_error( rc, "ia_queue_pop_frame()", "ia_pthread_cond_wait()" );
    }
    assert( q->count > 0 );

    while( obj == NULL ) {
        // pop image off queue
        obj = q->tail;
        while( obj != NULL ) {
            if( obj->i_pos == pos )
                break;
            obj = obj->next;
        }

        // if frame wasnt on list -> return null
        if( obj == NULL && 0 != (rc = ia_pthread_cond_wait( &q->cond_nonempty, &q->mutex )) )
            ia_pthread_error( rc, "ia_queue_pop_frame()", "ia_pthread_cond_timedwait()" );
    }

    // remove from list
    if( obj == q->tail && obj == q->head ) {
        q->tail =
        q->head = NULL;
    } else if( obj == q->tail ) {
        q->tail = obj->next;
        q->tail->last = NULL;
        if( q->tail == q->head )
            q->tail->next = NULL;
    } else if( obj == q->head ) {
        q->head = obj->last;
        q->head->next = NULL;
        if( q->tail == q->head )
            q->head->last = NULL;
    } else {
        obj->last->next = obj->next;
        obj->next->last = obj->last;
    }

    obj->last =
    obj->next = NULL;
    data = obj->data;
    q->count--;

    // if someone is waiting to push, send signal
    if( 0 != (rc = ia_pthread_cond_signal( &q->cond_nonfull )) )
        ia_pthread_error( rc, "ia_queue_pop_frame()", "ia_pthread_cond_signal()" );

    // unlock queue
    if( 0 != (rc = ia_pthread_mutex_unlock( &q->mutex )) )
        ia_pthread_error( rc, "ia_queue_pop_frame()", "ia_pthread_mutex_unlock()" );

    ia_queue_destroy_obj( obj );

    return data;
}

void* ia_queue_pop_item_unlocked( ia_queue_t* q, uint32_t pos )
{
    ia_queue_obj_t* obj;
    void* data;

    // pop image off queue
    obj = q->tail;
    while( obj != NULL ) {
        if( obj->i_pos == pos )
            break;
        obj = obj->next;
    }

    // if frame wasnt on list -> return null
    if( obj == NULL ) {
        return NULL;
    }

    // remove from list
    if( obj == q->tail && obj == q->head ) {
        q->tail =
        q->head = NULL;
    } else if( obj == q->tail ) {
        q->tail = obj->next;
        q->tail->last = NULL;
        if( q->tail == q->head )
            q->tail->next = NULL;
    } else if( obj == q->head ) {
        q->head = obj->last;
        q->head->next = NULL;
        if( q->tail == q->head )
            q->head->last = NULL;
    } else {
        obj->last->next = obj->next;
        obj->next->last = obj->last;
    }

    obj->last =
    obj->next = NULL;
    data = obj->data;
    q->count--;

    ia_queue_destroy_obj( obj );
    return data;
}

int ia_queue_is_full( ia_queue_t* q )
{
    int rc, status;

    if( 0 != (rc = ia_pthread_mutex_lock( &q->mutex )) )
        ia_pthread_error( rc, "ia_queue_pop()", "ia_pthread_mutex_lock()" );

    status = q->count >= q->size;

    if( 0 != (rc = ia_pthread_mutex_unlock( &q->mutex )) )
        ia_pthread_error( rc, "ia_queue_pop()", "ia_pthread_mutex_unlock()" );

    return status;
}

int ia_queue_is_empty( ia_queue_t* q )
{
    int rc, status;

    if( 0 != (rc = ia_pthread_mutex_lock( &q->mutex )) )
        ia_pthread_error( rc, "ia_queue_pop()", "ia_pthread_mutex_lock()" );

    status = q->count == 0;

    if( 0 != (rc = ia_pthread_mutex_unlock( &q->mutex )) )
        ia_pthread_error( rc, "ia_queue_pop()", "ia_pthread_mutex_unlock()" );

    return status;
}
