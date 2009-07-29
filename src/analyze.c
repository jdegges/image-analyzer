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

#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <errno.h>
#include <assert.h>
#include <unistd.h>

#include "common.h"
#include "iaio.h"
#include "image_analyzer.h"
#include "ia_sequence.h"
#include "analyze.h"
#include "filters/filters.h"

static inline ia_seq_t* analyze_init( ia_param_t* p )
{
    int j;
    ia_seq_t* ias = ia_seq_open( p );

    if( ias == NULL ) {
        fprintf( stderr, "ERROR: analyze_init(): couldnt open ia_seq\n" );
        return NULL;
    }

    init_filters();

    /* call any init functions */
    for ( j = 0; p->filter[j] != 0; j++ )
    {
        if( filters.init[p->filter[j]] ) {
            filters.init[p->filter[j]]( ias, &ias->fparam[p->filter[j]] );
        }
    }

    return ias;
}

static inline void analyze_deinit( ia_seq_t* s )
{
    int j;

    /* call any filter specific close functions */
    for ( j = 0; s->param->filter[j] != 0; j++ )
    {
        if( filters.clos[s->param->filter[j]] )
            filters.clos[s->param->filter[j]]( s->fparam[s->param->filter[j]] );
    }

    ia_seq_close( s );
}

void* analyze_exec( void* vptr )
{
    int no_filter = 0;
    ia_exec_t* iax = (ia_exec_t*) vptr;
    ia_seq_t* s = iax->ias;
    const int i_maxrefs = iax->ias->param->i_maxrefs;
    const int nrefs = iax->ias->nrefs;
    ia_image_t** iaim = malloc( sizeof(ia_image_t*)*i_maxrefs );

    if( !iaim )
        ia_pthread_exit( NULL );

    while( 1 )
    {
        ia_image_t *iaf, *iar;
        int j, rc;
        uint64_t i, current_frame;

        /* wait for input buf (wait for input manager signal) */
        iaf = ia_queue_pop( iax->ias->input_queue );
        if( iaf->eoi )
        {
            ia_queue_shove( iax->ias->output_queue, iaf, iaf->i_frame );
            break;
        }

        if( s->iaio->b_decode && 1 == iaio_freeimage_decode_image(s->iaio, iaf) ) {
            fprintf( stderr, "decoding image failed\n" );
            return NULL;
        }
        iaf->i_refcount = i_maxrefs;

        current_frame = iaf->i_frame;
        /* short curcuit the fancy reference frame gathering stuff if the
         * filter only needs one frame */
        if( 1 < i_maxrefs ) {
            int pos = current_frame % nrefs;

            if( 0 != (rc = ia_pthread_mutex_lock( &s->refs_mutex[pos] )) )
                ia_pthread_error( rc, "analyze_exec()", "ia_pthread_mutex_lock()" );

            /* make sure the current frame's slot in the ref list is open */
            while( s->refs[current_frame%nrefs] != NULL ) {
                if( 0 != (rc = ia_pthread_cond_wait( &s->refs_cond_nonfull[pos], &s->refs_mutex[pos] )) )
                    ia_pthread_error( rc, "analyze_exec()", "ia_pthread_cond_wait()" );
            }

            /* add the current frame to the ref list and signal anyone
             * waiting on this frame to wake up */
            s->refs[pos] = iaf;
            if( 0 != (rc = ia_pthread_cond_broadcast( &s->refs_cond_nonempty[pos] )) )
                ia_pthread_error( rc, "analyze_exec()", "ia_pthread_cond_broadcast()" );

            if( 0 != (rc = ia_pthread_mutex_unlock( &s->refs_mutex[pos] )) )
                ia_pthread_error( rc, "analyze_exec()", "ia_pthread_mutex_unlock()" );

            /* skip processing if this is one of the first frames in the ref
             * list (i.e. there are not enough ref frames to process) */
            if( current_frame < (uint32_t) i_maxrefs-1 ) {
                iaf->i_refcount = current_frame+1;
                continue;
            }

            /* pull out frames from the ref list to do the processing on */
            for( i = current_frame-(i_maxrefs-1), j = 0; i < current_frame; i++ ) {
                pos = i%nrefs;

                if( 0 != (rc = ia_pthread_mutex_lock( &s->refs_mutex[pos] )) )
                    ia_pthread_error( rc, "analyze_exec()", "ia_pthread_mutex_lock()" );

                /* wait for the frame we want to be available */
                while( s->refs[pos] == NULL || s->refs[pos]->i_frame != i) {
                    if( 0 != (rc = ia_pthread_cond_wait( &s->refs_cond_nonempty[pos], &s->refs_mutex[pos] )) )
                        ia_pthread_error( rc, "analyze_exec()", "ia_pthread_cond_wait()" );
                }

                iaim[j++] = s->refs[pos];

                if( 0 != (rc = ia_pthread_mutex_unlock( &s->refs_mutex[pos] )) )
                    ia_pthread_error( rc, "analyze_exec()", "ia_pthread_mutex_unlock()" );
            }
        } else {
            iaim[0] = iaf;
        }

        /* wait for output buf (wait for output manager signal) */
        iar = ia_image_create( iax->ias->param->i_width, iax->ias->param->i_height );

        iar->i_frame = current_frame;

        /* do processing */
        for ( j = 0; iax->ias->param->filter[j] != 0 && no_filter >= 0; j++ )
        {
            if( filters.exec[iax->ias->param->filter[j]] )
                filters.exec[iax->ias->param->filter[j]]( iax->ias, iax->ias->fparam[iax->ias->param->filter[j]], iaim, iar );
            else
                no_filter++;
        }

        /* mark the no filter flag if no filters were specified */
        if( no_filter == j || no_filter == -1 )
            no_filter = -1;
        else
            no_filter = 0;

        /* short curicuit the fancy reference frame gathering if we only need
         * one frame */
        if( 1 < i_maxrefs ) {
            /* decrement the ref count of each frame since its been used once.
             * each frame is only used i_maxrefs times */
            for( i = current_frame-(i_maxrefs-1); i <= current_frame; i++ ) {
                int pos = i % nrefs;
                if( 0 != (rc = ia_pthread_mutex_lock( &s->refs_mutex[pos] )) )
                    ia_pthread_error( rc, "analyze_exec()", "ia_pthread_mutex_lock()" );

                /* decrement the frames ref count */
                s->refs[pos]->i_refcount--;

                /* if the frame is no longer needed, free it up */
                if( s->refs[pos]->i_refcount == 0 ) {
                    ia_image_free( s->refs[pos] );
                    s->refs[pos] = NULL;

                    /* wake up anybody waiting to use this ref list slot */
                    if( 0 != (rc = ia_pthread_cond_broadcast( &s->refs_cond_nonfull[pos] )) )
                        ia_pthread_error( rc, "analyze_exec()", "ia_pthread_cond_broadcast()" );
                }
                
                if( 0 != (rc = ia_pthread_mutex_unlock( &s->refs_mutex[pos] )) )
                    ia_pthread_error( rc, "analyze_exec()", "ia_pthread_mutex_unlock()" );
            }
        } else {
            ia_image_free( iaim[0] );
        }

        /* close output buf (signal manage output) */
        ia_queue_push_sorted( iax->ias->output_queue, iar, iar->i_frame );
    }

    ia_free( iaim );
    ia_free( iax );
    ia_pthread_exit( NULL );
    return NULL;
}

int analyze( ia_param_t* p )
{
    int rc, i;
    void* status;
    pthread_t my_threads[MAX_THREADS];

    ia_seq_t* ias = analyze_init( p );
    if( ias == NULL ) {
        fprintf( stderr, "ERROR: analyze(): couln't init analyze\n" );
        return 1;
    }

    for( i = 0; i < ias->param->i_threads; i++ )
    {
        ia_exec_t* iax = malloc( sizeof(ia_exec_t) );
        if( iax == NULL )
            return 1;
        iax->ias = ias;
        iax->bufno = i;

        ia_error( "analyze: creating process thread with bufno %d\n", i );
        if( 0 != (rc = ia_pthread_create( &my_threads[i], &ias->attr, &analyze_exec, (void*) iax )) )
            ia_pthread_error( rc, "analyze()", "ia_pthread_create()" );
    }

    for( i = 0; i < ias->param->i_threads; i++ )
    {
        if( 0 != (rc = ia_pthread_join( my_threads[i], &status )) )
            ia_pthread_error( rc, "analyze()", "pthread_join()" );
    }

    analyze_deinit( ias );

    return 0;
}
