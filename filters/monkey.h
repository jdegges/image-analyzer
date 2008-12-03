#ifndef _H_MONKEY
#define _H_MONKEY

void monkey( ia_seq_t* s )
{
    int i,j;
    ia_pixel_t dev, avg;

    if( s->i_nrefs < s->param->i_maxrefs )
    {
        ia_memset( s->iar->pix,0,sizeof(ia_pixel_t)*s->param->i_size*3 );
        return;
    }

    i = s->param->i_size*3-1;
    while( i-- )
    {
        avg = s->iaf->pix[i];
        j = s->i_nrefs-1;
        while( j-- )
            avg += s->ref[j]->pix[i];

        avg /= ( s->i_nrefs + 1 );

        dev = ia_abs( avg - s->iaf->pix[i] );
        s->iar->pix[i] = s->iaf->pix[i];
        j = s->i_nrefs-1;
        while( j-- )
        {
            if( ia_abs(avg-s->ref[j]->pix[i]) > dev )
            {
                dev = ia_abs(avg-s->ref[j]->pix[i]);
                s->iar->pix[i] = s->ref[j]->pix[i];
            }
        }
    }
}

#endif
