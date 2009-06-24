#include "v4l.h"

#ifdef HAVE_V4L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <malloc.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>

#include <linux/videodev.h>

#define CLEAR(x) ia_memset (&(x), 0, sizeof (x))

static const struct {
    int palette;
    int depth;
    ia_pixel_format pix_fmt;
} video_formats [] = {
    /* YUV */
    {.palette = VIDEO_PALETTE_YUV422,   .depth = 16,
     .pix_fmt = IA_PIX_FMT_YUYV422},
    {.palette = VIDEO_PALETTE_YUYV,     .depth = 16,
     .pix_fmt = IA_PIX_FMT_YUYV422},
    {.palette = VIDEO_PALETTE_UYVY,     .depth = 16,
     .pix_fmt = IA_PIX_FMT_UYVY422},
    {.palette = VIDEO_PALETTE_YUV420,   .depth = 12,
     .pix_fmt = IA_PIX_FMT_YUV420P},
    {.palette = VIDEO_PALETTE_YUV411,   .depth = 12,
     .pix_fmt = IA_PIX_FMT_YUV411P},
    {.palette = VIDEO_PALETTE_YUV422P,  .depth = 16,
     .pix_fmt = IA_PIX_FMT_YUV422P},
    {.palette = VIDEO_PALETTE_YUV411P,  .depth = 12,
     .pix_fmt = IA_PIX_FMT_YUV411P},
    /* RGB */
    {.palette = VIDEO_PALETTE_RGB24,    .depth = 24,
     .pix_fmt = IA_PIX_FMT_RGB24},
/*    {.palette = VIDEO_PALETTE_RGB565,   .depth = 16,
     .pix_fmt = IA_PIX_FMT_BGR565}, XXX Currently not supported by swscale */
    {.palette = VIDEO_PALETTE_RGB555,   .depth = 16,
     .pix_fmt = IA_PIX_FMT_RGB555},
/*    {.palette = VIDEO_PALETTE_RGB32,    .depth = 32,
     .pix_fmt = IA_PIX_FMT_RGB32},  */
    {.palette = VIDEO_PALETTE_GREY,     .depth = 8,
     .pix_fmt = IA_PIX_FMT_GRAY8},
};

static const int vformat_num = 10;

static void
errno_exit                      (const char *           s)
{
    ia_fprintf (stderr, "%s error %d, %s\n",
             s, errno, ia_strerror (errno));

    ia_exit (EXIT_FAILURE);
}

static int
xioctl                          (int                    fd,
                                 int                    request,
                                 void *                 arg)
{
    int r;

    do r = ia_ioctl (fd, request, arg);
    while (-1 == r && EINTR == errno);

    return r;
}

int
v4l_read_frame                  (ia_v4l_t*              v,
                                 ia_image_t*            iaf)
{
    struct video_mmap vmmap;
    unsigned int i;

    io_method io            = v->io;
    int fd                  = v->fd;
    unsigned int n_buffers  = v->n_buffers;
    struct buffer* buffers  = v->buffers;
    unsigned int frame      = v->frame % n_buffers;
    int width               = v->width;
    int height              = v->height;
    int palette             = video_formats[v->pix_fmt].palette;
    assert (n_buffers > 0);
    int length;

    switch (io) {
        case IO_METHOD_READ:
            while (-1 == ia_read (fd, iaf->pix, iaf->i_size)
                   && (errno == EINTR || errno == EAGAIN));
            if (errno == EINVAL)
                errno_exit ("read");

            frame++;
            break;

        case IO_METHOD_MMAP:
            length = iaf->i_size < buffers[frame % n_buffers].length
                 ? iaf->i_size
                 : buffers[frame % n_buffers].length;

            /* wait for the driver to finish capturing the frame */
            for (;;) {
                if (-1 == xioctl (fd, VIDIOCSYNC, &frame)) {
                    if (errno == EAGAIN || errno == EINTR)
                        continue;
                    errno_exit ("VIDIOCSYNC");
                }
                break;
            }

            memcpy (iaf->pix, buffers[frame % n_buffers].start, length);
            frame++;

            vmmap.frame = (frame + n_buffers) % n_buffers;
            vmmap.height = height;
            vmmap.width = width;
            vmmap.format = palette;

            if (-1 == xioctl (fd, VIDIOCMCAPTURE, &vmmap)) {
                errno_exit ("VIDIOCMCAPTURE");
            }

            break;

        default:
            return -1;
    }

    v->frame = frame;
}

ia_image_t*
v4l_readimage                   (ia_v4l_t*              v,
                                 ia_image_t*            iaf)
{
    int fd = v->fd;

    for (;;) {
        if (v4l_read_frame (v, iaf))
            break;
        ia_usleep (10);
    }
    return iaf;
}

static void
start_capturing                 (ia_v4l_t*              v)
{
    struct video_mmap vmmap;
    unsigned int i;

    int fd                  = v->fd;
    unsigned int frame      = v->frame;
    unsigned int n_buffers  = v->n_buffers;
    int width               = v->width;
    int height              = v->height;
    int palette             = video_formats[v->pix_fmt].palette;

    /* tell the driver to start capturing frames */
    for (i = frame; i < n_buffers + frame; i++) {
        CLEAR (vmmap);

        vmmap.frame     = i % n_buffers;
        vmmap.height    = height;
        vmmap.width     = width;
        vmmap.format    = palette;

        if (-1 == xioctl (fd, VIDIOCMCAPTURE, &vmmap)) {
            errno_exit ("VIDIOCMCAPTURE");
        }
    }
}

static void
uninit_device                   (ia_v4l_t*              v)
{
    unsigned int i;

    io_method io            = v->io;
    void* buffer            = v->mmap_buffer;
    size_t length           = v->mmap_length;
    struct buffer* buffers  = v->buffers;

    switch (io) {
        case IO_METHOD_MMAP:
            if (-1 == ia_munmap (buffer, length))
                errno_exit ("munmap");
            ia_free (buffers);
            break;
        default:
            break;
    }
}

static void
init_mmap                       (ia_v4l_t*              v)
{
    struct video_mbuf vmbuf;

    int fd          = v->fd;
    char* dev_name  = v->dev_name;
    int width       = v->width;
    int height      = v->height;
    int pix_fmt     = v->pix_fmt;

    struct buffer* buffers;
    unsigned int n_buffers;
    void* buffer;

    CLEAR (vmbuf);

    /* query device for buffer parameters */
    if (-1 == xioctl (fd, VIDIOCGMBUF, &vmbuf)) {
        switch (errno) {
            case EINVAL:
                ia_fprintf (stderr, "%s does not support "
                            "memory mapping\n", dev_name);
                ia_exit (EXIT_FAILURE);
            default:
                errno_exit ("VIDIOCGMBUF");
        }
    }

    if (vmbuf.frames < 1) {
        ia_fprintf (stderr, "Insufficient buffer memory on %s\n",
                    dev_name);
        ia_exit (EXIT_FAILURE);
    }

    if (NULL == (buffers = ia_calloc (vmbuf.frames, sizeof (struct buffer)))) {
        ia_fprintf (stderr, "Out of memory\n");
        ia_exit (EXIT_FAILURE);
    }

    /* mmap a buffer from the device */
    if (MAP_FAILED == (buffer = ia_mmap (NULL, vmbuf.size,
                                         PROT_READ|PROT_WRITE, MAP_SHARED,
                                         fd, vmbuf.offsets[0]))) {
        /* if MAP_SHARED is not supported */
        if (MAP_FAILED == (buffer = ia_mmap (NULL, vmbuf.size,
                                             PROT_READ|PROT_WRITE, MAP_PRIVATE,
                                             fd, vmbuf.offsets[0]))) {
            errno_exit ("MMAP");
        }
    }

    /* try to do things the v4l2 way */
    for (n_buffers = 0; n_buffers < vmbuf.frames; ++n_buffers) {
        buffers[n_buffers].length = width * height *
                                    video_formats[pix_fmt].depth / 8;
        buffers[n_buffers].start = buffer + vmbuf.offsets[n_buffers];
    }

    v->buffers = buffers;
    v->n_buffers = n_buffers;
    v->mmap_buffer = buffer;
    v->mmap_length = vmbuf.size;
}

static void
init_device                     (ia_v4l_t*              v)
{
    struct video_capability vcap;
    struct video_window vwind;
    struct video_picture vpict;
    unsigned int i;

    int fd          = v->fd;
    char* dev_name  = v->dev_name;
    int width       = v->width;
    int height      = v->height;
    io_method io    = v->io;

    /* query the device for its capabilities */
    CLEAR (vcap);

    if (-1 == xioctl (fd, VIDIOCGCAP, &vcap)) {
        errno_exit ("VIDIOCGCAP");
    }

    /* make sure it supports capture to memory
     * and has at least one video channel */
    if (VID_TYPE_CAPTURE != vcap.type) {
        ia_fprintf (stderr, "%s does not support "
                    "capture to memory\n", dev_name);
        ia_exit (EXIT_FAILURE);
    }

    /* make sure it has at least one video channel */
    if (vcap.channels < 1) {
        ia_fprintf (stderr, "%s does not have any video channels\n", dev_name);
        ia_exit (EXIT_FAILURE);
    }

    /* if no width was provided then choose the maximum */
    if (width <= 0) {
        width = vcap.maxwidth;
    /* make sure any provided width is within bounds */
    } else if (vcap.maxwidth < width || width < vcap.minwidth) {
        ia_fprintf (stderr, "%s supports a maximum resolution of %dx%d\n",
                    dev_name, vcap.maxwidth, vcap.maxheight);
        ia_exit (EXIT_FAILURE);
    }

    /* if no height was provided then choose the maximum */
    if (height <= 0) {
        height = vcap.maxheight;
    /* make sure any provided height is within bounds */
    } else if (vcap.maxheight < height || height < vcap.minheight) {
        ia_fprintf (stderr, "%s supports a maximum resolution of %dx%d\n",
                    dev_name, vcap.maxwidth, vcap.maxheight);
        ia_exit (EXIT_FAILURE);
    }

    /* set capture window */
    CLEAR (vwind);

    vwind.x = 0;
    vwind.y = 0;
    vwind.width = width;
    vwind.height = height;

    if (-1 == xioctl (fd, VIDIOCSWIN, &vwind)) {
        errno_exit ("VIDIOCSWIN");
    }

    /* make sure the device took our settings */
    CLEAR (vwind);
    if (-1 == xioctl (fd, VIDIOCGWIN, &vwind)) {
        errno_exit ("VIDIOCGWIN");
    }

    /* if the settings didnt stick then exit with failure */
    if (0 != vwind.x || 0 != vwind.y ||
        width != vwind.width || height != vwind.height) {
        ia_fprintf (stderr, "%s did not hold our settings\n", dev_name);
        ia_exit (EXIT_FAILURE);
    }

    /* query image properties */
    CLEAR (vpict);

    if (-1 == xioctl (fd, VIDIOCGPICT, &vpict)) {
        errno_exit ("VIDIOCGPICT");
    }

    /* set the picture format */
    i = 7;
    vpict.palette = video_formats[i].palette;
    vpict.depth = video_formats[i].depth;
    
    if (-1 == xioctl (fd, VIDIOCSPICT, &vpict)) {
        for (i = 0; i < vformat_num; i++) {
            vpict.palette = video_formats[i].palette;
            vpict.depth = video_formats[i].depth;
            if (-1 != xioctl (fd, VIDIOCSPICT, &vpict)) {
                break;
            }
        }
        if (vformat_num <= i) {
            fprintf (stderr, "%s does not provide"
                     "any formats that I support\n", dev_name);
            ia_exit (EXIT_FAILURE);
        }
    }

    v->width = width;
    v->height = height;
    v->pix_fmt = i;
    v->ia_pix_fmt = video_formats[i].pix_fmt;

    switch (io) {
        case IO_METHOD_MMAP:
            init_mmap (v);
            break;
        case IO_METHOD_READ:
            v->n_buffers = 1;
        default:
            break;
    }
}

static void
close_device                    (ia_v4l_t*              v)
{
    int fd = v->fd;
    if (-1 == close (fd)) {
        errno_exit ("close");
    }

    fd = -1;
}

static void
open_device                     (ia_v4l_t*              v)
{
    struct stat st;
    char* dev_name = v->dev_name;
    int fd;

    if (-1 == ia_stat (dev_name, &st)) {
       ia_fprintf (stderr, "Cannot identify '%s': %d, %s\n",
                    dev_name, errno, ia_strerror (errno));
        ia_exit (EXIT_FAILURE);
    }

    if (!S_ISCHR (st.st_mode)) {
        ia_fprintf (stderr, "%s is no device\n", dev_name);
        ia_exit (EXIT_FAILURE);
    }

    fd = ia_open (dev_name, O_RDWR /* required */ | O_NONBLOCK, 0);

    if (-1 == fd) {
        ia_fprintf (stderr, "Cannot open '%s': %d, %s\n",
                    dev_name, errno, ia_strerror (errno));
        ia_exit (EXIT_FAILURE);
    }

    v->fd = fd;
}

ia_v4l_t*
v4l_open                        (int                    width,
                                 int                    height,
                                 const char*            dev_name)
{
    ia_v4l_t* v = ia_calloc (1, sizeof(ia_v4l_t));

    if (NULL == v) {
        return NULL;
    }

    v->width    = width;
    v->height   = height;
    v->io       = IO_METHOD_READ;
    ia_strncpy (v->dev_name, dev_name, 1031);

    open_device (v);
    init_device (v);
    start_capturing (v);

    return v;
}

void
v4l_close                       (ia_v4l_t*          v)
{
    uninit_device (v);
    close_device (v);
    ia_free (v);
}

#endif
