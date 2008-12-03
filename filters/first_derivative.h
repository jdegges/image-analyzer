#ifndef _H_FIRST_DERIVATIVE
#define _H_FIRST_DERIVATIVE

void fstderiv( ia_seq_t* s, ia_image_t** iaim, ia_image_t* iar )
{
    int i, j;
    double dxr, dxg, dxb;
    double dyr, dyg, dyb;
    const double op = 255/sqrt(pow(255*3,2)*2); //  = max / (lmax - lmin)
    ia_image_t* iaf = iaim[0];

    for(i = 0; i < s->param->i_height; i++)
    {
        for(j = 0; j < s->param->i_width; j++)
        {
            if( i == 0 || j == 0 || i == s->param->i_height-1 || j == s->param->i_width-1 )
            {
                iar->pix[offset(s->param->i_width,j,i,0)] = 0;
                iar->pix[offset(s->param->i_width,j,i,1)] = 0;
                iar->pix[offset(s->param->i_width,j,i,2)] = 0;
                continue;
            }

            dxr = dxg = dxb =
            dyr = dyg = dyb = 0;

            // left
            dxr -= iaf->pix[offset(s->param->i_width,j-1,i-1,0)];
            dxg -= iaf->pix[offset(s->param->i_width,j-1,i-1,1)];
            dxb -= iaf->pix[offset(s->param->i_width,j-1,i-1,2)];

            dxr -= iaf->pix[offset(s->param->i_width,j-1,i+0,0)];
            dxg -= iaf->pix[offset(s->param->i_width,j-1,i+0,1)];
            dxb -= iaf->pix[offset(s->param->i_width,j-1,i+0,2)];

            dxr -= iaf->pix[offset(s->param->i_width,j-1,i+1,0)];
            dxg -= iaf->pix[offset(s->param->i_width,j-1,i+1,1)];
            dxb -= iaf->pix[offset(s->param->i_width,j-1,i+1,2)];

            // center


            // right
            dxr += iaf->pix[offset(s->param->i_width,j+1,i-1,0)];
            dxg += iaf->pix[offset(s->param->i_width,j+1,i-1,1)];
            dxb += iaf->pix[offset(s->param->i_width,j+1,i-1,2)];

            dxr += iaf->pix[offset(s->param->i_width,j+1,i+0,0)];
            dxg += iaf->pix[offset(s->param->i_width,j+1,i+0,1)];
            dxb += iaf->pix[offset(s->param->i_width,j+1,i+0,2)];

            dxr += iaf->pix[offset(s->param->i_width,j+1,i+1,0)];
            dxg += iaf->pix[offset(s->param->i_width,j+1,i+1,1)];
            dxb += iaf->pix[offset(s->param->i_width,j+1,i+1,2)];

            // top
            dyr += iaf->pix[offset(s->param->i_width,j-1,i-1,0)];
            dyg += iaf->pix[offset(s->param->i_width,j-1,i-1,1)];
            dyb += iaf->pix[offset(s->param->i_width,j-1,i-1,2)];

            dyr += iaf->pix[offset(s->param->i_width,j+0,i-1,0)];
            dyg += iaf->pix[offset(s->param->i_width,j+0,i-1,1)];
            dyb += iaf->pix[offset(s->param->i_width,j+0,i-1,2)];

            dyr += iaf->pix[offset(s->param->i_width,j+1,i-1,0)];
            dyg += iaf->pix[offset(s->param->i_width,j+1,i-1,1)];
            dyb += iaf->pix[offset(s->param->i_width,j+1,i-1,2)];

            // middle

            // bottom
            dyr -= iaf->pix[offset(s->param->i_width,j-1,i+1,0)];
            dyg -= iaf->pix[offset(s->param->i_width,j-1,i+1,1)];
            dyb -= iaf->pix[offset(s->param->i_width,j-1,i+1,2)];

            dyr -= iaf->pix[offset(s->param->i_width,j+0,i+1,0)];
            dyg -= iaf->pix[offset(s->param->i_width,j+0,i+1,1)];
            dyb -= iaf->pix[offset(s->param->i_width,j+0,i+1,2)];

            dyr -= iaf->pix[offset(s->param->i_width,j+1,i+1,0)];
            dyg -= iaf->pix[offset(s->param->i_width,j+1,i+1,1)];
            dyb -= iaf->pix[offset(s->param->i_width,j+1,i+1,2)];

            iar->pix[offset(s->param->i_width,j,i,0)] = sqrt(pow(dxr,2)+pow(dyr,2)) * op;
            iar->pix[offset(s->param->i_width,j,i,1)] = sqrt(pow(dxg,2)+pow(dyg,2)) * op;
            iar->pix[offset(s->param->i_width,j,i,2)] = sqrt(pow(dxb,2)+pow(dyb,2)) * op;
        }
    }
}

#endif
