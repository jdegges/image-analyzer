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

//#include <opencv/cv.h>
//#include <opencv/highgui.h>

/* iaio includes */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <FreeImage.h>

#include "common.h"
#include "ia_sequence.h"
#include "iaio.h"


/*
 * ret val:
 * 1 = error
 * 0 = no error
 */
int iaio_getimage ( char* str,ia_image_t* iaf )
{
    FIBITMAP* dib, *dib24;
    FREE_IMAGE_FORMAT fif;

	dib = NULL;
	fif = FIF_UNKNOWN;
	fif = FreeImage_GetFileType ( str,0 );

	if ( fif == FIF_UNKNOWN )
		fif = FreeImage_GetFIFFromFilename ( str );
	if ( (fif != FIF_UNKNOWN) && FreeImage_FIFSupportsReading(fif) )
		dib = FreeImage_Load ( fif,str,0 );
	if ( dib == NULL )
    {
        fprintf( stderr,"Error opening image file\n" );
		return 1;
    }

    dib24 = FreeImage_ConvertTo24Bits( dib );
    FreeImage_Unload( dib );
    dib = dib24;

    ia_memcpy_uint8_to_pixel( iaf->pix,FreeImage_GetBits(dib),FreeImage_GetWidth(dib)*FreeImage_GetHeight(dib)*3 );

	FreeImage_Unload ( dib );
	return 0;
}

/* open image str and fill in ia_param_t width/height/size values */
int iaio_probeimage( ia_seq_t* s, const char* str )
{
    FIBITMAP* dib = NULL;
    FREE_IMAGE_FORMAT fif = FIF_UNKNOWN;

    fif = FreeImage_GetFileType( str,0 );
    if( fif == FIF_UNKNOWN )
        fif = FreeImage_GetFIFFromFilename( str );
    if( (fif != FIF_UNKNOWN) && FreeImage_FIFSupportsReading(fif) )
        dib = FreeImage_Load( fif,str,0 );
    if( dib == NULL )
    {
        fprintf( stderr,"Error opening image file\n" );
        return 1;
    }

    s->param->i_width   = FreeImage_GetWidth( dib );
    s->param->i_height  = FreeImage_GetHeight( dib );
    s->param->i_size = s->param->i_width * s->param->i_height;

    FreeImage_Unload( dib );
    return 0;
}

int iaio_saveimage ( ia_seq_t* s )
{
    if ( s->dib == NULL )
        return 1;

    ia_memcpy_pixel_to_uint8( FreeImage_GetBits(s->dib),s->iar->pix,s->param->i_size*3 );
    printf( "save image to: %s\n",s->name );

    if ( FreeImage_Save(FreeImage_GetFIFFromFilename(s->name),s->dib,s->name,0) )
        return 0;
    
    return 1;
}

/*
static int iaio_opencv_saveimage( ia_image_t* iaf )
{
    IplImage* cvi;
    int i;
    cvi = cvCreateImage( cvSize(iaf->width,iaf->height),IPL_DEPTH_8U,1 );
    for( i = 0; i < iaf->width*iaf->height; i++ )
        cvi->imageData[i] = iaf->ucpix[i];
    cvSaveImage( iaf->name,cvi );
    cvReleaseImage( &cvi );
    return 1;
}
*/

static inline int xioctl( int fd, int request, void* arg )
{
    int r;
    do
    {
        r = ioctl( fd,request,arg );
    } while ( r == -1 && errno == EINTR );
    return r;
}

static inline void errno_exit( ia_seq_t* s, const char* msg )
{
    if( s->param->b_verbose )
        fprintf ( stderr,"%s error %d, %s\n",msg,errno,strerror(errno) );
    close ( s->cam.fd );
    exit ( EXIT_FAILURE );
}

/* retval:
 * -1: error
 *  0: ok!
 */
int iaio_cam_init ( ia_seq_t* s )
{
    //FIXME: load_option has been called from main in parse_args()
    struct v4l2_capability cap;
    struct v4l2_format fmt;
    struct v4l2_requestbuffers reqbuf;
    enum v4l2_buf_type type;

    s->cam.pos = 0;

    if( s->param->b_verbose )
        printf( "Opening the device ...\n\n" );
    s->cam.fd = open( s->param->video_device,O_RDWR );
    if( s->cam.fd == -1 )
    {
        perror( "OPEN" );
        return -1;
    }

    /* get capabilities */
    if( xioctl(s->cam.fd,VIDIOC_QUERYCAP,&cap) )
    {
        if( errno == EINVAL )
        {
            if( s->param->b_verbose )
                fprintf( stderr,"%s is no V4L2 device\n",s->param->video_device );
            return -1;
        }
        else
        {
            errno_exit( s,"VIDIOC_QUERYCAP" );
        }
    }

    if( s->param->b_verbose )
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
    fmt.fmt.pix.width = s->param->i_width;
    fmt.fmt.pix.height = s->param->i_height;
    fmt.fmt.pix.field = V4L2_FIELD_NONE;

    if( s->param->b_verbose )
    {
        printf( "Setting the format ...\n" );
        printf( "width : %d\n",fmt.fmt.pix.width );
        printf( "height: %d\n",fmt.fmt.pix.height );
    }

    if( strcasecmp((char*)cap.driver,"SentechUSB") == 0 )
    {
        s->cam.palette = 1;
        fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_BGR24;
        s->cam.data_size = 3*s->param->i_size;
    }
    else if( strcasecmp((char*)cap.driver,"pwc") == 0 )
    {
        s->cam.palette = 2;
        s->cam.data_size = 1.5*s->param->i_size;
        fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUV420;
    }
    else if( strcasecmp((char*)cap.driver,"uvcvideo") == 0 )
    {
        s->cam.palette = 3;
        s->cam.data_size = 2*s->param->i_size;
        fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
    }

    if( xioctl(s->cam.fd,VIDIOC_S_FMT,&fmt) == -1 )
        errno_exit( s,"VIDIOC_S_FMT" );

    /* request buffers */
    if( s->param->b_verbose )
        printf( "Requesting the buffer ...\n" );
    ia_memset( &reqbuf,0,sizeof(reqbuf) );
    reqbuf.count = 4;
    reqbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    reqbuf.memory = V4L2_MEMORY_MMAP;

    if( xioctl(s->cam.fd,VIDIOC_REQBUFS,&reqbuf) == -1 )
        errno_exit( s,"VIDIOC_REQBUFS" );

    if( reqbuf.count < 2 )
    {
        if( s->param->b_verbose )
            fprintf( stderr,"Insufficient buffer memory on %s\n",s->param->video_device );
        return -1;
    }

    /* mmap buffers */
    if( s->param->b_verbose )
        printf( "Creating the buffers ...\n" );

    s->cam.buffers = (iaio_cam_buffer*) ia_calloc ( reqbuf.count,sizeof(iaio_cam_buffer) );
    if( !s->cam.buffers )
    {
        if( s->param->b_verbose )
            fprintf( stderr,"Out of memory\n" );
        return -1;
    }

    for( s->cam.i_buffers = 0; s->cam.i_buffers < reqbuf.count; ++s->cam.i_buffers )
    {
        struct v4l2_buffer buf;

        ia_memset( &buf,0,sizeof(buf) );

        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = s->cam.i_buffers;

        if( xioctl(s->cam.fd,VIDIOC_QUERYBUF,&buf) == -1 )
            errno_exit( s,"VIDIOC_QUERYBUF" );

        s->cam.buffers[s->cam.i_buffers].length = buf.length;
        s->cam.buffers[s->cam.i_buffers].start = mmap( NULL,buf.length,
                PROT_READ | PROT_WRITE, MAP_SHARED,
                s->cam.fd,buf.m.offset );

        if( xioctl(s->cam.fd,VIDIOC_QBUF,&buf) == -1 )
            errno_exit( s,"VIDIOC_QBUF" );
    }

    //FIXME: put the change_control stuff here if there is the option in params!
    // change_control();

    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if( xioctl(s->cam.fd,VIDIOC_STREAMON,&type) == -1 )
        errno_exit( s,"VIDIOC_STREAMON" );

    s->cam.capturing = 1;

    return 0;
}

/* retval
 * -1: error
 *  0: ok
 */
int iaio_cam_getimage ( ia_seq_t* s )
{
    fd_set fds;
    struct timeval tv;
    int r;
    struct v4l2_buffer buf;

    for(;;)
    {
        FD_ZERO( &fds );
        FD_SET( s->cam.fd,&fds );

        tv.tv_sec = 2;
        tv.tv_usec = 0;

        r = ia_select( s->cam.fd+1,&fds,NULL,NULL,&tv );

        if( r == -1 )
        {
            if( errno == EINTR )
            {
                continue;
            }
            errno_exit( s,"ia_select" );
        }

        if( r == 0 )
            errno_exit( s,"ia_select" );

        ia_memset( &buf,0,sizeof(buf) );
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;

        if( xioctl(s->cam.fd,VIDIOC_DQBUF,&buf) == -1 )
        {
            switch( errno )
            {
                case EAGAIN:
                    continue;
                default:
                    errno_exit( s,"VIDIOC_DQBUF" );
            }
        }

        assert( buf.index < s->cam.i_buffers );

        if( s->cam.palette == 1 )
            ia_memcpy_uint8_to_pixel( s->iaf->pix,s->cam.buffers[buf.index].start,s->cam.data_size );
        else if( s->cam.palette == 2 )
        {
//            yuv420torgb24( s->cam.buffers[buf.index].start,s->iaf->pix,s->param->i_width,s->param->i_height );
            ia_memcpy_uint8_to_pixel( s->iaf->pix,s->cam.buffers[buf.index].start,s->cam.data_size );
        }
        else if( s->cam.palette == 3 )
        {
//            yuyvtorgb24( s->cam.buffers[buf.index].start,s->iaf->pix,s->param->i_width,s->param->i_height );
            ia_memcpy_uint8_to_pixel( s->iaf->pix,s->cam.buffers[buf.index].start,s->cam.data_size );
        }

        if( xioctl(s->cam.fd,VIDIOC_QBUF,&buf) == -1 )
            errno_exit( s,"VIDIOC_QBUF" );

        return 0;
    }
}

int iaio_cam_close ( ia_seq_t* s )
{
    enum v4l2_buf_type type;
    unsigned int i;

    s->cam.capturing = 0;

    if( s->param->b_verbose )
        printf( "Stop capturing ... \n" );
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    ioctl( s->cam.fd,VIDIOC_STREAMOFF,&type );

    /* reset the buffer */
    if( s->param->b_verbose )
        printf( "Resetting the buffers\n" );
    for( i = 0; i < (unsigned int)s->cam.i_buffers; i++ )
    {
        struct v4l2_buffer buf;

        ia_memset( &buf,0,sizeof(buf) );
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;
        if( xioctl(s->cam.fd,VIDIOC_QUERYBUF,&buf) < 0 )
        {
            errno_exit( s,"VIDIOC_QUERYBUF" );
            return 0;
        }
        munmap( s->cam.buffers[i].start, s->cam.buffers[i].length );
    }

    if( s->param->b_verbose )
        printf( "Closing the device ...\n" );

    close( s->cam.fd );

    return 1;
}

inline int offset ( int width, int row, int col, int color )
{
    return row * width * 3 + col * 3 + color;
}

inline void yuv420torgb24( unsigned char* data, ia_pixel_t* pix, int width, int height )
{
    int row, col;
    unsigned char *y, *u, *v;
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

inline void yuyvtorgb24( unsigned char* data, ia_pixel_t* pix, int width, int height )
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
