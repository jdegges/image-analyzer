#ifndef _H_FLOW
#define _H_FLOW

static inline void flow( ia_seq_t* s )
{
    int i, j, h, k;
    double lmin, op;
    double dzr, dzg, dzb;

    if( s->i_nrefs < 2 )
    {
        ia_memset( s->iar->pix,0,sizeof(ia_pixel_t)*s->param->i_size*3 );
        return;
    }

    lmin = -255*9;
    op = 255.0 / (255*9*2);  //  = max / (lmax - lmin)

    for ( i = 0; i < s->param->i_height; i++ )
    {
        for ( j = 0; j < s->param->i_width; j++)
        {

            if ( i == 0 || j == 0 || j == s->param->i_width-1 || i == s->param->i_height-1 )
            {
                s->iar->pix[offset(s->param->i_width,j,i,0)] =
                s->iar->pix[offset(s->param->i_width,j,i,1)] =
                s->iar->pix[offset(s->param->i_width,j,i,2)] = 0;
                continue;
            }

            dzr = dzg = dzb = 0;
            for( h = i-1; h < i+1; h++ )
            {
                for( k = j-1; k < j+1; k++ )
                {
                    dzr += (s->iaf->pix[offset(s->param->i_width,k,h,0)]
                         - s->ref[1]->pix[offset(s->param->i_width,k,h,0)]);
                    dzg += (s->iaf->pix[offset(s->param->i_width,k,h,1)]
                         - s->ref[1]->pix[offset(s->param->i_width,k,h,1)]);
                    dzb += (s->iaf->pix[offset(s->param->i_width,k,h,2)]
                         - s->ref[1]->pix[offset(s->param->i_width,k,h,2)]);
                }
            }

            s->iar->pix[offset(s->param->i_width,j,i,0)] = ( dzr - lmin ) * op;
            s->iar->pix[offset(s->param->i_width,j,i,1)] = ( dzg - lmin ) * op;
            s->iar->pix[offset(s->param->i_width,j,i,2)] = ( dzb - lmin ) * op;
        }
    }
}

#endif
