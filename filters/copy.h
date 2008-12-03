#ifndef _H_COPY
#define _H_COPY

static inline void copy( ia_seq_t* s, ia_image_t** iaf, ia_image_t* iar )
{
    ia_memcpy_pixel( iar->pix,iaf[0]->pix,s->param->i_size*3 );
}

#endif
