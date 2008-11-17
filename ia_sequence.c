#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
//#include <stdlib.h>
//#include <string.h>

#include <FreeImage.h>
#include "common.h"
#include "iaio.h"
#include "ia_sequence.h"

ia_image_t** ia_seq_get_input_bufs( ia_seq_t* ias, uint64_t start, uint8_t size )
{
    ia_image_t** iab = malloc( sizeof(ia_image_t*)*size );
    if( iab == NULL ) {
        printf("ERROR\n");
        return NULL;
    }
    ia_image_t* iaf;
    int i = 0, k;

    if( size > ias->param->i_maxrefs )
        return NULL;

    while( i < size ) {
        for( k = 0; k < ias->param->i_maxrefs; k++ ) {
            if( ias->ref[k]->i_frame == start )
            {
                iaf = ias->ref[k];

                /* get lock on image */
                pthread_mutex_lock( &iaf->mutex );

                if( iaf->ready && iaf->i_frame == start ) {
                    iaf->users++;
                    iab[i++] = iaf;
                    start--;
                }
                else
                {
                    printf("the inb i need isnt ready\n"); fflush(stdout);
                }

                /* unlock image */
                pthread_mutex_unlock( &iaf->mutex );
            }
            else
                usleep( 10 );

        }
    }
    return iab;
}

inline void ia_seq_close_input_bufs( ia_image_t** iab, uint8_t size )
{
    int s = (int) size;
    if( iab == NULL ) return;
    printf("%u %u %u\n",size,size,size );

    size--;
    do
    {
        /* get lock on image */
        pthread_mutex_lock( &iab[size]->mutex );

        iab[size]->users--;
        iab[size]->ready = false;

        /* unlock image */
        pthread_mutex_unlock( &iab[size]->mutex );
        s--;
    } while( s > 0 );

//    ia_free( iab );
}

ia_image_t** ia_seq_get_output_bufs( ia_seq_t* ias, uint8_t size, uint64_t num )
{
    ia_image_t** iab = malloc( sizeof(ia_image_t*)*size );
    ia_image_t* iaf;
    int i, k;

    i = k = 0;

    if( size > ias->param->i_maxrefs )
        return NULL;

    /* acquire size output buffers */
    do {
        /* for each possible output buffer */
        for( k = 0; k < ias->param->i_maxrefs; k++ ) {
            /* if there is a chance this buffer is not being used */
            if( !ias->out[k]->ready && ias->out[k]->users == 0 )
            {
                iaf = ias->out[k];

                /* get lock on image */
                pthread_mutex_lock( &iaf->mutex );

                /* if the buffer is available -> claim it */
                if( !iaf->ready && iaf->users == 0 ) {
                    iaf->users++;
                    iab[i++] = iaf;
                    snprintf(iaf->name,1024,"image-%05lld.bmp",num);
                }

                /* unlock image */
                pthread_mutex_unlock( &iaf->mutex );
            }
            else
                usleep( 10 );

            if( i >= size )
                break;
        }
    } while( i < size );
    return iab;
}

inline void ia_seq_close_output_bufs( ia_image_t** iab, uint8_t size )
{
    if( iab == NULL ) return;

    while( size-- )
    {
        /* get lock on image */
        pthread_mutex_lock( &iab[size]->mutex );

        iab[size]->users--;
        iab[size]->ready = true;

        /* unlock image */
        pthread_mutex_unlock( &iab[size]->mutex );
    }

    free( iab );
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
    uint64_t i_maxrefs = ias->param->i_maxrefs - 1;
    uint64_t cframe = 0;

    /* while there is more input */
    for( ;; ) {
        iaf = ias->ref[i_maxrefs];

        /* get lock on oldest ref */
        pthread_mutex_lock( &iaf->mutex );

        /* if no one is using the ref, shift ref-list
         * down and replace with new frame */
        if( iaf->users == 0 && !iaf->ready ) {
            int i;
            i = i_maxrefs;


            /* shift refs down */
            while( i-- ) {
                ias->ref[i+1] = ias->ref[i];
             }
            ias->ref[0] = iaf;

            /* capture new frame, if error/eof -> exit */
            if( iaio_getimage(ias->iaio, iaf) )
            {
                fprintf( stderr, "EOI: ia_seq_manage_input(): end of input\n" );
                ias->iaio->eoi = true;
                pthread_mutex_unlock( &iaf->mutex );
                pthread_cancel( ias->tio[1] );
                pthread_exit( NULL );
            }
            snprintf( ias->ref[0]->name, 1024, "%s/image-%010lld.%s", ias->param->output_directory, cframe, ias->param->ext );
            ias->ref[0]->i_frame = ias->i_frame = cframe;
            cframe++;
            iaf->ready = true;
        }

        pthread_mutex_unlock( &iaf->mutex );
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

    /* while there is more output */
    for( ;; ) {
        for( i = 0; i < i_maxrefs; i++ ) {
            /* if the output buffer isnt ready to be written -> continue */
            if( !ias->out[i]->ready || ias->out[i]->users != 0 ) {
                //usleep( 1 );
                continue;
            }

            iar = ias->out[i];

            /* get lock on output buffer */
            pthread_mutex_lock( &iar->mutex );

            /* verify that its ready to be written -> write it out */
            if( iar->ready && iar->users == 0 ) {
                /* if error/nospc -> exit */
                if( iaio_saveimage(ias->iaio, iar) ) {
                    pthread_mutex_unlock( &iar->mutex );
                    pthread_exit( NULL );
                }
                iar->ready = false;
            }

            /* free up the lock */
            pthread_mutex_unlock( &iar->mutex );
        }
        //usleep( 10 );
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

                printf("pthread ESRCH = %d\n",ESRCH);
                rc = pthread_cancel( ias->tio[0] );
                printf("pthread_cancel errcode: %d\n",rc );
        
                /* wait a little bit before killing the output manager */
                usleep( 50 );
                rc = pthread_cancel( ias->tio[1] );
                printf( "pthread_cancel errcode: %d\n", rc );
                
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
    }

    s->i_frame = 0;

    pthread_attr_init( &s->attr );
    pthread_attr_setdetachstate( &s->attr, PTHREAD_CREATE_JOINABLE );

    rc = pthread_create( &s->tio[0], &s->attr, &ia_seq_manage_input, (void*) s );
    if( rc )
    {
        fprintf( stderr, "ERROR: ia_seq_open(): return code from pthread_create() is %d\n", rc );
        return NULL;
    }

    rc = pthread_create( &s->tio[1], &s->attr, &ia_seq_manage_output, (void*) s );
    if( rc )
    {
        fprintf( stderr, "ERROR: ia_seq_open(): return code from pthread_create() is %d\n", rc );
        return NULL;
    }

    return s;
}

inline void ia_seq_close( ia_seq_t* s )
{
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
        pthread_mutex_destroy( &s->ref[s->param->i_maxrefs]->mutex );
        pthread_mutex_destroy( &s->out[s->param->i_maxrefs]->mutex );

        ia_free( s->ref[s->param->i_maxrefs]->pix );
        ia_free( s->ref[s->param->i_maxrefs] );

        ia_free( s->out[s->param->i_maxrefs]->pix );
        ia_free( s->out[s->param->i_maxrefs] );
    }

    ia_free( s->ref );
    ia_free( s->out );

    ia_free( s );
}
