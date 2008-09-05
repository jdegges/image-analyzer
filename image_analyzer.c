#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
//#include <opencv/cv.h>
//#include <opencv/highgui.h>
#include <math.h>
#include <FreeImage.h>
#include <getopt.h>
#include <string.h>

#include "common.h"
#include "analyze.h"
#include "ia_sequence.h"

int parse_args ( ia_param_t* p,int argc,char** argv );

#define O( x,y ) ((WIDTH)*(y)+(x))

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
	char* fltr;

    memset( p->input_file,0,sizeof(char)*1024 );
    memset( p->output_directory,0,sizeof(char)*1024 );
    memset( p->filter,0,sizeof(int)*15 );
    strncpy( p->video_device,"/dev/video0",1024 );
    strncpy( p->ext,"bmp",16 );

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

	for ( ;; )
	{
		int i, option_index = 0;
		static struct option long_options[] = {
			{"input"        ,1,0,0},
			{"output"       ,1,0,0},
			{"filter"       ,1,0,0},
			{"block-size"   ,1,0,0},
			{"pretty"       ,0,0,0},
			{"stats"        ,0,0,0},
			{"help"         ,0,0,0},
            {"width"        ,1,0,0},
            {"height"       ,1,0,0},
            {"verbose"      ,0,0,0},
            {"video-device" ,1,0,0},
            {"refs"         ,1,0,0},
            {"ext"          ,1,0,0},
			{0              ,0,0,0}
		};
		char filters[][15] = {
			{"sad"},
			{"deriv"},
			{"flow"},
			{"curv"},
			{"ssd"},
			{"me"},
			{"blobs"},
            {"diff"},
            {"bhatta"},
            {"copy"},
            {"mbox"}
		};

		c = getopt_long ( argc,argv,"i:o:f:b:pshc:r:vd:",long_options,&option_index );
		if ( c == -1 )
			break;

		if ( (option_index == 0 && c == 0) || (option_index == 0 && c == 'i') )
		{
            strncpy( p->input_file,optarg,1024 );
            p->b_vdev = 0;
		}
		else if ( (option_index == 1 && c == 0) || (option_index == 0 && c == 'o') )
		{
			strncpy ( p->output_directory,optarg,1024 );
		}
		else if ( (option_index == 2 && c == 0 ) || (option_index == 0 && c == 'f') )
		{
			fltr = NULL;
			fltr = strtok ( optarg,"," );
			for ( c = 0; c < 10 && fltr != NULL; c++ )
			{
				for ( i = 0; i < 11; i++ )
				{
					if ( strcmp(fltr,filters[i]) == 0 )
						break;
				}
				if ( i >= 11 )
				{
					fprintf ( stderr,"Unknown filter %s\n",fltr );
					usage ();
					return 1;
				}
				p->filter[c] = i+1;
				fltr = strtok ( NULL,"," );
			}
		}
		else if ( (option_index == 3 && c == 0) || (option_index == 0 && c == 'b') )
		{
		}
		else if ( (option_index == 4 && c == 0) || (option_index == 0 && c == 'p') )
		{
		}
		else if ( (option_index == 5 && c == 0) || (option_index == 0 && c == 's') )
		{
		}
		else if ( (option_index == 6 && c == 0) || (option_index == 0 && c == 'h') )
		{
			usage ();
			return 1;
		}
        else if ( (option_index == 7 && c == 0) || (option_index == 0 && c == 'c') )
        {
            p->i_width = strtoul( optarg,NULL,10 );
        }
        else if ( (option_index == 8 && c == 0) || (option_index == 0 && c == 'r') )
        {
            p->i_height = strtoul( optarg,NULL,10 );
        }
        else if ( (option_index == 9 && c == 0) || (option_index == 0 && c == 'v') )
        {
            p->b_verbose = 1;
        }
        else if ( (option_index == 10 && c == 0) || (option_index == 0 && c == 'd') )
        {
            strncpy( p->video_device,optarg,1024 );
        }
        else if ( (option_index == 11 && c == 0) || (option_index == 0 && c == 'm') )
        {
            p->i_maxrefs = strtoul( optarg,NULL,10 );
        }
        else if ( (option_index == 12 && c == 0) || (option_index == 0 && c == 'x') )
        {
            strncpy( p->ext,optarg,10 );
        }
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
	printf ( "  -i, --input <string>            List of images to be processed\n" );
	printf ( "  -o, --output <string>           Directory to store output into\n" );
    printf ( "  -d, --video-device <string>     Video device to capture images from [/dev/video0]\n" );
	printf ( "\n" );
	printf ( "  -f, --filter <filter list>      List of filters to be used on sequence\n" );
	printf ( "                                      copy,bhatta,mbox,diff,sad,deriv,flow,curv,ssd,me,blobs\n" );
    printf ( "  -w, --width <int>               Image width, must be specified in video capture mode\n" );
    printf ( "  -h, --height <int>              Image height, must be specified in video capture mode\n" );
    printf ( "  -m, --refs <int>                Maximum number of refs to cache [4]\n" );
    printf ( "\n" );
	printf ( "  -s, --stats                     Calculates and prints statistics\n" );
    printf ( "  -v, --verbose                   Verbose/debug mode will display lots of additional information\n" );
	printf ( "  -h, --help                      Display this help menu\n" );
	printf ( "\n" );
}
