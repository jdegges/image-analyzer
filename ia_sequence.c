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

                /* get lock on image */
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
//                    printf("grabbed image %lld for processing. has %d users\n",iaf->i_frame,iaf->users);
                }
                else
                {
//                    printf("the inb i need isnt ready\n"); fflush(stdout);
                }

                /* unlock image */
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
        /* get lock on image */
        rc = pthread_mutex_lock( &iab[size]->mutex );
        if( rc != 0 ) {
            fprintf( stderr, "ERROR: ia_seq_close_input_bufs(): pthread_mutex_lock returned code %d\n", rc );
            return;
        }

        iab[size]->users--;
        if( iab[size]->users == 0 )
            iab[size]->ready = false;

        /* unlock image */
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

    /* acquire size output buffers */
    do {
        /* for each possible output buffer */
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

            /* if this buffer is not being used */
            if( !iaf->ready && iaf->users == 0 )
            {
                iaf->users++;
                iab[i++] = iaf;
                snprintf(iaf->name,1024,"image-%05lld.bmp",num);
            } else {
                /* unlock image */
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

        /* unlock image */
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

    /* make sure all output buffers have been written */
    for( i = 0; i < ias->param->i_maxrefs; i++ )
    {
        for( ;; ) {
            if( !ias->out[i]->ready && ias->out[i]->users == 0 ) {
                iaf = ias->out[i];

                /* get lock on image */
                rc = pthread_mutex_lock( &iaf->mutex );
                if( rc != 0 ) {
                    fprintf( stderr, "ERROR: ia_seq_get_output_bufs(): pthread_mutex_lock returned code %d\n", rc );
                    return;
                }

                /* make sure output buffer is not in use */
                if( !iaf->ready && iaf->users == 0 ) {
                    /* unlock image */
                    rc = ia_pthread_mutex_unlock( &iaf->mutex );
                    if( rc != 0 ) {
                        fprintf( stderr, "ERROR: ia_seq_get_output_bufs(): ia_pthread_mutex_unlock returned code %d\n", rc );
                        return;
                    }
                    break;
                }

                /* unlock image */
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
    int bufno;

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
    uint64_t i, i_maxrefs = ias->param->i_maxrefs;
    uint64_t i_frame = 0;
    int rc;
    int time_to_exit = 0;

    /* while there is more output */
    for( ;; ) {
        if( ias->iaio->eoi && time_to_exit++ > i_maxrefs ) {
            pthread_exit( NULL );
        }

        int bufno = i_frame % i_maxrefs;
        iar = ias->out[bufno];

        ia_error( "manage_output: about to lock mutex of buf %d\n", bufno );
        rc = ia_pthread_mutex_lock( &iar->mutex );
        ia_pthread_error( rc, "ia_seq_manage_output()", "ia_pthread_mutex_lock()" );
        ia_error( "manage_output: got lock on buf %d\n", bufno );

        if( !iar->ready )
        {
            ia_error( "manage_output: doing cond_ro wait on buf %d\n", bufno );
            rc = ia_pthread_cond_wait( &iar->cond_ro, &iar->mutex );
            ia_pthread_error( rc, "ia_seq_manage_output()", "ia_pthread_cond_wait()" );
            ia_error( "manage_output: cond_ro wait returned for buf %d\n", bufno );
        }
        assert( iar->ready );
        iar->users++;

        snprintf( iar->name, 1024, "%s/image-%010lld.%s", ias->param->output_directory, i_frame, ias->param->ext );
        if( iaio_saveimage(ias->iaio, iar) )
        {
            rc = ia_pthread_mutex_unlock( &iar->mutex );
            ia_pthread_error( rc, "ia_seq_manage_output()", "ia_pthread_mutex_unlock()" );
            pthread_exit( NULL );
        }
        i_frame++;

        iar->users--;
        if( !iar->users )
        {
            iar->ready = false;
            rc = ia_pthread_cond_signal( &iar->cond_rw );
            ia_pthread_error( rc, "ia_seq_manage_output()", "ia_pthread_cond_signal()" );
        }

        rc = ia_pthread_mutex_unlock( &iar->mutex );
        ia_pthread_error( rc, "ia_seq_manage_output()", "ia_pthread_mutex_unlock()" );
    }
}

/*
 * ia_seq_end:
 *  vptr: ia_sequence_t* data structure
 * clean up io threads if the end of input has been reached
 */
bool ia_seq_has_more_input( ia_seq_t* ias, uint64_t pos ) {
//printf("chcking for more input\n");
    if( ias->iaio->eoi ) {
        int rc;
        for( rc = 0; rc < ias->param->i_maxrefs; rc++ ) {
            if( pos > ias->ref[rc]->i_frame ) {


                printf("no more input\n");

                //printf("pthread ESRCH = %d\n",ESRCH);
                //rc = pthread_cancel( ias->tio[0] );
                //printf("pthread_cancel errcode: %d\n",rc );
        
                /* wait a little bit before killing the output manager */
                //usleep( 50 );
                //rc = pthread_cancel( ias->tio[1] );
                //printf( "pthread_cancel errcode: %d\n", rc );
                
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
    s->out = ia_malloc( sizeof(ia_image_t*)*s->param->i_maxrefs );
    if( s->out == NULL )
        return NULL;
    i = s->param->i_maxrefs;
    while( i-- ) {
        s->out[i] = ia_malloc( sizeof(ia_image_t) );
        if( s->out[i] == NULL )
            return NULL;
        ia_memset( s->out[i], 0, sizeof(ia_image_t) );
        s->out[i]->pix = ia_malloc( sizeof(ia_pixel_t)*s->param->i_size*3 );
        if( s->out[i]->pix == NULL )
            return NULL;
        pthread_mutex_init( &s->out[i]->mutex, NULL );
        ia_pthread_cond_init( &s->ref[i]->cond_ro, NULL );
        ia_pthread_cond_init( &s->ref[i]->cond_rw, NULL );
    }

    s->i_frame = 0;

    pthread_attr_init( &s->attr );
//    pthread_attr_setdetachstate( &s->attr, ia_pthread_create_JOINABLE );
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

    printf("closing iaio\n");
    fflush(stdout);

    iaio_close( s->iaio );

    printf("freeing buffers\n");
    fflush(stdout);

    while( s->param->i_maxrefs-- )
    {
//        printf("freeing bufs %d\n",s->param->i_maxrefs ); fflush(stdout);
        pthread_mutex_destroy( &s->ref[s->param->i_maxrefs]->mutex );
//        printf("free'd ref's mutex\n" ); fflush(stdout);
        pthread_cond_destroy( &s->ref[s->param->i_maxrefs]->cond_ro );
//        printf("free'd ref's condro\n" ); fflush(stdout);
        pthread_cond_destroy( &s->ref[s->param->i_maxrefs]->cond_rw );
//        printf("free'd ref's condrw\n" ); fflush(stdout);

        pthread_mutex_destroy( &s->out[s->param->i_maxrefs]->mutex );
//        printf("free'd out's mutex\n" ); fflush(stdout);
        pthread_cond_destroy( &s->out[s->param->i_maxrefs]->cond_rw );
//        printf("free'd out's condrw\n" ); fflush(stdout);
        pthread_cond_destroy( &s->out[s->param->i_maxrefs]->cond_ro );
//        printf("free'd out's condro\n" ); fflush(stdout);


        ia_free( s->ref[s->param->i_maxrefs]->pix );
        ia_free( s->ref[s->param->i_maxrefs] );

        ia_free( s->out[s->param->i_maxrefs]->pix );
        ia_free( s->out[s->param->i_maxrefs] );
    }

//    printf("free'd buffers\n"); fflush(stdout);

    ia_free( s->ref );
//    printf("free'd refs\n"); fflush(stdout);
    
    ia_free( s->out );
//    printf("free'd out\n"); fflush(stdout);

    ia_free( s );
    printf("closed ia_seq_t\n"); fflush(stdout);
}
