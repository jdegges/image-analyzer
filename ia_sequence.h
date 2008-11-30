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


typedef struct ia_seq_t
{
    ia_queue_t*         input_queue;    // input frames
    ia_queue_t*         input_free;

    ia_queue_t*         proc_queue;
    ia_queue_t*         output_queue;   // output queue
    ia_queue_t*         output_free;    // free buffer queue

    uint64_t            i_frame;    // position of iaf in sequence
    ia_param_t*         param;      // contains all parameters

    pthread_t           tio[2];     // 0 - id of read thread
                                    // 1 - id of write thread
    pthread_attr_t      attr;

    struct iaio_t*      iaio;       // used to hold specifics of io
    pthread_mutex_t     eoi_mutex;

    /* FIXME these should be moved to analyze.c. returned by init() as void*
     * and passed to bhatta() .. or just created in bhatta() each time */
    uint8_t*            mask;       // used in bhatta
    uint8_t*            diffImage;  // bhatta
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
