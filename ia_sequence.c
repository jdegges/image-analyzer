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

/*
 * ia_seq_manage_input:
 *  vptr: an ia_seq_t* data structure
 * keeps ref list up to-date by replacing old refs with newly captuerd frames
 */
void* ia_seq_manage_input( void* vptr )
{
    ia_seq_t* ias = (ia_seq_t*) vptr;
    ia_image_t* iaf;
    uint64_t i_maxrefs = ias->param->i_maxrefs;
    uint64_t i_frame = 0;

    /* while there is more input */
    for( ;; ) {
        iaf = ia_queue_pop( ias->input_free );
        
        /* capture new frame, if error/eof -> exit */
        if( i_frame > 200 || iaio_getimage(ias->iaio, iaf) )
        {
            fprintf( stderr, "EOI: ia_seq_manage_input(): end of input\n" );
            ias->iaio->eoi = true;
            ias->iaio->last_frame = i_frame;
            iaf->users = -1;
            while( i_maxrefs-- ) {
                ia_queue_push( ias->input_queue, iaf );
                iaf = ia_queue_pop( ias->input_free );
                iaf->users = -1;
            }
            ia_queue_push( ias->input_queue, iaf );
            fprintf( stderr, "closing input manager\n" );
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
    uint64_t i_maxrefs = ias->param->i_maxrefs;
    uint64_t i_frame = 0;
    int end = i_maxrefs;

    /* while there is more output */
    for( ;; ) {
        iar = ia_queue_pop( ias->output_queue );
        assert( iar != NULL );
        if( iar->users < 0 ) {
            end--;
            ia_queue_push( ias->output_free, iar );
            if( end == 0 ) {
                fprintf( stderr, "closing output manager\n" );
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

        ia_queue_push( ias->output_free, iar );
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
    s->input_queue = ia_queue_open( s->param->i_maxrefs );
    if( s->input_queue == NULL )
        return NULL;
    s->input_free = ia_queue_open( s->param->i_maxrefs );
    if( s->input_free == NULL )
        return NULL;
    i = s->param->i_maxrefs;
    while( i-- ) {
        iaf = ia_malloc( sizeof(ia_image_t) );
        if( iaf == NULL )
            return NULL;
        ia_memset( iaf, 0, sizeof(ia_image_t) );
        iaf->pix = ia_malloc( sizeof(ia_pixel_t)*s->param->i_size*3 );
        if( iaf->pix == NULL )
            return NULL;
        pthread_mutex_init( &iaf->mutex, NULL );
        ia_pthread_cond_init( &iaf->cond_ro, NULL );
        ia_pthread_cond_init( &iaf->cond_rw, NULL );
        ia_queue_push( s->input_free, iaf );
    }

    /* allocate output buffers */
    s->output_queue = ia_queue_open( s->param->i_maxrefs*3 );
    if( s->output_queue == NULL )
        return NULL;
    s->output_free = ia_queue_open( s->param->i_maxrefs*3 );
    if( s->output_free == NULL )
        return NULL;
    // fill free queue
    i = s->param->i_maxrefs*3;
    while( i-- ) {
        iaf = ia_malloc( sizeof(ia_image_t) );
        if( iaf == NULL )
            return NULL;
        ia_memset( iaf, 0, sizeof(ia_image_t) );
        iaf->pix = ia_malloc( sizeof(ia_pixel_t)*s->param->i_size*3 );
        if( iaf->pix == NULL )
            return NULL;
        pthread_mutex_init( &iaf->mutex, NULL );
        ia_pthread_cond_init( &iaf->cond_ro, NULL );
        ia_pthread_cond_init( &iaf->cond_rw, NULL );
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
    if( rc ) {
        fprintf( stderr, "ERROR in ia_sq_close1: %d\n", rc );
    }

    rc = ia_pthread_join( s->tio[1], &status );
    if( rc ) {
        fprintf( stderr, "ERROR in ia_seq_close2: %d\n", rc );
    }

    pthread_attr_destroy( &s->attr );

    iaio_close( s->iaio );

    ia_queue_close( s->output_free );
    ia_queue_close( s->output_queue );
    ia_queue_close( s->input_free );
    ia_queue_close( s->input_queue );

    ia_free( s );
}
