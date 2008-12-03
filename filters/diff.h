#ifndef _H_DIFF
#define _H_DIFF

void diff( ia_seq_t* s )
{
    int i;

    if( s->i_nrefs == 0 )
    {
        ia_memset( s->iar->pix,0,sizeof(ia_pixel_t)*s->param->i_size*3 );
        return;
    }
    
    for(i = 0; i < s->param->i_size*3; i++)
        s->iar->pix[i] = fabs( s->iaf->pix[i] - s->ref[0]->pix[i] );
}

#endif
