#include <stdio.h>
//#include <stdlib.h>
//#include <string.h>

#include <FreeImage.h>
#include "common.h"
#include "iaio.h"
#include "ia_sequence.h"

#define RANGE 5

#define CAM 1

/* retval
 * 1: error
 * 0: ok
 */
int ia_seq_getimage ( ia_seq_t* s )
{
    if( s->param->b_vdev )
    {
        if( iaio_cam_getimage(s) < 0 )
        {
            fprintf( stderr,"Error getting image from the cam\n" );
            return 1;
        }
    }
    else
    {
        if( ia_fgets(s->buf,1024,s->fin) == NULL )
        {
            fprintf( stderr,"Error reading image file name from input file\n" );
            return 1;
        }

        s->buf = ia_strtok( s->buf,"\n" );
        if( iaio_getimage(s->buf,s->iaf) )
        {
            fprintf( stderr,"Error getting image data\n" );
            return 1;
        }
    }
    snprintf( s->name,1024,"%s/image-%010lld.png",s->param->output_directory,++s->i_frame );

    return 0;
}

int ia_seq_probelist( ia_seq_t* s )
{
    if( ia_fgets(s->buf,1024,s->fin) == NULL )
    {
        fprintf( stderr,"Error reading image file name form input file\n" );
        return 1;
    }

    s->buf = ia_strtok( s->buf,"\n" );
    if( iaio_probeimage(s,s->buf) )
    {
        fprintf( stderr,"Error opening image\n" );
        return 1;
    }
    return 0;
}

void* ia_seq_saveimage( void* ptr )
{
    char* c = ia_strrchr(  ((ia_seq_t*)ptr)->name,'.' );
    c[1] = 'b';
    c[2] = 'm';
    c[3] = 'p';
    iaio_saveimage( (ia_seq_t*)ptr );
    return NULL;
}

/*
ia_image_t* ia_seq_copyimage( ia_image_t* iaf )
{
    ia_image_t* iar = (ia_image_t*) malloc ( sizeof(ia_image_t) );
    memcpy( iar,iaf,sizeof(iaf) );
    strncpy( iar->name,iaf->name,100 );
    iar->pix = (ia_pixel_t*) malloc ( sizeof(ia_pixel_t)*iar->width*iar->height );
    memset( iar->pix,0,sizeof(ia_pixel_t)*iar->width*iar->height );
    return iar;
}
*/

/* initialize a new ia_seq */
ia_seq_t*   ia_seq_open( ia_param_t* p )
{
    ia_seq_t* s = (ia_seq_t*) ia_malloc( sizeof(ia_seq_t) );
    if( s == NULL )
        return NULL;

    ia_memset( s,0,sizeof(ia_seq_t) );
    s->param = p;

    s->iaf = (ia_image_t*) ia_malloc( sizeof(ia_image_t) );
    if( s->iaf == NULL )
    {
        ia_free( s );
        fprintf( stderr,"Error allocating new image\n" );
        return NULL;
    }

    s->iar = (ia_image_t*) ia_malloc( sizeof(ia_image_t) );
    if( s->iar == NULL )
    {
        ia_free( s->iaf );
        ia_free( s );
        fprintf( stderr,"Error allocating new image\n" );
        return NULL;
    }

    s->buf = (char*) ia_malloc( sizeof(char)*1024 );
    if( s->buf == NULL )
    {
        ia_free( s->iaf );
        ia_free( s->iar );
        ia_free( s );
        fprintf( stderr,"Error allocating mem for buf\n" );
        return NULL;
    }

    /* if image names are being read from an input file
     * then probe first image to figure out width and height */
    if( !s->param->b_vdev )
    {
        s->fin = ia_fopen( s->param->input_file, "r" );
        if( s->fin == NULL )
        {
            ia_free( s );
            fprintf( stderr,"Error opening input file\n" );
            return NULL;
        }

        if( ia_seq_probelist(s) )
        {
            ia_free( s->iaf );
            ia_free( s->iar );
            ia_free( s );
            fprintf( stderr,"Error getting image\n" );
            return NULL;
        }
    
        ia_fclose( s->fin );
        s->fin = ia_fopen( s->param->input_file, "r" );
        if( s->fin == NULL )
        {
            ia_free( s->iaf );
            ia_free( s->iar );
            ia_free( s );
            fprintf( stderr,"Error opening input file\n" );
            return NULL;
        }
    }
    else
    {
        if( iaio_cam_init(s) < 0 )
        {
            fprintf( stderr,"Error initializing camera\n" );
            return NULL;
        }
    }

    ia_memset( s->iaf,0,sizeof(ia_image_t) );
    ia_memset( s->iar,0,sizeof(ia_image_t) );

    s->iaf->pix = ia_malloc( sizeof(ia_pixel_t)*s->param->i_size*3 );
    if( s->iaf->pix == NULL )
    {
        ia_free( s->iar );
        ia_free( s->iaf );
        ia_free( s );
    }

    s->iar->pix = ia_malloc( sizeof(ia_pixel_t)*s->param->i_size*3 );
    if( s->iar->pix == NULL )
    {
        ia_free( s->iaf->pix );
        ia_free( s->iar );
        ia_free( s->iaf );
        ia_free( s );
        return NULL;
    }

    s->i_frame = 0;
    s->dib = FreeImage_AllocateT( FIT_BITMAP,s->param->i_width,s->param->i_height,24,FI_RGBA_RED,FI_RGBA_GREEN,FI_RGBA_BLUE );

    return s;
}

inline void ia_seq_close( ia_seq_t* s )
{
    FreeImage_Unload( s->dib );

    if( !s->param->b_vdev )
        ia_fclose( s->fin );
    else
    {
        if( iaio_cam_close(s) )
            fprintf( stderr,"Error closing camera\n" );
    }

    ia_free( s->iaf->pix );
    ia_free( s->iaf );

    ia_free( s->iar->pix );
    ia_free( s->iar );

    ia_free( s->buf );

    ia_free( s );
}
