
/*****************************************************************************
 * frame.c: h264 encoder library
 *****************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "common.h"

x264_frame_t *x264_frame_new( x264_t *h )
{
    x264_frame_t   *frame = (x264_frame_t *)x264_aligned_alloc( sizeof( x264_frame_t ) );
    int i;

    int i_stride;
    int i_lines;

    /* allocate frame data (+64 for extra data for me) */
    i_stride = ( ( h->param.i_width  + 15 )&0xfffff0 )+ 64;
    i_lines  = ( ( h->param.i_height + 15 )&0xfffff0 );

    frame->i_plane = 3;
    for( i = 0; i < 3; i++ )
    {
        int i_div;
        i_div = ( i == 0 ) ? 1 : 2;

        frame->i_stride[i] = i_stride / i_div;
        frame->i_lines[i] = i_lines / i_div;
        frame->buffer[i] = x264_aligned_alloc( frame->i_stride[i] * ( frame->i_lines[i] + 64 / i_div ));

        frame->plane[i] = ((uint8_t*)frame->buffer[i]) + ( frame->i_stride[i] * 32 + 32 ) / i_div;
    }
    frame->i_stride[3] = 0;
    frame->i_lines[3] = 0;
    frame->buffer[3] = NULL;
    frame->plane[3] = NULL;

    frame->i_poc = -1;

    return frame;
}

void x264_frame_delete( x264_frame_t *frame )
{
    int i;
    for( i = 0; i < frame->i_plane; i++ )
    {
        x264_free( frame->buffer[i] );
    }
    x264_free( frame );
}

void x264_frame_copy_picture( x264_frame_t *dst, x264_picture_t *src )
{
    int i;
    int ch;
    int i_stride;

    /* y */
    i_stride = X264_MIN( src->i_stride[0], dst->i_stride[0] );
    for( i = 0; i < src->i_height; i++ )
    {
        memcpy( &dst->plane[0][ i * dst->i_stride[0]],
                &src->plane[0][ i * src->i_stride[0]],
                i_stride );
    }

    /* uv */
    for( ch = 0; ch < 2; ch++ )
    {
        i_stride = X264_MIN( src->i_stride[1+ch], dst->i_stride[1+ch] );
        for( i = 0; i < src->i_height/2; i++ )
        {
            memcpy( &dst->plane[1+ch][ i * dst->i_stride[1+ch]],
                    &src->plane[1+ch][ i * src->i_stride[1+ch]],
                    i_stride );
        }
    }
}



void x264_frame_expand_border( x264_frame_t *frame )
{
    int w;
    int i, y;
    for( i = 0; i < frame->i_plane; i++ )
    {
#define PPIXEL(x, y) ( frame->plane[i] + (x) +(y)*frame->i_stride[i] )
        w = ( i == 0 ) ? 32 : 16;

        for( y = 0; y < w; y++ )
        {
            /* upper band */
            memcpy( PPIXEL(0,-y-1), PPIXEL(0,0), frame->i_stride[i] - 2 * w);
            /* up left corner */
            memset( PPIXEL(-w,-y-1 ), PPIXEL(0,0)[0], w );
            /* up right corner */
            memset( PPIXEL(frame->i_stride[i] - 2*w,-y-1), PPIXEL( frame->i_stride[i]-1-2*w,0)[0], w );

            /* lower band */
            memcpy( PPIXEL(0, frame->i_lines[i]+y), PPIXEL(0,frame->i_lines[i]-1), frame->i_stride[i] - 2 * w );
            /* low left corner */
            memset( PPIXEL(-w, frame->i_lines[i]+y), PPIXEL(0,frame->i_lines[i]-1)[0], w);
            /* low right corner */
            memset( PPIXEL(frame->i_stride[i]-2*w, frame->i_lines[i]+y), PPIXEL(frame->i_stride[i]-1-2*w,frame->i_lines[i]-1)[0], w);

        }
        for( y = 0; y < frame->i_lines[i]; y++ )
        {
            /* left band */
            memset( PPIXEL( -w, y ), PPIXEL( 0, y )[0], w );
            /* right band */
            memset( PPIXEL( frame->i_stride[i]-2*w, y ), PPIXEL( frame->i_stride[i] - 1-2*w, y )[0], w );
        }
#undef PPIXEL
    }
}

/* FIXME theses tables are duplicated with the ones in macroblock.c */
static const uint8_t block_idx_xy[4][4] =
{
    { 0, 2, 8,  10},
    { 1, 3, 9,  11},
    { 4, 6, 12, 14},
    { 5, 7, 13, 15}
};
static const int i_chroma_qp_table[52] =
{
     0,  1,  2,  3,  4,  5,  6,  7,  8,  9,
    10, 11, 12, 13, 14, 15, 16, 17, 18, 19,
    20, 21, 22, 23, 24, 25, 26, 27, 28, 29,
    29, 30, 31, 32, 32, 33, 34, 34, 35, 35,
    36, 36, 37, 37, 37, 38, 38, 38, 39, 39,
    39, 39
};

/* Deblocking filter (p153) */
static const int i_alpha_table[52] =
{
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  4,  4,  5,  6,
     7,  8,  9, 10, 12, 13, 15, 17, 20, 22,
    25, 28, 32, 36, 40, 45, 50, 56, 63, 71,
    80, 90,101,113,127,144,162,182,203,226,
    255, 255
};
static const int i_beta_table[52] =
{
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  2,  2,  2,  3,
     3,  3,  3,  4,  4,  4,  6,  6,  7,  7,
     8,  8,  9,  9, 10, 10, 11, 11, 12, 12,
    13, 13, 14, 14, 15, 15, 16, 16, 17, 17,
    18, 18
};
static const int i_tc0_table[52][3] =
{
    { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 },
    { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 },
    { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 1 },
    { 0, 0, 1 }, { 0, 0, 1 }, { 0, 0, 1 }, { 0, 1, 1 }, { 0, 1, 1 }, { 1, 1, 1 },
    { 1, 1, 1 }, { 1, 1, 1 }, { 1, 1, 1 }, { 1, 1, 2 }, { 1, 1, 2 }, { 1, 1, 2 },
    { 1, 1, 2 }, { 1, 2, 3 }, { 1, 2, 3 }, { 2, 2, 3 }, { 2, 2, 4 }, { 2, 3, 4 },
    { 2, 3, 4 }, { 3, 3, 5 }, { 3, 4, 6 }, { 3, 4, 6 }, { 4, 5, 7 }, { 4, 5, 8 },
    { 4, 6, 9 }, { 5, 7,10 }, { 6, 8,11 }, { 6, 8,13 }, { 7,10,14 }, { 8,11,16 },
    { 9,12,18 }, {10,13,20 }, {11,15,23 }, {13,17,25 }
};

/* From ffmpeg */
static inline int clip_uint8( int a )
{
    if (a&(~255))
        return (-a)>>31;
    else
        return a;
}

static inline void deblocking_filter_edgev( x264_t *h, uint8_t *pix, int i_pix_stride, int bS[4], int i_QP )
{
    int i, d;
    const int i_index_a = x264_clip3( i_QP + h->sh.i_alpha_c0_offset, 0, 51 );
    const int alpha = i_alpha_table[i_index_a];
    const int beta  = i_beta_table[x264_clip3( i_QP + h->sh.i_beta_offset, 0, 51 )];

    for( i = 0; i < 4; i++ )
    {
        if( bS[i] == 0 )
        {
            pix += 4 * i_pix_stride;
            continue;
        }

        if( bS[i] < 4 )
        {
            const int tc0 = i_tc0_table[i_index_a][bS[i] - 1];

            /* 4px edge length */
            for( d = 0; d < 4; d++ )
            {
                const int p0 = pix[-1];
                const int p1 = pix[-2];
                const int p2 = pix[-3];
                const int q0 = pix[0];
                const int q1 = pix[1];
                const int q2 = pix[2];

                if( abs( p0 - q0 ) < alpha &&
                    abs( p1 - p0 ) < beta &&
                    abs( q1 - q0 ) < beta )
                {
                    int tc = tc0;
                    int i_delta;

                    if( abs( p2 - p0 ) < beta )
                    {
                        pix[-2] = p1 + x264_clip3( ( p2 + ( ( p0 + q0 + 1 ) >> 1 ) - ( p1 << 1 ) ) >> 1, -tc0, tc0 );
                        tc++;
                    }
                    if( abs( q2 - q0 ) < beta )
                    {
                        pix[1] = q1 + x264_clip3( ( q2 + ( ( p0 + q0 + 1 ) >> 1 ) - ( q1 << 1 ) ) >> 1, -tc0, tc0 );
                        tc++;
                    }

                    i_delta = x264_clip3( (((q0 - p0 ) << 2) + (p1 - q1) + 4) >> 3, -tc, tc );
                    pix[-1] = clip_uint8( p0 + i_delta );    /* p0' */
                    pix[0]  = clip_uint8( q0 - i_delta );    /* q0' */
                }
                pix += i_pix_stride;
            }
        }
        else
        {
            /* 4px edge length */
            for( d = 0; d < 4; d++ )
            {
                const int p0 = pix[-1];
                const int p1 = pix[-2];
                const int p2 = pix[-3];

                const int q0 = pix[0];
                const int q1 = pix[1];
                const int q2 = pix[2];

                if( abs( p0 - q0 ) < alpha &&
                    abs( p1 - p0 ) < beta &&
                    abs( q1 - q0 ) < beta )
                {
                    if( abs( p0 - q0 ) < (( alpha >> 2 ) + 2 ) )
                    {
                        if( abs( p2 - p0 ) < beta )
                        {
                            const int p3 = pix[-4];
                            /* p0', p1', p2' */
                            pix[-1] = ( p2 + 2*p1 + 2*p0 + 2*q0 + q1 + 4 ) >> 3;
                            pix[-2] = ( p2 + p1 + p0 + q0 + 2 ) >> 2;
                            pix[-3] = ( 2*p3 + 3*p2 + p1 + p0 + q0 + 4 ) >> 3;
                        }
                        else
                        {
                            /* p0' */
                            pix[-1] = ( 2*p1 + p0 + q1 + 2 ) >> 2;
                        }
                        if( abs( q2 - q0 ) < beta )
                        {
                            const int q3 = pix[3];
                            /* q0', q1', q2' */
                            pix[0] = ( p1 + 2*p0 + 2*q0 + 2*q1 + q2 + 4 ) >> 3;
                            pix[1] = ( p0 + q0 + q1 + q2 + 2 ) >> 2;
                            pix[2] = ( 2*q3 + 3*q2 + q1 + q0 + p0 + 4 ) >> 3;
                        }
                        else
                        {
                            /* q0' */
                            pix[0] = ( 2*q1 + q0 + p1 + 2 ) >> 2;
                        }
                    }
                    else
                    {
                        /* p0', q0' */
                        pix[-1] = ( 2*p1 + p0 + q1 + 2 ) >> 2;
                        pix[0] = ( 2*q1 + q0 + p1 + 2 ) >> 2;
                    }
                }
                pix += i_pix_stride;
            }
        }
    }
}

static inline void deblocking_filter_edgecv( x264_t *h, uint8_t *pix, int i_pix_stride, int bS[4], int i_QP )
{
    int i, d;
    const int i_index_a = x264_clip3( i_QP + h->sh.i_alpha_c0_offset, 0, 51 );
    const int alpha = i_alpha_table[i_index_a];
    const int beta  = i_beta_table[x264_clip3( i_QP + h->sh.i_beta_offset, 0, 51 )];

    for( i = 0; i < 4; i++ )
    {
        if( bS[i] == 0 )
        {
            pix += 2 * i_pix_stride;
            continue;
        }

        if( bS[i] < 4 )
        {
            const int tc = i_tc0_table[i_index_a][bS[i] - 1] + 1;
            /* 2px edge length (because we use same bS than the one for luma) */
            for( d = 0; d < 2; d++ )
            {
                const int p0 = pix[-1];
                const int p1 = pix[-2];
                const int q0 = pix[0];
                const int q1 = pix[1];

                if( abs( p0 - q0 ) < alpha &&
                    abs( p1 - p0 ) < beta &&
                    abs( q1 - q0 ) < beta )
                {
                    const int i_delta = x264_clip3( (((q0 - p0 ) << 2) + (p1 - q1) + 4) >> 3, -tc, tc );

                    pix[-1] = clip_uint8( p0 + i_delta );    /* p0' */
                    pix[0]  = clip_uint8( q0 - i_delta );    /* q0' */
                }
                pix += i_pix_stride;
            }
        }
        else
        {
            /* 2px edge length (because we use same bS than the one for luma) */
            for( d = 0; d < 2; d++ )
            {
                const int p0 = pix[-1];
                const int p1 = pix[-2];
                const int q0 = pix[0];
                const int q1 = pix[1];

                if( abs( p0 - q0 ) < alpha &&
                    abs( p1 - p0 ) < beta &&
                    abs( q1 - q0 ) < beta )
                {
                    pix[-1] = ( 2*p1 + p0 + q1 + 2 ) >> 2;   /* p0' */
                    pix[0]  = ( 2*q1 + q0 + p1 + 2 ) >> 2;   /* q0' */
                }
                pix += i_pix_stride;
            }
        }
    }
}

static inline void deblocking_filter_edgeh( x264_t *h, uint8_t *pix, int i_pix_stride, int bS[4], int i_QP )
{
    int i, d;
    const int i_index_a = x264_clip3( i_QP + h->sh.i_alpha_c0_offset, 0, 51 );
    const int alpha = i_alpha_table[i_index_a];
    const int beta  = i_beta_table[x264_clip3( i_QP + h->sh.i_beta_offset, 0, 51 )];

    int i_pix_next  = i_pix_stride;

    for( i = 0; i < 4; i++ )
    {
        if( bS[i] == 0 )
        {
            pix += 4;
            continue;
        }

        if( bS[i] < 4 )
        {
            const int tc0 = i_tc0_table[i_index_a][bS[i] - 1];
            /* 4px edge length */
            for( d = 0; d < 4; d++ )
            {
                const int p0 = pix[-i_pix_next];
                const int p1 = pix[-2*i_pix_next];
                const int p2 = pix[-3*i_pix_next];
                const int q0 = pix[0];
                const int q1 = pix[1*i_pix_next];
                const int q2 = pix[2*i_pix_next];

                if( abs( p0 - q0 ) < alpha &&
                    abs( p1 - p0 ) < beta &&
                    abs( q1 - q0 ) < beta )
                {
                    int tc = tc0;
                    int i_delta;

                    if( abs( p2 - p0 ) < beta )
                    {
                        pix[-2*i_pix_next] = p1 + x264_clip3( ( p2 + ( ( p0 + q0 + 1 ) >> 1 ) - ( p1 << 1 ) ) >> 1, -tc0, tc0 );
                        tc++;
                    }
                    if( abs( q2 - q0 ) < beta )
                    {
                        pix[i_pix_next] = q1 + x264_clip3( ( q2 + ( ( p0 + q0 + 1 ) >> 1 ) - ( q1 << 1 ) ) >> 1, -tc0, tc0 );
                        tc++;
                    }

                    i_delta = x264_clip3( (((q0 - p0 ) << 2) + (p1 - q1) + 4) >> 3, -tc, tc );
                    pix[-i_pix_next] = clip_uint8( p0 + i_delta );    /* p0' */
                    pix[0]           = clip_uint8( q0 - i_delta );    /* q0' */
                }
                pix++;
            }
        }
        else
        {
            /* 4px edge length */
            for( d = 0; d < 4; d++ )
            {
                const int p0 = pix[-i_pix_next];
                const int p1 = pix[-2*i_pix_next];
                const int p2 = pix[-3*i_pix_next];
                const int q0 = pix[0];
                const int q1 = pix[1*i_pix_next];
                const int q2 = pix[2*i_pix_next];

                if( abs( p0 - q0 ) < alpha &&
                    abs( p1 - p0 ) < beta &&
                    abs( q1 - q0 ) < beta )
                {
                    const int p3 = pix[-4*i_pix_next];
                    const int q3 = pix[ 3*i_pix_next];

                    if( abs( p0 - q0 ) < (( alpha >> 2 ) + 2 ) )
                    {
                        if( abs( p2 - p0 ) < beta )
                        {
                            /* p0', p1', p2' */
                            pix[-1*i_pix_next] = ( p2 + 2*p1 + 2*p0 + 2*q0 + q1 + 4 ) >> 3;
                            pix[-2*i_pix_next] = ( p2 + p1 + p0 + q0 + 2 ) >> 2;
                            pix[-3*i_pix_next] = ( 2*p3 + 3*p2 + p1 + p0 + q0 + 4 ) >> 3;
                        }
                        else
                        {
                            /* p0' */
                            pix[-1*i_pix_next] = ( 2*p1 + p0 + q1 + 2 ) >> 2;
                        }
                        if( abs( q2 - q0 ) < beta )
                        {
                            /* q0', q1', q2' */
                            pix[0*i_pix_next] = ( p1 + 2*p0 + 2*q0 + 2*q1 + q2 + 4 ) >> 3;
                            pix[1*i_pix_next] = ( p0 + q0 + q1 + q2 + 2 ) >> 2;
                            pix[2*i_pix_next] = ( 2*q3 + 3*q2 + q1 + q0 + p0 + 4 ) >> 3;
                        }
                        else
                        {
                            /* q0' */
                            pix[0*i_pix_next] = ( 2*q1 + q0 + p1 + 2 ) >> 2;
                        }
                    }
                    else
                    {
                        /* p0' */
                        pix[-1*i_pix_next] = ( 2*p1 + p0 + q1 + 2 ) >> 2;
                        /* q0' */
                        pix[0*i_pix_next] = ( 2*q1 + q0 + p1 + 2 ) >> 2;
                    }
                }
                pix++;
            }

        }
    }
}

static inline void deblocking_filter_edgech( x264_t *h, uint8_t *pix, int i_pix_stride, int bS[4], int i_QP )
{
    int i, d;
    const int i_index_a = x264_clip3( i_QP + h->sh.i_alpha_c0_offset, 0, 51 );
    const int alpha = i_alpha_table[i_index_a];
    const int beta  = i_beta_table[x264_clip3( i_QP + h->sh.i_beta_offset, 0, 51 )];

    int i_pix_next  = i_pix_stride;

    for( i = 0; i < 4; i++ )
    {
        if( bS[i] == 0 )
        {
            pix += 2;
            continue;
        }
        if( bS[i] < 4 )
        {
            int tc = i_tc0_table[i_index_a][bS[i] - 1] + 1;
            /* 2px edge length (see deblocking_filter_edgecv) */
            for( d = 0; d < 2; d++ )
            {
                const int p0 = pix[-1*i_pix_next];
                const int p1 = pix[-2*i_pix_next];
                const int q0 = pix[0];
                const int q1 = pix[1*i_pix_next];

                if( abs( p0 - q0 ) < alpha &&
                    abs( p1 - p0 ) < beta &&
                    abs( q1 - q0 ) < beta )
                {
                    int i_delta = x264_clip3( (((q0 - p0 ) << 2) + (p1 - q1) + 4) >> 3, -tc, tc );

                    pix[-i_pix_next] = clip_uint8( p0 + i_delta );    /* p0' */
                    pix[0]           = clip_uint8( q0 - i_delta );    /* q0' */
                }
                pix++;
            }
        }
        else
        {
            /* 2px edge length (see deblocking_filter_edgecv) */
            for( d = 0; d < 2; d++ )
            {
                const int p0 = pix[-1*i_pix_next];
                const int p1 = pix[-2*i_pix_next];
                const int q0 = pix[0];
                const int q1 = pix[1*i_pix_next];

                if( abs( p0 - q0 ) < alpha &&
                    abs( p1 - p0 ) < beta &&
                    abs( q1 - q0 ) < beta )
                {
                    pix[-i_pix_next] = ( 2*p1 + p0 + q1 + 2 ) >> 2;   /* p0' */
                    pix[0]           = ( 2*q1 + q0 + p1 + 2 ) >> 2;   /* q0' */
                }
                pix++;
            }
        }
    }
}

void x264_frame_deblocking_filter( x264_t *h, int i_slice_type )
{
    int mb_xy;

    for( mb_xy = 0; mb_xy < h->sps->i_mb_width * h->sps->i_mb_height; mb_xy++ )
    {
        x264_macroblock_t *mb;  /* current macroblock */
        int i_edge;
        int i_dir;

        mb = &h->mb[mb_xy];

        /* i_dir == 0 -> vertical edge
         * i_dir == 1 -> horizontal edge */
        for( i_dir = 0; i_dir < 2; i_dir++ )
        {
            int i_start;
            int i_QP;

            i_start = (( i_dir == 0 && mb->i_mb_x != 0 ) || ( i_dir == 1 && mb->i_mb_y != 0 ) ) ? 0 : 1;

            for( i_edge = i_start; i_edge < 4; i_edge++ )
            {
                x264_macroblock_t *mbn;
                int bS[4];  /* filtering strength */

                /* *** neighbour macroblock for the current edge (or current) *** */
                mbn = i_edge > 0 ? mb : ( i_dir == 0 ? mb - 1 : mb - h->sps->i_mb_width );

                /* *** Get bS for each 4px for the current edge *** */
                if( IS_INTRA( mb->i_type ) || IS_INTRA( mbn->i_type ) )
                {
                    bS[0] = bS[1] = bS[2] = bS[3] = ( i_edge == 0 ? 4 : 3 );
                }
                else
                {
                    int i;
                    for( i = 0; i < 4; i++ )
                    {
                        int x, y;
                        int xn, yn;

                        x = i_dir == 0 ? i_edge : i;
                        y = i_dir == 0 ? i      : i_edge;

                        xn = (x - (i_dir == 0 ? 1 : 0 ))&0x03;
                        yn = (y - (i_dir == 0 ? 0 : 1 ))&0x03;

                        if( mb->block[block_idx_xy[x][y]].i_non_zero_count != 0 ||
                            mbn->block[block_idx_xy[xn][yn]].i_non_zero_count != 0 )
                        {
                            bS[i] = 2;
                        }
                        else if( i_slice_type == SLICE_TYPE_B )
                        {
                            /* FIXME */
                            fprintf( stderr, "deblocking filter doesn't work yet with B slice\n" );
                            return;
                        }
                        else    /* SLICE_TYPE_P */
                        {
                            if( mb->partition[x][y].i_ref[0]  != mbn->partition[xn][yn].i_ref[0] ||
                                abs( mb->partition[x][y].mv[0][0] - mbn->partition[xn][yn].mv[0][0] ) >= 4 ||
                                abs( mb->partition[x][y].mv[0][1] - mbn->partition[xn][yn].mv[0][1] ) >= 4 )
                            {
                                bS[i] = 1;
                            }
                            else
                            {
                                bS[i] = 0;
                            }
                        }
                    }
                }

                /* *** filter *** */
                /* Y plane */
                i_QP = ( mb->i_qp + mbn->i_qp + 1 ) >> 1;
                if( i_dir == 0 )
                {
                    /* vertical edge */
                    deblocking_filter_edgev( h, &h->fdec->plane[0][16 * mb->i_mb_y * h->fdec->i_stride[0]+ 16 * mb->i_mb_x + 4 * i_edge],
                                                h->fdec->i_stride[0], bS, i_QP );
                    if( (i_edge % 2) == 0  )
                    {
                        /* U/V planes */
                        i_QP = ( i_chroma_qp_table[mb->i_qp] + i_chroma_qp_table[mbn->i_qp] + 1 ) >> 1;
                        deblocking_filter_edgecv( h, &h->fdec->plane[1][8*(mb->i_mb_y*h->fdec->i_stride[1]+mb->i_mb_x)+i_edge*2],
                                                      h->fdec->i_stride[1], bS, i_QP );
                        deblocking_filter_edgecv( h, &h->fdec->plane[2][8*(mb->i_mb_y*h->fdec->i_stride[2]+mb->i_mb_x)+i_edge*2],
                                                  h->fdec->i_stride[2], bS, i_QP );
                    }
                }
                else
                {
                    /* horizontal edge */
                    deblocking_filter_edgeh( h, &h->fdec->plane[0][(16*mb->i_mb_y + 4 * i_edge) * h->fdec->i_stride[0]+ 16 * mb->i_mb_x],
                                                h->fdec->i_stride[0], bS, i_QP );
                    /* U/V planes */
                    if( ( i_edge % 2  ) == 0 )
                    {
                        i_QP = ( i_chroma_qp_table[mb->i_qp] + i_chroma_qp_table[mbn->i_qp] + 1 ) >> 1;
                        deblocking_filter_edgech( h, &h->fdec->plane[1][8*(mb->i_mb_y*h->fdec->i_stride[1]+mb->i_mb_x)+i_edge*2*h->fdec->i_stride[1]],
                                                 h->fdec->i_stride[1], bS, i_QP );
                        deblocking_filter_edgech( h, &h->fdec->plane[2][8*(mb->i_mb_y*h->fdec->i_stride[2]+mb->i_mb_x)+i_edge*2*h->fdec->i_stride[2]],
                                                 h->fdec->i_stride[2], bS, i_QP );
                    }
                }
            }
        }
    }
}





