#ifndef _H_IMAGE_ANALYZER
#define _H_IMAGE_ANALYZER

#include "common.h"

typedef struct
{
    char input_file[1024];
    char output_directory[1024];
    char video_device[1024];
    char ext[16];
    int filter[20];

    int i_maxrefs;  // maximum number of refs to keep in memory
    int i_size;     // width*height
    int i_width;
    int i_height;
    int i_mb_size; 
    int i_threads;
    int b_verbose;
    int b_vdev;     // true if capturing from video device
    int display;
    uint64_t i_vframes;

    /* bgsub code params */
    struct {
        char ImLoc[500];
        char OutLoc[500];

        int BgStartFrame;
        int StartFrame;

        int NumBgFrames; // i_bgframes;
        int NumFrames;

        int bgCount;
        int BgStep;
        int Step;

        double Threshold;

        int nChannels;

        int useHSV;
    } Settings;

    struct {
        int NumBins1;
        int NumBins2;
        int NumBins3;
        int SizePatch;
        double Alpha;               // background update amount 
        int useBgUpdateMask;
    } BhattaSettings;

    struct {
        int NumBins1;
        int NumBins2;
        int NumBins3;
        int MaxSizePatch;
        double Regularizer;
        double Alpha;               // background update amount 
        int useBgUpdateMask;
    } BhattaOptSettings;

} ia_param_t;

typedef struct
{
    unsigned int**** bg_hist;
    unsigned int***  pixel_hist;
} ia_bhatta_t;

#endif
