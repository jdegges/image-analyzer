#ifndef _H_FIRST_DERIVATIVE
#define _H_FIRST_DERIVATIVE

void fstderiv( ia_seq_t* s, ia_image_t** iaim, ia_image_t* iar )
{
    int i = s->param->i_height;
    int j, ci, cj, pix;
    double dx[3]; // {r, g, b}
    double dy[3]; // {r, g, b}
    ia_image_t* iaf = iaim[0];
    //const double op = 255/sqrt(pow(255*3,2)*2); //  = max / (lmax - lmin)

    while( i-- && (j = s->param->i_width) )
    {
        while( j-- )
        {
            if( i == 0 || j == 0 || i == s->param->i_height-1 || j == s->param->i_width-1 )
            {
                memset( &iar->pix[offset(s->param->i_width,j,i,0)], 0, sizeof(ia_pixel_t)*3 );
                continue;
            }

            pix = 3;
            while( pix-- ) {
                dx[pix] =
                dy[pix] = 0;
            }

            // left and right
            for( ci = i-1; ci <= i+1; ci++ ) {
                pix = 3;
                while( pix-- ) {
                    dx[pix] -= iaf->pix[offset(s->param->i_width,j-1,ci,pix)]; // left
                    dx[pix] += iaf->pix[offset(s->param->i_width,j+1,ci,pix)]; // right
                }
            }

            // top and bottom
            for( cj = j-1; cj <= j+1; cj++ ) {
                pix = 3;
                while( pix-- ) {
                    dy[pix] += iaf->pix[offset(s->param->i_width,cj,i-1,pix)];  // top
                    dy[pix] -= iaf->pix[offset(s->param->i_width,cj,i+1,pix)];  // bottom
                }
            }

            // to scale the resultant pixel down to its appropriate value it
            // should be multiplied by op, but visually, it looks better to
            // clip it... this may change in the future.
            pix = 3;
            while( pix-- ) {
                iar->pix[offset(s->param->i_width,j,i,pix)] = clip_uint8( sqrt(pow(dx[pix],2)+pow(dy[pix],2)) );
            }
        }
    }
}

#endif
