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

/* cam includes */
#include <sys/time.h>
#include <linux/videodev2.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>

/* iaio includes */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "iaio.h"

static inline int iaio_displayimage( iaio_t* iaio, ia_image_t* iar )
{
    if( SDL_MUSTLOCK(iaio->screen) ) {
        if( SDL_LockSurface(iaio->screen) < 0 ) {
            fprintf( stderr, "Can't lock screen: %s\n", SDL_GetError() );
            return 1;
        }
    }

    ia_memcpy_pixel_to_uint8( (uint8_t*) iaio->screen->pixels, iar->pix, iaio->i_size*3 );

    if( SDL_MUSTLOCK(iaio->screen) ) {
        SDL_UnlockSurface( iaio->screen );
    }

    SDL_UpdateRect( iaio->screen, 0, 0, 0, 0 );
    return 0;
}

static inline int iaio_saveimage ( iaio_t* iaio, ia_image_t* iar )
{
    if ( iaio->dib == NULL )
        return 1;

    ia_memcpy_pixel_to_uint8( FreeImage_GetBits(iaio->dib),iar->pix,iaio->i_size*3 );

    if ( FreeImage_Save(FreeImage_GetFIFFromFilename(iar->name),iaio->dib,iar->name,0) )
        return 0;

    fprintf( stderr, "iaio_saveimage(): FAILED write to %s\n", iar->name );
    
    return 1;
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

static inline int xioctl( int fd, int request, void* arg )
{
    int r;
    do
    {
        r = ioctl( fd,request,arg );
    } while ( r == -1 && errno == EINTR );
    return r;
}

static inline void errno_exit( int fd, const char* msg )
{
    fprintf ( stderr,"ERROR: errno_exit(): %s error %d, %s\n",msg,errno,strerror(errno) );
    close ( fd );
    exit ( EXIT_FAILURE );
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
    if( ia_fgets(iaio->fin.buf, 1024, iaio->fin.filp) == NULL ) {
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
    FIBITMAP* dib, *dib24;
    FREE_IMAGE_FORMAT fif;

    if( ia_fgets(iaio->fin.buf, 1024, iaio->fin.filp) == NULL ) {
        //fprintf( stderr, "ERROR: iaio_file_getimage(): error getting file name off input file\n" );
        return 1;
    }
    char* str = ia_strtok( iaio->fin.buf, "\n" );

    dib = NULL;
    fif = FIF_UNKNOWN;
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

    dib24 = FreeImage_ConvertTo24Bits( dib );
    FreeImage_Unload( dib );
    dib = dib24;

    ia_memcpy_uint8_to_pixel( iaf->pix,FreeImage_GetBits(dib),iaio->i_size*3 );

    FreeImage_Unload ( dib );
    return 0;
}

/* retval
 * -1: error
 *  0: ok
 */
int iaio_cam_getimage( iaio_t* iaio, ia_image_t* iaf )
{
    fd_set fds;
    struct timeval tv;
    int r;
    struct v4l2_buffer buf;

    for(;;)
    {
        FD_ZERO( &fds );
        FD_SET( iaio->cam.fd,&fds );

        tv.tv_sec = 2;
        tv.tv_usec = 0;

        r = ia_select( iaio->cam.fd+1,&fds,NULL,NULL,&tv );

        if( r == -1 )
        {
            if( errno == EINTR )
            {
                continue;
            }
            errno_exit( iaio->cam.fd,"ia_select" );
        }

        if( r == 0 )
            errno_exit( iaio->cam.fd,"ia_select" );

        ia_memset( &buf,0,sizeof(buf) );
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;

        if( xioctl(iaio->cam.fd,VIDIOC_DQBUF,&buf) == -1 )
        {
            switch( errno )
            {
                case EAGAIN:
                    continue;
                default:
                    errno_exit( iaio->cam.fd,"VIDIOC_DQBUF" );
            }
        }

        assert( buf.index < iaio->cam.i_buffers );
        if( iaio->cam.palette == 1 )
            ia_memcpy_uint8_to_pixel( iaf->pix, iaio->cam.buffers[buf.index].start, iaio->cam.data_size );
        else if( iaio->cam.palette == 2 )
            yuv420torgb24( iaio->cam.buffers[buf.index].start, iaf->pix, iaio->i_width, iaio->i_height );
        else if( iaio->cam.palette == 3 )
            yuyvtorgb24( iaio->cam.buffers[buf.index].start, iaf->pix, iaio->i_width, iaio->i_height );
        
        if( xioctl(iaio->cam.fd,VIDIOC_QBUF,&buf) == -1 )
            errno_exit( iaio->cam.fd,"VIDIOC_QBUF" );

        return 0;
    }
}

/*
 * retval:
 * 1 = error
 * 0 = no error
 */
int iaio_getimage( iaio_t* iaio, ia_image_t* iaf )
{
    /* if reading from list of images */
    if( iaio->input_type & IAIO_FILE ) {
        if( iaio_file_getimage(iaio, iaf) ) {
            //fprintf( stderr, "ERROR: iaio_getimage(): couldn't open image from file\n" );
            return 1;
        }
    }
    /* if reading from camera */
    if( iaio->input_type & IAIO_CAMERA ) {
        if( iaio_cam_getimage(iaio, iaf) < 0 ) {
            fprintf( stderr, "ERROR: iaio_getimage(): couldn't get image from cam\n" );
            return 1;
        }
    }

    return 0;
}

/* retval:
 * -1: error
 *  0: ok!
 */
int iaio_cam_init ( iaio_t* iaio, ia_param_t* param )
{
    struct v4l2_capability cap;
    struct v4l2_format fmt;
    struct v4l2_requestbuffers reqbuf;
    enum v4l2_buf_type type;
    iaio_cam_t* cam = &iaio->cam;

    iaio->i_width = param->i_width;
    iaio->i_height = param->i_height;
    iaio->i_size = param->i_size;
    cam->pos = 0;

    if( param->b_verbose )
        printf( "Opening the device ...\n\n" );
    cam->fd = open( param->video_device,O_RDWR );
    if( cam->fd == -1 )
    {
        perror( "OPEN" );
        return -1;
    }

    /* get capabilities */
    if( xioctl(cam->fd,VIDIOC_QUERYCAP,&cap) )
    {
        if( errno == EINVAL )
        {
            if( param->b_verbose )
                fprintf( stderr,"%s is no V4L2 device\n",param->video_device );
            return -1;
        }
        else
        {
            errno_exit( cam->fd,"VIDIOC_QUERYCAP" );
        }
    }
    if( param->b_verbose )
    {
        printf( "Driver: %s\n",cap.driver );
        printf( "Card  : %s\n",cap.card );
        printf( "Capabilities:\n" );
        if( cap.capabilities & V4L2_CAP_VIDEO_CAPTURE )
            printf( "capture\n" );
        if( cap.capabilities & V4L2_CAP_STREAMING )
            printf( "streaming\n" );
        if( cap.capabilities & V4L2_CAP_READWRITE )
            printf( "READ/WRITE\n\n" );
    }

    /* set the format */
    ia_memset( &fmt,0,sizeof(fmt) );
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width = param->i_width;
    fmt.fmt.pix.height = param->i_height;
    fmt.fmt.pix.field = V4L2_FIELD_NONE;

    if( param->b_verbose )
    {
        printf( "Setting the format ...\n" );
        printf( "width : %d\n",fmt.fmt.pix.width );
        printf( "height: %d\n",fmt.fmt.pix.height );
    }

    if( strcasecmp((char*)cap.driver,"SentechUSB") == 0 )
    {
        cam->palette = 1;
        fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_BGR24;
        cam->data_size = 3*param->i_size;
    }
    else if( strcasecmp((char*)cap.driver,"pwc") == 0 )
    {
        cam->palette = 2;
        cam->data_size = 1.5*param->i_size;
        fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUV420;
    }
    else if( strcasecmp((char*)cap.driver,"uvcvideo") == 0 )
    {
        cam->palette = 3;
        cam->data_size = 2*param->i_size;
        fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
    }

    if( xioctl(cam->fd,VIDIOC_S_FMT,&fmt) == -1 )
        errno_exit( cam->fd,"VIDIOC_S_FMT" );

    /* request buffers */
    if( param->b_verbose )
        printf( "Requesting the buffer ...\n" );
    ia_memset( &reqbuf,0,sizeof(reqbuf) );
    reqbuf.count = 4;
    reqbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    reqbuf.memory = V4L2_MEMORY_MMAP;

    if( xioctl(cam->fd,VIDIOC_REQBUFS,&reqbuf) == -1 )
        errno_exit( cam->fd,"VIDIOC_REQBUFS" );

    if( reqbuf.count < 2 )
    {
        if( param->b_verbose )
            fprintf( stderr,"Insufficient buffer memory on %s\n",param->video_device );
        return -1;
    }

    /* mmap buffers */
    if( param->b_verbose )
        printf( "Creating the buffers ...\n" );

    cam->buffers = (iaio_cam_buffer*) ia_calloc ( reqbuf.count,sizeof(iaio_cam_buffer) );
    if( !cam->buffers )
    {
        if( param->b_verbose )
            fprintf( stderr,"Out of memory\n" );
        return -1;
    }

    for( cam->i_buffers = 0; cam->i_buffers < reqbuf.count; ++cam->i_buffers )
    {
        struct v4l2_buffer buf;

        ia_memset( &buf,0,sizeof(buf) );

        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = cam->i_buffers;

        if( xioctl(cam->fd,VIDIOC_QUERYBUF,&buf) == -1 )
            errno_exit( cam->fd, "VIDIOC_QUERYBUF" );

        cam->buffers[cam->i_buffers].length = buf.length;
        cam->buffers[cam->i_buffers].start = mmap( NULL,buf.length,
                PROT_READ | PROT_WRITE, MAP_SHARED,
                cam->fd,buf.m.offset );

        if( xioctl(cam->fd,VIDIOC_QBUF,&buf) == -1 )
            errno_exit( cam->fd,"VIDIOC_QBUF" );
    }

    //FIXME: put the change_control stuff here if there is the option in params!
    // change_control();

    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if( xioctl(cam->fd,VIDIOC_STREAMON,&type) == -1 )
        errno_exit( cam->fd,"VIDIOC_STREAMON" );

    cam->capturing = 1;

    return 0;
}

int iaio_cam_close ( iaio_t* iaio )
{
    enum v4l2_buf_type type;
    unsigned int i;

    iaio->cam.capturing = 0;

    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    ioctl( iaio->cam.fd,VIDIOC_STREAMOFF,&type );

    /* reset the buffer */
    for( i = 0; i < (unsigned int)iaio->cam.i_buffers; i++ )
    {
        struct v4l2_buffer buf;

        ia_memset( &buf,0,sizeof(buf) );
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;
        if( xioctl(iaio->cam.fd,VIDIOC_QUERYBUF,&buf) < 0 )
        {
            errno_exit( iaio->cam.fd,"VIDIOC_QUERYBUF" );
            return 0;
        }
        munmap( iaio->cam.buffers[i].start, iaio->cam.buffers[i].length );
    }

    close( iaio->cam.fd );

    return 1;
}

/* initialize image input file */
inline int iaio_file_init( iaio_t* iaio, ia_param_t* param )
{
    iaio_file_t* fin = &iaio->fin;

    fin->filename = param->input_file;
    fin->buf = malloc( sizeof(char)*1024 );
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
    if( ia_fgets(iaio->fin.buf, 1024, iaio->fin.filp) == NULL ) {
        fprintf( stderr, "ERROR: iaio_file_init(): error getting file name off input file\n" );
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

    return 0;
}

static inline void iaio_display_close( void )
{
    SDL_Quit();
}

iaio_t* iaio_open( ia_seq_t* ias )
{
    iaio_t* iaio = malloc( sizeof(iaio_t) );

    iaio->fin.filp = NULL;
    iaio->fin.buf = NULL;
    iaio->input_type =
    iaio->output_type = 0;

    /* if cam input */
    if( ias->param->b_vdev )
    {
        iaio->input_type |= IAIO_CAMERA;
        if( iaio_cam_init(iaio, ias->param) )
        {
            fprintf( stderr, "ERROR: iaio_open(): failed to initialize input file\n" );
            return NULL;
        }
    }
    /* if file input */
    else
    {
        iaio->input_type |= IAIO_FILE;
        if( iaio_file_init(iaio, ias->param) )
        {
            fprintf( stderr, "ERROR: iaio_open(): failed to initialize camera\n" );
            return NULL;
        }
    }

    if( ias->param->output_directory[0] )
    {
        iaio->output_type |= IAIO_DISK;
        iaio->dib = FreeImage_AllocateT( FIT_BITMAP, iaio->i_width, iaio->i_height, 24,
                                     FI_RGBA_RED,FI_RGBA_GREEN,FI_RGBA_BLUE );

    }
    if( ias->param->display )
    {
        iaio->output_type |= IAIO_DISPLAY;
        iaio_display_init( iaio );
    }

    iaio->eoi = false;

    return iaio;
}

inline void iaio_close( iaio_t* iaio )
{
    if( iaio->output_type & IAIO_DISK )
        FreeImage_Unload( iaio->dib );
    if( iaio->output_type & IAIO_DISPLAY )
        iaio_display_close();

    if( iaio->input_type & IAIO_CAMERA )
        iaio_cam_close( iaio );
    if( iaio->input_type & IAIO_FILE )
        iaio_file_close( iaio );

    free( iaio );
}
