/******************************************************************************
 * Copyright (c) 2008 Joey Degges
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

#ifndef _H_COMMON
#define _H_COMMON

#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 500
#endif

#include "config.h"
#include <sys/select.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#if   defined (HAVE_STDINT_H)
#include <stdint.h>
#elif defined (HAVE_INTTYPES_H)
#include <inttypes.h>
#endif
#include <stdbool.h>
#include <math.h>
#include <pthread.h>

#define ia_pixel_t uint8_t
static const int debug = 0;

#if HAVE_LIBAVCODEC && HAVE_LIBAVFORMAT && HAVE_LIBAVUTIL && HAVE_LIBSWSCALE && HAVE_LIBZ
#define HAVE_FFMPEG
#endif

#ifdef HAVE_LINUX_VIDEODEV2_H
#define HAVE_V4L2 1
#include <linux/videodev2.h>
#endif

/* ia_image_t: image data structure
 * i_frame: position of this frame in image stream
 * pix    : pixel data
 * name   : output name
 * thumbname : thumbnail name
 * users  : number of threads using this image
 * ready  : if input image  -> must be set to read data
 *          if output image -> must be set for system to save data
 * lock   : you must have this lock in order to write to this image
*/
typedef struct ia_image_t
{
    char        name[1024];
    char        thumbname[1024];
    uint64_t    i_frame;
    int32_t     i_refcount;
    uint64_t    i_size;
    ia_pixel_t* pix;
    void*       dib;
    struct ia_image_t* next;
    struct ia_image_t* last;
    bool        eoi;
    pthread_mutex_t mutex;
    pthread_cond_t cond_ro;
    pthread_cond_t cond_rw;
} ia_image_t;

typedef enum {
    IA_PIX_FMT_NONE= -1,
    IA_PIX_FMT_YUV420P,   ///< planar YUV 4:2:0, 12bpp, (1 Cr & Cb sample per 2x2 Y samples)
    IA_PIX_FMT_YUYV422,   ///< packed YUV 4:2:2, 16bpp, Y0 Cb Y1 Cr
    IA_PIX_FMT_RGB24,     ///< packed RGB 8:8:8, 24bpp, RGBRGB...
    IA_PIX_FMT_BGR24,     ///< packed RGB 8:8:8, 24bpp, BGRBGR...
    IA_PIX_FMT_YUV422P,   ///< planar YUV 4:2:2, 16bpp, (1 Cr & Cb sample per 2x1 Y samples)
    IA_PIX_FMT_YUV444P,   ///< planar YUV 4:4:4, 24bpp, (1 Cr & Cb sample per 1x1 Y samples)
    IA_PIX_FMT_RGB32,     ///< packed RGB 8:8:8, 32bpp, (msb)8A 8R 8G 8B(lsb), in CPU endianness
    IA_PIX_FMT_YUV410P,   ///< planar YUV 4:1:0,  9bpp, (1 Cr & Cb sample per 4x4 Y samples)
    IA_PIX_FMT_YUV411P,   ///< planar YUV 4:1:1, 12bpp, (1 Cr & Cb sample per 4x1 Y samples)
    IA_PIX_FMT_RGB565,    ///< packed RGB 5:6:5, 16bpp, (msb)   5R 6G 5B(lsb), in CPU endianness
    IA_PIX_FMT_RGB555,    ///< packed RGB 5:5:5, 16bpp, (msb)1A 5R 5G 5B(lsb), in CPU endianness, most significant bit to 0
    IA_PIX_FMT_GRAY8,     ///<        Y        ,  8bpp
    IA_PIX_FMT_MONOWHITE, ///<        Y        ,  1bpp, 0 is white, 1 is black
    IA_PIX_FMT_MONOBLACK, ///<        Y        ,  1bpp, 0 is black, 1 is white
    IA_PIX_FMT_PAL8,      ///< 8 bit with IA_PIX_FMT_RGB32 palette
    IA_PIX_FMT_YUVJ420P,  ///< planar YUV 4:2:0, 12bpp, full scale (JPEG)
    IA_PIX_FMT_YUVJ422P,  ///< planar YUV 4:2:2, 16bpp, full scale (JPEG)
    IA_PIX_FMT_YUVJ444P,  ///< planar YUV 4:4:4, 24bpp, full scale (JPEG)
    IA_PIX_FMT_XVMC_MPEG2_MC,///< XVideo Motion Acceleration via common packet passing
    IA_PIX_FMT_XVMC_MPEG2_IDCT,
    IA_PIX_FMT_UYVY422,   ///< packed YUV 4:2:2, 16bpp, Cb Y0 Cr Y1
    IA_PIX_FMT_UYYVYY411, ///< packed YUV 4:1:1, 12bpp, Cb Y0 Y1 Cr Y2 Y3
    IA_PIX_FMT_BGR32,     ///< packed RGB 8:8:8, 32bpp, (msb)8A 8B 8G 8R(lsb), in CPU endianness
    IA_PIX_FMT_BGR565,    ///< packed RGB 5:6:5, 16bpp, (msb)   5B 6G 5R(lsb), in CPU endianness
    IA_PIX_FMT_BGR555,    ///< packed RGB 5:5:5, 16bpp, (msb)1A 5B 5G 5R(lsb), in CPU endianness, most significant bit to 1
    IA_PIX_FMT_BGR8,      ///< packed RGB 3:3:2,  8bpp, (msb)2B 3G 3R(lsb)
    IA_PIX_FMT_BGR4,      ///< packed RGB 1:2:1,  4bpp, (msb)1B 2G 1R(lsb)
    IA_PIX_FMT_BGR4_BYTE, ///< packed RGB 1:2:1,  8bpp, (msb)1B 2G 1R(lsb)
    IA_PIX_FMT_RGB8,      ///< packed RGB 3:3:2,  8bpp, (msb)2R 3G 3B(lsb)
    IA_PIX_FMT_RGB4,      ///< packed RGB 1:2:1,  4bpp, (msb)1R 2G 1B(lsb)
    IA_PIX_FMT_RGB4_BYTE, ///< packed RGB 1:2:1,  8bpp, (msb)1R 2G 1B(lsb)
    IA_PIX_FMT_NV12,      ///< planar YUV 4:2:0, 12bpp, 1 plane for Y and 1 for UV
    IA_PIX_FMT_NV21,      ///< as above, but U and V bytes are swapped

    IA_PIX_FMT_RGB32_1,   ///< packed RGB 8:8:8, 32bpp, (msb)8R 8G 8B 8A(lsb), in CPU endianness
    IA_PIX_FMT_BGR32_1,   ///< packed RGB 8:8:8, 32bpp, (msb)8B 8G 8R 8A(lsb), in CPU endianness

    IA_PIX_FMT_GRAY16BE,  ///<        Y        , 16bpp, big-endian
    IA_PIX_FMT_GRAY16LE,  ///<        Y        , 16bpp, little-endian
    IA_PIX_FMT_YUV440P,   ///< planar YUV 4:4:0 (1 Cr & Cb sample per 1x2 Y samples)
    IA_PIX_FMT_YUVJ440P,  ///< planar YUV 4:4:0 full scale (JPEG)
    IA_PIX_FMT_YUVA420P,  ///< planar YUV 4:2:0, 20bpp, (1 Cr & Cb sample per 2x2 Y & A samples)
    IA_PIX_FMT_VDPAU_H264,///< H.264 HW decoding with VDPAU, data[0] contains a vdpau_render_state struct which contains the bitstream of the slices as well as various fields extracted from headers
    IA_PIX_FMT_VDPAU_MPEG1,///< MPEG-1 HW decoding with VDPAU, data[0] contains a vdpau_render_state struct which contains the bitstream of the slices as well as various fields extracted from headers
    IA_PIX_FMT_VDPAU_MPEG2,///< MPEG-2 HW decoding with VDPAU, data[0] contains a vdpau_render_state struct which contains the bitstream of the slices as well as various fields extracted from headers
    IA_PIX_FMT_VDPAU_WMV3,///< WMV3 HW decoding with VDPAU, data[0] contains a vdpau_render_state struct which contains the bitstream of the slices as well as various fields extracted from headers
    IA_PIX_FMT_VDPAU_VC1, ///< VC-1 HW decoding with VDPAU, data[0] contains a vdpau_render_state struct which contains the bitstream of the slices as well as various fields extracted from headers
    IA_PIX_FMT_RGB48BE,   ///< packed RGB 16:16:16, 48bpp, 16R, 16G, 16B, big-endian
    IA_PIX_FMT_RGB48LE,   ///< packed RGB 16:16:16, 48bpp, 16R, 16G, 16B, little-endian
    IA_PIX_FMT_VAAPI_MOCO, ///< HW acceleration through VA API at motion compensation entry-point, Picture.data[3] contains a vaapi_render_state struct which contains macroblocks as well as various fields extracted from headers
    IA_PIX_FMT_VAAPI_IDCT, ///< HW acceleration through VA API at IDCT entry-point, Picture.data[3] contains a vaapi_render_state struct which contains fields extracted from headers
    IA_PIX_FMT_VAAPI_VLD,  ///< HW decoding through VA API, Picture.data[3] contains a vaapi_render_state struct which contains the bitstream of the slices as well as various fields extracted from headers
    IA_PIX_FMT_NB,        ///< number of pixel formats, DO NOT USE THIS if you want to link with shared libav* because the number of formats might differ between versions
} ia_pixel_format;

typedef enum {
    IA_PIX_FMT_FFMPEG,
    IA_PIX_FMT_V4L2,
} ia_pixel_format_type;

static inline ia_pixel_format ia_convert_format( int pix_fmt, ia_pixel_format_type type )
{
#ifdef HAVE_FFMPEG
    if( type == IA_PIX_FMT_FFMPEG ) {
        return pix_fmt;
    }
#endif

#ifdef HAVE_V4L2
    if( type == IA_PIX_FMT_V4L2 ) {
        switch( pix_fmt ) {
            case V4L2_PIX_FMT_RGB332:   return IA_PIX_FMT_RGB32;
            case V4L2_PIX_FMT_RGB444:   return IA_PIX_FMT_NONE;
            case V4L2_PIX_FMT_RGB555:   return IA_PIX_FMT_RGB555;
            case V4L2_PIX_FMT_RGB565:   return IA_PIX_FMT_RGB565;
            case V4L2_PIX_FMT_RGB555X:  return IA_PIX_FMT_NONE;
            case V4L2_PIX_FMT_RGB565X:  return IA_PIX_FMT_BGR565;
            case V4L2_PIX_FMT_BGR24:    return IA_PIX_FMT_BGR24;
            case V4L2_PIX_FMT_RGB24:    return IA_PIX_FMT_RGB24;
            case V4L2_PIX_FMT_BGR32:    return IA_PIX_FMT_BGR32;
            case V4L2_PIX_FMT_RGB32:    return IA_PIX_FMT_RGB32;
            case V4L2_PIX_FMT_GREY:     return IA_PIX_FMT_GRAY8;
            case V4L2_PIX_FMT_Y16:      return IA_PIX_FMT_GRAY16LE;
            case V4L2_PIX_FMT_PAL8:     return IA_PIX_FMT_PAL8;
            case V4L2_PIX_FMT_YVU410:   return IA_PIX_FMT_YUV410P;
            case V4L2_PIX_FMT_YVU420:   return IA_PIX_FMT_YUV420P;
            case V4L2_PIX_FMT_YUYV:     return IA_PIX_FMT_YUYV422;
            case V4L2_PIX_FMT_UYVY:     return IA_PIX_FMT_UYVY422;
            case V4L2_PIX_FMT_YUV422P:  return IA_PIX_FMT_YUV422P;
            case V4L2_PIX_FMT_YUV411P:  return IA_PIX_FMT_YUV411P;
            case V4L2_PIX_FMT_Y41P:     return IA_PIX_FMT_UYYVYY411;
            case V4L2_PIX_FMT_YUV444:   return IA_PIX_FMT_YUV444P;
            case V4L2_PIX_FMT_YUV555:   return IA_PIX_FMT_NONE;
            case V4L2_PIX_FMT_YUV565:   return IA_PIX_FMT_NONE;
            case V4L2_PIX_FMT_YUV32:    return IA_PIX_FMT_NONE;
            case V4L2_PIX_FMT_NV12:     return IA_PIX_FMT_NV12;
            case V4L2_PIX_FMT_NV21:     return IA_PIX_FMT_NV21;
            case V4L2_PIX_FMT_YUV410:   return IA_PIX_FMT_YUV410P;
            case V4L2_PIX_FMT_YUV420:   return IA_PIX_FMT_YUV420P;
            case V4L2_PIX_FMT_YYUV:     return IA_PIX_FMT_UYVY422;
            case V4L2_PIX_FMT_HI240:    return IA_PIX_FMT_NONE;
            case V4L2_PIX_FMT_HM12:     return IA_PIX_FMT_NONE;
            case V4L2_PIX_FMT_SBGGR8:   return IA_PIX_FMT_NONE;
            case V4L2_PIX_FMT_SBGGR16:  return IA_PIX_FMT_NONE;
            case V4L2_PIX_FMT_MJPEG:    return IA_PIX_FMT_NONE;
            case V4L2_PIX_FMT_JPEG:     return IA_PIX_FMT_YUVJ444P;
            case V4L2_PIX_FMT_DV:       return IA_PIX_FMT_NONE;
            case V4L2_PIX_FMT_MPEG:     return IA_PIX_FMT_VDPAU_MPEG1;
            case V4L2_PIX_FMT_WNVA:     return IA_PIX_FMT_NONE;
            case V4L2_PIX_FMT_SN9C10X:  return IA_PIX_FMT_NONE;
            case V4L2_PIX_FMT_PWC1:     return IA_PIX_FMT_NONE;
            case V4L2_PIX_FMT_PWC2:     return IA_PIX_FMT_NONE;
            case V4L2_PIX_FMT_ET61X251: return IA_PIX_FMT_NONE;
            default:                    return IA_PIX_FMT_NONE;
        }
    }
#endif

    return IA_PIX_FMT_NONE;
}

/* you can use this to copy a uint8 array to uint16 with the efficiency of memcpy */
static inline void* ia_memcpy_uint8_to_pixel( void* d, const void* s, size_t n )
{
    if( d < s )
    {
        const uint8_t* firsts = (const uint8_t*) s;
        ia_pixel_t* firstd = (ia_pixel_t*) d;
        while( n-- )
        {
/* this doesnt do much :/
            asm( "xor %%ax,%%ax; mov %1, %%al; mov %%ax, %0;"
                :"=r"(*firstd++)
                :"r"(*firsts++)
                :"%ax"
               );
*/
            *firstd++ = *firsts++;
        }
    }
    else
    {
        const uint8_t* lasts = (const uint8_t*) s + (n-1);
        ia_pixel_t* lastd = (ia_pixel_t*) d + (n-1);
        while( n--)
        {
// this doesnt do much :/
//            asm( "xor %%ax,%%ax; mov %1, %%al; mov %%ax, %0;"
//                :"=r"(*lastd--)
//                :"r"(*lasts--)
//                :"%ax"
//               );
//
            *lastd-- = *lasts--;
        }
    }
    return d;
}

static inline void* ia_memcpy_pixel_to_uint8( void* d, const void* s, size_t n )
{
    if( d < s )
    {
        const ia_pixel_t* firsts = (const ia_pixel_t*) s;
        uint8_t* firstd = (uint8_t*) d;
        while( n-- )
            *firstd++ = *firsts++;
    }
    else
    {
        const ia_pixel_t* lasts = (const ia_pixel_t*) s + (n-1);
        uint8_t* lastd = (uint8_t*) d + (n-1);
        while( n--)
            *lastd-- = *lasts--;
    }
    return d;
}

static inline void* ia_memcpy_pixel( void* d, const void* s, size_t n )
{
    if( d < s )
    {
        const ia_pixel_t* firsts = (const ia_pixel_t*) s;
        ia_pixel_t* firstd = (ia_pixel_t*) d;
        while( n-- )
            *firstd++ = *firsts++;
    }
    else
    {
        const ia_pixel_t* lasts = (const ia_pixel_t*) s + (n-1);
        ia_pixel_t* lastd = (ia_pixel_t*) d + (n-1);
        while( n--)
            *lastd-- = *lasts--;
    }
    return d;
}

#define ia_select select
#define ia_memset memset
#define ia_memcpy memcpy
#define ia_fgets fgets
#define ia_strtok strtok
#define ia_strncpy strncpy
#define ia_strrchr strrchr
#define ia_malloc malloc
#define ia_calloc calloc
#define ia_free free
#define ia_fopen fopen
#define ia_fclose fclose
#define ia_abs fabs

static inline int clip_uint8( int v )
{
    return ( (v < 0) ? 0 : (v > 255) ? 255 : v );
}

#ifdef HAVE_LIBPTHREAD
#define ia_pthread_t                pthread_t
#define ia_pthread_create           pthread_create
#define ia_pthread_join             pthread_join
#define ia_pthread_mutex_t          pthread_mutex_t
#define ia_pthread_mutex_init       pthread_mutex_init
#define ia_pthread_mutex_destroy    pthread_mutex_destroy
#define ia_pthread_mutex_lock       pthread_mutex_lock
#define ia_pthread_mutex_unlock     pthread_mutex_unlock
#define ia_pthread_cond_t           pthread_cond_t
#define ia_pthread_cond_init        pthread_cond_init
#define ia_pthread_cond_destroy     pthread_cond_destroy
#define ia_pthread_cond_broadcast   pthread_cond_broadcast
#define ia_pthread_cond_signal      pthread_cond_signal
#define ia_pthread_cond_wait        pthread_cond_wait
#define ia_pthread_exit             pthread_exit
#endif

static inline void ia_pthread_error( int rc, char* a, char* b )
{
    if( rc ) {
        fprintf( stderr, "%s: the call to %s returned error code %d\n", a, b, rc );
        //exit( 1 );
        ia_pthread_exit( NULL );
    }
}

ia_image_t* ia_image_create( size_t width, size_t height );
void ia_image_free( ia_image_t* iaf );

#define ia_error(format, ...) { if(debug) fprintf(stderr,format, ## __VA_ARGS__); }

#endif
