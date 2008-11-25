#include <stdio.h>
#include <assert.h>
#include "queue.h"
#include "common.h"

int main ( void )
{
    ia_queue_t* q = ia_queue_open( 10 );
    int i = 10;
    while( i-- )
    {
        ia_image_t* im = malloc( sizeof(ia_image_t) );
        ia_memset( im, 0, sizeof(ia_image_t) );
        im->pix = ia_malloc( sizeof(ia_pixel_t)*1024*768*3 );
        ia_queue_push( q, im );
        assert( im == ia_queue_pop(q) );
        ia_queue_push( q, im );
    }



    return 0;
}
