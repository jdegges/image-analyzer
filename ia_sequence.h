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

#ifndef _H_IA_SEQUENCE
#define _H_IA_SEQUENCE

#include <stdint.h>
#include <pthread.h>
#include <stdio.h>
#include <stdbool.h>

#include "common.h"
#include "image_analyzer.h"
#include "iaio.h"
#include "queue.h"

#define MAX_THREADS 32

typedef void* ia_filter_param_t;

typedef struct ia_seq_t
{
    ia_queue_t*         input_queue;    // input frames
    ia_queue_t*         input_free;

    ia_queue_t*         proc_queue;
    ia_queue_t*         output_queue;   // output queue
    ia_queue_t*         output_free;    // free buffer queue

    uint64_t            i_frame;        // position of iaf in sequence
    ia_param_t*         param;          // contains all sequence parameters
    ia_filter_param_t   fparam[20];     // contains filter parameters

    pthread_t           tio[2];         // 0 - id of read thread
                                        // 1 - id of write thread
    pthread_attr_t      attr;

    struct iaio_t*      iaio;           // used to hold specifics of io
    pthread_mutex_t     eoi_mutex;

    /* FIXME these should be moved to analyze.c. returned by init() as void*
     * and passed to bhatta() .. or just created in bhatta() each time */
    uint8_t*            mask;           // used in bhatta
    uint8_t*            diffImage;      // bhatta
} ia_seq_t;


typedef struct ia_exec_t
{
    ia_seq_t* ias;
    int bufno;
} ia_exec_t;

/* opens and initializes an ia_seq_t object with paramenters p */
ia_seq_t*           ia_seq_open ( ia_param_t* p );

/* closes ia sequence */
inline void         ia_seq_close ( ia_seq_t* s );

/* returns true if there is more input to be processed, false otherwise */
inline bool         ia_seq_has_more_input( ia_seq_t* ias, uint64_t pos );

/* get a list of current images */
ia_image_t**        ia_seq_get_input_bufs( ia_seq_t* ias, uint8_t num );

/* close input buffers */
inline void         ia_seq_close_input_bufs( ia_image_t** iab, int8_t size );

/* get size output buffers */
ia_image_t**        ia_seq_get_output_bufs( ia_seq_t* ias, uint8_t size, uint64_t num );

/* close either list of output buffers */
inline void         ia_seq_close_output_bufs( ia_image_t** iab, int8_t size );

#endif
