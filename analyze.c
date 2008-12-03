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
#include "filters.h"

/* functions that are not yet fully operable *****************************************
int* q(ia_pixel_t* im)
{
        int window, s, width, height, x, y, h, k, *q, *q1, *q2; //, stddev, n, th, tk;
        ia_pixel_t* i, sum, threshold, B; //, t;
        q = (int*)malloc(sizeof(int)*WIDTH*HEIGHT);
    q1 = (int*)malloc(sizeof(int)*WIDTH*HEIGHT);
    q2 = (int*)malloc(sizeof(int)*WIDTH*HEIGHT);
    i = (ia_pixel_t*)malloc(sizeof(ia_pixel_t)*WIDTH*HEIGHT);
    window = 3;
    //stddev = 5;
    s = 0;
    width = WIDTH;
    height = HEIGHT;
threshold = 60;
B = 5;
    //threshold = 80;
    //B = (255) / ((window*window-1)/2);

        for(y = s; y < height; y++)
        {
                for(x = s; x < width; x++)
                {
                        sum = 0;
            for(h = y-(window/2); h < y+(window/2); h++)
                        {
                                for(k = x-(window/2); k < x+(window/2); k++)
                                {
                                        sum += pow(im[h*WIDTH+k], 2);
                                }
                        }
                        sum = sqrt(sum/((window*window)));
            i[y*WIDTH+x] = sum;
//if(sum > 40) printf("%f %f: ", i[y*WIDTH+x], threshold);
            if(sum > threshold)
            {
                q[y*WIDTH+x] = 255;
                //q1[y*WIDTH+x] = 255;
                //q2[y*WIDTH+x] = 255;
            }
            else
            {
                q[y*WIDTH+x] = 0;
                //q1[y*WIDTH+x] = 0;
                //q2[y*WIDTH+x] = 0;
            }
                }
        }
    free(i);
    free(im);
    free(q1);
    free(q2);
    return q;
}

ia_pixel_t *get_gaussian(int ksize)
{
    ia_pixel_t *gaussian = (ia_pixel_t*)malloc(sizeof(ia_pixel_t) * ksize*ksize);
    ia_pixel_t sigma = (ksize/2.0 - 1)*.3 + .8;
    ia_pixel_t sq_sigma = sigma*sigma;
    int i;
    int y = ksize/2;
    int x = ksize/2;
    for(i = 0; i < ksize*ksize; i++)
    {
        y = abs(i/ksize - ksize/2);
        x = abs(i%ksize - ksize/2);
        gaussian[i] = 1/(2*3.1415926*sq_sigma) * exp(-(ia_pixel_t)(y*y + x*x)/(2*sq_sigma));
    }
    return gaussian;
}

ia_pixel_t* get_reflectance_image(IplImage* im, ia_pixel_t* gaussian, int ksize)
{
    int width = WIDTH, height = HEIGHT, s = 0;
    ia_pixel_t *image = (ia_pixel_t*)malloc(WIDTH*HEIGHT*sizeof(ia_pixel_t));
    ia_pixel_t *blur = (ia_pixel_t*)malloc(WIDTH*HEIGHT*sizeof(ia_pixel_t));
        int x, y, tx, ty, h, k;
        ia_pixel_t sum;
    ia_pixel_t lmin = 10000, lmax = 0, max = 255.0, op;
        for(y = 0; y < HEIGHT; y++)
        {
                for(x = 0; x < WIDTH; x++)
                {
                if(((uchar*)(im->imageData + y*im->widthStep))[x] < 1)
                    image[y*WIDTH+x] = 0;
                else
                    //image[y*WIDTH+x] = log((ia_pixel_t)pow(((uchar*)(im->imageData + y*im->widthStep))[x], .4));
                    image[y*WIDTH+x] = log((ia_pixel_t)((uchar*)(im->imageData + y*im->widthStep))[x]);
        }
        }
        for(y = s; y < height; y++)
        {
        for(x = s; x < width; x++)
                {
                        sum = 0;
                        for(h = 0; h < ksize; h++)
                        {
                ty = y+h-ksize/2;
                                for(k = 0; k < ksize; k++)
                                {
                                        tx = x+k-ksize/2;
                    sum += gaussian[h*ksize+k] * image[ty*WIDTH+tx];
                                }
                        }
            blur[y*WIDTH+x] = exp(fabs(sum - image[y*WIDTH+x]));
            if(blur[y*WIDTH+x] > lmax)
                lmax = blur[y*WIDTH+x];
            if(blur[y*WIDTH+x] < lmin)
                lmin = blur[y*WIDTH+x];
        }
    }
    op = (ia_pixel_t)max/(lmax - lmin);
    for(y = s; y < height; y++)
    {
        for(x = s; x < width; x++)
        {
            blur[y*WIDTH+x] = (blur[y*WIDTH+x] - lmin) * op;
            //((uchar*)(im->imageData + y*im->widthStep))[x] = (int)blur[y*WIDTH+x];
        }
    }
    free(image);
    return blur;
}

ia_pixel_t* patch_diff(ia_pixel_t* ima, ia_pixel_t* imb, int ksize)
{
    int i, j, h, k, width, height, s;
    ia_pixel_t* out = (ia_pixel_t*)malloc(sizeof(ia_pixel_t)*WIDTH*HEIGHT), sum;
    width = WIDTH;
    height = HEIGHT;
    s = 0;
    for(i = s; i < height; i++)
    {
        for(j = s; j < width; j++)
        {
            sum = 0;
            for(h = i-ksize/2; h < i+ksize/2; h++)
                for(k = j-ksize/2; k < j+ksize/2; k++)
                    sum += pow(ima[h*WIDTH+k] - imb[h*WIDTH+k], 2);
            out[i*WIDTH+j] = sqrt(sum/ksize);
            if(out[i*WIDTH+j] < 40)
                out[i*WIDTH+j] = 0;
        }
    }
    return out;
}

ia_pixel_t* Iplblur(IplImage* im, ia_pixel_t* gaussian, int ksize)
{
    int y, x, h, k, height = HEIGHT, width = WIDTH, s = 0, tx, ty;
    ia_pixel_t sum, *out = (ia_pixel_t*)malloc(sizeof(ia_pixel_t)*WIDTH*HEIGHT);
        for(y = s; y < height; y++)
        {
                for(x = s; x < width; x++)
                {
                        sum = 0;
                        for(h = 0; h < ksize; h++)
                        {
                                ty = y+h-ksize/2;
                                for(k = 0; k < ksize; k++)
                                {
                                        tx = x+k-ksize/2;
                                        sum += gaussian[h*ksize+k] * ((uchar*)(im->imageData + ty*im->widthStep))[tx];
                                }
                        }
                        out[y*WIDTH+x] = sum;
                }
        }
    return out;
}

void dialate(ia_pixel_t* im, int ksize)
{
    int i, j, h, k, height = HEIGHT, width = WIDTH, s = 0, n;

    for(i = s; i < height; i++)
    {
        for(j = s; j < width; j++)
        {
            n = 0;
            for(h = i-ksize/2; h < i+ksize/2; h++)
            {
                for(k = j-ksize/2; k < j+ksize/2; j++)
                {
                    if(im[h*WIDTH+k] == 255)
                        n++;
                }
            }
            if(n >= 3)
                im[h*WIDTH+k] = 255;
        }
    }
}

**************************************************************************************/

#define IA_PRINT( s )\
{\
    if( p->b_verbose )\
        printf( s );\
}

#define THREADED 0

static inline ia_seq_t* analyze_init( ia_param_t* p )
{
    int j;
    ia_seq_t* ias = ia_seq_open( p );

    if( ias == NULL ) {
        fprintf( stderr, "ERROR: analyze_init(): couldnt open ia_seq\n" );
        return NULL;
    }

    /* call any init functions */
    for ( j = 0; p->filter[j] != 0; j++ )
    {
        if( filters.filters_init[p->filter[j]] )
            filters.filters_init[p->filter[j]]( ias );
    }

    init_filters();

    return ias;
}

static inline void analyze_deinit( ia_seq_t* s )
{
    int j;

    /* call any filter specific close functions */
    for ( j = 0; s->param->filter[j] != 0; j++ )
    {
        if( filters.filters_clos[s->param->filter[j]] != NULL )
            filters.filters_clos[s->param->filter[j]]();
    }

    ia_seq_close( s );
}

void* analyze_exec( void* vptr )
{
    int j, no_filter = 0;
    ia_exec_t* iax = (ia_exec_t*) vptr;
    ia_image_t *iaf, *iar;
    while( 1 )
    {
        /* wait for input buf (wait for input manager signal) */
        iaf = ia_queue_pop( iax->ias->input_queue );
        if( iaf->eoi ) {
            iar = iaf;
            ia_queue_push( iax->ias->output_queue, iar );
            break;
        }

        /* wait for output buf (wait for output manager signal) */
        if( ia_queue_is_empty(iax->ias->output_free) )
            iar = ia_image_create( iax->ias->param->i_size*3 );
        else
            iar = ia_queue_pop( iax->ias->output_free );
        iar->i_frame = iaf->i_frame;

        /* do processing */
        for ( j = 0; iax->ias->param->filter[j] != 0 && no_filter >= 0; j++ )
        {
            if( filters.filters_exec[iax->ias->param->filter[j]] != NULL ) {
                filters.filters_exec[iax->ias->param->filter[j]]( iax->ias, &iaf, iar );
            } else {
                no_filter++;
            }
        }

        /* clean up buffers and register output jobs */
        if( no_filter == j || no_filter == -1 )
        {
            no_filter = -1;
            ia_queue_push( iax->ias->output_queue, iaf );
            ia_queue_push( iax->ias->input_free, iar );
        }
        else
        {
            no_filter = 0;
            /* close input buf (signal manage input) */
            ia_queue_push( iax->ias->input_free, iaf );

            /* close output buf (signal manage output) */
            ia_queue_shove( iax->ias->output_queue, iar );
        }
    }

    ia_free( iax );
    pthread_exit( NULL );
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
        rc = ia_pthread_create( &my_threads[i], &ias->attr, &analyze_exec, (void*) iax );
        if( rc ) {
            fprintf( stderr, "analyze(): ia_pthread_create() returned code %d\n", rc );
            return 1;
        }
    }

    while( !ias->iaio->eoi )
        usleep( 100 );
    
    for( i = 0; i < ias->param->i_threads; i++ )
    {
        rc = pthread_join( my_threads[i], &status );
        ia_pthread_error( rc, "analyze()", "pthread_join()" );
    }

    analyze_deinit( ias );

    return 0;
}
