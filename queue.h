#ifndef _H_QUEUE
#define _H_QUEUE

#include <pthread.h>
#include "common.h"

typedef struct ia_queue_t
{
    ia_image_t* head;
    ia_image_t* tail;
    uint64_t count, size;
    pthread_mutex_t mutex;
    pthread_cond_t cond_ro;
    pthread_cond_t cond_rw;
} ia_queue_t;

ia_queue_t* ia_queue_open( size_t size );
void ia_queue_close( ia_queue_t* q );
void ia_queue_push( ia_queue_t* q, ia_image_t* iaf );
void ia_queue_shove( ia_queue_t* q, ia_image_t* iaf );
ia_image_t* ia_queue_pop( ia_queue_t* q );
int ia_queue_is_full( ia_queue_t* q );
int ia_queue_is_empty( ia_queue_t* q );

//void ia_queue_push_to( ia_queue_t* q, ia_image_t* iaf, uint64_t frameno );
//ia_image_t* ia_queue_pop_from( ia_queue_t* q, uint64_t frameno );
//void ia_queue_release_from( ia_queue_t* q, uint64_t frameno );

#endif
