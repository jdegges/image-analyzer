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

#ifndef _H_IAIO
#define _H_IAIO

#include <FreeImage.h>
#include <SDL/SDL.h>
#include "common.h"
#include "image_analyzer.h"

#define IAIO_DISK       1
#define IAIO_DISPLAY    2
#define IAIO_CAMERA     3
#define IAIO_FILE       4

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
    /* file io */
    FILE*       filp;
    char*       filename;
    char*       buf;

    /* stream io */
    FreeImageIO io;
    FILE*       output_stream;
    char        mime_type[16];
} iaio_file_t;

typedef struct iaio_t
{
    uint8_t         input_type;
    uint8_t         output_type;

    iaio_cam_t      cam;        // for cam input
    iaio_file_t     fin;        // for file input

    SDL_Surface*    screen;     // for displaying video
    FIBITMAP*       dib;        // for writing images to disk
    uint64_t        last_frame;
    uint32_t        i_size;
    uint32_t        i_width;
    uint32_t        i_height;
    bool            eoi;
} iaio_t;

/* captures image from iaio stream and stores into the iaf image object */
int iaio_getimage( iaio_t* iaio, ia_image_t* iaf );

/* stores image data from iaf into file specified by str */
int iaio_outputimage( iaio_t* iaio, ia_image_t* iar );

/* open iaio object */
iaio_t* iaio_open( ia_param_t* p );

/* close iaio object */
void iaio_close( iaio_t* iaio );

inline void yuv420torgb24( unsigned char* data, ia_pixel_t* pix, int width, int height );
inline void yuyvtorgb24( unsigned char* data, ia_pixel_t* pix, int width, int height );

#endif
