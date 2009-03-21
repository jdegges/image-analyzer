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

    int32_t i_maxrefs;  // maximum number of refs to keep in memory
    int32_t i_size;     // width*height
    int32_t i_width;
    int32_t i_height;
    int32_t i_mb_size; 
    int32_t i_threads;
    int32_t b_verbose;
    int32_t b_vdev;     // true if capturing from video device
    int32_t display;
    int32_t stream;
    uint64_t i_vframes;
    bool b_thumbnail;

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

#endif
