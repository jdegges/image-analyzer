#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <assert.h>
//#include <stdlib.h>
//#include <string.h>

#include <FreeImage.h>
#include "common.h"
#include "iaio.h"
#include "ia_sequence.h"
#include "analyze.h"
#include "queue.h"

/*
 * ia_seq_manage_input:
 *  vptr: an ia_seq_t* data structure
 * keeps ref list up to-date by replacing old refs with newly captuerd frames
 */
void* ia_seq_manage_input( void* vptr )
{
    ia_seq_t* ias = (ia_seq_t*) vptr;
    ia_image_t* iaf;
    uint64_t i_threads = ias->param->i_threads;
    uint64_t i_frame = 0;

    /* while there is more input */
    for( ;; ) {
        iaf = ia_queue_pop( ias->input_free );
        
        /* capture new frame, if error/eof -> exit */
        if( i_frame > 200 || iaio_getimage(ias->iaio, iaf) )
        {
            ias->iaio->eoi = true;
            ias->iaio->last_frame = i_frame;
            iaf->eoi = true;
            while( i_threads-- ) {
                ia_queue_push( ias->input_queue, iaf );
                iaf = ia_queue_pop( ias->input_free );
                iaf->eoi = true;
            }
            ia_queue_push( ias->input_queue, iaf );
            pthread_exit( NULL );
        }
        snprintf( iaf->name, 1024, "%s/image-%010lld.%s", ias->param->output_directory, i_frame, ias->param->ext );
        iaf->i_frame = ias->i_frame = i_frame++;

        ia_queue_push( ias->input_queue, iaf );
    }
}

/*
 * ia_seq_manage_output:
 *  vptr: an ia_sequence_t* data structure
 * writes all output buffers to disk and free's up the output buffer entry
 */
void* ia_seq_manage_output( void* vptr )
{
    ia_seq_t* ias = (ia_seq_t*) vptr;
    ia_image_t* iar;
    uint64_t i_threads = ias->param->i_threads;
    uint64_t i_frame = 0;
    int end = i_threads;

    /* while there is more output */
    for( ;; ) {
        iar = ia_queue_pop( ias->output_queue );
        assert( iar != NULL );
        if( iar->eoi ) {
            end--;
            ia_queue_shove( ias->output_free, iar );
            if( end == 0 ) {
                pthread_exit( NULL );
            }
            continue;
        }

        snprintf( iar->name, 1024, "%s/image-%010lld.%s", ias->param->output_directory, i_frame, ias->param->ext );
        if( iaio_outputimage(ias->iaio, iar) )
        {
            fprintf( stderr, "unable to save image\n" );
            pthread_exit( NULL );
        }
        i_frame++;

        ia_queue_shove( ias->output_free, iar );
    }
}

/* initialize a new ia_seq */
ia_seq_t*   ia_seq_open( ia_param_t* p )
{
    int i, rc;
    ia_image_t* iaf;

    /* allocate/initialize sequence object */
    ia_seq_t* s = (ia_seq_t*) ia_malloc( sizeof(ia_seq_t) );
    if( s == NULL ) {
        fprintf( stderr, "ERROR: ia_seq_open(): couldnt alloc ia_seq_t*\n" );
        return NULL;
    }

    ia_memset( s,0,sizeof(ia_seq_t) );
    s->param = p;

    s->iaio = iaio_open( s );
    if( s->iaio == NULL )
    {
        fprintf( stderr, "ERROR: ia_seq_open(): couldnt open iaio_t object\n" );
        return NULL;
    }

    pthread_mutex_init( &s->eoi_mutex, NULL );

    /* allocate input buffers */
    s->input_queue = ia_queue_open( s->param->i_threads*2 );
    if( s->input_queue == NULL )
        return NULL;
    s->input_free = ia_queue_open( s->param->i_threads*2 );
    if( s->input_free == NULL )
        return NULL;
    i = s->param->i_threads*2;
    while( i-- ) {
        iaf = ia_image_create( s->param->i_size*3 );
        if( iaf == NULL )
            return NULL;
        ia_queue_push( s->input_free, iaf );
    }

    /* allocate output buffers */
    s->output_queue = ia_queue_open( s->param->i_threads );
    if( s->output_queue == NULL )
        return NULL;
    s->output_free = ia_queue_open( s->param->i_threads );
    if( s->output_free == NULL )
        return NULL;
    // fill free queue
    i = s->param->i_threads;
    while( i-- ) {
        iaf = ia_image_create( s->param->i_size*3 );
        if( iaf == NULL )
            return NULL;
        ia_queue_push( s->output_free, iaf );
    }
    s->i_frame = 0;

    pthread_attr_init( &s->attr );
    pthread_attr_setdetachstate( &s->attr, PTHREAD_CREATE_JOINABLE );

    rc = ia_pthread_create( &s->tio[0], &s->attr, &ia_seq_manage_input, (void*) s );
    ia_pthread_error( rc, "ia_seq_open()", "ia_pthread_create()" );

    rc = ia_pthread_create( &s->tio[1], &s->attr, &ia_seq_manage_output, (void*) s );
    ia_pthread_error( rc, "ia_seq_open()", "ia_pthread_create()" );

    return s;
}

inline void ia_seq_close( ia_seq_t* s )
{
    void* status;
    int rc;

    rc = ia_pthread_join( s->tio[0], &status );
    ia_pthread_error( rc, "ia_seq_close()", "ia_pthread_join()" );

    rc = ia_pthread_join( s->tio[1], &status );
    ia_pthread_error( rc, "ia_seq_close()", "ia_pthread_join()" );

    pthread_attr_destroy( &s->attr );

    iaio_close( s->iaio );

    ia_queue_close( s->output_free );
    ia_queue_close( s->output_queue );
    ia_queue_close( s->input_free );
    ia_queue_close( s->input_queue );

    ia_free( s );
}
