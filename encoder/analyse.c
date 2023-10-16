
/*****************************************************************************
 * analyse.c: h264 encoder library

 *****************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <math.h>

#include "../core/common.h"
#include "me.h"

static const uint8_t block_idx_x[16] = { 0, 1, 0, 1, 2, 3, 2, 3, 0, 1, 0, 1, 2, 3, 2, 3 };
static const uint8_t block_idx_y[16] = { 0, 0, 1, 1, 0, 0, 1, 1, 2, 2, 3, 3, 2, 2, 3, 3 };

/*
 * Handle intra mb
 */
/* Max = 4 */
static void predict_16x16_mode_available( x264_macroblock_t *mb, int *mode, int *pi_count )
{
    if( ( mb->i_neighbour & (MB_LEFT|MB_TOP) ) == (MB_LEFT|MB_TOP) )
    {
        /* top and left avaible */
        *mode++ = I_PRED_16x16_V;
        *mode++ = I_PRED_16x16_H;
        *mode++ = I_PRED_16x16_DC;
        *mode++ = I_PRED_16x16_P;
        *pi_count = 4;
    }
    else if( ( mb->i_neighbour & MB_LEFT ) )
    {
        /* left available*/
        *mode++ = I_PRED_16x16_DC_LEFT;
        *mode++ = I_PRED_16x16_H;
        *pi_count = 2;
    }
    else if( ( mb->i_neighbour & MB_TOP ) )
    {
        /* top available*/
        *mode++ = I_PRED_16x16_DC_TOP;
        *mode++ = I_PRED_16x16_V;
        *pi_count = 2;
    }
    else
    {
        /* none avaible */
        *mode = I_PRED_16x16_DC_128;
        *pi_count = 1;
    }
}

/* Max = 4 */
static void predict_8x8_mode_available( x264_macroblock_t *mb, int *mode, int *pi_count )
{
    if( ( mb->i_neighbour & (MB_LEFT|MB_TOP) ) == (MB_LEFT|MB_TOP) )
    {
        /* top and left avaible */
        *mode++ = I_PRED_CHROMA_V;
        *mode++ = I_PRED_CHROMA_H;
        *mode++ = I_PRED_CHROMA_DC;
        *mode++ = I_PRED_CHROMA_P;
        *pi_count = 4;
    }
    else if( ( mb->i_neighbour & MB_LEFT ) )
    {
        /* left available*/
        *mode++ = I_PRED_CHROMA_DC_LEFT;
        *mode++ = I_PRED_CHROMA_H;
        *pi_count = 2;
    }
    else if( ( mb->i_neighbour & MB_TOP ) )
    {
        /* top available*/
        *mode++ = I_PRED_CHROMA_DC_TOP;
        *mode++ = I_PRED_CHROMA_V;
        *pi_count = 2;
    }
    else
    {
        /* none avaible */
        *mode = I_PRED_CHROMA_DC_128;
        *pi_count = 1;
    }
}

/* MAX = 8 */
static void predict_4x4_mode_available( x264_macroblock_t *mb, int idx, int *mode, int *pi_count )
{
    int b_a, b_b, b_c;
    static const int needmb[16] =
    {
        MB_LEFT|MB_TOP, MB_TOP,
        MB_LEFT,        MB_PRIVATE,
        MB_TOP,         MB_TOP|MB_TOPRIGHT,
        0,              MB_PRIVATE,
        MB_LEFT,        0,
        MB_LEFT,        MB_PRIVATE,
        0,              MB_PRIVATE,
        0,              MB_PRIVATE
    };

    /* FIXME even when b_c == 0 there is some case where missing pixels
     * are emulated and thus more mode are available TODO
     * analysis and encode should be fixed too */
    b_a = (needmb[idx]&mb->i_neighbour&MB_LEFT) == (needmb[idx]&MB_LEFT);
    b_b = (needmb[idx]&mb->i_neighbour&MB_TOP) == (needmb[idx]&MB_TOP);
    b_c = (needmb[idx]&mb->i_neighbour&(MB_TOPRIGHT|MB_PRIVATE)) == (needmb[idx]&(MB_TOPRIGHT|MB_PRIVATE));

    if( b_a && b_b )
    {
        *mode++ = I_PRED_4x4_DC;
        *mode++ = I_PRED_4x4_H;
        *mode++ = I_PRED_4x4_V;
        *mode++ = I_PRED_4x4_DDR;
        *mode++ = I_PRED_4x4_VR;
        *mode++ = I_PRED_4x4_HD;
        *mode++ = I_PRED_4x4_HU;

        *pi_count = 7;

        if( b_c )
        {
            *mode++ = I_PRED_4x4_DDL;
            *mode++ = I_PRED_4x4_VL;
            (*pi_count) += 2;
        }
    }
    else if( b_a && !b_b )
    {
        *mode++ = I_PRED_4x4_DC_LEFT;
        *mode++ = I_PRED_4x4_H;
        *pi_count = 2;
    }
    else if( !b_a && b_b )
    {
        *mode++ = I_PRED_4x4_DC_TOP;
        *mode++ = I_PRED_4x4_V;
        *pi_count = 2;
    }
    else
    {
        *mode++ = I_PRED_4x4_DC_128;
        *pi_count = 1;
    }
}

typedef struct
{
    /* conduct the analysis using this lamda and QP */
    int i_lambda;
    int i_qp;


    /* I: Intra part */
    /* Luma part 16x16 and 4x4 modes stats */
    int i_sad_i16x16;
    int i_predict16x16;

    int i_sad_i4x4;
    int i_predict4x4[4][4];

    /* Chroma part */
    int i_sad_i8x8;
    int i_predict8x8;

    /* II: Inter part P frame */
    int i_sad_p16x16;
    int i_ref_p16x16;
    int i_mv_p16x16[2];

    int i_sad_p16x8;
    int i_ref_p16x8;
    int i_mv_p16x8[2][2];

    int i_sad_p8x16;
    int i_ref_p8x16;
    int i_mv_p8x16[2][2];

    int i_sad_p8x8;
    int i_ref_p8x8;
    int i_sub_partition_p8x8[4];
    int i_mv_p8x8[4][4][2];

    /* II: Inter part B frame */
    int i_sad_b16x16_l0;
    int i_ref_b16x16_l0;
    int i_mv_b16x16_l0[2];

    int i_sad_b16x16_l1;
    int i_ref_b16x16_l1;
    int i_mv_b16x16_l1[2];

    int i_sad_b16x16_bi;    /* used the same ref and mv as l0 and l1 (at least for now) */

    int i_sad_b16x8_l0;
    int i_ref_b16x8_l0;
    int i_mv_b16x8_l0[2];

    int i_sad_b8x16_l0;
    int i_ref_b8x16_l0;
    int i_mv_b8x16_l0[2];



} x264_mb_analysis_t;


//* gives equation lambda_mode=0.85*pow(2,(qp-12)/3) and lambda_motion=pow(lambda_mode,0.5),it is different from x264
/* So for the mode decision, if you're using RD throughout you would fully encode the block using motion vectors (MVs) corresponding to each partition,
 * calculate the sum of squared differences (SSD), run it through the entropy coder to get the bits used to code using that partition arrangement.
 * Then you calculate the rate distortion via the equation RD = SSD + bits*lambda. It should be clear that we wish to minimise this value.
 * The lambda values are looked up from the tables aforementioned.
 * I think the values in the tables have been adjusted slightly to perform best with x264.
 */

static const int i_qp0_cost_table[52] = {  
   1, 1, 1, 1, 1, 1, 1, 1,  /*  0-7 */  
   1, 1, 1, 1,              /*  8-11 */  
   1, 1, 1, 1, 2, 2, 2, 2,  /* 12-19 */  
   3, 3, 3, 4, 4, 4, 5, 6,  /* 20-27 */  
   6, 7, 8, 9,10,11,13,14,  /* 28-35 */  
  16,18,20,23,25,29,32,36,  /* 36-43 */  
  40,45,51,57,64,72,81,91   /* 44-51 */  
}; 

static void x264_mb_analyse_intra_chroma( x264_t *h, x264_macroblock_t *mb, x264_mb_analysis_t *res )
{
    x264_mb_context_t *ctx = mb->context;

    int i;

    int i_max;
    int predict_mode[9];

    uint8_t *p_dstc[2], *p_srcc[2];
    int      i_dstc[2], i_srcc[2];

    /* 8x8 prediction selection for chroma */
    p_dstc[0] = ctx->p_fdec[1]; i_dstc[0] = ctx->i_fdec[1];
    p_dstc[1] = ctx->p_fdec[2]; i_dstc[1] = ctx->i_fdec[2];
    p_srcc[0] = ctx->p_img[1];  i_srcc[0] = ctx->i_img[1];
    p_srcc[1] = ctx->p_img[2];  i_srcc[1] = ctx->i_img[2];

    predict_8x8_mode_available( mb, predict_mode, &i_max );
    res->i_sad_i8x8 = -1;
    for( i = 0; i < i_max; i++ )
    {
        int i_sad;
        int i_mode;

        i_mode = predict_mode[i];

        /* we do the prediction */
        h->predict_8x8[i_mode]( p_dstc[0], i_dstc[0] );
        h->predict_8x8[i_mode]( p_dstc[1], i_dstc[1] );

        /* we calculate the cost */
        i_sad = h->pixf.satd[PIXEL_8x8]( p_dstc[0], i_dstc[0], p_srcc[0], i_srcc[0] ) +
                h->pixf.satd[PIXEL_8x8]( p_dstc[1], i_dstc[1], p_srcc[1], i_srcc[1] ) +
                res->i_lambda * bs_size_ue( x264_mb_pred_mode8x8_fix[i_mode] );

        /* if i_score is lower it is better */
        if( res->i_sad_i8x8 == -1 || res->i_sad_i8x8 > i_sad )
        {
            res->i_predict8x8 = i_mode;
            res->i_sad_i8x8     = i_sad;
        }
    }
}


static void x264_mb_analyse_intra( x264_t *h, x264_macroblock_t *mb, x264_mb_analysis_t *res )
{
    int i, idx;

    int i_max;
    int predict_mode[9];

    uint8_t *p_dst, *p_src;
    int      i_dst, i_src;

    p_dst = mb->context->p_fdec[0]; i_dst = mb->context->i_fdec[0];
    p_src = mb->context->p_img[0];  i_src = mb->context->i_img[0];

    /*---------------- Try all mode and calculate their score ---------------*/

    /* 16x16 prediction selection */
    mb->i_type = I_16x16;
    res->i_sad_i16x16 = -1;
    predict_16x16_mode_available( mb, predict_mode, &i_max );
    for( i = 0; i < i_max; i++ )
    {
        int i_sad;
        int i_mode;

        i_mode = predict_mode[i];

        /* we do the prediction */
        h->predict_16x16[i_mode]( p_dst, i_dst );

        /* we calculate the diff and get the square sum of the diff */
        i_sad = h->pixf.satd[PIXEL_16x16]( p_dst, i_dst, p_src, i_src ) +
                res->i_lambda * bs_size_ue( x264_mb_pred_mode16x16_fix[i_mode] );
        /* if i_score is lower it is better */
        if( res->i_sad_i16x16 == -1 || res->i_sad_i16x16 > i_sad )
        {
            res->i_predict16x16 = i_mode;
            res->i_sad_i16x16     = i_sad;
        }
    }

    /* 4x4 prediction selection */
    mb->i_type = I_4x4;
    res->i_sad_i4x4 = 0;
    for( idx = 0; idx < 16; idx++ )
    {
        uint8_t *p_src_by;
        uint8_t *p_dst_by;
        int     i_best;
        int x, y;
        int i_pred_mode;

        i_pred_mode= x264_mb_predict_intra4x4_mode( h, mb, idx );
        x = block_idx_x[idx];
        y = block_idx_y[idx];

        p_src_by = p_src + 4 * x + 4 * y * i_src;
        p_dst_by = p_dst + 4 * x + 4 * y * i_dst;

        i_best = -1;
        predict_4x4_mode_available( mb, idx, predict_mode, &i_max );
        for( i = 0; i < i_max; i++ )
        {
            int i_sad;
            int i_mode;

            i_mode = predict_mode[i];

            /* we do the prediction */
            h->predict_4x4[i_mode]( p_dst_by, i_dst );

            /* we calculate diff and get the square sum of the diff */
            i_sad = h->pixf.satd[PIXEL_4x4]( p_dst_by, i_dst, p_src_by, i_src );

            i_sad += res->i_lambda * (i_pred_mode == x264_mb_pred_mode4x4_fix[i_mode] ? 1 : 4);

            /* if i_score is lower it is better */
            if( i_best == -1 || i_best > i_sad )
            {
                res->i_predict4x4[x][y] = i_mode;
                i_best = i_sad;
            }
        }
        res->i_sad_i4x4 += i_best;

        /* we need to encode this mb now (for next ones) */
        mb->block[idx].i_intra4x4_pred_mode = res->i_predict4x4[x][y];
        h->predict_4x4[res->i_predict4x4[x][y]]( p_dst_by, i_dst );
        x264_mb_encode_i4x4( h, mb, idx, res->i_qp );
    }
    res->i_sad_i4x4 += res->i_lambda * 24;    /* from JVT (SATD0) */
}

static void x264_mb_analyse_inter_p_p8x8( x264_t *h, x264_macroblock_t *mb, x264_mb_analysis_t *res )
{
    x264_mb_context_t *ctx = mb->context;
    int i_ref = res->i_ref_p16x16;

    uint8_t *p_fref = ctx->p_fref0[i_ref][0];
    int      i_fref = ctx->i_fref0[i_ref][0];
    uint8_t *p_img  = ctx->p_img[0];
    int      i_img  = ctx->i_img[0];

    int i;

    res->i_ref_p8x8 = i_ref;
    res->i_sad_p8x8 = 0;
    mb->i_partition = D_8x8;

    for( i = 0; i < 4; i++ )
    {
        static const int test8x8_mode[4] = { D_L0_8x8, D_L0_8x4, D_L0_4x8, D_L0_4x4 };
        static const int test8x8_pix[4]  = { PIXEL_8x8, PIXEL_8x4, PIXEL_4x8, PIXEL_4x4 };
        static const int test8x8_pos_x[4][4] = { { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 4, 0, 0 }, { 0, 4, 0, 4 } };
        static const int test8x8_pos_y[4][4] = { { 0, 0, 0, 0 }, { 0, 4, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 4, 4 } };
        int i_test;
        int mvp[4][2];
        int mv[4][2];

        int x, y;
        int i_sub;
        int i_b_satd;

        y = 8 * (i / 2);
        x = 8 * (i % 2);
        i_b_satd = -1;

        i_test = 0;
        /* FIXME as it's tooooooo slow test only 8x8 */
        //for( i_test = 0; i_test < 4; i_test++ )
        {
            int i_satd;

            i_satd = 0;

            mb->i_sub_partition[i] = test8x8_mode[i_test];

            for( i_sub = 0; i_sub < x264_mb_partition_count_table[test8x8_mode[i_test]]; i_sub++ )
            {
                x264_mb_predict_mv( mb, 0, i, i_sub, mvp[i_sub] );
                mv[i_sub][0] = mvp[i_sub][0];
                mv[i_sub][1] = mvp[i_sub][1];

                i_satd += h->me( h,
                                 &p_fref[(y+test8x8_pos_y[i_test][i_sub])*i_fref +x+test8x8_pos_x[i_test][i_sub]], i_fref,
                                 &p_img[(y+test8x8_pos_y[i_test][i_sub])*i_img +x+test8x8_pos_x[i_test][i_sub]], i_img,
                                 test8x8_pix[i_test],
                                 res->i_lambda,
                                 &mv[i_sub][0], &mv[i_sub][1] );
            }

            switch( test8x8_mode[i_test] )
            {
                case D_L0_8x8:
                    i_satd += res->i_lambda * bs_size_ue( 0 );
                    break;
                case D_L0_8x4:
                    i_satd += res->i_lambda * bs_size_ue( 1 );
                    break;
                case D_L0_4x8:
                    i_satd += res->i_lambda * bs_size_ue( 2 );
                    break;
                case D_L0_4x4:
                    i_satd += res->i_lambda * bs_size_ue( 3 );
                    break;
                default:
                    fprintf( stderr, "internal error (invalid sub type)\n" );
                    break;
            }

            if( i_b_satd == -1 || i_b_satd > i_satd )
            {
                i_b_satd = i_satd;
                res->i_sub_partition_p8x8[i] = test8x8_mode[i_test];;
                for( i_sub = 0; i_sub < x264_mb_partition_count_table[test8x8_mode[i_test]]; i_sub++ )
                {
                    res->i_mv_p8x8[i][i_sub][0] = mv[i_sub][0];
                    res->i_mv_p8x8[i][i_sub][1] = mv[i_sub][1];
                }
            }
        }

        res->i_sad_p8x8 += i_b_satd;
        /* needed for the next block */
        mb->i_sub_partition[i] = res->i_sub_partition_p8x8[i];
        for( i_sub = 0; i_sub < x264_mb_partition_count_table[res->i_sub_partition_p8x8[i]]; i_sub++ )
        {
            x264_mb_partition_set( mb, 0, i, i_sub,
                                           res->i_ref_p8x8,
                                           res->i_mv_p8x8[i][i_sub][0],
                                           res->i_mv_p8x8[i][i_sub][1] );
        }
    }

    res->i_sad_p8x8 += 4*res->i_lambda * bs_size_te( h->sh.i_num_ref_idx_l0_active - 1, i_ref );
}

static void x264_mb_analyse_inter_p( x264_t *h, x264_macroblock_t *mb, x264_mb_analysis_t *res )
{
    x264_mb_context_t *ctx = mb->context;

    int i_ref;

    /* int res */
    res->i_sad_p16x16 = -1;
    res->i_sad_p16x8  = -1;
    res->i_sad_p8x16  = -1;
    res->i_sad_p8x8   = -1;

    /* 16x16 Search on all ref frame */
    mb->i_type = P_L0;  /* beurk fix that */
    mb->i_partition = D_16x16;
    for( i_ref = 0; i_ref < h->i_ref0; i_ref++ )
    {
        int i_sad;
        int mvp[2];
        int mvx, mvy;

        /* Get the predicted MV */
        x264_mb_partition_set( mb, 0, 0, 0, i_ref, 0, 0 );
        x264_mb_predict_mv( mb, 0, 0, 0, mvp );

        mvx = mvp[0]; mvy = mvp[1];
        i_sad = h->me( h, ctx->p_fref0[i_ref][0], ctx->i_fref0[i_ref][0],
                          ctx->p_img[0],         ctx->i_img[0],
                          PIXEL_16x16, res->i_lambda, &mvx, &mvy );
        if( mvx == mvp[0] && mvy == mvp[1] )
        {
            i_sad -= 16 * res->i_lambda;
        }
        i_sad += res->i_lambda * bs_size_te( h->sh.i_num_ref_idx_l0_active - 1, i_ref );

        if( res->i_sad_p16x16 == -1 || i_sad < res->i_sad_p16x16 )
        {
            res->i_sad_p16x16   = i_sad;
            res->i_ref_p16x16   = i_ref;
            res->i_mv_p16x16[0] = mvx;
            res->i_mv_p16x16[1] = mvy;
        }
    }

    /* Now do the rafinement (using the ref found in 16x16 mode) */
    i_ref = res->i_ref_p16x16;
    x264_mb_partition_set( mb, 0, 0, 0, i_ref, 0, 0 );

    /* try 16x8 */
    /* XXX we test i_predict16x16 to try shape with the same direction than edge
     * We should do a better algo of course (the one with edge dectection to be used
     * for intra mode too)
     * */

    if( res->i_predict16x16 != I_PRED_16x16_V )
    {
        int mvp[2][2];

        mb->i_partition = D_16x8;

        res->i_ref_p16x8   = i_ref;
        x264_mb_predict_mv( mb, 0, 0, 0, mvp[0] );
        x264_mb_predict_mv( mb, 0, 1, 0, mvp[1] );

        res->i_mv_p16x8[0][0] = mvp[0][0]; res->i_mv_p16x8[0][1] = mvp[0][1];
        res->i_mv_p16x8[1][0] = mvp[1][0]; res->i_mv_p16x8[1][1] = mvp[1][1];

        res->i_sad_p16x8 = h->me( h,
                                  ctx->p_fref0[i_ref][0], ctx->i_fref0[i_ref][0],
                                  ctx->p_img[0],          ctx->i_img[0],
                                  PIXEL_16x8,
                                  res->i_lambda,
                                  &res->i_mv_p16x8[0][0], &res->i_mv_p16x8[0][1] ) +
                           h->me( h,
                                  &ctx->p_fref0[i_ref][0][8*ctx->i_fref0[i_ref][0]], ctx->i_fref0[i_ref][0],
                                  &ctx->p_img[0][8*ctx->i_img[0]],                   ctx->i_img[0],
                                  PIXEL_16x8,
                                  res->i_lambda,
                                  &res->i_mv_p16x8[1][0], &res->i_mv_p16x8[1][1] );

        res->i_sad_p16x8 += 2*res->i_lambda * bs_size_te( h->sh.i_num_ref_idx_l0_active - 1, i_ref );
    }

    /* try 8x16 */
    if( res->i_predict16x16 != I_PRED_16x16_H )
    {
        int mvp[2][2];

        mb->i_partition = D_8x16;

        res->i_ref_p8x16   = i_ref;
        x264_mb_predict_mv( mb, 0, 0, 0, mvp[0] );
        x264_mb_predict_mv( mb, 0, 1, 0, mvp[1] );

        res->i_mv_p8x16[0][0] = mvp[0][0]; res->i_mv_p8x16[0][1] = mvp[0][1];
        res->i_mv_p8x16[1][0] = mvp[1][0]; res->i_mv_p8x16[1][1] = mvp[1][1];

        res->i_sad_p8x16 = h->me( h,
                                  ctx->p_fref0[i_ref][0], ctx->i_fref0[i_ref][0],
                                  ctx->p_img[0],          ctx->i_img[0],
                                  PIXEL_8x16,
                                  res->i_lambda,
                                  &res->i_mv_p8x16[0][0], &res->i_mv_p8x16[0][1] ) +
                           h->me( h,
                                  &ctx->p_fref0[i_ref][0][8], ctx->i_fref0[i_ref][0],
                                  &ctx->p_img[0][8],          ctx->i_img[0],
                                  PIXEL_8x16,
                                  res->i_lambda,
                                  &res->i_mv_p8x16[1][0], &res->i_mv_p8x16[1][1] );

        res->i_sad_p8x16 += 2*res->i_lambda * bs_size_te( h->sh.i_num_ref_idx_l0_active - 1, i_ref );
    }

    /* a bit heuristique : if 4x4 is prefered, the block is probably not homegenous
     * for now disabled because too slow for too few bits saved */
    if( res->i_sad_i4x4 < res->i_sad_i16x16 )
    {
        x264_mb_analyse_inter_p_p8x8( h,mb, res );
    }
}

static void x264_mb_analyse_inter_b( x264_t *h, x264_macroblock_t *mb, x264_mb_analysis_t *res )
{
    x264_mb_context_t *ctx = mb->context;
    uint8_t pix1[16*16], pix2[16*16];

    int i_ref;
    /* int i_ref0, i_ref1; */

    int mvp[2];

    res->i_sad_b16x16_l0 = -1;
    res->i_sad_b16x16_l1 = -1;
    res->i_sad_b16x16_bi = -1;

    /* 16x16 L0 Search on all ref frame */
    mb->i_type = B_L0_L0;  /* beurk fix that */
    mb->i_partition = D_16x16;
    for( i_ref = 0; i_ref < h->i_ref0; i_ref++ )
    {
        int i_sad;
        int mvx, mvy;

        /* Get the predicted MV */
        x264_mb_partition_set( mb, 0, 0, 0, i_ref, 0, 0 );
        x264_mb_predict_mv( mb, 0, 0, 0, mvp );

        mvx = mvp[0]; mvy = mvp[1];
        i_sad = h->me( h, ctx->p_fref0[i_ref][0], ctx->i_fref0[i_ref][0],
                          ctx->p_img[0],         ctx->i_img[0],
                          PIXEL_16x16, res->i_lambda, &mvx, &mvy );
        i_sad += res->i_lambda * bs_size_te( h->sh.i_num_ref_idx_l0_active - 1, i_ref );

        if( res->i_sad_b16x16_l0 == -1 || i_sad < res->i_sad_b16x16_l0 )
        {
            res->i_sad_b16x16_l0   = i_sad;
            res->i_ref_b16x16_l0   = i_ref;
            res->i_mv_b16x16_l0[0] = mvx;
            res->i_mv_b16x16_l0[1] = mvy;
        }
    }

    /* 16x16 L1 Search on all ref frame */
    mb->i_type = B_L1_L1;  /* beurk fix that */
    mb->i_partition = D_16x16;
    for( i_ref = 0; i_ref < h->i_ref1; i_ref++ )
    {
        int i_sad;
        int mvx, mvy;

        /* Get the predicted MV */
        x264_mb_partition_set( mb, 1, 0, 0, i_ref, 0, 0 );
        x264_mb_predict_mv( mb, 1, 0, 0, mvp );

        mvx = mvp[0]; mvy = mvp[1];
        i_sad = h->me( h, ctx->p_fref1[i_ref][0], ctx->i_fref1[i_ref][0],
                          ctx->p_img[0],         ctx->i_img[0],
                          PIXEL_16x16, res->i_lambda, &mvx, &mvy );
        i_sad += res->i_lambda * bs_size_te( h->sh.i_num_ref_idx_l1_active - 1, i_ref );

        if( res->i_sad_b16x16_l1 == -1 || i_sad < res->i_sad_b16x16_l1 )
        {
            res->i_sad_b16x16_l1   = i_sad;
            res->i_ref_b16x16_l1   = i_ref;
            res->i_mv_b16x16_l1[0] = mvx;
            res->i_mv_b16x16_l1[1] = mvy;
        }
    }

    /* calculate i_sad_b16x16_bi */
    h->mc[MC_LUMA]( ctx->p_fref0[res->i_ref_b16x16_l0][0], ctx->i_fref0[res->i_ref_b16x16_l0][0],
                    pix1, 16,
                    res->i_mv_b16x16_l0[0],
                    res->i_mv_b16x16_l0[1],
                    16, 16 );
    h->mc[MC_LUMA]( ctx->p_fref1[res->i_ref_b16x16_l1][0], ctx->i_fref1[res->i_ref_b16x16_l1][0],
                    pix2, 16,
                    res->i_mv_b16x16_l1[0],
                    res->i_mv_b16x16_l1[1],
                    16, 16 );
    h->pixf.avg[PIXEL_16x16]( pix1, 16, pix2, 16 );

    res->i_sad_b16x16_bi = h->pixf.sad[PIXEL_16x16]( ctx->p_img[0], ctx->i_img[0], pix1, 16 );
    x264_mb_partition_set( mb, 0, 0, 0, res->i_ref_b16x16_l0, res->i_mv_b16x16_l0[0], res->i_mv_b16x16_l0[1] );
    x264_mb_partition_set( mb, 1, 0, 0, res->i_ref_b16x16_l1, res->i_mv_b16x16_l1[0], res->i_mv_b16x16_l1[1] );

    x264_mb_predict_mv( mb, 0, 0, 0, mvp );
    res->i_sad_b16x16_bi += res->i_lambda * bs_size_te( h->sh.i_num_ref_idx_l0_active - 1, res->i_ref_b16x16_l0 );

    x264_mb_predict_mv( mb, 1, 0, 0, mvp );
    res->i_sad_b16x16_bi += res->i_lambda * bs_size_te( h->sh.i_num_ref_idx_l1_active - 1, res->i_ref_b16x16_l1 );

#if 0
    /* Now do the rafinement (using the ref found in 16x16 mode) */
    i_ref0 = res->i_ref_b16x16_l0;
    i_ref1 = res->i_ref_b16x16_l1;
    x264_mb_partition_set( mb, 0, 0, 0, i_ref0, 0, 0 );
    x264_mb_partition_set( mb, 1, 0, 0, i_ref1, 0, 0 );

    /* now do 16x8 */
#endif
}

/*****************************************************************************
 * x264_macroblock_analyse:
 *****************************************************************************/
void x264_macroblock_analyse( x264_t *h, x264_macroblock_t *mb )
{
    x264_mb_analysis_t analysis;
    int i;

    /* qp TODO */
    mb->i_qp = x264_clip3( h->pps->i_pic_init_qp + h->sh.i_qp_delta + 0, 0, 51 );

    /* init analysis */
    analysis.i_qp = mb->i_qp;
    analysis.i_lambda = i_qp0_cost_table[analysis.i_qp];

    /*--------------------------- Do the analysis ---------------------------*/
    x264_mb_analyse_intra( h, mb, &analysis );
    if( h->sh.i_type == SLICE_TYPE_P )
    {
        x264_mb_analyse_inter_p( h, mb, &analysis );
    }
    else if( h->sh.i_type == SLICE_TYPE_B )
    {
        x264_mb_analyse_inter_b( h, mb, &analysis );
    }

    /*-------------------- Chose the macroblock mode ------------------------*/
#define BEST_TYPE( type, partition, satd ) \
        if( satd != -1 && satd < i_satd ) \
        {   \
            i_satd = satd;  \
            mb->i_type = type; \
            mb->i_partition = partition; \
        }
    if( h->sh.i_type == SLICE_TYPE_I )
    {
        mb->i_type = analysis.i_sad_i4x4 < analysis.i_sad_i16x16 ? I_4x4 : I_16x16;
    }
    else if( h->sh.i_type == SLICE_TYPE_P )
    {
        int i_satd = analysis.i_sad_i4x4;
        mb->i_type = I_4x4;

        BEST_TYPE( I_16x16, -1,    analysis.i_sad_i16x16 );
        BEST_TYPE( P_L0,  D_16x16, analysis.i_sad_p16x16 );
        BEST_TYPE( P_L0,  D_16x8 , analysis.i_sad_p16x8  );
        BEST_TYPE( P_L0,  D_8x16 , analysis.i_sad_p8x16  );
        BEST_TYPE( P_8x8, D_8x8  , analysis.i_sad_p8x8   );
    }
    else    /* B */
    {
        int i_satd = analysis.i_sad_i4x4;
        mb->i_type = I_4x4;

        BEST_TYPE( I_16x16, -1,      analysis.i_sad_i16x16 );
        BEST_TYPE( B_L0_L0, D_16x16, analysis.i_sad_b16x16_l0 );
        BEST_TYPE( B_L1_L1, D_16x16, analysis.i_sad_b16x16_l1 );
        BEST_TYPE( B_BI_BI, D_16x16, analysis.i_sad_b16x16_bi );
    }
#undef BEST_TYPE

    if( IS_INTRA( mb->i_type ) )
    {
        x264_mb_analyse_intra_chroma( h, mb, &analysis );
    }

    /*-------------------- Update MB from the analysis ----------------------*/
    switch( mb->i_type )
    {
        case I_4x4:
            for( i = 0; i < 16; i++ )
            {
                mb->block[i].i_intra4x4_pred_mode = analysis.i_predict4x4[block_idx_x[i]][block_idx_y[i]];
            }
            mb->i_chroma_pred_mode = analysis.i_predict8x8;
            break;
        case I_16x16:
            mb->i_intra16x16_pred_mode = analysis.i_predict16x16;
            mb->i_chroma_pred_mode = analysis.i_predict8x8;
            break;
        case P_L0:
            switch( mb->i_partition )
            {
                case D_16x16:
                    x264_mb_partition_set( mb, 0, 0, 0, analysis.i_ref_p16x16, analysis.i_mv_p16x16[0], analysis.i_mv_p16x16[1] );
                    break;
                case D_16x8:
                    x264_mb_partition_set( mb, 0, 0, 0, analysis.i_ref_p16x8, analysis.i_mv_p16x8[0][0], analysis.i_mv_p16x8[0][1] );
                    x264_mb_partition_set( mb, 0, 1, 0, analysis.i_ref_p16x8, analysis.i_mv_p16x8[1][0], analysis.i_mv_p16x8[1][1] );
                    break;
                case D_8x16:
                    x264_mb_partition_set( mb, 0, 0, 0, analysis.i_ref_p8x16, analysis.i_mv_p8x16[0][0], analysis.i_mv_p8x16[0][1] );
                    x264_mb_partition_set( mb, 0, 1, 0, analysis.i_ref_p8x16, analysis.i_mv_p8x16[1][0], analysis.i_mv_p8x16[1][1] );
                    break;
                default:
                    fprintf( stderr, "internal error\n" );
                    break;
            }
            break;

        case P_8x8:
            for( i = 0; i < 4; i++ )
            {
                int i_sub;

                mb->i_sub_partition[i] = analysis.i_sub_partition_p8x8[i];
                for( i_sub = 0; i_sub < x264_mb_partition_count_table[mb->i_sub_partition[i]]; i_sub++ )
                {
                    x264_mb_partition_set( mb, 0, i, i_sub,
                                                   analysis.i_ref_p8x8,
                                                   analysis.i_mv_p8x8[i][i_sub][0],
                                                   analysis.i_mv_p8x8[i][i_sub][1] );
                }
            }
            break;
        case B_L0_L0:
            switch( mb->i_partition )
            {
                case D_16x16:
                    x264_mb_partition_set( mb, 0, 0, 0, analysis.i_ref_b16x16_l0, analysis.i_mv_b16x16_l0[0], analysis.i_mv_b16x16_l0[1] );
                    /* l1 not used/avaiable */
                    x264_mb_partition_set( mb, 1, 0, 0, -1, 0, 0 );
                    break;
                default:
                    fprintf( stderr, "internal error\n" );
                    break;
            }
            break;
        case B_L1_L1:
            switch( mb->i_partition )
            {
                case D_16x16:
                    /* l0 not used/avaiable */
                    x264_mb_partition_set( mb, 0, 0, 0, -1, 0, 0 );
                    x264_mb_partition_set( mb, 1, 0, 0, analysis.i_ref_b16x16_l1, analysis.i_mv_b16x16_l1[0], analysis.i_mv_b16x16_l1[1] );
                    break;

                default:
                    fprintf( stderr, "internal error\n" );
                    break;
            }
            break;
        case B_BI_BI:
            switch( mb->i_partition )
            {
                case D_16x16:
                    x264_mb_partition_set( mb, 0, 0, 0, analysis.i_ref_b16x16_l0, analysis.i_mv_b16x16_l0[0], analysis.i_mv_b16x16_l0[1] );
                    x264_mb_partition_set( mb, 1, 0, 0, analysis.i_ref_b16x16_l1, analysis.i_mv_b16x16_l1[0], analysis.i_mv_b16x16_l1[1] );
                    break;

                default:
                    fprintf( stderr, "internal error\n" );
                    break;
            }
            break;

        default:
            fprintf( stderr, "internal error\n" );
            break;
    }
}

