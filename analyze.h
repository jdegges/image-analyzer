#ifndef _H_ANALYZE
#define _H_ANALYZE

#include "image_analyzer.h"

/* FILTER Indexes */
#define SAD     1
#define DERIV   2
#define FLOW    3
#define CURV    4
#define SSD     5
#define ME      6
#define BLOBS   7
#define DIFF    8
#define BHATTA  9
#define COPY    10
#define MBOX    11
#define MONKEY  12


ia_bhatta_t b;

int     analyze( ia_param_t* p );
void*   analyze_exec( void* vptr );

/*
// these functions do not yet jive with ia yet, so dont call them
// the implementations are also commented out in analyze.c
void zoom_in(ia_pixel_t* im, ia_pixel_t* out);
void k(ia_pixel_t* im, ia_pixel_t* out);
int* q(ia_pixel_t* im);
ia_pixel_t *get_gaussian(int ksize);
ia_pixel_t* get_reflectance_image(IplImage* im, ia_pixel_t* gaussian, int ksize);
ia_pixel_t* patch_diff(ia_pixel_t* ima, ia_pixel_t* imb, int ksize);
ia_pixel_t* Iplblur(IplImage* im, ia_pixel_t* gaussian, int ksize);
void dialate(ia_pixel_t* im, int ksize);


// these actually work but you wont ever need to call them outside of analyze.c
// so there is no real reason to have them in this interface
void fstderiv( ia_image_t* iai, ia_image_t* iar );
void diff( ia_image_t* iaa, ia_image_t* iab, ia_image_t* iar );
void bhatta( ia_seq_t* s );
void bhatta_init( void );
*/

#endif
