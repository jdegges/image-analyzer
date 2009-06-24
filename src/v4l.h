#ifndef _H_IAIO_V4L
#define _H_IAIO_V4L

#include "common.h"
#include "v4l2.h"

typedef struct ia_v4l_t {
    char                dev_name[1031];
    io_method           io;
    int                 fd;
    struct buffer *     buffers;
    unsigned int        n_buffers;
    int                 width;
    int                 height;

    unsigned int        frame;
    int                 pix_fmt;
    ia_pixel_format     ia_pix_fmt;
    void*               mmap_buffer;
    size_t              mmap_length;
} ia_v4l_t;


#ifdef HAVE_V4L

ia_image_t*
v4l_readimage (ia_v4l_t* v, ia_image_t* im);

ia_v4l_t*
v4l_open (int width, int height, const char* dev_name);

void
v4l_close (ia_v4l_t* v);

#endif
#endif
