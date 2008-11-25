#ifndef __H_QUEUE
#define __H_QUEUE

#include <pthread.h>
#include "common.h"

typedef struct ia_queue_t
{
    ia_image_t** list;
    size_t r, w;
    pthread_mutex_t mutex;
    size_t size;
} ia_queue_t;

ia_queue_t* ia_queue_open( size_t size, int image_size );
void ia_queue_close( ia_queue_t* iaq );
void ia_queue_push( ia_image_t* iaf );
ia_image_t* ia_queue_pop( void );

#endif
