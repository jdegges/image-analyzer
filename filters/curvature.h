#ifndef _H_CURVATURE
#define _H_CURVATURE

static inline void curvature( ia_seq_t* s, ia_image_t** iaim, ia_image_t* iar )
{
    int i, j;
    double kr, kg, kb, op;
    ia_image_t* iaf = iaim[0];

    op = 255.0 / (255 * 8);

    for( i = 0; i < s->param->i_height; i++ )
    {
        for( j = 0; j < s->param->i_width; j++ )
        {
            if( i == 0 || j == 0 || j == s->param->i_width-1 || i == s->param->i_height-1 )
            {
                iar->pix[offset(s->param->i_width,j,i,0)] =
                iar->pix[offset(s->param->i_width,j,i,1)] =
                iar->pix[offset(s->param->i_width,j,i,2)] = 0;
                continue;
            }

            kr = (fabs( iaf->pix[offset(s->param->i_width,j-1,i-1,0)] - iaf->pix[offset(s->param->i_width,j-1,i  ,0)] )
                + fabs( iaf->pix[offset(s->param->i_width,j-1,i  ,0)] - iaf->pix[offset(s->param->i_width,j-1,i+1,0)] )
                + fabs( iaf->pix[offset(s->param->i_width,j-1,i+1,0)] - iaf->pix[offset(s->param->i_width,j  ,i+1,0)] )
                + fabs( iaf->pix[offset(s->param->i_width,j  ,i+1,0)] - iaf->pix[offset(s->param->i_width,j+1,i+1,0)] )
                + fabs( iaf->pix[offset(s->param->i_width,j+1,i+1,0)] - iaf->pix[offset(s->param->i_width,j+1,i  ,0)] )
                + fabs( iaf->pix[offset(s->param->i_width,j+1,i  ,0)] - iaf->pix[offset(s->param->i_width,j+1,i-1,0)] )
                + fabs( iaf->pix[offset(s->param->i_width,j+1,i-1,0)] - iaf->pix[offset(s->param->i_width,j  ,i-1,0)] )
                + fabs( iaf->pix[offset(s->param->i_width,j  ,i-1,0)] - iaf->pix[offset(s->param->i_width,j-1,i-1,0)] )) * op;

            kg = (fabs( iaf->pix[offset(s->param->i_width,j-1,i-1,1)] - iaf->pix[offset(s->param->i_width,j-1,i  ,1)] )
                + fabs( iaf->pix[offset(s->param->i_width,j-1,i  ,1)] - iaf->pix[offset(s->param->i_width,j-1,i+1,1)] )
                + fabs( iaf->pix[offset(s->param->i_width,j-1,i+1,1)] - iaf->pix[offset(s->param->i_width,j  ,i+1,1)] )
                + fabs( iaf->pix[offset(s->param->i_width,j  ,i+1,1)] - iaf->pix[offset(s->param->i_width,j+1,i+1,1)] )
                + fabs( iaf->pix[offset(s->param->i_width,j+1,i+1,1)] - iaf->pix[offset(s->param->i_width,j+1,i  ,1)] )
                + fabs( iaf->pix[offset(s->param->i_width,j+1,i  ,1)] - iaf->pix[offset(s->param->i_width,j+1,i-1,1)] )
                + fabs( iaf->pix[offset(s->param->i_width,j+1,i-1,1)] - iaf->pix[offset(s->param->i_width,j  ,i-1,1)] )
                + fabs( iaf->pix[offset(s->param->i_width,j  ,i-1,1)] - iaf->pix[offset(s->param->i_width,j-1,i-1,1)] )) * op;


            kb = (fabs( iaf->pix[offset(s->param->i_width,j-1,i-1,2)] - iaf->pix[offset(s->param->i_width,j-1,i  ,2)] )
                + fabs( iaf->pix[offset(s->param->i_width,j-1,i  ,2)] - iaf->pix[offset(s->param->i_width,j-1,i+1,2)] )
                + fabs( iaf->pix[offset(s->param->i_width,j-1,i+1,2)] - iaf->pix[offset(s->param->i_width,j  ,i+1,2)] )
                + fabs( iaf->pix[offset(s->param->i_width,j  ,i+1,2)] - iaf->pix[offset(s->param->i_width,j+1,i+1,2)] )
                + fabs( iaf->pix[offset(s->param->i_width,j+1,i+1,2)] - iaf->pix[offset(s->param->i_width,j+1,i  ,2)] )
                + fabs( iaf->pix[offset(s->param->i_width,j+1,i  ,2)] - iaf->pix[offset(s->param->i_width,j+1,i-1,2)] )
                + fabs( iaf->pix[offset(s->param->i_width,j+1,i-1,2)] - iaf->pix[offset(s->param->i_width,j  ,i-1,2)] )
                + fabs( iaf->pix[offset(s->param->i_width,j  ,i-1,2)] - iaf->pix[offset(s->param->i_width,j-1,i-1,2)] )) * op;

            iar->pix[offset(s->param->i_width,j,i,0)] = kr;
            iar->pix[offset(s->param->i_width,j,i,1)] = kg;
            iar->pix[offset(s->param->i_width,j,i,2)] = kb;
        }
    }
}

#endif
