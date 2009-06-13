/*
 *  V4L2 video capture example
 *
 *  This program can be used and distributed without restrictions.
 */

#include "v4l2.h"

#ifdef HAVE_V4L2

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <getopt.h>             /* getopt_long() */

#include <fcntl.h>              /* low-level i/o */
#include <unistd.h>
#include <errno.h>
#include <malloc.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>

#include <asm/types.h>          /* for videodev2.h */

#include <linux/videodev2.h>

#define CLEAR(x) memset (&(x), 0, sizeof (x))

static void
errno_exit                      (const char *           s)
{
    fprintf (stderr, "%s error %d, %s\n",
             s, errno, strerror (errno));

    exit (EXIT_FAILURE);
}

static int
xioctl                          (int                    fd,
                                 int                    request,
                                 void *                 arg)
{
    int r;

    do r = ioctl (fd, request, arg);
    while (-1 == r && EINTR == errno);

    return r;
}

int
read_frame          (ia_v4l2_t*             v,
                     ia_image_t*            iaf)
{
    struct v4l2_buffer buf;
    unsigned int i;

    io_method io = v->io;
    int fd = v->fd;
    int n_buffers = v->n_buffers;
    struct buffer* buffers = v->buffers;


    switch (io) {
        case IO_METHOD_READ:
            if (-1 == read (fd, buffers[0].start, buffers[0].length)) {
                switch (errno) {
                    case EAGAIN:
                        return 0;

                    case EIO:
                        /* Could ignore EIO, see spec. */

                        /* fall through */

                    default:
                        errno_exit ("read");
                }
            }

            assert (v->width*v->height*3 <= buffers[0].length);
            ia_memcpy_uint8_to_pixel (iaf->pix, buffers[0].start, buffers[0].length);

            break;

        case IO_METHOD_MMAP:
            CLEAR (buf);

            buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            buf.memory = V4L2_MEMORY_MMAP;

            if (-1 == xioctl (fd, VIDIOC_DQBUF, &buf)) {
                switch (errno) {
                    case EAGAIN:
                        return 0;

                    case EIO:
                        /* Could ignore EIO, see spec. */

                        /* fall through */

                    default:
                        errno_exit ("VIDIOC_DQBUF");
                }
            }

            assert (buf.index < n_buffers);

            assert (buf.bytesused <= v->width*v->height*3);
            iaf->i_size = buf.bytesused;
            ia_memcpy_uint8_to_pixel (iaf->pix, buffers[buf.index].start, buf.bytesused);

            if (-1 == xioctl (fd, VIDIOC_QBUF, &buf))
                errno_exit ("VIDIOC_QBUF");

            break;

        case IO_METHOD_USERPTR:
            CLEAR (buf);

            buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            buf.memory = V4L2_MEMORY_USERPTR;

            if (-1 == xioctl (fd, VIDIOC_DQBUF, &buf)) {
                switch (errno) {
                    case EAGAIN:
                        return 0;

                    case EIO:
                    /* Could ignore EIO, see spec. */

                    /* fall through */

                    default:
                        errno_exit ("VIDIOC_DQBUF");
                }
            }

            for (i = 0; i < n_buffers; ++i)
                if (buf.m.userptr == (unsigned long) buffers[i].start
                    && buf.length == buffers[i].length)
                    break;

            assert (i < n_buffers);

            assert (v->width*v->height*3 <= buf.length);
            ia_memcpy_uint8_to_pixel (iaf->pix, (void*)buf.m.userptr, buf.length);

            if (-1 == xioctl (fd, VIDIOC_QBUF, &buf))
                errno_exit ("VIDIOC_QBUF");

            break;
    }

    return 1;
}

ia_image_t*
v4l2_readimage                        (ia_v4l2_t*                v,
                                       ia_image_t*             iaf)
{
    int fd          = v->fd;

    for (;;) {
        fd_set fds;
        struct timeval tv;
        int r;

        FD_ZERO (&fds);
        FD_SET (fd, &fds);

        // Timeout.
        tv.tv_sec = 2;
        tv.tv_usec = 0;

        r = select (fd + 1, &fds, NULL, NULL, &tv);

        if (-1 == r) {
            if (EINTR == errno)
                continue;
            errno_exit ("select");
        }

        if (0 == r) {
            fprintf (stderr, "select timeout\n");
            exit (EXIT_FAILURE);
        }

        if (read_frame (v, iaf))
            break;
    
        // EAGAIN - continue select loop.
    }

    return iaf;
}

static void
stop_capturing                  (ia_v4l2_t*             v)
{
    enum v4l2_buf_type type;

    io_method io    = v->io;
    int fd          = v->fd;

    switch (io) {
        case IO_METHOD_READ:
            /* Nothing to do. */
            break;

        case IO_METHOD_MMAP:
        case IO_METHOD_USERPTR:
            type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

            if (-1 == xioctl (fd, VIDIOC_STREAMOFF, &type))
                errno_exit ("VIDIOC_STREAMOFF");

            break;
    }
}

static void
start_capturing                 (ia_v4l2_t*             v)
{
    unsigned int i;
    enum v4l2_buf_type type;

    io_method io            = v->io;
    int n_buffers           = v->n_buffers;
    int fd                  = v->fd;
    struct buffer* buffers  = v->buffers;


    switch (io) {
        case IO_METHOD_READ:
            /* Nothing to do. */
            break;

        case IO_METHOD_MMAP:
            for (i = 0; i < n_buffers; ++i) {
                struct v4l2_buffer buf;

                CLEAR (buf);

                buf.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                buf.memory      = V4L2_MEMORY_MMAP;
                buf.index       = i;

                if (-1 == xioctl (fd, VIDIOC_QBUF, &buf))
                    errno_exit ("VIDIOC_QBUF");
            }
        
            type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

            if (-1 == xioctl (fd, VIDIOC_STREAMON, &type))
                errno_exit ("VIDIOC_STREAMON");
            
            break;

        case IO_METHOD_USERPTR:
            for (i = 0; i < n_buffers; ++i) {
                struct v4l2_buffer buf;

                CLEAR (buf);

                buf.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                buf.memory      = V4L2_MEMORY_USERPTR;
                buf.index       = i;
                buf.m.userptr   = (unsigned long) buffers[i].start;
                buf.length      = buffers[i].length;

                if (-1 == xioctl (fd, VIDIOC_QBUF, &buf))
                errno_exit ("VIDIOC_QBUF");
            }

            type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

            if (-1 == xioctl (fd, VIDIOC_STREAMON, &type))
                errno_exit ("VIDIOC_STREAMON");

            break;
    }
}

static void
uninit_device                   (ia_v4l2_t*         v)
{
    unsigned int i;

    io_method io            = v->io;
    struct buffer* buffers  = v->buffers;
    int n_buffers           = v->n_buffers;

    switch (io) {
        case IO_METHOD_READ:
            free (buffers[0].start);
            break;
        case IO_METHOD_MMAP:
            for (i = 0; i < n_buffers; ++i)
                if (-1 == munmap (buffers[i].start, buffers[i].length))
                    errno_exit ("munmap");
            break;

        case IO_METHOD_USERPTR:
            for (i = 0; i < n_buffers; ++i)
                free (buffers[i].start);
            break;
    }

    free (buffers);
}

static void
init_read           (ia_v4l2_t*             v,
                     unsigned int           buffer_size)
{
    struct buffer* buffers = calloc (1, sizeof (struct buffer));

    if (!buffers) {
        fprintf (stderr, "Out of memory\n");
        exit (EXIT_FAILURE);
    }

    buffers[0].length = buffer_size;
    buffers[0].start = malloc (buffer_size);

    if (!buffers[0].start) {
        fprintf (stderr, "Out of memory\n");
        exit (EXIT_FAILURE);
    }

    v->buffers = buffers;
}

static void
init_mmap           (ia_v4l2_t*             v)
{
    struct v4l2_requestbuffers req;

    int fd = v->fd;
    char* dev_name = v->dev_name;
    struct buffer* buffers;
    int n_buffers;

    CLEAR (req);

    req.count               = 4;
    req.type                = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory              = V4L2_MEMORY_MMAP;

    if (-1 == xioctl (fd, VIDIOC_REQBUFS, &req)) {
        if (EINVAL == errno) {
            fprintf (stderr, "%s does not support "
                     "memory mapping\n", dev_name);
            exit (EXIT_FAILURE);
        } else {
            errno_exit ("VIDIOC_REQBUFS");
        }
    }

    if (req.count < 2) {
        fprintf (stderr, "Insufficient buffer memory on %s\n",
                 dev_name);
        exit (EXIT_FAILURE);
    }

    buffers = calloc (req.count, sizeof (struct buffer));

    if (!buffers) {
        fprintf (stderr, "Out of memory\n");
        exit (EXIT_FAILURE);
    }

    for (n_buffers = 0; n_buffers < req.count; ++n_buffers) {
        struct v4l2_buffer buf;

        CLEAR (buf);
    
        buf.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory      = V4L2_MEMORY_MMAP;
        buf.index       = n_buffers;

        if (-1 == xioctl (fd, VIDIOC_QUERYBUF, &buf))
            errno_exit ("VIDIOC_QUERYBUF");

        buffers[n_buffers].length = buf.length;
        buffers[n_buffers].start =
            mmap (NULL /* start anywhere */,
                  buf.length,
                  PROT_READ | PROT_WRITE /* required */,
                  MAP_SHARED /* recommended */,
                  fd, buf.m.offset);

        if (MAP_FAILED == buffers[n_buffers].start)
            errno_exit ("mmap");
    }

    v->buffers = buffers;
    v->n_buffers = n_buffers;
}

static void
init_userp          (ia_v4l2_t*                 v,
                     unsigned int               buffer_size)
{
    struct v4l2_requestbuffers req;
    unsigned int page_size;

    int fd          = v->fd;
    char* dev_name  = v->dev_name;
    struct buffer* buffers;
    int n_buffers;


    page_size = getpagesize ();
    buffer_size = (buffer_size + page_size - 1) & ~(page_size - 1);

    CLEAR (req);

    req.count               = 4;
    req.type                = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory              = V4L2_MEMORY_USERPTR;

    if (-1 == xioctl (fd, VIDIOC_REQBUFS, &req)) {
        if (EINVAL == errno) {
            fprintf (stderr, "%s does not support "
            "user pointer i/o\n", dev_name);
            exit (EXIT_FAILURE);
        } else {
            errno_exit ("VIDIOC_REQBUFS");
        }
    }

    buffers = calloc (4, sizeof (struct buffer));

    if (!buffers) {
        fprintf (stderr, "Out of memory\n");
        exit (EXIT_FAILURE);
    }

    for (n_buffers = 0; n_buffers < 4; ++n_buffers) {
        buffers[n_buffers].length = buffer_size;
        buffers[n_buffers].start = memalign (/* boundary */ page_size,
                                             buffer_size);

        if (!buffers[n_buffers].start) {
            fprintf (stderr, "Out of memory\n");
            exit (EXIT_FAILURE);
        }
    }

    v->buffers = buffers;
    v->n_buffers = n_buffers;
}

static void
init_device                     (ia_v4l2_t*                 v)
{
    struct v4l2_capability cap;
    struct v4l2_cropcap cropcap;
    struct v4l2_crop crop;
    struct v4l2_format fmt;
    struct v4l2_fmtdesc fmtdesc;
    struct v4l2_frmsizeenum frmsize;
    unsigned int min;
    int i, j;
    int available_formats[500] = {0};
    int available_frmsize[500][2] = {0,0};

    int fd          = v->fd;
    char* dev_name  = v->dev_name;
    io_method io    = v->io;
    int width       = v->width;
    int height      = v->height;

    if (-1 == xioctl (fd, VIDIOC_QUERYCAP, &cap)) {
        if (EINVAL == errno) {
            fprintf (stderr, "%s is no V4L2 device\n",
                     dev_name);
            exit (EXIT_FAILURE);
        } else {
            errno_exit ("VIDIOC_QUERYCAP");
        }
    }

    if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
        fprintf (stderr, "%s is no video capture device\n",
                 dev_name);
        exit (EXIT_FAILURE);
    }

    if (io == IO_METHOD_MMAP || io == IO_METHOD_USERPTR) {
        if (!(cap.capabilities & V4L2_CAP_STREAMING)) {
            fprintf (stderr, "%s does not support streaming i/o\n",
                     dev_name);

            /* try read method */
            io = IO_METHOD_READ;
        }
    }

    if (io == IO_METHOD_READ) {
        if (!(cap.capabilities & V4L2_CAP_READWRITE)) {
            fprintf (stderr, "%s does not support read i/o\n",
                     dev_name);
            exit (EXIT_FAILURE);
        }
    }


    /* Select video input, video standard and tune here. */


    CLEAR (cropcap);

    cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (0 == xioctl (fd, VIDIOC_CROPCAP, &cropcap)) {
        crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        crop.c = cropcap.defrect; /* reset to default */

        if (-1 == xioctl (fd, VIDIOC_S_CROP, &crop)) {
            switch (errno) {
                case EINVAL:
                    /* Cropping not supported. */
                    break;
                default:
                    /* Errors ignored. */
                    break;
            }
        }
    } else {    
        /* Errors ignored. */
    }


    CLEAR (fmt);

    fmt.type                = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width       = -1; 
    fmt.fmt.pix.height      = -1;
    fmt.fmt.pix.pixelformat = -1;
    fmt.fmt.pix.field       = V4L2_FIELD_NONE;

    // discover what pixel formats and frmsizes are supported by the camera 
    for (i = 0;; i++) {
        ia_pixel_format pf;

        CLEAR (fmtdesc);
        fmtdesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        fmtdesc.index = i;

        if (-1 == xioctl (fd, VIDIOC_ENUM_FMT, &fmtdesc))
            break;

        pf = ia_convert_format (fmtdesc.pixelformat, IA_PIX_FMT_V4L2);
        if (-1 != pf) {
            available_formats[pf] = fmtdesc.pixelformat;

            // get best available frmsize 
            for (j = 0;; j++) {
                CLEAR (frmsize);
                frmsize.pixel_format = fmtdesc.pixelformat;
                frmsize.index = j;

                if (-1 == xioctl (fd, VIDIOC_ENUM_FRAMESIZES, &frmsize))
                    break;

                // if the desired width and height were not specified then try
                // to get the largest possible frame size
                if (width*height == 0) {
                    if (available_frmsize[i][0]*available_frmsize[i][1] <
                        frmsize.discrete.width*frmsize.discrete.height) {
                        available_frmsize[i][0] = frmsize.discrete.width;
                        available_frmsize[i][1] = frmsize.discrete.height;
                    }
                // else, try to get the frame size that best matches the 
                // desired width and height
                } else {
                    if (fabs(frmsize.discrete.width*frmsize.discrete.height
                            - width*height) <
                        fabs(available_frmsize[i][0]*available_frmsize[i][1]
                            - width*height)) {
                        available_frmsize[i][0] = frmsize.discrete.width;
                        available_frmsize[i][1] = frmsize.discrete.height;
                    }
                }
            }
        }
    }


    // select the format that has the smallest index
    for (i--; -1 < i; i--) {
        if (available_formats[i]) {
            fmt.fmt.pix.pixelformat = available_formats[i];
            fmt.fmt.pix.width = available_frmsize[i][0];
            fmt.fmt.pix.height = available_frmsize[i][1];
        }
    }

    /* if no supported formats were found, error out */
    if (-1 == fmt.fmt.pix.pixelformat ||
        -1 == fmt.fmt.pix.width ||
        -1 == fmt.fmt.pix.height)
        errno_exit ("VIDIOC_ENUM_FMT");

    if (-1 == xioctl (fd, VIDIOC_S_FMT, &fmt)) {
        printf("got error here\n");
        errno_exit ("VIDIOC_S_FMT");
    }

    /* Note VIDIOC_S_FMT may change width and height. */

    /* Buggy driver paranoia. */
    min = fmt.fmt.pix.width * 2;
    if (fmt.fmt.pix.bytesperline < min)
        fmt.fmt.pix.bytesperline = min;
    min = fmt.fmt.pix.bytesperline * fmt.fmt.pix.height;
    if (fmt.fmt.pix.sizeimage < min)
        fmt.fmt.pix.sizeimage = min;

    switch (io) {
        case IO_METHOD_READ:
            init_read (v, fmt.fmt.pix.sizeimage);
            break;
        case IO_METHOD_MMAP:
            init_mmap (v);
            break;
        case IO_METHOD_USERPTR:
            init_userp (v, fmt.fmt.pix.sizeimage);
            break;
    }

    v->width = fmt.fmt.pix.width;
    v->height = fmt.fmt.pix.height;
}

static void
close_device                    (ia_v4l2_t*             v)
{
    int fd = v->fd;
    if (-1 == close (fd))
        errno_exit ("close");

    fd = -1;
}

static void
open_device                     (ia_v4l2_t*             v)
{
    struct stat st;
    char* dev_name = v->dev_name;
    int fd;

    if (-1 == stat (dev_name, &st)) {
        fprintf (stderr, "Cannot identify '%s': %d, %s\n",
                 dev_name, errno, strerror (errno));
        exit (EXIT_FAILURE);
    }

    if (!S_ISCHR (st.st_mode)) {
        fprintf (stderr, "%s is no device\n", dev_name);
        exit (EXIT_FAILURE);
    }

    fd = open (dev_name, O_RDWR /* required */ | O_NONBLOCK, 0);

    if (-1 == fd) {
        fprintf (stderr, "Cannot open '%s': %d, %s\n",
        dev_name, errno, strerror (errno));
        exit (EXIT_FAILURE);
    }

    v->fd = fd;
}

ia_v4l2_t*
v4l2_open                       (int                    width,
                                 int                    height,
                                 const char*            dev_name)
{
    ia_v4l2_t* v = malloc (sizeof(ia_v4l2_t));

    if (!v) return NULL;
    ia_memset (v, 0, sizeof(ia_v4l2_t));
    v->width    = width;
    v->height   = height;
    strncpy (v->dev_name, dev_name, 1031);
    v->io       = IO_METHOD_MMAP;

    open_device (v);
    init_device (v);
    start_capturing (v);
    return v;
}

void
v4l2_close                      (ia_v4l2_t*             v)
{
    stop_capturing (v);
    uninit_device (v);
    close_device (v);
    ia_free (v);
}

#endif
