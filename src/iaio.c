/******************************************************************************
 * Copyright (c) 2008 Joey Degges and Vincent Simoes
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

#include <sys/time.h>
#include <time.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <regex.h>

#include "iaio.h"

static inline int iaio_displayimage( iaio_t* iaio, ia_image_t* iar )
{
    uint32_t i;

#ifdef HAVE_LIBSDL
    if( SDL_MUSTLOCK(iaio->screen) ) {
        if( SDL_LockSurface(iaio->screen) < 0 ) {
            fprintf( stderr, "Can't lock screen: %s\n", SDL_GetError() );
            return 1;
        }
    }

    for( i = 0; i < iaio->i_height; i++ )
        ia_memcpy_pixel_to_uint8( (uint8_t*)iaio->screen->pixels + iaio->i_width*3 * i,
                                  iar->pix + iaio->i_width*3 * (iaio->i_height-i-1),
                                  iaio->i_width*3 );

    if( SDL_MUSTLOCK(iaio->screen) ) {
        SDL_UnlockSurface( iaio->screen );
    }

    SDL_UpdateRect( iaio->screen, 0, 0, 0, 0 );
#endif
    return 0;
}

static inline int iaio_saveimage ( iaio_t* iaio, ia_image_t* iar )
{
    if( iaio->fin.output_stream != NULL )
    {
        fprintf( iaio->fin.output_stream,
                 "--myboundary\nContent-type: image/%s\n\n",
                 iaio->fin.mime_type );

        if( !FreeImage_SaveToHandle(FreeImage_GetFIFFromFilename(iar->name),
                               (FIBITMAP*)iar->dib,
                               &iaio->fin.io,
                               (fi_handle)iaio->fin.output_stream,
                               0) )
        {
            fprintf( stderr, "iaio_saveimage(): FAILED to write to stream\n" );
            return 1;
        }
    }
    else
    {
        assert( iar->dib != NULL );
        if( !FreeImage_Save(FreeImage_GetFIFFromFilename(iar->name),
                           (FIBITMAP*)iar->dib,
                           iar->name,
                           0) )
        {
            fprintf( stderr, "iaio_saveimage(): FAILED to write %s\n", iar->name );
            return 1;
        }

        if( iaio->b_thumbnail )
        {
            FIBITMAP* thumbnail = FreeImage_MakeThumbnail( (FIBITMAP*)iar->dib, 100, true );
            if( !FreeImage_Save(FreeImage_GetFIFFromFilename(iar->name),
                               thumbnail,
                               iar->thumbname,
                               0) )
            {
                fprintf( stderr, "iaio_saveimage(): FAILED to write %s\n", iar->name );
                return 1;
            }
            FreeImage_Unload( thumbnail );
        }
    }

    return 0;
}

int iaio_outputimage( iaio_t* iaio, ia_image_t* iar )
{
    if( iaio->output_type & IAIO_DISK )
    {
        if( iaio_saveimage(iaio, iar) )
            return 1;
    }
    if( iaio->output_type & IAIO_DISPLAY )
    {
        if( iaio_displayimage(iaio, iar) )
            return 1;
    }
    return 0;
}

inline int offset ( int width, int row, int col, int color )
{
    return row * width * 3 + col * 3 + color;
}

inline void yuv420torgb24( uint8_t* data, ia_pixel_t* pix, int width, int height )
{
    int row, col;
    uint8_t *y, *u, *v;
    int yy, uu, vv;
    int r, g, b;

    y = data;
    u = y + width*height;
    v = u + width*height/4;

    for( row = 0; row < height; row += 2 )
    {
        for( col = 0; col < width; col += 2 )
        {
            yy = ( *(y + col + (row * width)) - 16 )*298;
            uu = *u - 128;
            vv = *v - 128;

            /* calculate r,g,b for pixel (0,0) */
            r = (yy + vv*409 + 128 ) >> 8;
            g = (yy - uu*100 - vv*208 + 128) >> 8;
            b = (yy + uu*516) >> 8;

            *(pix + offset(width,row,col,0)) = clip_uint8( b );
            *(pix + offset(width,row,col,1)) = clip_uint8( g );
            *(pix + offset(width,row,col,2)) = clip_uint8( r );

            yy = ( *(y + row * width + col + 1) - 16 ) * 298;

            /* calculate r,g,b for pixel (0,1) */
            r = (yy + vv*409 + 128) >> 8;
            g = (yy - uu*100 - vv*208 + 128) >> 8;
            b = (yy + uu*516) >> 8;

            *(pix + offset(width,row,col,3)) = clip_uint8( b );
            *(pix + offset(width,row,col,4)) = clip_uint8( g );
            *(pix + offset(width,row,col,5)) = clip_uint8( r );

            yy = ( *( y +(row + 1) * width + col) - 16 ) * 298;

            /* calculate r,g,b for pixel (1,0) */
            r = (yy + vv*409 + 128) >> 8;
            g = (yy - uu*100 - vv*208 + 128) >> 8;
            b = (yy + uu*516) >> 8;

            *(pix + offset(width,row+1,col,0)) = clip_uint8( b );
            *(pix + offset(width,row+1,col,1)) = clip_uint8( g );
            *(pix + offset(width,row+1,col,2)) = clip_uint8( r );

            yy = ( *(y + (row + 1) * width + col + 1) - 16 ) * 298;

            /* calculate r,g,b for pixel (1,1) */
            r = (yy + vv*409 + 128) >> 8;
            g = (yy - uu*100 - vv*208 + 128) >> 8;
            b = (yy + uu*516) >> 8;

            *(pix + offset(width,row+1,col,3)) = clip_uint8( b );
            *(pix + offset(width,row+1,col,4)) = clip_uint8( g );
            *(pix + offset(width,row+1,col,5)) = clip_uint8( r );

            u++;
            v++;
        }
    }
}

inline void yuyvtorgb24( uint8_t* data, ia_pixel_t* pix, int width, int height )
{
    int row, col;
    int y, cb, cr;
    int r, g, b;

    for( row = 0; row < height; row++ )
    {
        for( col = 0; col < width; col += 2 )
        {
            /* calculate y,cb,cr */
            y  = (*data - 16) * 298;
            cb = *(data +  1) - 128;
            cr = *(data +  3) - 128;

            /* calculate r,g,b for pixel (0,0) */
            r = (y + 409*cr + 128) >> 8;
            g = (y - 100*cb - 208*cr + 128) >> 8;
            b = (y + 516*cb) >> 8;

            /* store value into memory */
            *(pix + offset(width,row,col,0)) = clip_uint8( b );
            *(pix + offset(width,row,col,1)) = clip_uint8( g );
            *(pix + offset(width,row,col,2)) = clip_uint8( r );

            data += 2;
            y = (*data - 16) * 298;

            /* calculate r,g,b for pixel 0,1 */
            r = (y + 409*cr + 128) >> 8;
            g = (y - 100*cb - 208*cr + 128) >> 8;
            b = (y + 526*cb) >> 8;

            *(pix + offset(width,row,col,3)) = clip_uint8( b );
            *(pix + offset(width,row,col,4)) = clip_uint8( g );
            *(pix + offset(width,row,col,5)) = clip_uint8( r );

            data += 2;
        }
    }
}

/* open first image on image list and store the images width and height into
 * the iaio
 * return: 0 - success
 *         1 - failure */
int iaio_file_probeimage( iaio_t* iaio )
{
    FIBITMAP* dib = NULL;
    FREE_IMAGE_FORMAT fif = FIF_UNKNOWN;

    if( iaio->fin.filp == NULL ) {
        iaio->fin.filp = fopen( iaio->fin.filename, "r" );
        if( iaio->fin.filp == NULL ) {
            fprintf( stderr, "ERROR: iaio_file_probeimage(): couldnt open file %s\n", iaio->fin.filename );
            return 1;
        }
    }
    if( ia_fgets(iaio->fin.buf, 1031, iaio->fin.filp) == NULL ) {
        fprintf( stderr, "ERROR: iaio_file_probeimage(): error getting file name off input file\n" );
        return 1;
    }
    char* fn = ia_strtok( iaio->fin.buf, "\n" );
    fclose( iaio->fin.filp );
    iaio->fin.filp = NULL;

    fif = FreeImage_GetFileType( fn, 0 );
    if( fif == FIF_UNKNOWN )
        fif = FreeImage_GetFIFFromFilename( fn );
    if( (fif != FIF_UNKNOWN) && FreeImage_FIFSupportsReading(fif) )
        dib = FreeImage_Load( fif, fn, 0 );
    if( dib == NULL )
    {
        fprintf( stderr,"Error opening image file\n" );
        return 1;
    }

    iaio->i_width   = FreeImage_GetWidth( dib );
    iaio->i_height  = FreeImage_GetHeight( dib );
    iaio->i_size    = iaio->i_width * iaio->i_height;

    FreeImage_Unload( dib );
    return 0;
}

/*
 * ret val:
 * 1 = error
 * 0 = no error
 */
int iaio_file_getimage( iaio_t* iaio, ia_image_t* iaf )
{
    FIBITMAP* dib = NULL;
    FREE_IMAGE_FORMAT fif;

    if( ia_fgets(iaio->fin.buf, 1031, iaio->fin.filp) == NULL ) {
        return 1;
    }
    char* str = ia_strtok( iaio->fin.buf, "\n" );

    fif = FreeImage_GetFileType ( str,0 );
    if ( fif == FIF_UNKNOWN )
        fif = FreeImage_GetFIFFromFilename ( str );
    if ( (fif != FIF_UNKNOWN) && FreeImage_FIFSupportsReading(fif) )
        dib = FreeImage_Load ( fif, str, 0 );
    if ( dib == NULL )
    {
        fprintf( stderr,"ERROR: iaio_file_getimage(): opening image file %s\n", str );
        return 1;
    }

    if( iaf->dib ) {
        FreeImage_Unload( (FIBITMAP*)iaf->dib );
    }
    iaf->dib = FreeImage_ConvertTo24Bits( dib );
    iaf->pix = FreeImage_GetBits( (FIBITMAP*)iaf->dib );
    FreeImage_Unload( dib );

    return 0;
}

/* retval
 * -1: error
 *  0: ok
 */
int iaio_cam_getimage( iaio_t* iaio, ia_image_t* iaf )
{
#if HAVE_V4L2 && HAVE_LIBSWSCALE
    iaf = v4l2_readimage( iaio->v4l2, iaf );
    if( !iaf ) {
        return -1;
    }

    ia_swscale( iaio->c, iaf, iaio->i_width, iaio->i_height );
#endif
    return 0;
}

/*
 * retval:
 * 1 = error
 * 0 = no error
 */
int iaio_getimage( iaio_t* iaio, ia_image_t* iaf )
{
    /* if reading from list of images */
    if( iaio->input_type == IAIO_FILE ) {
        if( iaio_file_getimage(iaio, iaf) ) {
            //fprintf( stderr, "ERROR: iaio_getimage(): couldn't open image from file\n" );
            return 1;
        }
    }
    /* if reading from camera */
    if( iaio->input_type == IAIO_CAMERA ) {
        if( iaio_cam_getimage(iaio, iaf) < 0 ) {
            fprintf( stderr, "ERROR: iaio_getimage(): couldn't get image from cam\n" );
            return 1;
        }
    }
    if( iaio->input_type == IAIO_MOVIE ) {
#ifdef HAVE_FFMPEG
        if( ia_ffmpeg_read_frame(iaio->ffio, iaf) < 0 ) {
            fprintf( stderr, "ERROR: iaio_getimage(): couldn't get image from movie file\n" );
            return 1;
        }
#endif
    }
    return 0;
}

/* retval:
 * -1: error
 *  0: ok!
 */
int iaio_cam_init ( iaio_t* iaio, ia_param_t* param )
{
#if HAVE_V4L2 && HAVE_LIBSWSCALE
    iaio->i_width = param->i_width;
    iaio->i_height = param->i_height;

    iaio->v4l2 = v4l2_open( iaio->i_width, iaio->i_height, param->video_device );
    if( !iaio->v4l2 )
        return -1;

    iaio->c = ia_swscale_init( iaio->i_width, iaio->i_height, PIX_FMT_YUYV422 );
    assert( iaio->c != NULL );
#else
    return -1;
#endif

    return 0;
}

void iaio_cam_close ( iaio_t* iaio )
{
#if HAVE_V4L2 && HAVE_LIBSWSCALE
    v4l2_close( iaio->v4l2 );
    ia_swscale_close( iaio->c );
#endif
}

/* initialize image input file */
inline int iaio_file_init( iaio_t* iaio, ia_param_t* param )
{
    iaio_file_t* fin = &iaio->fin;

    fin->filename = param->input_file;
    fin->buf = malloc( sizeof(char)*1031 );
    if( fin->buf == NULL ) {
        fprintf( stderr, "ERROR: iaio_file_init(): couldnt allocate buf\n" );
        return 1;
    }

    /* get width and height from first image */
    if( iaio_file_probeimage(iaio) ) {
        fprintf( stderr, "ERROR: iaio_file_init(): couldn't probe list\n" );
        return 1;
    }
    param->i_width = iaio->i_width;
    param->i_height = iaio->i_height;
    param->i_size = iaio->i_size;

    iaio->fin.filp = fopen( param->input_file, "r" );
    if( iaio->fin.filp == NULL ) {
        fprintf( stderr, "ERROR: iaio_file_init(): couldnt open file %s\n", iaio->fin.filename );
        return 1;
    }

    return 0;
}

static inline void iaio_file_close( iaio_t* iaio )
{
    fclose( iaio->fin.filp );
    ia_free( iaio->fin.buf );
}

static inline int iaio_display_init( iaio_t* iaio )
{
#ifdef HAVE_LIBSDL
    iaio->screen = NULL;

    if( SDL_Init(SDL_INIT_VIDEO) < 0 ) {
        fprintf( stderr, "Couldn't initialize SDL: %s\n", SDL_GetError() );
        return 1;
    }

    iaio->screen = SDL_SetVideoMode( iaio->i_width, iaio->i_height, 24, SDL_SWSURFACE ); 
    if( iaio->screen == NULL ) {
        fprintf( stderr, "Couldn't set 1024x768x8 video mode: %s\n", SDL_GetError() );
        return 1;
    }
#endif

    return 0;
}

static inline void iaio_display_close( void )
{
#ifdef HAVE_LIBSDL
    SDL_Quit();
#endif
}

iaio_t* iaio_open( ia_param_t* p )
{
    char pattern[] = "^.+\\.([tT][xX][tT])$";
    int status;
    regex_t re;
    iaio_t* iaio = malloc( sizeof(iaio_t) );

    iaio->fin.filp = NULL;
    iaio->fin.buf = NULL;
    iaio->input_type =
    iaio->output_type = 0;

    /* if cam input */
    if( p->b_vdev )
    {
#if HAVE_V4L2 && HAVE_LIBSWSCALE
        iaio->input_type = IAIO_CAMERA;
        if( iaio_cam_init(iaio, p) )
        {
            fprintf( stderr, "ERROR: iaio_open(): failed to initialize input file\n" );
            return NULL;
        }
#else
        fprintf( stderr, "Video device IO is not supported, recompile with libswscale and v4l2 to gain support.\n" );
        return NULL;
#endif
    }
    else
    {
        if( regcomp(&re, pattern, REG_EXTENDED|REG_NOSUB) != 0 )
            return NULL;
        status = regexec( &re, p->input_file, (size_t) 0, NULL, 0 );
        regfree( &re );

        // text file input
        if( status == 0 )
        {
            iaio->input_type = IAIO_FILE;
            if( iaio_file_init(iaio, p) )
            {
                fprintf( stderr, "ERROR: iaio_open(): failed to initialize camera\n" );
                return NULL;
            }
        }
        // movie file input
        else
        {
#ifdef HAVE_FFMPEG
            iaio->input_type = IAIO_MOVIE;
            iaio->ffio = ia_ffmpeg_init( p->input_file );
            if( !iaio->ffio ) {
                fprintf(stderr,"failed to open ffmpeg stuff\n");
                return NULL;
            }
            iaio->i_width = iaio->ffio->i_width;
            iaio->i_height = iaio->ffio->i_height;
            iaio->i_size = iaio->ffio->i_size;
#else
            fprintf( stderr, "Video IO is not supported, recompile with ffmpeg to gain support.\n" );
            return NULL;
#endif
        }
    }
    
    if( p->output_directory[0] )
    {
        iaio->output_type |= IAIO_DISK;

        /* set stream parameters if writing to stream */
        if( p->stream )
        {
            iaio_file_t* fin = &iaio->fin;
            fin->io.read_proc = NULL;
            fin->io.write_proc = (void*) &fwrite;
            fin->io.seek_proc = (void*) &fseek;
            fin->io.tell_proc = (void*) &ftell;

            if( !strncmp("-", p->output_directory, 1) )
                fin->output_stream = stdout;
            else
                fin->output_stream = fopen( p->output_directory, "w" );
            assert( fin->output_stream != NULL );

            if( !strncmp("gif", p->ext, 16) )
                snprintf( fin->mime_type, 16, "gif" );
            else if( !strncmp("png", p->ext, 16) )
                snprintf( fin->mime_type, 16, "x-png" );
            else if( !strncmp("bmp", p->ext, 16) )
                snprintf( fin->mime_type, 16, "x-ms-bmp" );
            else
                snprintf( fin->mime_type, 16, "jpeg" );
        }
        else
        {
            iaio->fin.output_stream = NULL;
        }
    }

    if( p->display )
    {
        iaio->output_type |= IAIO_DISPLAY;
        iaio_display_init( iaio );
    }

    iaio->eoi = false;
    iaio->b_thumbnail = p->b_thumbnail;
    p->i_width = iaio->i_width;
    p->i_height = iaio->i_height;
    p->i_size = iaio->i_width*iaio->i_height;

    return iaio;
}

inline void iaio_close( iaio_t* iaio )
{
#ifdef HAVE_FFMPEG
    if( iaio->input_type == IAIO_MOVIE )
        ia_ffmpeg_close( iaio->ffio );
#endif
    if( iaio->output_type & IAIO_DISPLAY )
        iaio_display_close();
#if HAVE_V4L2 && HAVE_LIBSWSCALE
    if( iaio->input_type == IAIO_CAMERA )
        iaio_cam_close( iaio );
#endif
    if( iaio->input_type == IAIO_FILE )
        iaio_file_close( iaio );

    free( iaio );
}
