#ifndef _H_IAIO
#define _H_IAIO

/* TODO do some fancy stuff to determine what lib to use to read/save images */
#include <FreeImage.h>
#include "common.h"
#include "image_analyzer.h"
#include "ia_sequence.h"


/* video device interface object */
typedef struct iaio_cam_buffers
{
    void* start;
    size_t length;
} iaio_cam_buffer;

/* camera io parameters */
typedef struct iaio_cam_t
{
    int fd;
    int palette;

    iaio_cam_buffer* buffers;

    unsigned int i_buffers;
    int capturing;

    int data_size;
    int pos;
} iaio_cam_t;

/* image list-file io parameters */
typedef struct iaio_file_t
{
    FILE*   filp;
    char*   filename;
    char*   buf;
} iaio_file_t;

typedef struct iaio_t
{
    iaio_cam_t      cam;        // for cam io
    iaio_file_t     fin;        // for file io

    FIBITMAP*   dib;            // buffer for writing images to disk
    uint64_t    last_frame;
    uint32_t    i_size;
    uint32_t    i_width;
    uint32_t    i_height;
    bool        eoi;
} iaio_t;

/* captures image from iaio stream and stores into the iaf image object */
int iaio_getimage( iaio_t* iaio, ia_image_t* iaf );

/* reads image data from image file `str' into `iaf' */
//XXX no need for this to be exposed
//int	iaio_file_getimage( char* str, ia_image_t* iaf );

/* fill ia_param_t width/height with width/height of specified image */
//XXX no need for this to be exposed
//int iaio_file_probeimage( iaio_t* iaio );

/* stores image data from iaf into file specified by str */
int	iaio_saveimage( iaio_t* iaio, ia_image_t* iar );
//static int  iaio_opencv_saveimage( ia_image_t* iaf );

/* open iaio object */
iaio_t* iaio_open( ia_seq_t* ias );

/* close iaio object */
void iaio_close( iaio_t* iaio );

/*
 * camera io functions
 */

/* init camera device */
//XXX no need for this to be exposed
//int  iaio_cam_init( iaio_t* iaio, ia_param_t* param );

/* fills ia_image_t struct with an image captured from camera */
//XXX no need for this to be exposed
//int  iaio_cam_getimage ( iaio_t* iaio, ia_image_t* iaf );

/* closes camera device */
//XXX no need for this to be exposed
//int  iaio_cam_close ( iaio_t* s );

inline void yuv420torgb24( unsigned char* data, ia_pixel_t* pix, int width, int height );
inline void yuyvtorgb24( unsigned char* data, ia_pixel_t* pix, int width, int height );

#endif
