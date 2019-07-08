    /***************************************************************************** 
     * me.c: h264 encoder library (Motion Estimation) 
     *****************************************************************************/  
      
    #include <stdlib.h>  
    #include <stdio.h>  
    #include <string.h>  
    #include <stdint.h>  
      
    #include "../core/common.h"  
      
    static inline int me_mv_test( x264_t *h,  
                                  int *pi_sad, int *pi_mvx, int *pi_mvy,  
                                  int i_sad_mode, uint8_t *img, int i_img, uint8_t *test, int i_test_stride,  
                                  int mvx, int mvy,  
                                  int i_lambda_motion, int mvxp, int mvyp )  
    {  
        int i_sad;  
      
        i_sad = h->pixf.sad[i_sad_mode]( img, i_img, &test[mvy*i_test_stride+mvx], i_test_stride );  
        if( i_sad + i_lambda_motion * ( bs_size_se((mvx<<2) - mvxp)+bs_size_se((mvy<<2) - mvyp) ) <  
           *pi_sad + i_lambda_motion * ( bs_size_se(((*pi_mvx)<<2) - mvxp)+bs_size_se(((*pi_mvy)<<2) - mvyp) ) )  
        {  
            *pi_sad = i_sad;  
            *pi_mvx = mvx;  
            *pi_mvy = mvy;  
            return 1;  
        }  
      
        return 0;  
    }  
      
    static inline int me_mv_test_buffer( x264_t *h,  
                                         int *pi_sad, int *pi_mvx, int *pi_mvy,  
                                         int i_sad_mode, uint8_t *img, int i_img, uint8_t *test, int i_test_stride,  
                                         int mvx, int mvy,  
                                          int i_lambda_motion, int mvxp, int mvyp )  
    {  
        int i_sad;  
      
        i_sad = h->pixf.satd[i_sad_mode]( img, i_img, test, i_test_stride );  
        if( i_sad  + i_lambda_motion * ( bs_size_se(mvx - mvxp)     + bs_size_se(mvy - mvyp) ) <  
           *pi_sad + i_lambda_motion * ( bs_size_se((*pi_mvx)-mvxp) + bs_size_se((*pi_mvy) - mvyp) ) )  
        {  
            *pi_sad = i_sad;  
            *pi_mvx = mvx;  
            *pi_mvy = mvy;  
            return 1;  
        }  
      
        return 0;  
    }  
      
    /* Range : -16:16 */  
    static int x264_me_umhexagons( x264_t *h, uint8_t *p_ref, int i_ref,  
                                   uint8_t *p_img, int i_img,  
                                   int i_sad_mode,  
                                   int i_lambda_motion,  
                                   int *pi_mvx, int *pi_mvy )  
    {  
        int i, n, dx, dy;  
      
        int i_bsatd;  
        int i_bmx,i_bmy;  
        int mvx, mvy;  
        int bw, bh;  
      
        int ok;  
      
        static const int hgrid_x[16]= { -4,-4,-4,-2, 0, 2, 4, 4, 4, 4, 4, 2, 0,-2,-4,-4 };  
        static const int hgrid_y[16]= {  0, 1, 2, 3, 4, 3, 2, 1, 0,-1,-2,-3,-4,-3, 2, 1 };  
      
        static const int hexa_x[6] = { -2,-1, 1, 2, 1,-1 };  
        static const int hexa_y[6] = { 0,  2, 2, 0, 2, 2 };  
      
        static const int star_x[4] = { -1, 0, 1,  0 };  
        static const int star_y[4] = {  0, 1, 0, -1 };  
      
        /* block size */  
        switch( i_sad_mode )  
        {  
            case PIXEL_16x16:  
                bw = bh = 16;  
                break;  
            case PIXEL_16x8:  
                bw = 16; bh = 8;  
                break;  
            case PIXEL_8x16:  
                bw = 8; bh = 16;  
                break;  
            case PIXEL_8x8:  
                bw = 8; bh = 8;  
                break;  
            case PIXEL_8x4:  
                bw = 8; bh = 4;  
                break;  
            case PIXEL_4x8:  
                bw = 4; bh = 8;  
                break;  
            case PIXEL_4x4:  
            default:  
                bw = 4; bh = 4;  
                break;  
        }  
      
        /* mv start (clipped) */  
        mvx = x264_clip3( (*pi_mvx)>>2, -16, 16 );  
        mvy = x264_clip3( (*pi_mvy)>>2, -16, 16 );  
      
        /* init with center */  
        i_bmx = mvx;  
        i_bmy = mvy;  
        i_bsatd = h->pixf.sad[i_sad_mode]( p_img, i_img, &p_ref[mvy*i_ref+mvx], i_ref );  
      
        /* First step: Unsymetrical cross edge */  
        for( dx = mvx - 16; dx <= mvx + 16; dx += 2 )  
        {  
            if( dx >= -16 && dx <= 16 )  
            {  
                me_mv_test( h, &i_bsatd, &i_bmx, &i_bmy,  
                            i_sad_mode, p_img, i_img, p_ref, i_ref, dx, mvy,  
                            i_lambda_motion, *pi_mvx, *pi_mvy );  
            }  
        }  
        for( dy = mvy - 8; dy <= mvy + 8; dy += 2 )  
        {  
            if( dy >= - 16 && dy <= 16 )  
            {  
                me_mv_test( h, &i_bsatd, &i_bmx, &i_bmy,  
                            i_sad_mode, p_img, i_img, p_ref, i_ref, mvx, dy,  
                            i_lambda_motion, *pi_mvx, *pi_mvy );  
            }  
        }  
      
        /* Second step-part 1: full search -2/2 */  
        mvx = i_bmx; mvy = i_bmy;  
        for( dy = -2; dy <= 2; dy++ )  
        {  
            for( dx = -2; dx <= 2; dx++ )  
            {  
                int mx, my;  
                mx = mvx+dx;  
                my = mvy+dy;  
      
                if( mx >= -16 && mx <= 16 && my >= -16 && my < 16 )  
                {  
                    me_mv_test( h, &i_bsatd, &i_bmx, &i_bmy,  
                                i_sad_mode, p_img, i_img, p_ref, i_ref, mx, my,  
                                i_lambda_motion, *pi_mvx, *pi_mvy );  
                }  
            }  
        }  
        /* Second step-part 2: multi hexa research */  
        ok = 1;  
        mvx = i_bmx; mvy = i_bmy;  
        for( n = 1; n <= 4; n++ )  
        {  
            for( i = 0; i < 16; i++ )  
            {  
                dx = mvx + hgrid_x[i] * n;  
                dy = mvy + hgrid_y[i] * n;  
                if( dx >= -16 && dx <= 16 && dy >= -16 && dy <= 16 )  
                {  
                    if( me_mv_test( h, &i_bsatd, &i_bmx, &i_bmy,  
                                    i_sad_mode, p_img, i_img, p_ref, i_ref, dx, dy,  
                                    i_lambda_motion, *pi_mvx, *pi_mvy ) )  
                    {  
                        ok = 0; /* we will have to do EHS step */  
                    }  
                }  
            }  
        }  
      
        /* Third step: EHS */  
        if( !ok )  
        {  
            do  
            {  
                ok = 1;  
                mvx = i_bmx; mvy = i_bmy;  
      
                for( i = 0; i < 6; i++ )  
                {  
                    dx = mvx + hexa_x[i];  
                    dy = mvy + hexa_y[i];  
                    if( dx >= -16 && dx <= 16 && dy >= -16 && dy <= 16 )  
                    {  
                        if( me_mv_test( h, &i_bsatd, &i_bmx, &i_bmy,  
                                        i_sad_mode, p_img, i_img, p_ref, i_ref, dx, dy,  
                                        i_lambda_motion, *pi_mvx, *pi_mvy ) )  
                        {  
                            ok = 0;  
                        }  
                    }  
                }  
            } while( !ok );  
      
            do  
            {  
                ok = 1;  
                mvx = i_bmx; mvy = i_bmy;  
      
                for( i = 0; i < 4; i++ )  
                {  
                    dx = mvx + star_x[i];  
                    dy = mvy + star_y[i];  
                    if( dx >= -16 && dx <= 16 && dy >= -16 && dy <= 16 )  
                    {  
                        if( me_mv_test( h, &i_bsatd, &i_bmx, &i_bmy,  
                                        i_sad_mode, p_img, i_img, p_ref, i_ref, dx, dy,  
                                        i_lambda_motion, *pi_mvx, *pi_mvy ) )  
                        {  
                            ok = 0;  
                        }  
                    }  
                }  
            } while( !ok );  
        }  
      
        /* 1/4 pixel rafinement using diamond research */  
        i_bsatd = h->pixf.satd[i_sad_mode]( p_img, i_img, &p_ref[i_bmx+i_bmy*i_ref], i_ref ) +  
                    i_lambda_motion * ( bs_size_se((i_bmx<<2) - (*pi_mvx))+bs_size_se((i_bmy<<2) - (*pi_mvy)) );  
      
        i_bmx <<= 2;  
        i_bmy <<= 2;  
        {  
            uint8_t pix[16*16];  
            int satd[4];  
            int i, i_b;  
      
    #define TEST_MV( s, mx, my ) \
            h->mc[MC_LUMA]( p_ref, i_ref, pix, bw, (mx), (my), bw, bh ); \
            (s) = h->pixf.satd[i_sad_mode]( p_img, i_img, pix, bw ) + \
                i_lambda_motion * ( bs_size_se((mx) - (*pi_mvx))+bs_size_se((my) - (*pi_mvy)) )  
      
            TEST_MV( satd[0], i_bmx,   i_bmy-1 );  
            TEST_MV( satd[1], i_bmx,   i_bmy+1 );  
            TEST_MV( satd[2], i_bmx-1, i_bmy );  
            TEST_MV( satd[3], i_bmx+1, i_bmy );  
            for( i = 1, i_b = 0; i < 4; i++ )  
            {  
                if( satd[i] < satd[i_b] )  
                {  
                    i_b = i;  
                }  
            }  
            if( satd[i_b] < i_bsatd )  
            {  
                if( i_b == 0 )      i_bmy--;  
                else if( i_b == 1 ) i_bmy++;  
                else if( i_b == 2 ) i_bmx--;  
                else if( i_b == 3 ) i_bmx++;  
                i_bsatd = satd[i_b];  
      
                if( i_b != 1 ) { TEST_MV( satd[0], i_bmx,   i_bmy-1 ); } else { satd[0] = i_bsatd; }  
                if( i_b != 0 ) { TEST_MV( satd[1], i_bmx,   i_bmy+1 ); } else { satd[1] = i_bsatd; }  
                if( i_b != 3 ) { TEST_MV( satd[2], i_bmx-1, i_bmy   ); } else { satd[2] = i_bsatd; }  
                if( i_b != 2 ) { TEST_MV( satd[3], i_bmx+1, i_bmy   ); } else { satd[3] = i_bsatd; }  
      
                for( i = 1, i_b = 0; i < 4; i++ )  
                {  
                    if( satd[i] < satd[i_b] )  
                    {  
                        i_b = i;  
                    }  
                }  
                if( satd[i_b] < i_bsatd )  
                {  
                    if( i_b == 0 )      i_bmy--;  
                    else if( i_b == 1 ) i_bmy++;  
                    else if( i_b == 2 ) i_bmx--;  
                    else if( i_b == 3 ) i_bmx++;  
                    i_bsatd = satd[i_b];  
                }  
            }  
    #undef TEST_MV  
        }  
        *pi_mvx = i_bmx;  
        *pi_mvy = i_bmy;  
      
        return i_bsatd;  
    }  
      
    static int x264_me_diamond( x264_t *h,  
                                uint8_t *p_ref, int i_ref,  
                                uint8_t *p_img, int i_img,  
                                int i_sad_mode,  
                                int i_lambda_motion,  
                                int *pi_mvx, int *pi_mvy )  
    {  
        int bw, bh;  
        int i_bsatd;  
        int i_bmx, i_bmy;  
      
        /* block size */  
        switch( i_sad_mode )  
        {  
            case PIXEL_16x16:  
                bw = bh = 16;  
                break;  
            case PIXEL_16x8:  
                bw = 16; bh = 8;  
                break;  
            case PIXEL_8x16:  
                bw = 8; bh = 16;  
                break;  
            case PIXEL_8x8:  
                bw = 8; bh = 8;  
                break;  
            case PIXEL_8x4:  
                bw = 8; bh = 4;  
                break;  
            case PIXEL_4x8:  
                bw = 4; bh = 8;  
                break;  
            case PIXEL_4x4:  
            default:  
                bw = 4; bh = 4;  
                break;  
        }  
      
        /* init with center */  
        i_bmx = x264_clip3( (*pi_mvx)>>2, -16, 16 );  
        i_bmy = x264_clip3( (*pi_mvy)>>2, -16, 16 );  
        i_bsatd = h->pixf.sad[i_sad_mode]( p_img, i_img, &p_ref[i_bmy*i_ref+i_bmx], i_ref );  
      
        for( ;; )  
        {  
            int satd[4];  
            int i, i_b;  
            if( i_bmx < -16 || i_bmx > 16 || i_bmy < -16 || i_bmy > 16 )  
            {  
                break;  
            }  
    #define TEST_MV( mx, my ) \
            h->pixf.sad[i_sad_mode]( p_img, i_img, &p_ref[(my)*i_ref+(mx)], i_ref ) + \
                i_lambda_motion * ( bs_size_se(((mx)<<2) - (*pi_mvx))+bs_size_se(((my)<<2) - (*pi_mvy)) )  
            satd[0] = TEST_MV( i_bmx,   i_bmy-1 );  
            satd[1] = TEST_MV( i_bmx,   i_bmy+1 );  
            satd[2] = TEST_MV( i_bmx-1, i_bmy );  
            satd[3] = TEST_MV( i_bmx+1, i_bmy );  
    #undef TEST_MV  
      
            for( i = 1, i_b = 0; i < 4; i++ )  
            {  
                if( satd[i] < satd[i_b] )  
                {  
                    i_b = i;  
                }  
            }  
            if( satd[i_b] >= i_bsatd )  
            {  
                break;  
            }  
      
            if( i_b == 0 )      i_bmy--;  
            else if( i_b == 1 ) i_bmy++;  
            else if( i_b == 2 ) i_bmx--;  
            else if( i_b == 3 ) i_bmx++;  
            i_bsatd = satd[i_b];  
        }  
      
        /* 1/4 pixel rafinement using diamond research */  
        i_bsatd = h->pixf.satd[i_sad_mode]( p_img, i_img, &p_ref[i_bmx+i_bmy*i_ref], i_ref ) +  
                    i_lambda_motion * ( bs_size_se((i_bmx<<2) - (*pi_mvx))+bs_size_se((i_bmy<<2) - (*pi_mvy)) );  
        i_bmx <<= 2;  
        i_bmy <<= 2;  
        {  
            uint8_t pix[16*16];  
            int satd[4];  
            int i, i_b;  
      
    #define TEST_MV( s, mx, my ) \
            h->mc[MC_LUMA]( p_ref, i_ref, pix, bw, (mx), (my), bw, bh ); \
            (s) = h->pixf.satd[i_sad_mode]( p_img, i_img, pix, bw ) + \
                i_lambda_motion * ( bs_size_se((mx) - (*pi_mvx))+bs_size_se((my) - (*pi_mvy)) )  
      
            TEST_MV( satd[0], i_bmx,   i_bmy-1 );  
            TEST_MV( satd[1], i_bmx,   i_bmy+1 );  
            TEST_MV( satd[2], i_bmx-1, i_bmy );  
            TEST_MV( satd[3], i_bmx+1, i_bmy );  
            for( i = 1, i_b = 0; i < 4; i++ )  
            {  
                if( satd[i] < satd[i_b] )  
                {  
                    i_b = i;  
                }  
            }  
            if( satd[i_b] < i_bsatd )  
            {  
                if( i_b == 0 )      i_bmy--;  
                else if( i_b == 1 ) i_bmy++;  
                else if( i_b == 2 ) i_bmx--;  
                else if( i_b == 3 ) i_bmx++;  
                i_bsatd = satd[i_b];  
      
                if( i_b != 1 ) { TEST_MV( satd[0], i_bmx,   i_bmy-1 ); } else { satd[0] = i_bsatd; }  
                if( i_b != 0 ) { TEST_MV( satd[1], i_bmx,   i_bmy+1 ); } else { satd[1] = i_bsatd; }  
                if( i_b != 3 ) { TEST_MV( satd[2], i_bmx-1, i_bmy   ); } else { satd[2] = i_bsatd; }  
                if( i_b != 2 ) { TEST_MV( satd[3], i_bmx+1, i_bmy   ); } else { satd[3] = i_bsatd; }  
      
                for( i = 1, i_b = 0; i < 4; i++ )  
                {  
                    if( satd[i] < satd[i_b] )  
                    {  
                        i_b = i;  
                    }  
                }  
                if( satd[i_b] < i_bsatd )  
                {  
                    if( i_b == 0 )      i_bmy--;  
                    else if( i_b == 1 ) i_bmy++;  
                    else if( i_b == 2 ) i_bmx--;  
                    else if( i_b == 3 ) i_bmx++;  
                    i_bsatd = satd[i_b];  
                }  
            }  
    #undef TEST_MV  
        }  
      
      
        *pi_mvx = i_bmx;  
        *pi_mvy = i_bmy;  
      
        return i_bsatd;  
    }  
      
    void x264_me_init( int cpu, x264_me_t pf[2] )  
    {  
        pf[X264_ME_UMHEXAGONS] = x264_me_umhexagons;  
        pf[X264_ME_DIAMOND]    = x264_me_diamond;  
    }  
