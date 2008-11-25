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
ia_image_t** ia_seq_get_input_bufs( ia_seq_t* ias, uint64_t start, uint8_t size )
{
    ia_image_t** iab = malloc( sizeof(ia_image_t*)*size );
    if( iab == NULL ) {
        printf("ERROR\n");
        return NULL;
    }
    ia_image_t* iaf;
    int i = 0, k, rc, j;
    uint64_t frameno = start;

    if( size > ias->param->i_maxrefs )
        return NULL;

    for( j = 0; j < size; ) {
        for( k = 0; k < ias->param->i_maxrefs && j < size; k++ ) {
            if( ias->iaio->eoi && ias->iaio->last_frame <= start ) {
                return NULL;
            }
            if( !ias->ref[k]->ready ) {
                usleep( 20 );
                continue;
            }

            if( ias->ref[k]->i_frame == start )
            {
                iaf = ias->ref[k];

                // get lock on image
                rc = pthread_mutex_lock( &iaf->mutex );
                if( rc != 0 ) {
                    fprintf( stderr, "ERROR: ia_seq_get_input_bufs(): pthread_mutex_lock returned code %d\n", rc );
                    return NULL;
                }

                if( iaf->ready && iaf->i_frame == start ) {
                    iaf->users++;
                    iab[i++] = iaf;
                    frameno--;
                    j++;
                }

                // unlock image
                rc = ia_pthread_mutex_unlock( &iaf->mutex );
                if( rc != 0 ) {
                    fprintf( stderr, "ERROR: ia_seq_get_input_bufs(): ia_pthread_mutex_unlock returned code %d\n", rc );
                    return NULL;
                }
            }
            else
                usleep( 10 );
        }
    }
    return iab;
}

inline void ia_seq_close_input_bufs( ia_image_t** iab, int8_t size )
{
    int rc;
    if( iab == NULL || size < 0 ) return;

    while( size-- )
    {
        rc = pthread_mutex_lock( &iab[size]->mutex );
        if( rc != 0 ) {
            fprintf( stderr, "ERROR: ia_seq_close_input_bufs(): pthread_mutex_lock returned code %d\n", rc );
            return;
        }

        iab[size]->users--;
        if( iab[size]->users == 0 )
            iab[size]->ready = false;

        rc = ia_pthread_mutex_unlock( &iab[size]->mutex );
        if( rc != 0 ) {
            fprintf( stderr, "ERROR: ia_seq_close_input_bufs(): ia_pthread_mutex_unlock returned code %d\n", rc );
            return;
        }
    }

    ia_free( iab );
}

ia_image_t** ia_seq_get_output_bufs( ia_seq_t* ias, uint8_t size, uint64_t num )
{
    ia_image_t** iab = malloc( sizeof(ia_image_t*)*size );
    ia_image_t* iaf;
    int i, k, rc;

    i = k = 0;

    if( size > ias->param->i_maxrefs )
        return NULL;

    do {
        for( k = 0; k < ias->param->i_maxrefs && i < size; k++ ) {
            iaf = ias->out[k];

            rc = ia_pthread_mutex_trylock( &iaf->mutex );
            if( rc == EBUSY ) {
                usleep( 20 );
                continue;
            }

            if( rc != 0 ) {
                fprintf( stderr, "ERROR: ia_seq_get_output_bufs(): pthread_mutex_lock returned code %d\n", rc );
                return NULL;
            }

            if( !iaf->ready && iaf->users == 0 )
            {
                iaf->users++;
                iab[i++] = iaf;
                snprintf(iaf->name,1024,"image-%05lld.bmp",num);
            } else {
                rc = ia_pthread_mutex_unlock( &iaf->mutex );
                if( rc != 0 ) {
                    fprintf( stderr, "ERROR: ia_seq_get_output_bufs(): ia_pthread_mutex_unlock returned code %d\n", rc );
                    return NULL;
                }
            }
        }
    } while( i < size );
    return iab;
}

inline void ia_seq_close_output_bufs( ia_image_t** iab, int8_t size )
{
    int rc;
    if( iab == NULL ) return;

    while( size-- )
    {
        iab[size]->users--;
        assert( iab[size]->users == 0 );
        iab[size]->ready = true;

        rc = ia_pthread_mutex_unlock( &iab[size]->mutex );
        if( rc != 0 ) {
            fprintf( stderr, "ERROR: ia_seq_close_output_bufs(): ia_pthread_mutex_unlock returned code %d\n", rc );
            return;
        }
    }

    free( iab );
}

void ia_seq_wait_for_output( ia_seq_t* ias )
{
    ia_image_t* iaf;
    int i, k, rc;

    i = k = 0;

    for( i = 0; i < ias->param->i_maxrefs; i++ )
    {
        for( ;; ) {
            if( !ias->out[i]->ready && ias->out[i]->users == 0 ) {
                iaf = ias->out[i];

                rc = pthread_mutex_lock( &iaf->mutex );
                if( rc != 0 ) {
                    fprintf( stderr, "ERROR: ia_seq_get_output_bufs(): pthread_mutex_lock returned code %d\n", rc );
                    return;
                }

                if( !iaf->ready && iaf->users == 0 ) {
                    rc = ia_pthread_mutex_unlock( &iaf->mutex );
                    if( rc != 0 ) {
                        fprintf( stderr, "ERROR: ia_seq_get_output_bufs(): ia_pthread_mutex_unlock returned code %d\n", rc );
                        return;
                    }
                    break;
                }

                rc = ia_pthread_mutex_unlock( &iaf->mutex );
                if( rc != 0 ) {
                    fprintf( stderr, "ERROR: ia_seq_get_output_bufs(): ia_pthread_mutex_unlock returned code %d\n", rc );
                    return;
                }
            }
            else usleep( 33 );
        }
    }
    return;
}
*/

/*
 * ia_seq_manage_input:
 *  vptr: an ia_sequence_t* data structure
 * keeps ref list up to-date by replacing old refs with newly captuerd frames
 */
void* ia_seq_manage_input( void* vptr )
{
    ia_seq_t* ias = (ia_seq_t*) vptr;
    ia_image_t* iaf;
    uint64_t i_maxrefs = ias->param->i_maxrefs;
    uint64_t i_frame = 0;
    int rc;
    //int bufno;

    /* while there is more input */
    for( ;; ) {
        iaf = ias->ref[i_frame % i_maxrefs];

        /* get lock on next ref */
        //ia_error( "manage_input: -l%d input\n", bufno
        rc = ia_pthread_mutex_lock( &iaf->mutex );
        ia_pthread_error( rc, "ia_seq_manage_input()", "ia_pthread_mutex_lock()" );

        if( iaf->ready || iaf->users )
        {
            rc = ia_pthread_cond_wait( &iaf->cond_rw, &iaf->mutex );
            ia_pthread_error( rc, "ia_seq_manage_input()", "ia_pthread_cond_wait()" );
        }
        assert( !iaf->ready && !iaf->users );
        iaf->users++;

        /* capture new frame, if error/eof -> exit */
        if( i_frame > 200 || iaio_getimage(ias->iaio, iaf) )
        {
            fprintf( stderr, "EOI: ia_seq_manage_input(): end of input\n" );
            ias->iaio->eoi = true;
            ias->iaio->last_frame = i_frame;
            ia_pthread_mutex_unlock( &iaf->mutex );
            pthread_exit( NULL );
        }
        snprintf( iaf->name, 1024, "%s/image-%010lld.%s", ias->param->output_directory, i_frame, ias->param->ext );
        iaf->i_frame = ias->i_frame = i_frame++;

        iaf->ready = true;
        iaf->users = 0;

        rc = ia_pthread_cond_signal( &iaf->cond_ro );
        ia_pthread_error( rc, "ia_seq_manage_input()", "ia_pthread_cond_signal()" );

        rc = ia_pthread_mutex_unlock( &iaf->mutex );
        ia_pthread_error( rc, "ia_seq_manage_input()", "ia_pthread_mutex_unlock()" );
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
    //int rc;
    uint64_t time_to_exit = 0;

    /* while there is more output */
    for( ;; ) {
        if( ias->iaio->eoi && time_to_exit++ > i_maxrefs ) {
            pthread_exit( NULL );
        }

        iar = ia_queue_pop( ias->output ); // replacement for code from above
        assert( iar != NULL );

        snprintf( iar->name, 1024, "%s/image-%010lld.%s", ias->param->output_directory, i_frame, ias->param->ext );
        if( iaio_saveimage(ias->iaio, iar) )
        {
            fprintf( stderr, "unable to save image\n" );
            pthread_exit( NULL );
        }
        i_frame++;

        ia_queue_push( ias->free, iar );
    }
}

/*
 * ia_seq_end:
 *  vptr: ia_sequence_t* data structure
 * clean up io threads if the end of input has been reached
 */
bool ia_seq_has_more_input( ia_seq_t* ias, uint64_t pos ) {
    if( ias->iaio->eoi ) {
        int rc;
        for( rc = 0; rc < ias->param->i_maxrefs; rc++ ) {
            if( pos > ias->ref[rc]->i_frame ) {
                printf("no more input\n");
                return false;
            }
        }
    }
    return true;
}

/* initialize a new ia_seq */
ia_seq_t*   ia_seq_open( ia_param_t* p )
{
    int i, rc;

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
    s->ref = (ia_image_t**) ia_malloc( sizeof(ia_image_t*)*s->param->i_maxrefs );
    if( s->ref == NULL )
        return NULL;
    i = s->param->i_maxrefs;
    while( i-- ) {
        s->ref[i] = ia_malloc( sizeof(ia_image_t) );
        if( s->ref[i] == NULL )
            return NULL;
        ia_memset( s->ref[i], 0, sizeof(ia_image_t) );
        s->ref[i]->pix = ia_malloc( sizeof(ia_pixel_t)*s->param->i_size*3 );
        if( s->ref[i]->pix == NULL )
            return NULL;
        pthread_mutex_init( &s->ref[i]->mutex, NULL );
        ia_pthread_cond_init( &s->ref[i]->cond_ro, NULL );
        ia_pthread_cond_init( &s->ref[i]->cond_rw, NULL );
    }

    /* allocate output buffers */
    s->output = ia_queue_open( s->param->i_maxrefs*10 );
    if( s->output == NULL )
        return NULL;
    s->free = ia_queue_open( s->param->i_maxrefs*10 );
    if( s->free == NULL )
        return NULL;
    // fill free queue
    ia_image_t* iaf;
    i = s->param->i_maxrefs*10;
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
        ia_queue_push( s->free, iaf );
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

    pthread_cancel( s->tio[0] );
    pthread_cancel( s->tio[1] );

    iaio_close( s->iaio );

    while( s->free->count )
        ia_free( ia_queue_pop(s->free) );
    while( s->output->count )
        ia_free( ia_queue_pop(s->output) );
    ia_queue_close( s->free );
    ia_queue_close( s->output );

    while( s->param->i_maxrefs-- )
    {
        pthread_mutex_destroy( &s->ref[s->param->i_maxrefs]->mutex );
        pthread_cond_destroy( &s->ref[s->param->i_maxrefs]->cond_ro );
        pthread_cond_destroy( &s->ref[s->param->i_maxrefs]->cond_rw );

//        pthread_mutex_destroy( &s->out[s->param->i_maxrefs]->mutex );
//        pthread_cond_destroy( &s->out[s->param->i_maxrefs]->cond_rw );
//        pthread_cond_destroy( &s->out[s->param->i_maxrefs]->cond_ro );


        ia_free( s->ref[s->param->i_maxrefs]->pix );
        ia_free( s->ref[s->param->i_maxrefs] );

//        ia_free( s->out[s->param->i_maxrefs]->pix );
//        ia_free( s->out[s->param->i_maxrefs] );
    }

    ia_free( s->ref );
//    ia_free( s->out );
    ia_free( s );
}
