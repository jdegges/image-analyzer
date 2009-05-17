#ifndef _H_FFMPEG
#define _H_FFMPEG

#include "common.h"

#ifdef HAVE_FFMPEG
#include <libavutil/avutil.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>

typedef struct ia_ffmpeg_t {
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
} ia_ffmpeg_t;

ia_ffmpeg_t* ia_ffmpeg_init( const char* file );
int ia_ffmpeg_read_frame( ia_ffmpeg_t* ffio, ia_image_t* iaf );
void ia_ffmpeg_close (ia_ffmpeg_t* ffio);
#endif

#endif
