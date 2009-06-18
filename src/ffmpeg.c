#include "ffmpeg.h"

#ifdef HAVE_FFMPEG

ia_ffmpeg_t* ia_ffmpeg_init( const char* input )
{
    AVFormatContext *pFormatCtx;
    int i, videoStream;
    AVCodecContext *pCodecCtx;
    AVCodec *pCodec;
    AVFrame *pFrame;
    AVFrame *pFrameRGB;
    int numBytes;
    uint8_t *buffer;
    struct SwsContext *img_convert_ctx;
    ia_ffmpeg_t *ffio;

    // Register all formats and codecs
    av_register_all();

    // Open video file
    if(av_open_input_file(&pFormatCtx, input, NULL, 0, NULL)!=0) {
        fprintf(stderr, "Couldn't open input file: %s\n", input);
        return NULL;
    }
    
    // Retrieve stream information
    if(av_find_stream_info(pFormatCtx)<0) {
        fprintf(stderr, "Couldn't find stream info\n");
        return NULL;
    }

    // Dump information about file onto standard error
    //dump_format(pFormatCtx, 0, input, 0);

    // Find the first video stream
    videoStream=-1;
    for(i=0; i<pFormatCtx->nb_streams; i++)
        if(pFormatCtx->streams[i]->codec->codec_type==CODEC_TYPE_VIDEO) {
            videoStream=i;
            break;
        }
    if(videoStream==-1) {
        fprintf(stderr, "Didn't find a video stream\n");
        return NULL;
    }

    // Get a pointer to the codec context for the video stream
    pCodecCtx=pFormatCtx->streams[videoStream]->codec;
    
    // Find the decoder for the video stream
    pCodec=avcodec_find_decoder(pCodecCtx->codec_id);
    if(pCodec==NULL) {
        fprintf(stderr, "Unsupported codec!\n");
        return NULL;
    }
    // Open codec
    if(avcodec_open(pCodecCtx, pCodec)<0) {
        fprintf(stderr, "Could not open codec\n");
        return NULL;
    }

    // Allocate video frame
    pFrame = avcodec_alloc_frame();

    // Allocate an AVFrame structure
    pFrameRGB=avcodec_alloc_frame();
    if(pFrameRGB==NULL) {
        fprintf(stderr, "Couldn't alloc an avcodec frame\n");
        return NULL;
    }

    // Determine required buffer size and allocate buffer
    numBytes=avpicture_get_size(PIX_FMT_BGR24, pCodecCtx->width,
                                pCodecCtx->height);
    buffer=(uint8_t *)av_malloc(numBytes*sizeof(uint8_t));

    // Assign appropriate parts of buffer to image planes in pFrameRGB
    // Note that pFrameRGB is an AVFrame, but AVFrame is a superset
    // of AVPicture
    avpicture_fill((AVPicture *)pFrameRGB, buffer, PIX_FMT_BGR24,
                   pCodecCtx->width, pCodecCtx->height);

    img_convert_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height,
                                     pFormatCtx->streams[videoStream]->codec->pix_fmt,
                                     pCodecCtx->width, pCodecCtx->height,
                                     PIX_FMT_BGR24, SWS_BICUBIC,
                                     NULL, NULL, NULL);
    if(img_convert_ctx == NULL) {
        fprintf(stderr, "Cannot initialize the conversion context!\n");
        return NULL;
    }

    ffio = malloc (sizeof(ia_ffmpeg_t));
    if(!ffio) {
        fprintf(stderr, "Couldn't alloc an ffio object\n");
        return NULL;
    }

    ffio->pFormatCtx = pFormatCtx;
    ffio->videoStream = videoStream;
    ffio->pCodecCtx = pCodecCtx;
    ffio->pCodec = pCodec;
    ffio->pFrame = pFrame;
    ffio->pFrameRGB = pFrameRGB;
    ffio->numBytes = numBytes;
    ffio->buffer = buffer;
    ffio->img_convert_ctx = img_convert_ctx;
    ffio->i_width = pCodecCtx->width;
    ffio->i_height = pCodecCtx->height;
    ffio->i_size = pCodecCtx->width * pCodecCtx->height;

    return ffio;
}

void SaveFrame(AVFrame *pFrame, int width, int height, ia_image_t* iaf) {
    int y;
    for(y=0; y<height; y++)
        memcpy(iaf->pix+y*pFrame->linesize[0], pFrame->data[0]+(height-y-1)*pFrame->linesize[0], width*3);
}

int ia_ffmpeg_read_frame( ia_ffmpeg_t* ffio, ia_image_t* iaf )
{
    AVPacket        packet;
    int             frameFinished;

    // Read frames and save to disk
    while(av_read_frame(ffio->pFormatCtx, &packet)>=0) {
        // Is this a packet from the video stream?
        if(packet.stream_index==ffio->videoStream) {
            // Decode video frame
#if LIBAVCODEC_VERSION_MAJOR <= 52 &&   \
    LIBAVCODEC_VERSION_MINOR <= 30 &&   \
    LIBAVCODEC_VERSION_MICRO <  2
                avcodec_decode_video(ffio->pCodecCtx, ffio->pFrame, &frameFinished,
                                     packet.data, packet.size);
#else
                avcodec_decode_video2(ffio->pCodecCtx, ffio->pFrame, &frameFinished,
                                      &packet);
#endif
            // Did we get a video frame?
            if(frameFinished) {
                // Convert the image from its native format to RGB
                sws_scale(ffio->img_convert_ctx, ffio->pFrame->data, ffio->pFrame->linesize,
                          0, ffio->pCodecCtx->height, ((AVPicture *)ffio->pFrameRGB)->data, ((AVPicture *)ffio->pFrameRGB)->linesize);

                // Save the frame
                SaveFrame(ffio->pFrameRGB, ffio->pCodecCtx->width,
                          ffio->pCodecCtx->height, iaf);
                break;
            }
        }
        // Free the packet that was allocated by av_read_frame
        av_free_packet(&packet);
    }
    av_free_packet(&packet);
    return 0;
}

void ia_ffmpeg_close (ia_ffmpeg_t* ffio)
{
    ia_free( ffio->img_convert_ctx );
    // Free the RGB image
    av_free(ffio->buffer);
    av_free(ffio->pFrameRGB);

    // Free the YUV frame
    av_free(ffio->pFrame);

    // Close the codec
    avcodec_close(ffio->pCodecCtx);

    // Close the video file
    av_close_input_file(ffio->pFormatCtx);

    ia_free( ffio );
}

#endif
