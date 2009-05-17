#ifndef _H_SWSCALE
#define _H_SWSCALE

#include "common.h"

#ifdef HAVE_LIBSWSCALE
#include <libswscale/swscale.h>

typedef struct SwsContext ia_swscale_t;

ia_swscale_t* ia_swscale_init( int32_t width, int32_t height, int32_t s_fmt);
int ia_swscale( ia_swscale_t* c, ia_image_t* iaf, int32_t width,
                int32_t height );
void ia_swscale_close( ia_swscale_t* c );

#endif
#endif
