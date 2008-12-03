#ifndef _H_ZOOM_IN
#define _H_ZOOM_IN

void zoom_in(ia_pixel_t* im, ia_pixel_t* out)
{
    int i, j, height, width;
    height = HEIGHT*2;
    width = WIDTH*2;
    for(i = 0; i < HEIGHT; i++)
    {
        for(j = 0; j < WIDTH; j++)
        {
            out[width*(i*2)+(j*2)] = im[WIDTH*(i)+(j)];
        }
    }
    
    for(i = 0; i < height; i++)
    {
        for(j = 0; j < width; j++)
        {
            if(j%2 != 0 && i%2 != 0)
                out[width*i+j] = (((out[width*(i-1)+(j-1)] + out[width*(i+1)+(j+1)])/2) + ((out[width*(i+1)+(j-1)] + out[width*(i-1)+(j+1)])/2))/2;
            else if(j%2 == 0 && i%2 != 0)
                out[width*i+j] = (out[width*(i-1)+j]+out[width*(i+1)+j])/2;
            else if(i%2 == 0 && j%2 != 0)
                out[width*i+j] = (out[width*i+(j-1)]+out[width*i+(j+1)])/2;
        }
    }
}

#endif
