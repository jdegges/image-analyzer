#ifndef _H_BHATTA
#define _H_BHATTA

typedef struct
{
    unsigned int**** bg_hist;
    unsigned int***  pixel_hist;
} ia_bhatta_t;

ia_bhatta_t b;

void bhatta_init ( ia_seq_t* s )
{
    int i, cb, cg;

    // initialize background model histogram
    b.bg_hist = (unsigned int****) ia_malloc ( sizeof(unsigned int***)*s->param->i_size );
    for( i = 0; i < s->param->i_size; i++ )
    {
        b.bg_hist[i] = (unsigned int***) ia_malloc ( sizeof(unsigned int**)*s->param->BhattaSettings.NumBins1 );
        for( cb = 0; cb < s->param->BhattaSettings.NumBins1; cb++ )
        {
            b.bg_hist[i][cb] = (unsigned int**) ia_malloc ( sizeof(unsigned int*)*s->param->BhattaSettings.NumBins2 );
            for( cg = 0; cg < s->param->BhattaSettings.NumBins2; cg++ )
            {
                b.bg_hist[i][cb][cg] = (unsigned int*) ia_malloc ( sizeof(unsigned int)*s->param->BhattaSettings.NumBins3 );
                ia_memset( b.bg_hist[i][cb][cg],0,sizeof(unsigned int)*s->param->BhattaSettings.NumBins3 );
            }
        }
    }

    b.pixel_hist = (unsigned int***) ia_malloc ( sizeof(unsigned int**)*s->param->BhattaSettings.NumBins1 );
    for( cb = 0; cb < s->param->BhattaSettings.NumBins1; cb++ )
    {
        b.pixel_hist[cb] = (unsigned int**) ia_malloc ( sizeof(unsigned int*)*s->param->BhattaSettings.NumBins2 );
        for( cg = 0; cg < s->param->BhattaSettings.NumBins2; cg++ )
        {
            b.pixel_hist[cb][cg] = (unsigned int*) ia_malloc ( sizeof(unsigned int)*s->param->BhattaSettings.NumBins3 );
            ia_memset( b.pixel_hist[cb][cg],0,sizeof(unsigned int)*s->param->BhattaSettings.NumBins3 );
        }
    }

    s->mask = (uint8_t*) ia_malloc( sizeof(uint8_t)*s->param->i_size );
    s->diffImage = (uint8_t*) ia_malloc( sizeof(uint8_t)*s->param->i_size*3 );
}

void bhatta_close ( ia_seq_t* s )
{
    int i, cb, cg;

    ia_free( s->mask );
    ia_free( s->diffImage );

    for( i = 0; i < s->param->i_size; i++ )
    {
        for( cb = 0; cb < s->param->BhattaSettings.NumBins1; cb++ )
        {
            for( cg = 0; cg < s->param->BhattaSettings.NumBins2; cg++ )
            {
                ia_free( b.bg_hist[i][cb][cg] );
            }
            ia_free( b.bg_hist[i][cb] );
        }
        ia_free( b.bg_hist[i] );
    }
    ia_free( b.bg_hist );

    for( cb = 0; cb < s->param->BhattaSettings.NumBins1; cb++ )
    {
        for( cg = 0; cg < s->param->BhattaSettings.NumBins2; cg++ )
        {
            ia_free( b.pixel_hist[cb][cg] );
        }
        ia_free( b.pixel_hist[cb] );
    }
    ia_free( b.pixel_hist );
}

void bhatta_populatebg( ia_seq_t* s )
{
    // Patches are centered around the pixel.  [p.x p.y]-[offset offset] gives the upperleft corner of the patch.
    int offset = s->param->BhattaSettings.SizePatch/2;

    // To reduce divisions.  Used to take a pixel value and calculate the histogram bin it falls in.
    double inv_sizeBins1 = 1.f/ceil(256/(double)s->param->BhattaSettings.NumBins1);
    double inv_sizeBins2 = 1.f/ceil(256/(double)s->param->BhattaSettings.NumBins2);
    double inv_sizeBins3 = 1.f/ceil(256/(double)s->param->BhattaSettings.NumBins3);

    int i = 0, k=0;  // k = current pixel index
    int binB, binG, binR;
    int r,c,ro,co,roi,coi;
    int cg, cb, cr;
    s->param->Settings.bgCount = 0;


    // Histograms are for each pixel by creating create a histogram of the left most pixel in a row.
    // Then the next pixel's histogram in the row is calculated by:
    //   1. removing pixels in the left most col of the previous patch from the histogram and 
    //   2. adding pixels in the right most col of the current pixel's patch to the histogram

    // create temp hist to store working histogram
    unsigned int*** tempHist = (unsigned int***) ia_malloc( sizeof(unsigned int**)*s->param->BhattaSettings.NumBins1 );
    for( cb = 0; cb < s->param->BhattaSettings.NumBins1; cb++ )
    {
        tempHist[cb] = (unsigned int**) ia_malloc( sizeof(unsigned int*)*s->param->BhattaSettings.NumBins2 );
        for( cg = 0; cg < s->param->BhattaSettings.NumBins2; cg++ )
        {
            tempHist[cb][cg] = (unsigned int*) ia_malloc( sizeof(unsigned int)*s->param->BhattaSettings.NumBins3 );
        }
    }

    for( r = 0; r < s->param->i_height; r++ )
    {
        // clear temp patch
        for( cb = 0; cb < s->param->BhattaSettings.NumBins1; cb++ )
            for( cg = 0; cg < s->param->BhattaSettings.NumBins2; cg++ )
                ia_memset( tempHist[cb][cg],0,sizeof(unsigned int)*s->param->BhattaSettings.NumBins3);

        // create the left most pixel's histogram from scratch
        c = 0;
        int roEnd = r-offset+s->param->BhattaSettings.SizePatch;  // end of patch
        int coEnd = c-offset+s->param->BhattaSettings.SizePatch;  // end of patch
        for( ro = r-offset; ro < roEnd; ro++ )
        {  // cover the row
            roi = (ro < 0 ? -ro-1 : (ro >= s->param->i_height ? 2*s->param->i_height-1-ro :ro));
            for( co = c-offset; co < coEnd; co++ )
            { // cover the col
                coi = (co < 0 ? -co-1 : (co >= s->param->i_width ? 2*s->param->i_width-1-co :co));

                // get the pixel loation
                i = (roi*s->param->i_width+coi)*3;
                //figure out whicch bin
                binB = (int) (s->iaf->pix[i]*inv_sizeBins1);
                binG = (int) (s->iaf->pix[i+1]*inv_sizeBins2);
                binR = (int) (s->iaf->pix[i+2]*inv_sizeBins3);
                // add to temporary histogram
                tempHist[binB][binG][binR]++;

            }
        }
        // copy temp histogram to left most patch
        for( cb = 0; cb < s->param->BhattaSettings.NumBins1; cb++ )
            for( cg = 0; cg < s->param->BhattaSettings.NumBins2; cg++ )
                for( cr = 0; cr < s->param->BhattaSettings.NumBins3; cr++ )
                    b.bg_hist[k][cb][cg][cr] += tempHist[cb][cg][cr];

        // increment pixel index
        k++;

        // compute the top row of histograms
        for( c = 1; c < s->param->i_width; c++ )
        {
            // subtract left col
            co = c-offset-1;
            coi = (co < 0 ? -co-1 : (co >= s->param->i_width ? 2*s->param->i_width-1-co :co));
            for( ro = r-offset; ro < r-offset+s->param->BhattaSettings.SizePatch; ro++ )
            {
                roi = (ro < 0 ? -ro-1 : (ro >= s->param->i_height ? 2*s->param->i_height-1-ro :ro));
                i = (roi*s->param->i_width+coi)*3;
                binB = (int) (s->iaf->pix[i]*inv_sizeBins1);
                binG = (int) (s->iaf->pix[i+1]*inv_sizeBins2);
                binR = (int) (s->iaf->pix[i+2]*inv_sizeBins3);

                tempHist[binB][binG][binR]--;
            }

            // add right col
            co = c-offset+s->param->BhattaSettings.SizePatch-1;
            coi = (co < 0 ? -co-1 : (co >= s->param->i_width ? 2*s->param->i_width-1-co :co));
            for( ro = r-offset; ro < r-offset+s->param->BhattaSettings.SizePatch; ro++ )
            {
                roi = (ro < 0 ? -ro-1 : (ro >= s->param->i_height ? 2*s->param->i_height-1-ro :ro));
                i = (roi*s->param->i_width+coi)*3;
                binB = (int) (s->iaf->pix[i]*inv_sizeBins1);
                binG = (int) (s->iaf->pix[i+1]*inv_sizeBins2);
                binR = (int) (s->iaf->pix[i+2]*inv_sizeBins3);
                tempHist[binB][binG][binR]++;
            }

            // copy over            
            for( cb = 0; cb < s->param->BhattaSettings.NumBins1; cb++ )
                for( cg = 0; cg < s->param->BhattaSettings.NumBins2; cg++ )
                    for( cr = 0; cr < s->param->BhattaSettings.NumBins3; cr++ )
                        b.bg_hist[k][cb][cg][cr] += tempHist[cb][cg][cr];

            // increment pixel index
            k++;
        }
    }
}

int bhatta_estimatefg( ia_seq_t* s )
{
    // Patches are centered around the pixel.  [p.x p.y]-[offset offset] gives the upperleft corner of the patch.              
    int offset = s->param->BhattaSettings.SizePatch/2;
    // To reduce divisions.  Used to take a pixel value and calculate the histogram bin it falls in.
    double inv_sizeBins1 = 1.f/ceil(256/(double)s->param->BhattaSettings.NumBins1);
    double inv_sizeBins2 = 1.f/ceil(256/(double)s->param->BhattaSettings.NumBins2);
    double inv_sizeBins3 = 1.f/ceil(256/(double)s->param->BhattaSettings.NumBins3);
    double inv_nPixels = 1.f/((double) (s->param->BhattaSettings.SizePatch*s->param->BhattaSettings.SizePatch*s->param->BhattaSettings.SizePatch*s->param->BhattaSettings.SizePatch*s->param->Settings.NumBgFrames));

    int binB, binG, binR;
    int i = 0, cb, cg, cr;
    double diff = 0;
    // for each pixel
    int coi=0,roi=0,r=0,c=0,co=0,ro=0,pIndex=0;
    int pos = 0;

    // clear s->mask/diff image
    ia_memset( s->mask,0,s->param->i_size*sizeof(uint8_t) );
    ia_memset( s->diffImage,0,s->param->i_size*3*sizeof(uint8_t) );


    // as in the populateBg(), we compute the histogram by subtracting/adding cols

    for( r = 0; r < s->param->i_height; r++ )
    {
        c=0;
        pIndex = r*s->param->i_width+c;

        // for the first pixel

        // clear patch histogram
        for( cb = 0; cb < s->param->BhattaSettings.NumBins1; cb++ )
            for( cg = 0; cg < s->param->BhattaSettings.NumBins2; cg++ )
                ia_memset( b.pixel_hist[cb][cg],0,s->param->BhattaSettings.NumBins3*sizeof(unsigned int) );

        // compute patch
        int roEnd = r-offset+s->param->BhattaSettings.SizePatch;
        int coEnd = c-offset+s->param->BhattaSettings.SizePatch;
        for( ro = r-offset; ro < roEnd; ro++ )
        {
            roi = (ro < 0 ? -ro-1 : (ro >= s->param->i_height ? 2*s->param->i_height-1-ro :ro));
            for( co = c-offset; co < coEnd; co++ )
            {
                coi = (co < 0 ? -co-1 : (co >= s->param->i_width ? 2*s->param->i_width-1-co :co));
                i = (roi*s->param->i_width+coi)*3;
                binB = (int) (s->iaf->pix[i]*inv_sizeBins1);
                binG = (int) (s->iaf->pix[i+1]*inv_sizeBins2);
                binR = (int) (s->iaf->pix[i+2]*inv_sizeBins3);
                b.pixel_hist[binB][binG][binR]++;
            }
        }
        // compare histograms
        diff = 0;
        int temp1 = 0;
        int temp2 = 0;
        for( cb = 0; cb < s->param->BhattaSettings.NumBins1; cb++ )
        {
            for( cg = 0; cg < s->param->BhattaSettings.NumBins2; cg++ )
            {
                for( cr = 0; cr < s->param->BhattaSettings.NumBins3; cr++ )
                {
                    diff += sqrt((double)b.pixel_hist[cb][cg][cr] * (double)b.bg_hist[pIndex][cb][cg][cr] * inv_nPixels);
                    temp1 += b.pixel_hist[cb][cg][cr];
                    temp2 += b.bg_hist[pIndex][cb][cg][cr];
                }
            }
        }

        // renormalize diff so that 255 = very diff, 0 = same
        // create result images
        s->diffImage[pos++] = (uint8_t) (255-(int)(diff*255));
        s->diffImage[pos++] = (uint8_t) (255-(int)(diff*255));
        s->diffImage[pos++] = (uint8_t) (255-(int)(diff*255));
        s->mask[pIndex] = (s->diffImage[pos-1] > s->param->Settings.Threshold ? 255 : 0);


        // iterate through the rest of the row
        for( c = 1; c < s->param->i_width; c++ )
        {
            pIndex = r*s->param->i_width+c;

            //remove left col
            co = c-offset-1;
            coi = (co < 0 ? -co-1 : (co >= s->param->i_width ? 2*s->param->i_width-1-co :co));
            for( ro = r-offset; ro < r-offset+s->param->BhattaSettings.SizePatch; ro++ )
            {
                roi = (ro < 0 ? -ro-1 : (ro >= s->param->i_height ? 2*s->param->i_height-1-ro :ro));
                i = (roi*s->param->i_width+coi)*3;
                binB = (int) (s->iaf->pix[i]*inv_sizeBins1);
                binG = (int) (s->iaf->pix[i+1]*inv_sizeBins2);
                binR = (int) (s->iaf->pix[i+2]*inv_sizeBins3);

                b.pixel_hist[binB][binG][binR]--;
            }

            // add right col
            co = c-offset+s->param->BhattaSettings.SizePatch-1;
            coi = (co < 0 ? -co-1 : (co >= s->param->i_width ? 2*s->param->i_width-1-co :co));
            for( ro = r-offset; ro < r-offset+s->param->BhattaSettings.SizePatch; ro++ )
            {
                roi = (ro < 0 ? -ro-1 : (ro >= s->param->i_height ? 2*s->param->i_height-1-ro :ro));
                i = (roi*s->param->i_width+coi)*3;
                binB = (int) (s->iaf->pix[i]*inv_sizeBins1);
                binG = (int) (s->iaf->pix[i+1]*inv_sizeBins2);
                binR = (int) (s->iaf->pix[i+2]*inv_sizeBins3);
                b.pixel_hist[binB][binG][binR]++;
            }
            // compare histograms
            diff = 0;
            temp1 = 0;
            temp2 = 0;
            for( cb = 0; cb < s->param->BhattaSettings.NumBins1; cb++ )
            {
                for( cg = 0; cg < s->param->BhattaSettings.NumBins2; cg++ )
                {
                    for( cr = 0; cr < s->param->BhattaSettings.NumBins3; cr++ )
                    {
                        diff += sqrt((double)b.pixel_hist[cb][cg][cr] * (double)b.bg_hist[pIndex][cb][cg][cr] * inv_nPixels);
                        temp1 += b.pixel_hist[cb][cg][cr];
                        temp2 += b.bg_hist[pIndex][cb][cg][cr];
                    }
                }
            }

            // create result images     
            s->diffImage[pos++] = (uint8_t) (255-(int)(diff*255));
            s->diffImage[pos++] = (uint8_t) (255-(int)(diff*255));
            s->diffImage[pos++] = (uint8_t) (255-(int)(diff*255));
            s->mask[pIndex] = (s->diffImage[pos-1] > s->param->Settings.Threshold ? 255 : 0);
        }
    }
    
    ia_memcpy_uint8_to_pixel( s->iar->pix,s->diffImage,s->param->i_size*3 );
//    for( i = 0; i < s->param->i_size*3; i++ )
//        s->iar->pix[i] = s->diffImage[i];
    return 0;
}

int bhatta( ia_seq_t* s )
{
    if( (int)s->i_frame < s->param->Settings.NumBgFrames)
    {
        bhatta_populatebg( s );
    }
    else
    {
        if( bhatta_estimatefg(s) )
            return 1;
//
//        p->Settings.bgCount++;
//        if( p->Settings.bgCount == p->Settings.BgStep)
//        {
//            if( b.BhattaOptSettings.useBgUpdateMask )
//                bhatta_updatebg( iaf,diff,mask );
//            else
//                bhatta_updatebg( iaf,diff,NULL );
//            p->Settings.bgCount = 0;
//        }
//
    }
    return 0;
}

#endif
