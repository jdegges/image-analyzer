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

#ifndef _H_QUEUE
#define _H_QUEUE

#include <pthread.h>
#include "common.h"

typedef enum ia_queue_pushtype_t
{
    QUEUE_TAP,      // non blocking push
    QUEUE_PUSH,     // blocking push
    QUEUE_SHOVE,    // non blocking push + dynamic resize
    QUEUE_PSORT,    // push items in sorted order
    QUEUE_SSORT     // shove items in sorted order
} ia_queue_pushtype_t;

typedef struct ia_queue_obj_t
{
    uint32_t                i_pos;
    void*                   data;
    uint8_t                 i_refcount;
    struct ia_queue_obj_t*  next;
    struct ia_queue_obj_t*  last;
    pthread_mutex_t         mutex;
    pthread_cond_t          cond_nonempty;
    pthread_cond_t          cond_nonfull;
} ia_queue_obj_t;

typedef struct ia_queue_t
{
    ia_queue_obj_t* head;
    ia_queue_obj_t* tail;
    uint32_t        count;
    uint32_t        size;
    uint32_t        time;
    pthread_mutex_t mutex;
    pthread_cond_t  cond_nonempty;
    pthread_cond_t  cond_nonfull;
} ia_queue_t;

ia_queue_t* ia_queue_open( size_t size );
void ia_queue_close( ia_queue_t* q );
int ia_queue_tap( ia_queue_t* q, void* data, uint32_t pos );
void ia_queue_push( ia_queue_t* q, void* data, uint32_t pos );
int ia_queue_shove( ia_queue_t* q, void* data, uint32_t pos );
int ia_queue_push_sorted( ia_queue_t* q, void* data, uint32_t pos );
int ia_queue_shove_sorted( ia_queue_t* q, void* data, uint32_t pos );
void* ia_queue_pop( ia_queue_t* q );
void* ia_queue_pek( ia_queue_t* q, uint32_t pos );
void ia_queue_sht( ia_queue_t* q, ia_queue_t* f, void* data, uint8_t count );
void* ia_queue_pop_item( ia_queue_t* q, uint32_t pos );
int ia_queue_is_full( ia_queue_t* q );
int ia_queue_is_empty( ia_queue_t* q );

#endif
