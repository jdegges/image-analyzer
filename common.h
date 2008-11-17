#ifndef _H_COMMON
#define _H_COMMON

#include <sys/select.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
//#include <FreeImage.h>
#include <math.h>

#define ia_pixel_t double

/* ia_image_t: image data structure
 * i_frame: position of this frame in image stream
 * pix    : pixel data
 * name   : output name
 * users  : number of threads using this image
 * ready  : if input image  -> must be set to read data
 *          if output image -> must be set for system to save data
 * lock   : you must have this lock in order to write to this image
*/
typedef struct
{
    uint64_t    i_frame;
    ia_pixel_t* pix;
    char        name[1024];
    uint8_t     users;
    bool        ready;
    pthread_mutex_t mutex;
} ia_image_t;

static inline int ia_select( int n, fd_set* r, fd_set* w, fd_set* e, struct timeval* t )
{
    return select( n,r,w,e,t );
}

static inline void* ia_memset( void* s, int c, size_t n )
{
    return memset( s,c,n );
}

static inline void* ia_memcpy( void* d, const void* s, size_t n )
{
    return memcpy( d,s,n );
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


static inline char* ia_fgets( char* s, int size, FILE* stream )
{
    return fgets( s, size, stream );
}

static inline char* ia_strtok( char *str, const char *delim )
{
    return strtok( str, delim );
}

static inline char* ia_strncpy( char* dest, const char* src, size_t n )
{
    return strncpy( dest, src, n );
}

static inline char* ia_strrchr( const char* s, int c )
{
    return strrchr( s, c );
}

static inline void* ia_malloc( size_t size )
{
    return malloc( size );
}

static inline void* ia_calloc( size_t nmemb, size_t size )
{
    return calloc( nmemb, size );
}

static inline void ia_free( void* ptr )
{
    free( ptr );
}

static inline FILE* ia_fopen( const char* path, const char* mode )
{
    return fopen( path,mode );
}

static inline int ia_fclose( FILE* fp )
{
    return fclose( fp );
}

static inline int clip_uint8( int v )
{
    return ( (v < 0) ? 0 : (v > 255) ? 255 : v );
}

static inline ia_pixel_t ia_abs( ia_pixel_t v )
{
    return fabs( v );
}

#endif
