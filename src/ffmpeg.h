#ifndef _H_FFMPEG
#define _H_FFMPEG

#include "common.h"

#ifdef HAVE_FFMPEG
#include <ffmpeg/avcodec.h>
#include <ffmpeg/avformat.h>
#include <ffmpeg/swscale.h>

typedef struct iaio_ffmpeg_t {
    AVFormatContext *pFormatCtx;
    int             videoStream;
    AVCodecContext  *pCodecCtx;
    AVCodec         *pCodec;
    AVFrame         *pFrame;
    AVFrame         *pFrameRGB;
    int             numBytes;
    uint8_t         *buffer;
    struct SwsContext *img_convert_ctx;
    int i_width;
    int i_height;
    int i_size;
} iaio_ffmpeg_t;

iaio_ffmpeg_t* iaio_ffmpeg_init( const char* file );
int iaio_ffmpeg_read_frame( iaio_ffmpeg_t* ffio, ia_image_t* iaf );
void iaio_ffmpeg_close (iaio_ffmpeg_t* ffio);
#endif

#endif
