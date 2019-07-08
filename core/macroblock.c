
/*****************************************************************************
 * macroblock.c: h264 encoder library

 *****************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "common.h"

static const uint8_t block_idx_x[16] ={
    0, 1, 0, 1, 2, 3, 2, 3, 0, 1, 0, 1, 2, 3, 2, 3
};
static const uint8_t block_idx_y[16] ={
    0, 0, 1, 1, 0, 0, 1, 1, 2, 2, 3, 3, 2, 2, 3, 3
};
static const uint8_t block_idx_xy[4][4] ={
    { 0, 2, 8, 10},
    { 1, 3, 9, 11},
    { 4, 6, 12, 14},
    { 5, 7, 13, 15}
};

static const int dequant_mf[6][4][4] ={
    {
        {10, 13, 10, 13},
        {13, 16, 13, 16},
        {10, 13, 10, 13},
        {13, 16, 13, 16}
    },
    {
        {11, 14, 11, 14},
        {14, 18, 14, 18},
        {11, 14, 11, 14},
        {14, 18, 14, 18}
    },
    {
        {13, 16, 13, 16},
        {16, 20, 16, 20},
        {13, 16, 13, 16},
        {16, 20, 16, 20}
    },
    {
        {14, 18, 14, 18},
        {18, 23, 18, 23},
        {14, 18, 14, 18},
        {18, 23, 18, 23}
    },
    {
        {16, 20, 16, 20},
        {20, 25, 20, 25},
        {16, 20, 16, 20},
        {20, 25, 20, 25}
    },
    {
        {18, 23, 18, 23},
        {23, 29, 23, 29},
        {18, 23, 18, 23},
        {23, 29, 23, 29}
    }
};

#if 0
static const int i_chroma_qp_table[52] ={
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9,
    10, 11, 12, 13, 14, 15, 16, 17, 18, 19,
    20, 21, 22, 23, 24, 25, 26, 27, 28, 29,
    29, 30, 31, 32, 32, 33, 34, 34, 35, 35,
    36, 36, 37, 37, 37, 38, 38, 38, 39, 39,
    39, 39
};
#endif

/****************************************************************************
 * x264_macroblocks_new: allocate and init macroblock.
 ****************************************************************************/
x264_macroblock_t *x264_macroblocks_new(int i_mb_width, int i_mb_height) {LOG_CALL;
    x264_macroblock_t *mb =( x264_macroblock_t *) x264_aligned_alloc(i_mb_width * i_mb_height * sizeof ( x264_macroblock_t));
    int x, y;

    for (y = 0; y < i_mb_height; y++)
    {
        for (x = 0; x < i_mb_width; x++)
        {
            int i_mb_xy = y * i_mb_width + x;

            mb[i_mb_xy].i_mb_x = x;
            mb[i_mb_xy].i_mb_y = y;

            mb[i_mb_xy].i_neighbour = 0;
            mb[i_mb_xy].i_neighbour |= (x > 0) ? MB_LEFT : 0x00;
            mb[i_mb_xy].i_neighbour |= (y > 0) ? MB_TOP : 0x00;
            mb[i_mb_xy].i_neighbour |= (y > 0 && x + 1 < i_mb_width) ? MB_TOPRIGHT : 0;
        }
    }

    return mb;
}

int x264_mb_predict_intra4x4_mode(x264_t *h, x264_macroblock_t *mb, int idx) {LOG_CALL;
    x264_macroblock_t *mba = mb->context->block[idx].mba;
    x264_macroblock_t *mbb = mb->context->block[idx].mbb;

    int i_mode_a = I_PRED_4x4_DC;
    int i_mode_b = I_PRED_4x4_DC;

    if (!mba || !mbb)
    {
        return I_PRED_4x4_DC;
    }

    if (mba->i_type == I_4x4)
    {
        i_mode_a = mb->context->block[idx].bka->i_intra4x4_pred_mode;
    }
    if (mbb->i_type == I_4x4)
    {
        i_mode_b = mb->context->block[idx].bkb->i_intra4x4_pred_mode;
    }

    return X264_MIN(i_mode_a, i_mode_b);
}

int x264_mb_predict_non_zero_code(x264_t *h, x264_macroblock_t *mb, int idx) {LOG_CALL;
    int i_z_a = 0x80, i_z_b = 0x80;
    int i_ret;

    /* none avail -> 0, one avail -> this one, both -> (a+b+1)>>1 */
    if (mb->context->block[idx].mba)
    {
        i_z_a = mb->context->block[idx].bka->i_non_zero_count;
    }
    if (mb->context->block[idx].mbb)
    {
        i_z_b = mb->context->block[idx].bkb->i_non_zero_count;
    }

    i_ret = i_z_a + i_z_b;
    if (i_ret < 0x80)
    {
        i_ret = (i_ret + 1) >> 1;
    }
    return i_ret & 0x7f;
}

/****************************************************************************
 * Scan and Quant functions
 ****************************************************************************/
void x264_mb_dequant_2x2_dc(int16_t dct[2][2], int i_qscale) {LOG_CALL;
    const int i_qbits = i_qscale / 6 - 1;

    if (i_qbits >= 0)
    {
        const int i_dmf = dequant_mf[i_qscale % 6][0][0] << i_qbits;

        dct[0][0] = dct[0][0] * i_dmf;
        dct[0][1] = dct[0][1] * i_dmf;
        dct[1][0] = dct[1][0] * i_dmf;
        dct[1][1] = dct[1][1] * i_dmf;
    } else
    {
        const int i_dmf = dequant_mf[i_qscale % 6][0][0];

        dct[0][0] = (dct[0][0] * i_dmf) >> 1;
        dct[0][1] = (dct[0][1] * i_dmf) >> 1;
        dct[1][0] = (dct[1][0] * i_dmf) >> 1;
        dct[1][1] = (dct[1][1] * i_dmf) >> 1;
    }
}

void x264_mb_dequant_4x4_dc(int16_t dct[4][4], int i_qscale) {LOG_CALL;
    const int i_qbits = i_qscale / 6 - 2;
    int x, y;

    if (i_qbits >= 0)
    {
        const int i_dmf = dequant_mf[i_qscale % 6][0][0] << i_qbits;

        for (y = 0; y < 4; y++)
        {
            for (x = 0; x < 4; x++)
            {
                dct[y][x] = dct[y][x] * i_dmf;
            }
        }
    } else
    {
        const int i_dmf = dequant_mf[i_qscale % 6][0][0];
        const int f = 1 << (1 + i_qbits);

        for (y = 0; y < 4; y++)
        {
            for (x = 0; x < 4; x++)
            {
                dct[y][x] = (dct[y][x] * i_dmf + f) >> (-i_qbits);
            }
        }
    }
}

void x264_mb_dequant_4x4(int16_t dct[4][4], int i_qscale) {LOG_CALL;
    const int i_mf = i_qscale % 6;
    const int i_qbits = i_qscale / 6;
    int y;

    for (y = 0; y < 4; y++)
    {
        dct[y][0] = (dct[y][0] * dequant_mf[i_mf][y][0]) << i_qbits;
        dct[y][1] = (dct[y][1] * dequant_mf[i_mf][y][1]) << i_qbits;
        dct[y][2] = (dct[y][2] * dequant_mf[i_mf][y][2]) << i_qbits;
        dct[y][3] = (dct[y][3] * dequant_mf[i_mf][y][3]) << i_qbits;
    }
}

void x264_mb_partition_set(x264_macroblock_t *mb, int i_list, int i_part, int i_sub, int i_ref, int mx, int my) {LOG_CALL;
    int x, y;
    int w, h;
    int dx, dy;

    x264_mb_partition_getxy(mb, i_part, i_sub, &x, &y);
    x264_mb_partition_size(mb, i_part, i_sub, &w, &h);

    for (dx = 0; dx < w; dx++)
    {
        for (dy = 0; dy < h; dy++)
        {
            mb->partition[x + dx][y + dy].i_ref[i_list] = i_ref;
            mb->partition[x + dx][y + dy].mv[i_list][0] = mx;
            mb->partition[x + dx][y + dy].mv[i_list][1] = my;
        }
    }
}

void x264_mb_partition_get(x264_macroblock_t *mb, int i_list, int i_part, int i_sub, int *pi_ref, int *pi_mx, int *pi_my) {LOG_CALL;
    int x, y;

    x264_mb_partition_getxy(mb, i_part, i_sub, &x, &y);

    if (pi_ref)
    {
        *pi_ref = mb->partition[x][y].i_ref[i_list];
    }
    if (pi_mx && pi_my)
    {
        *pi_mx = mb->partition[x][y].mv[i_list][0];
        *pi_my = mb->partition[x][y].mv[i_list][1];
    }
}

/* ARrrrg so unbeautifull, and unoptimised for common case */
void x264_mb_predict_mv(x264_macroblock_t *mb, int i_list, int i_part, int i_subpart, int mv[2]) {LOG_CALL;
    int x, y, xn, yn;
    int w, h;
    int i_ref;

    int i_refa = -1;
    int i_refb = -1;
    int i_refc = -1;

    int mvxa = 0, mvxb = 0, mvxc = 0;
    int mvya = 0, mvyb = 0, mvyc = 0;

    x264_macroblock_t *mbn;


    x264_mb_partition_getxy(mb, i_part, i_subpart, &x, &y);
    x264_mb_partition_size(mb, i_part, i_subpart, &w, &h);
    i_ref = mb->partition[x][y].i_ref[i_list];

    /* Left  pixel (-1,0)*/
    xn = x - 1;
    mbn = mb;
    if (xn < 0)
    {
        xn += 4;
        mbn = mb->context->mba;
    }
    if (mbn)
    {
        i_refa = -2;
        if (!IS_INTRA(mbn->i_type) && mbn->partition[xn][y].i_ref[i_list] != -1)
        {
            i_refa = mbn->partition[xn][y].i_ref[i_list];
            mvxa = mbn->partition[xn][y].mv[i_list][0];
            mvya = mbn->partition[xn][y].mv[i_list][1];
        }
    }

    /* Up ( pixel(0,-1)*/
    yn = y - 1;
    mbn = mb;
    if (yn < 0)
    {
        yn += 4;
        mbn = mb->context->mbb;
    }
    if (mbn)
    {
        i_refb = -2;
        if (!IS_INTRA(mbn->i_type) && mbn->partition[x][yn].i_ref[i_list] != -1)
        {
            i_refb = mbn->partition[x][yn].i_ref[i_list];
            mvxb = mbn->partition[x][yn].mv[i_list][0];
            mvyb = mbn->partition[x][yn].mv[i_list][1];
        }
    }

    /* Up right pixel(width,-1)*/
    xn = x + w;
    yn = y - 1;

    mbn = mb;
    if (yn < 0 && xn >= 4)
    {
        if (mb->context->mbc)
        {
            xn -= 4;
            yn += 4;
            mbn = mb->context->mbc;
        } else
        {
            mbn = NULL;
        }
    } else if (yn < 0)
    {
        yn += 4;
        mbn = mb->context->mbb;
    } else if (xn >= 4 || (xn == 2 && (yn == 0 || yn == 2)))
    {
        mbn = NULL; /* not yet decoded */
    }

    if (mbn == NULL)
    {
        /* load top left pixel(-1,-1) */
        xn = x - 1;
        yn = y - 1;

        mbn = mb;
        if (yn < 0 && xn < 0)
        {
            if (mb->context->mba && mb->context->mbb)
            {
                xn += 4;
                yn += 4;
                mbn = mb->context->mbb - 1;
            } else
            {
                mbn = NULL;
            }
        } else if (yn < 0)
        {
            yn += 4;
            mbn = mb->context->mbb;
        } else if (xn < 0)
        {
            xn += 4;
            mbn = mb->context->mba;
        }
    }

    if (mbn)
    {
        i_refc = -2;
        if (!IS_INTRA(mbn->i_type) && mbn->partition[xn][yn].i_ref[i_list] != -1)
        {
            i_refc = mbn->partition[xn][yn].i_ref[i_list];
            mvxc = mbn->partition[xn][yn].mv[i_list][0];
            mvyc = mbn->partition[xn][yn].mv[i_list][1];
        }
    }

    if (mb->i_partition == D_16x8 && i_part == 0 && i_refb == i_ref)
    {
        mv[0] = mvxb;
        mv[1] = mvyb;
    } else if (mb->i_partition == D_16x8 && i_part == 1 && i_refa == i_ref)
    {
        mv[0] = mvxa;
        mv[1] = mvya;
    } else if (mb->i_partition == D_8x16 && i_part == 0 && i_refa == i_ref)
    {
        mv[0] = mvxa;
        mv[1] = mvya;
    } else if (mb->i_partition == D_8x16 && i_part == 1 && i_refc == i_ref)
    {
        mv[0] = mvxc;
        mv[1] = mvyc;
    } else
    {
        int i_count;

        i_count = 0;
        if (i_refa == i_ref) i_count++;
        if (i_refb == i_ref) i_count++;
        if (i_refc == i_ref) i_count++;

        if (i_count > 1)
        {
            mv[0] = x264_median(mvxa, mvxb, mvxc);
            mv[1] = x264_median(mvya, mvyb, mvyc);
        } else if (i_count == 1)
        {
            if (i_refa == i_ref)
            {
                mv[0] = mvxa;
                mv[1] = mvya;
            } else if (i_refb == i_ref)
            {
                mv[0] = mvxb;
                mv[1] = mvyb;
            } else
            {
                mv[0] = mvxc;
                mv[1] = mvyc;
            }
        } else if (i_refb == -1 && i_refc == -1 && i_refa != -1)
        {
            mv[0] = mvxa;
            mv[1] = mvya;
        } else
        {
            mv[0] = x264_median(mvxa, mvxb, mvxc);
            mv[1] = x264_median(mvya, mvyb, mvyc);
        }
    }
}

void x264_mb_predict_mv_pskip(x264_macroblock_t *mb, int mv[2]) {LOG_CALL;
    int x, y, xn, yn;

    int i_refa = -1;
    int i_refb = -1;

    int mvxa = 0, mvxb = 0;
    int mvya = 0, mvyb = 0;

    x264_macroblock_t *mbn;


    x264_mb_partition_getxy(mb, 0, 0, &x, &y);

    /* Left  pixel (-1,0)*/
    xn = x - 1;
    mbn = mb;
    if (xn < 0)
    {
        xn += 4;
        mbn = mb->context->mba;
    }
    if (mbn)
    {
        i_refa = -2;
        if (!IS_INTRA(mbn->i_type) && mbn->partition[xn][y].i_ref[0] != -1)
        {
            i_refa = mbn->partition[xn][y].i_ref[0];
            mvxa = mbn->partition[xn][y].mv[0][0];
            mvya = mbn->partition[xn][y].mv[0][1];
        }
    }

    /* Up ( pixel(0,-1)*/
    yn = y - 1;
    mbn = mb;
    if (yn < 0)
    {
        yn += 4;
        mbn = mb->context->mbb;
    }
    if (mbn)
    {
        i_refb = -2;
        if (!IS_INTRA(mbn->i_type) && mbn->partition[x][yn].i_ref[0] != -1)
        {
            i_refb = mbn->partition[x][yn].i_ref[0];
            mvxb = mbn->partition[x][yn].mv[0][0];
            mvyb = mbn->partition[x][yn].mv[0][1];
        }
    }

    if (i_refa == -1 || i_refb == -1 ||
            (i_refa == 0 && mvxa == 0 && mvya == 0) ||
            (i_refb == 0 && mvxb == 0 && mvyb == 0))
    {
        mv[0] = 0;
        mv[1] = 0;
    } else
    {
        x264_mb_predict_mv(mb, 0, 0, 0, mv);
    }
}

static inline void x264_mb_mc_partition_lx(x264_t *h, x264_macroblock_t *mb, int i_list, int i_part, int i_sub) {LOG_CALL;
    const x264_mb_context_t *ctx = mb->context;

    int mx, my;
    int i_ref;
    int i_width, i_height;
    int x, y;
    int ch;
    int i_dst;
    uint8_t *p_dst;
    int i_src;
    uint8_t *p_src;

    x264_mb_partition_get(mb, i_list, i_part, i_sub, &i_ref, &mx, &my);
    x264_mb_partition_getxy(mb, i_part, i_sub, &x, &y);
    x264_mb_partition_size(mb, i_part, i_sub, &i_width, &i_height);

    if ((i_list == 0 && i_ref > h->i_ref0) ||
            (i_list == 1 && i_ref > h->i_ref1) ||
            i_ref < 0)
    {
        fprintf(stderr, "invalid ref frame\n");
        return;
    }

    i_dst = ctx->i_fdec[0];
    p_dst = ctx->p_fdec[0];
    if (i_list == 0)
    {
        i_src = ctx->i_fref0[i_ref][0];
        p_src = ctx->p_fref0[i_ref][0];
    } else
    {
        i_src = ctx->i_fref1[i_ref][0];
        p_src = ctx->p_fref1[i_ref][0];
    }
    h->mc[MC_LUMA](&p_src[4 * (x + y * i_src)], i_src,
            &p_dst[4 * (x + y * i_dst)], i_dst,
            mx, my, 4 * i_width, 4 * i_height);


    for (ch = 0; ch < 2; ch++)
    {
        i_dst = ctx->i_fdec[1 + ch];
        p_dst = ctx->p_fdec[1 + ch];
        if (i_list == 0)
        {
            i_src = ctx->i_fref0[i_ref][1 + ch];
            p_src = ctx->p_fref0[i_ref][1 + ch];
        } else
        {
            i_src = ctx->i_fref1[i_ref][1 + ch];
            p_src = ctx->p_fref1[i_ref][1 + ch];
        }
        h->mc[MC_CHROMA](&p_src[2 * (x + y * i_src)], i_src,
                &p_dst[2 * (x + y * i_dst)], i_dst,
                mx, my, 2 * i_width, 2 * i_height);
    }
}

static void x264_mb_mc_partition_bi(x264_t *h, x264_macroblock_t *mb, int i_part, int i_sub) {LOG_CALL;
    const x264_mb_context_t *ctx = mb->context;

    int mx, my;
    int i_ref;
    int i_width, i_height;
    int x, y;
    int i_dst;
    uint8_t *p_dst;
    int i_src;
    uint8_t *p_src;

    int ch;

    uint8_t tmp[16 * 16];

    x264_mb_partition_getxy(mb, i_part, i_sub, &x, &y);
    x264_mb_partition_size(mb, i_part, i_sub, &i_width, &i_height);

    /* first do l0 */
    x264_mb_partition_get(mb, 0, i_part, i_sub, &i_ref, &mx, &my);
    if (i_ref > h->i_ref0 || i_ref < 0)
    {
        fprintf(stderr, "invalid ref frame\n");
        return;
    }
    i_dst = ctx->i_fdec[0];
    p_dst = ctx->p_fdec[0];
    i_src = ctx->i_fref0[i_ref][0];
    p_src = ctx->p_fref0[i_ref][0];
    h->mc[MC_LUMA](&p_src[4 * (x + y * i_src)], i_src,
            &p_dst[4 * (x + y * i_dst)], i_dst,
            mx, my, 4 * i_width, 4 * i_height);
    for (ch = 0; ch < 2; ch++)
    {
        i_dst = ctx->i_fdec[1 + ch];
        p_dst = ctx->p_fdec[1 + ch];
        i_src = ctx->i_fref0[i_ref][1 + ch];
        p_src = ctx->p_fref0[i_ref][1 + ch];

        h->mc[MC_CHROMA](&p_src[2 * (x + y * i_src)], i_src,
                &p_dst[2 * (x + y * i_dst)], i_dst,
                mx, my, 2 * i_width, 2 * i_height);
    }


    /* next avg with l1 */
    x264_mb_partition_get(mb, 1, i_part, i_sub, &i_ref, &mx, &my);
    if (i_ref > h->i_ref1 || i_ref < 0)
    {
        fprintf(stderr, "invalid ref frame\n");
        return;
    }

    i_dst = ctx->i_fdec[0];
    p_dst = ctx->p_fdec[0];
    i_src = ctx->i_fref1[i_ref][0];
    p_src = ctx->p_fref1[i_ref][0];
    h->mc[MC_LUMA](&p_src[4 * (x + y * i_src)], i_src,
            tmp, 16,
            mx, my, 4 * i_width, 4 * i_height);

    if (mb->i_partition == D_16x16)
    {
        h->pixf.avg[PIXEL_16x16](&p_dst[4 * (x + y * i_dst)], i_dst, tmp, 16);
    } else if (mb->i_partition == D_16x8)
    {
        h->pixf.avg[PIXEL_16x8](&p_dst[4 * (x + y * i_dst)], i_dst, tmp, 16);
    } else if (mb->i_partition == D_8x16)
    {
        h->pixf.avg[PIXEL_8x16](&p_dst[4 * (x + y * i_dst)], i_dst, tmp, 16);
    } else
    {
        fprintf(stderr, "MC BI with D_8x8 unsupported\n");
    }
    for (ch = 0; ch < 2; ch++)
    {
        i_dst = ctx->i_fdec[1 + ch];
        p_dst = ctx->p_fdec[1 + ch];
        i_src = ctx->i_fref1[i_ref][1 + ch];
        p_src = ctx->p_fref1[i_ref][1 + ch];

        h->mc[MC_CHROMA](&p_src[2 * (x + y * i_src)], i_src,
                tmp, 8,
                mx, my, 2 * i_width, 2 * i_height);

        if (mb->i_partition == D_16x16)
        {
            h->pixf.avg[PIXEL_8x8](&p_dst[2 * (x + y * i_dst)], i_dst, tmp, 8);
        } else if (mb->i_partition == D_16x8)
        {
            h->pixf.avg[PIXEL_8x4](&p_dst[2 * (x + y * i_dst)], i_dst, tmp, 8);
        } else if (mb->i_partition == D_8x16)
        {
            h->pixf.avg[PIXEL_4x8](&p_dst[2 * (x + y * i_dst)], i_dst, tmp, 8);
        } else
        {
            fprintf(stderr, "MC BI with D_8x8 unsupported\n");
        }
    }
}

void x264_mb_mc(x264_t *h, x264_macroblock_t *mb) {LOG_CALL;
    const x264_mb_context_t *ctx = mb->context;

    if (mb->i_type == P_L0)
    {
        const int i_ref0 = mb->partition[0][0].i_ref[0];
        const int mvx0 = mb->partition[0][0].mv[0][0];
        const int mvy0 = mb->partition[0][0].mv[0][1];


        if (mb->i_partition == D_16x16)
        {
            h->mc[MC_LUMA](ctx->p_fref0[i_ref0][0], ctx->i_fref0[i_ref0][0],
                    ctx->p_fdec[0], ctx->i_fdec[0],
                    mvx0, mvy0, 16, 16);

            h->mc[MC_CHROMA](ctx->p_fref0[i_ref0][1], ctx->i_fref0[i_ref0][1],
                    ctx->p_fdec[1], ctx->i_fdec[1],
                    mvx0, mvy0, 8, 8);
            h->mc[MC_CHROMA](ctx->p_fref0[i_ref0][2], ctx->i_fref0[i_ref0][2],
                    ctx->p_fdec[2], ctx->i_fdec[2],
                    mvx0, mvy0, 8, 8);
        } else if (mb->i_partition == D_16x8)
        {
            const int i_ref1 = mb->partition[0][2].i_ref[0];
            const int mvx1 = mb->partition[0][2].mv[0][0];
            const int mvy1 = mb->partition[0][2].mv[0][1];

            h->mc[MC_LUMA](ctx->p_fref0[i_ref0][0], ctx->i_fref0[i_ref0][0],
                    ctx->p_fdec[0], ctx->i_fdec[0],
                    mvx0, mvy0, 16, 8);
            h->mc[MC_CHROMA](ctx->p_fref0[i_ref0][1], ctx->i_fref0[i_ref0][1],
                    ctx->p_fdec[1], ctx->i_fdec[1],
                    mvx0, mvy0, 8, 4);
            h->mc[MC_CHROMA](ctx->p_fref0[i_ref0][2], ctx->i_fref0[i_ref0][2],
                    ctx->p_fdec[2], ctx->i_fdec[2],
                    mvx0, mvy0, 8, 4);


            h->mc[MC_LUMA](&ctx->p_fref0[i_ref1][0][8 * ctx->i_fref0[i_ref1][0]], ctx->i_fref0[i_ref1][0],
                    &ctx->p_fdec[0][8 * ctx->i_fdec[0]], ctx->i_fdec[0],
                    mvx1, mvy1, 16, 8);
            h->mc[MC_CHROMA](&ctx->p_fref0[i_ref1][1][4 * ctx->i_fref0[i_ref1][1]], ctx->i_fref0[i_ref1][1],
                    &ctx->p_fdec[1][4 * ctx->i_fdec[1]], ctx->i_fdec[1],
                    mvx1, mvy1, 8, 4);
            h->mc[MC_CHROMA](&ctx->p_fref0[i_ref1][2][4 * ctx->i_fref0[i_ref1][2]], ctx->i_fref0[i_ref1][2],
                    &ctx->p_fdec[2][4 * ctx->i_fdec[2]], ctx->i_fdec[2],
                    mvx1, mvy1, 8, 4);
        } else if (mb->i_partition == D_8x16)
        {
            const int i_ref1 = mb->partition[2][0].i_ref[0];
            const int mvx1 = mb->partition[2][0].mv[0][0];
            const int mvy1 = mb->partition[2][0].mv[0][1];

            h->mc[MC_LUMA](ctx->p_fref0[i_ref0][0], ctx->i_fref0[i_ref0][0],
                    ctx->p_fdec[0], ctx->i_fdec[0],
                    mvx0, mvy0, 8, 16);
            h->mc[MC_CHROMA](ctx->p_fref0[i_ref0][1], ctx->i_fref0[i_ref0][1],
                    ctx->p_fdec[1], ctx->i_fdec[1],
                    mvx0, mvy0, 4, 8);
            h->mc[MC_CHROMA](ctx->p_fref0[i_ref0][2], ctx->i_fref0[i_ref0][2],
                    ctx->p_fdec[2], ctx->i_fdec[2],
                    mvx0, mvy0, 4, 8);

            h->mc[MC_LUMA](&ctx->p_fref0[i_ref1][0][8], ctx->i_fref0[i_ref1][0],
                    &ctx->p_fdec[0][8], ctx->i_fdec[0],
                    mvx1, mvy1, 8, 16);
            h->mc[MC_CHROMA](&ctx->p_fref0[i_ref1][1][4], ctx->i_fref0[i_ref1][1],
                    &ctx->p_fdec[1][4], ctx->i_fdec[1],
                    mvx1, mvy1, 4, 8);
            h->mc[MC_CHROMA](&ctx->p_fref0[i_ref1][2][4], ctx->i_fref0[i_ref1][2],
                    &ctx->p_fdec[2][4], ctx->i_fdec[2],
                    mvx1, mvy1, 4, 8);
        }
    } else if (mb->i_type == P_8x8)
    {
        int i_part;
        int i_sub;

        for (i_part = 0; i_part < 4; i_part++)
        {
            for (i_sub = 0; i_sub < x264_mb_partition_count_table[mb->i_sub_partition[i_part]]; i_sub++)
            {
                x264_mb_mc_partition_lx(h, mb, 0, i_part, i_sub);
            }
        }
    } else if (mb->i_type == B_8x8 || mb->i_type == B_DIRECT)
    {
        fprintf(stderr, "mc_luma with unsupported mb\n");
        return;
    } else /* B_*x* */
    {
        int b_list0[2];
        int b_list1[2];

        int i;

        /* init ref list utilisations */
        for (i = 0; i < 2; i++)
        {
            b_list0[i] = x264_mb_type_list0_table[mb->i_type][i];
            b_list1[i] = x264_mb_type_list1_table[mb->i_type][i];
        }

        for (i = 0; i < x264_mb_partition_count_table[mb->i_partition]; i++)
        {
            if (b_list0[i] && b_list1[i])
            {
                x264_mb_mc_partition_bi(h, mb, i, 0);
            } else if (b_list0[i])
            {
                x264_mb_mc_partition_lx(h, mb, 0, i, 0);
            } else if (b_list1[i])
            {
                x264_mb_mc_partition_lx(h, mb, 1, i, 0);
            }
        }
    }
}

/*****************************************************************************
 * x264_macroblock_neighbour_load:
 *****************************************************************************/
void x264_macroblock_context_load(x264_t *h, x264_macroblock_t *mb, x264_mb_context_t *context) {LOG_CALL;
    int ch;
    int i, j;

    x264_macroblock_t *a = NULL;
    x264_macroblock_t *b = NULL;

    mb->context = context;

    if (mb->i_neighbour & MB_LEFT)
    {
        a = mb - 1;
    }
    context->mba = a;

    if (mb->i_neighbour & MB_TOP)
    {
        b = mb - h->sps->i_mb_width;
    }
    context->mbb = b;

    context->mbc = NULL;
    if (mb->i_neighbour & MB_TOPRIGHT)
    {
        context->mbc = b + 1;
    }

#define LOAD_PTR( dst, src ) \
    context->p_##dst[0] = (src)->plane[0] + 16 * ( mb->i_mb_x + mb->i_mb_y * (src)->i_stride[0] ); \
    context->p_##dst[1] = (src)->plane[1] +  8 * ( mb->i_mb_x + mb->i_mb_y * (src)->i_stride[1] ); \
    context->p_##dst[2] = (src)->plane[2] +  8 * ( mb->i_mb_x + mb->i_mb_y * (src)->i_stride[2] ); \
    context->i_##dst[0] = (src)->i_stride[0]; \
    context->i_##dst[1] = (src)->i_stride[1]; \
    context->i_##dst[2] = (src)->i_stride[2]

    LOAD_PTR(img, h->picture);
    LOAD_PTR(fdec, h->fdec);
    for (i = 0; i < h->i_ref0; i++)
    {
        LOAD_PTR(fref0[i], h->fref0[i]);
    }
    for (i = 0; i < h->i_ref1; i++)
    {
        LOAD_PTR(fref1[i], h->fref1[i]);
    }
#undef LOAD_PTR

    for (i = 0; i < 4; i++)
    {
        /* left */
        context->block[block_idx_xy[0][i]].mba = a;
        context->block[block_idx_xy[0][i]].bka = a ? &a->block[block_idx_xy[3][i]] : NULL;
        
        myPrintf("context->block (%d%d -> %d%d), left src= %d , dest=%d \n", 3,i,0,i, block_idx_xy[3][i], block_idx_xy[0][i]  );

        /* up */
        context->block[block_idx_xy[i][0]].mbb = b;
        context->block[block_idx_xy[i][0]].bkb = b ? &b->block[block_idx_xy[i][3]] : NULL;
        
        myPrintf("context->block (%d%d -> %d%d), up src= %d , dest=%d \n",i,3,i,0, block_idx_xy[i][3], block_idx_xy[i][0]  );

        /* rest */
        for (j = 1; j < 4; j++)
        {
            context->block[block_idx_xy[j][i]].mba = mb;
            context->block[block_idx_xy[j][i]].bka = &mb->block[block_idx_xy[j - 1][i]];
            
            myPrintf("context->block (%d%d -> %d%d), rest src= %d , dest=%d \n",j-1,i,j,i, block_idx_xy[j - 1][i], block_idx_xy[j][i]  );

            context->block[block_idx_xy[i][j]].mbb = mb;
            context->block[block_idx_xy[i][j]].bkb = &mb->block[block_idx_xy[i][j - 1]];
            
            myPrintf("context->block (%d%d -> %d%d), rest src= %d , dest=%d \n", i,j-1, j,i, block_idx_xy[i][j - 1], block_idx_xy[j][i]  );
        }
    }

    myPrintf("context->block uv  \n" );
    
    for (ch = 0; ch < 2; ch++)
    {
        if(! ch)
        myPrintf("context->block u \n" );
        else
        myPrintf("context->block v \n" );
            
        for (i = 0; i < 2; i++)
        {
            /* left */
            context->block[16 + 4 * ch + block_idx_xy[0][i]].mba = a;
            context->block[16 + 4 * ch + block_idx_xy[0][i]].bka = a ? &a->block[16 + 4 * ch + block_idx_xy[1][i]] : NULL;
            myPrintf("context->block (%d%d -> %d%d), left src= %d , dest=%d \n", 1,i,0,i,  16 + 4 * ch + block_idx_xy[1][i], 16 + 4 * ch + block_idx_xy[0][i]  );
            /* up */
            context->block[16 + 4 * ch + block_idx_xy[i][0]].mbb = b;
            context->block[16 + 4 * ch + block_idx_xy[i][0]].bkb = b ? &b->block[16 + 4 * ch + block_idx_xy[i][1]] : NULL;
            myPrintf("context->block (%d%d -> %d%d), up src= %d , dest=%d \n", i,1,i,0, 16 + 4 * ch + block_idx_xy[i][1], 16 + 4 * ch + block_idx_xy[i][0] );
            /* rest */
            context->block[16 + 4 * ch + block_idx_xy[1][i]].mba = mb;
            context->block[16 + 4 * ch + block_idx_xy[1][i]].bka = &mb->block[16 + 4 * ch + block_idx_xy[0][i]];
            myPrintf("context->block (%d%d -> %d%d), rest src= %d , dest=%d \n", 0,i,1,i, 16 + 4 * ch + block_idx_xy[0][i], 16 + 4 * ch + block_idx_xy[1][i] );
            context->block[16 + 4 * ch + block_idx_xy[i][1]].mbb = mb;
            context->block[16 + 4 * ch + block_idx_xy[i][1]].bkb = &mb->block[16 + 4 * ch + block_idx_xy[i][0]];
            myPrintf("context->block (%d%d -> %d%d), rest src= %d , dest=%d \n", i,0,i,1, 16 + 4 * ch + block_idx_xy[i][0], 16 + 4 * ch + block_idx_xy[i][1]  );
        }
        
        
    }
}
