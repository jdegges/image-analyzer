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

#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
//#include <opencv/cv.h>
//#include <opencv/highgui.h>
#include <math.h>
#include <FreeImage.h>
#include <getopt.h>
#include <string.h>
#include <ctype.h>

#include "common.h"
#include "analyze.h"
#include "filters/filters.h"

int parse_args ( ia_param_t* p,int argc,char** argv );
void usage ( void );

int main ( int argc,char** argv )
{
    FreeImage_Initialise(0);
    ia_param_t param;

	if ( parse_args(&param,argc,argv) != 0 )
    {
		return 1;
    }
    if ( analyze(&param) )
    {
        fprintf( stderr,"analyze error\n" );
		return 1;
    }
/*
    if ( !print_statistics() )
        return 1;
    if ( !free_memory() )
        return 1;
*/
    FreeImage_DeInitialise();
	return 0;
}

/*
 * parse_args return value:
 * 1 - failure
 * 0 - success
*/
int parse_args ( ia_param_t* p,int argc,char** argv )
{
	int c;

    memset( p->input_file,0,sizeof(char)*1031 );
    memset( p->output_directory,0,sizeof(char)*1031 );
    memset( p->filter,0,sizeof(int)*15 );
    strncpy( p->video_device,"/dev/video0",1031 );
    strncpy( p->ext,"bmp",16 );

    p->b_thumbnail = 0;
    p->i_duration = 0;
    p->i_spf = 0;
    p->stream = 0;
    p->i_mb_size = 15;
    p->i_maxrefs = 4;
    p->i_size = 0;
    p->b_verbose = 0;
    p->Settings.BgStartFrame = 0;
    p->Settings.StartFrame = 0;
    p->Settings.NumBgFrames = 20;
    p->Settings.NumFrames = 100;
    p->Settings.bgCount = 0;
    p->Settings.BgStep = 1;
    p->Settings.Step = 1;
    p->Settings.Threshold = 128;
    p->Settings.nChannels = 3;
    p->Settings.useHSV = 0;

    p->BhattaSettings.NumBins1 = 16;
    p->BhattaSettings.NumBins2 = 2;
    p->BhattaSettings.NumBins3 = 2;
    p->BhattaSettings.SizePatch = 30;
    p->BhattaSettings.Alpha = 0;

    p->BhattaOptSettings.useBgUpdateMask = 0;
    p->i_width = 0;
    p->i_height = 0;
    p->b_vdev = 1;
    p->display = 0;
    p->i_threads = 1;
    p->i_vframes = 0;

	for ( ;; )
	{
		int option_index = 0;
		static struct option long_options[] = {
			{"input"        ,1,0,0},
			{"output"       ,1,0,0},
			{"filter"       ,1,0,0},
			{"mb-size"      ,1,0,0},
			{"display"      ,0,0,0},
			{"stream"       ,0,0,0},
			{"help"         ,0,0,0},
            {"width"        ,1,0,0},
            {"height"       ,1,0,0},
            {"verbose"      ,0,0,0},
            {"video-device" ,1,0,0},
            {"refs"         ,1,0,0},
            {"ext"          ,1,0,0},
            {"threads"      ,1,0,0},
            {"vframes"      ,1,0,0},
            {"thumbnail"    ,0,0,0},
            {"duration"     ,1,0,0},
            {"spf"          ,1,0,0},
			{0              ,0,0,0}
		};

        char formats[][25] = {
            {"bmp"},
            {"exr"},
            {"gif"},
            {"hdr"},
            {"ico"},
            {"jpg"},
            {"jpeg"},
            {"jif"},
            {"jp2"},
            {"jpx"},
            {"jpc"},
            {"j2k"},
            {"jp2"},
            {"pbm"},
            {"pgm"},
            {"png"},
            {"ppm"},
            {"targa"},
            {"tiff"},
            {"wbmp"},
            {"xpm"},
            {0}
        };

		c = getopt_long ( argc,argv,"i:o:f:b:psw:h:c:r:vd:tj:l:u:",long_options,&option_index );
		if ( c == -1 )
			break;

		if( (option_index == 0 && c == 0) || (option_index == 0 && c == 'i') )
		{
            strncpy( p->input_file, optarg, 1031 );
            p->b_vdev = 0;
		}
		else if( (option_index == 1 && c == 0) || (option_index == 0 && c == 'o') )
			strncpy ( p->output_directory, optarg, 1031 );
		else if( (option_index == 2 && c == 0 ) || (option_index == 0 && c == 'f') )
		{
			char* fltr = NULL;
			fltr = strtok( optarg,"," );
			for( c = 0; c < 15 && fltr != NULL; c++ )
			{
                int i;
                for( i = 0; *FILTERS[i] != 0; i++ )
                {
                    int pos;
                    for( pos = 0; (fltr[pos] != '\0' && FILTERS[i][pos] != '\0')
                                  && (toupper(fltr[pos])
                                      == toupper(FILTERS[i][pos])); pos++ );
                    if( fltr[pos] == '\0' && FILTERS[i][pos] == '\0' )
                        break;
                }
                if( *FILTERS[i] == 0 )
				{
					fprintf( stderr,"Unknown filter %s\n",fltr );
					usage();
					return 1;
				}
				p->filter[c] = i+1;
				fltr = strtok( NULL, "," );
			}
		}
		else if( (option_index == 3 && c == 0) || (option_index == 0 && c == 'b') )
            p->i_mb_size = strtoul( optarg, NULL, 10 );
		else if( (option_index == 4 && c == 0) || (option_index == 0 && c == 'p') )
        {
#ifndef HAVE_LIBSDL
                printf( "SDL is disabled, recompile with --enable-sdl to use this feature.\n" );
                usage();
                return 1;
#endif
            p->display = 1;
        }
		else if( (option_index == 5 && c == 0) || (option_index == 0 && c == 's') )
            p->stream = 1;
		else if( option_index == 6 && c == 0 )
		{
			usage();
			return 1;
		}
        else if( (option_index == 7 && c == 0) || (option_index == 0 && c == 'w') )
            p->i_width = strtoul( optarg, NULL, 10 );
        else if( (option_index == 8 && c == 0) || (option_index == 0 && c == 'h') )
            p->i_height = strtoul( optarg, NULL, 10 );
        else if( (option_index == 9 && c == 0) || (option_index == 0 && c == 'v') )
            p->b_verbose = 1;
        else if( (option_index == 10 && c == 0) || (option_index == 0 && c == 'd') )
            strncpy( p->video_device, optarg, 1031 );
        else if( (option_index == 11 && c == 0) || (option_index == 0 && c == 'm') )
        {
            p->i_maxrefs = strtoul( optarg, NULL, 10 );
            p->i_maxrefs = p->i_maxrefs ? p->i_maxrefs : 1;
        }
        else if( (option_index == 12 && c == 0) || (option_index == 0 && c == 'x') )
        {
            strncpy( p->ext, optarg, 10 );
            int i;
            for( i = 0; !*formats[i]; i++ )
            {
                char* a = p->ext;
                char* b = formats[i];

                while( (*a != '\0' && *b != '\0')
                        && (toupper(*a++) == toupper(*b++)) );
                if( *a == '\0' && *b == '\0' )
                    break;
            }
            if( !*formats[i] )
            {
                fprintf( stderr,"Unknown format %s\n", p->ext );
                usage();
                return 1;
            }
        }
        else if( (option_index == 13 && c == 0) || (option_index == 0 && c == 'j') )
        {
            p->i_threads = strtoul( optarg, NULL, 10 );
            p->i_threads = p->i_threads ? p->i_threads : 1;
        }
        else if( (option_index == 14 && c == 0) )
            p->i_vframes = strtoul( optarg, NULL, 10 );
        else if( (option_index == 15 && c == 0) || (option_index == 0 && c == 't') )
            p->b_thumbnail = true;
        else if( (option_index == 16 && c == 0) || (option_index == 0 && c == 'l') )
            p->i_duration = strtoul( optarg, NULL, 10 );
        else if( (option_index == 17 && c == 0) || (option_index == 0 && c == 'u') )
            p->i_spf = strtoul( optarg, NULL, 10 );
		else
		{
			fprintf ( stderr,"Unrecognized option -%c\n",c );
			usage ();
			return 1;
		}
	}
	
    if( p->input_file[0] == 0 && (p->i_width == 0 || p->i_height == 0) )
    {
        fprintf( stderr,"You must specify width and height when using video device\n" );
        usage ();
        return 1;
    }
    p->i_size = p->i_width*p->i_height;

	return 0;
}

void usage ( void )
{
    printf ( "\n" );
	printf ( "Syntax: ia --input=list.txt --output=outdir/ --filter=sad,flow\n" );
	printf ( "\n" );
	printf ( "Options:\n" );
	printf ( "  -i, --input <string>            List of images to be processed or a video file (requires ffmpeg)\n" );
	printf ( "  -o, --output <string>           Directory to store output into\n" );
    printf ( "  -d, --video-device <string>     Video device to capture images from [/dev/video0]\n" );
    printf ( "  -x, --ext <string>              Output file name extension [bmp]\n" );
#ifdef HAVE_LIBSDL
    printf ( "  -p, --display                   Display live output\n" );
#endif
    printf ( "  -s, --stream                    Save images to one file, specified by -o\n" );
    printf ( "  -t, --thumbnail                 Create thumbnails [disabled]\n" );
	printf ( "\n" );
	printf ( "  -f, --filter <filter list>      List of filters to be used on sequence:\n" );
	printf ( "                                      copy,bhatta,mbox,diff,sad,deriv,flow,\n" );
    printf ( "                                      curv,ssd,me,blobs,monkey,normal,grayscale,blur\n" );
    printf ( "  -w, --width <int>               Image width, must be specified in video capture mode\n" );
    printf ( "  -h, --height <int>              Image height, must be specified in video capture mode\n" );
    printf ( "  -m, --refs <int>                Maximum number of refs to cache [4]\n" );
    printf ( "  -b, --mb-size <int>             Macroblock size to use in filters that use macroblocks [15]\n" );
    printf ( "\n" );
    printf ( "  --vframes <int>                 The number of frames to process\n" );
    printf ( "  -j, --threads <int>             Parallel processing\n" );
    printf ( "  -l, --duration <int>            How long to record for, measured in seconds [0]\n" );
    printf ( "  -u, --spf <int>                 Length of time it takes to record one frame, measured in seconds [0]\n" );
    printf ( "  -v, --verbose                   Verbose/debug mode will display lots of additional information\n" );
	printf ( "  --help                          Display this help menu\n" );
	printf ( "\n" );
}
