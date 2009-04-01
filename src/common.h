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

#ifndef _H_COMMON
#define _H_COMMON

#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 500
#endif

#include <sys/select.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
//#include <FreeImage.h>
#include <math.h>
#include <pthread.h>

#include "../config.h"
#define ia_pixel_t uint8_t
static const int debug = 0;

/* ia_image_t: image data structure
 * i_frame: position of this frame in image stream
 * pix    : pixel data
 * name   : output name
 * thumbname : thumbnail name
 * users  : number of threads using this image
 * ready  : if input image  -> must be set to read data
 *          if output image -> must be set for system to save data
 * lock   : you must have this lock in order to write to this image
*/
typedef struct ia_image_t
{
    char        name[1024];
    char        thumbname[1024];
    uint64_t    i_frame;
    int32_t     i_refcount;
    ia_pixel_t* pix;
    void*       dib;
    struct ia_image_t* next;
    struct ia_image_t* last;
    bool        eoi;
    pthread_mutex_t mutex;
    pthread_cond_t cond_ro;
    pthread_cond_t cond_rw;
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

static inline void ia_pthread_exit( void* value_ptr )
{
    pthread_exit( value_ptr );
}

static inline int ia_pthread_create (pthread_t *__restrict thread,
                                     __const pthread_attr_t *__restrict attr,
                                     void *(*start_routine) (void *),
                                     void *__restrict arg)
{
    return pthread_create( thread, attr, start_routine, arg );
}

static inline int ia_pthread_join( pthread_t thread, void **value_ptr )
{
    return pthread_join( thread, value_ptr );
}

static inline int ia_pthread_mutex_init( pthread_mutex_t *__restrict mutex,
                                         const pthread_mutexattr_t *__restrict attr)
{
    return pthread_mutex_init( mutex, attr );
}

static inline int ia_pthread_mutex_destroy( pthread_mutex_t *mutex )
{
    return pthread_mutex_destroy( mutex );
}


static inline int ia_pthread_mutex_lock( pthread_mutex_t *mutex )
{
    return pthread_mutex_lock( mutex );
}

static inline int ia_pthread_mutex_trylock( pthread_mutex_t *mutex )
{
    return pthread_mutex_trylock( mutex );
}

static inline int ia_pthread_mutex_unlock( pthread_mutex_t *mutex )
{
    return pthread_mutex_unlock( mutex );
}

static inline int ia_pthread_cond_signal( pthread_cond_t *cond )
{
    return pthread_cond_signal( cond );
}

static inline int ia_pthread_cond_wait( pthread_cond_t *__restrict cond,
                                        pthread_mutex_t *__restrict mutex)
{
    return pthread_cond_wait( cond, mutex );
}

static inline int ia_pthread_cond_init( pthread_cond_t *__restrict cond,
                                        const pthread_condattr_t *__restrict attr)
{
    return pthread_cond_init( cond, attr );
}

static inline int ia_pthread_cond_destroy( pthread_cond_t *cond)
{
    return pthread_cond_destroy( cond );
}

static inline void ia_pthread_error( int rc, char* a, char* b )
{
    if( rc ) {
        fprintf( stderr, "%s: the call to %s returned error code %d\n", a, b, rc );
        //exit( 1 );
        ia_pthread_exit( NULL );
    }
}

ia_image_t* ia_image_create( size_t width, size_t height );
void ia_image_free( ia_image_t* iaf );

#define ia_error(format, ...) { if(debug) fprintf(stderr,format, ## __VA_ARGS__); }

#endif
