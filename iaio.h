#ifndef _H_IAIO
#define _H_IAIO

/* TODO do some fancy stuff to determine what lib to use to read/save images */
#include <FreeImage.h>
#include "common.h"
#include "ia_sequence.h"
#include "image_analyzer.h"

/* reads image data from image specified by str and stores image data into iaframe iaf */
int	iaio_getimage( char* str,ia_image_t* iaf );

/* fill ia_param_t width/height with width/height of specified image */
int iaio_probeimage( ia_seq_t* s, const char* str );

/* stores image data from iaf into file specified by str */
int	iaio_saveimage( ia_seq_t* ias );
//static int  iaio_opencv_saveimage( ia_image_t* iaf );

/*
 * camera io functions
 */

/* init camera device */
int  iaio_cam_init ( ia_seq_t* p );

/* fills ia_image_t struct with an image captured from camera */
int  iaio_cam_getimage ( ia_seq_t* s );

/* closes camera device */
int  iaio_cam_close ( ia_seq_t* s );

inline void yuv420torgb24( unsigned char* data, ia_pixel_t* pix, int width, int height );
inline void yuyvtorgb24( unsigned char* data, ia_pixel_t* pix, int width, int height );

#endif
