#ifndef _H_IAIO_V4L2
#define _H_IAIO_V4L2

#include "common.h"

#ifdef HAVE_V4L2

typedef enum {
    IO_METHOD_READ,
    IO_METHOD_MMAP,
    IO_METHOD_USERPTR,
} io_method;

typedef struct buffer {
    void *                  start;
    size_t                  length;
} buffer;

typedef struct ia_v4l2_t {
    char                dev_name[1024];
    io_method           io;
    int                 fd;
    struct buffer *     buffers;
    unsigned int        n_buffers;
    int                 width;
    int                 height;
} ia_v4l2_t;

ia_image_t*
v4l2_readimage (ia_v4l2_t* v, ia_image_t* im);

ia_v4l2_t*
v4l2_open (int width, int height, const char* dev_name);

void
v4l2_close (ia_v4l2_t* v);

#endif
#endif
