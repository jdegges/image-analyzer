#ifndef _H_QUEUE
#define _H_QUEUE

#include <pthread.h>
#include "common.h"

typedef struct ia_queue_t
{
    ia_image_t** list;
    uint64_t r, w, count, size;
    pthread_mutex_t mutex;
    pthread_cond_t cond_ro;
    pthread_cond_t cond_rw;
} ia_queue_t;

ia_queue_t* ia_queue_open( size_t size );
void ia_queue_close( ia_queue_t* q );
void ia_queue_push( ia_queue_t* q, ia_image_t* iaf );
ia_image_t* ia_queue_pop( ia_queue_t* q );

#endif
