#ifndef _H_DRAW_BEST_BOX
#define _H_DRAW_BEST_BOX

void draw_best_box( ia_seq_t* s, ia_image_t** iaim, ia_image_t* iar )
{
    int i, j;
    int top, bottom, left, right;
    ia_image_t* iaf = iaim[0];

    top    = left  =  INT_MAX;
    bottom = right = -INT_MAX;

    for( i = 0; i < s->param->i_height; i++ )
    {
        for( j = 0; j < s->param->i_width; j++ )
        {
            if( iaf->pix[O(i,j)] > 128 )
            {
                top    = (   top > i) ? i : top;
                left   = (  left > j) ? j : left;
                bottom = (bottom < i) ? i : bottom;
                right  = ( right < j) ? j : right;
            }

            iar->pix[O(i,j)+0] = 0;
            iar->pix[O(i,j)+1] = 0;
            iar->pix[O(i,j)+2] = 0;
        }
    }

    for( i = top; i <= bottom ; i++ )
    {
        for( j = left; j <= right; j++ )
        {
            iar->pix[O(i,j)+0] = 255;
            iar->pix[O(i,j)+1] = 255;
            iar->pix[O(i,j)+2] = 255;
        }
    }
}

#endif
