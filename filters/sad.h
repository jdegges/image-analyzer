#ifndef _H_SAD
#define _H_SAD

static inline void sad( ia_seq_t* s )
{
    int i, j, h, k;
    const double op = 1.0 / (s->param->i_mb_size*s->param->i_mb_size);

    if( s->i_nrefs < 1 )
    {
        ia_memset( s->iar->pix,0,sizeof(ia_pixel_t)*s->param->i_size*3 );
        return;
    }

    for( i = 0; i < s->param->i_height; i++ )
    {
        for( j = 0; j < s->param->i_width; j++ )
        {
            const int cr = offset(s->param->i_width,j,i,0);
            const int cg = cr + 1;
            const int cb = cg + 1;

            s->iar->pix[cr] =
            s->iar->pix[cg] =
            s->iar->pix[cb] = 0;

            if( i <= s->param->i_mb_size/2 || j <= s->param->i_mb_size/2
                || i >= s->param->i_height - s->param->i_mb_size/2
                || j >= s->param->i_width  - s->param->i_mb_size/2 )
            {
                continue;
            }

            for( h = i - s->param->i_mb_size/2; h <= i + s->param->i_mb_size/2; h++ )
            {
                for( k = j - s->param->i_mb_size/2; k <= j + s->param->i_mb_size/2; k++ )
                {
                    const int lr = offset( s->param->i_width,k,h,0 );
                    const int lg = lr + 1;
                    const int lb = lg + 1;

                    s->iar->pix[cr] += fabs(s->iaf->pix[lr] - s->ref[0]->pix[lr]);
                    s->iar->pix[cg] += fabs(s->iaf->pix[lg] - s->ref[0]->pix[lg]);
                    s->iar->pix[cb] += fabs(s->iaf->pix[lb] - s->ref[0]->pix[lb]);
                }
            }

            s->iar->pix[cr] *= op; //clip_uint8( s->iar->pix[cr] ); //op;
            s->iar->pix[cg] *= op; //clip_uint8( s->iar->pix[cg] ); //op;
            s->iar->pix[cb] *= op; //clip_uint8( s->iar->pix[cb] ); //op;
        }
    }
}

#endif
