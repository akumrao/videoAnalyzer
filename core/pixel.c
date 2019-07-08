
/*****************************************************************************
 * pixel.c: h264 encoder

 *****************************************************************************/

#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "../x264.h"
#include "pixel.h"

#ifdef HAVE_MMXEXT
#   include "i386/pixel.h"
#endif
#ifdef HAVE_ALTIVEC
#   include "ppc/pixel.h"
#endif


/****************************************************************************
 * pixel_sad_WxH
 ****************************************************************************/
#define PIXEL_SAD_C( name, lx, ly ) \
static int name( uint8_t *pix1, int i_stride_pix1,  \
                 uint8_t *pix2, int i_stride_pix2 ) \
{                                                   \
    int i_sum = 0;                                  \
    int x, y;                                       \
    for( y = 0; y < ly; y++ )                       \
    {                                               \
        for( x = 0; x < lx; x++ )                   \
        {                                           \
            i_sum += abs( pix1[x] - pix2[x] );      \
        }                                           \
        pix1 += i_stride_pix1;                      \
        pix2 += i_stride_pix2;                      \
    }                                               \
    return i_sum;                                   \
}


PIXEL_SAD_C( pixel_sad_16x16, 16, 16 )
PIXEL_SAD_C( pixel_sad_16x8,  16,  8 )
PIXEL_SAD_C( pixel_sad_8x16,   8, 16 )
PIXEL_SAD_C( pixel_sad_8x8,    8,  8 )
PIXEL_SAD_C( pixel_sad_8x4,    8,  4 )
PIXEL_SAD_C( pixel_sad_4x8,    4,  8 )
PIXEL_SAD_C( pixel_sad_4x4,    4,  4 )

static void pixel_sub_4x4( int16_t diff[4][4], uint8_t *pix1, int i_pix1, uint8_t *pix2, int i_pix2 )
{
    int y, x;
    for( y = 0; y < 4; y++ )
    {
        for( x = 0; x < 4; x++ )
        {
            diff[y][x] = pix1[x] - pix2[x];
        }
        pix1 += i_pix1;
        pix2 += i_pix2;
    }
}

static void pixel_sub_8x8( int16_t diff[4][4][4], uint8_t *pix1, int i_pix1, uint8_t *pix2, int i_pix2 )
{
    pixel_sub_4x4( diff[0], &pix1[0],          i_pix1, &pix2[0],          i_pix2 );
    pixel_sub_4x4( diff[1], &pix1[4],          i_pix1, &pix2[4],          i_pix2 );
    pixel_sub_4x4( diff[2], &pix1[4*i_pix1],   i_pix1, &pix2[4*i_pix2],   i_pix2 );
    pixel_sub_4x4( diff[3], &pix1[4*i_pix1+4], i_pix1, &pix2[4*i_pix2+4], i_pix2 );
}

static void pixel_sub_16x16( int16_t diff[16][4][4], uint8_t *pix1, int i_pix1, uint8_t *pix2, int i_pix2 )
{
    pixel_sub_8x8( &diff[ 0], &pix1[0],          i_pix1, &pix2[0],          i_pix2 );
    pixel_sub_8x8( &diff[ 4], &pix1[8],          i_pix1, &pix2[8],          i_pix2 );
    pixel_sub_8x8( &diff[ 8], &pix1[8*i_pix1],   i_pix1, &pix2[8*i_pix2],   i_pix2 );
    pixel_sub_8x8( &diff[12], &pix1[8*i_pix1+8], i_pix1, &pix2[8*i_pix2+8], i_pix2 );
}

static void pixel_add_4x4( uint8_t *dst, int i_dst, int16_t diff[4][4] )
{
    int x, y;

    for( y = 0; y < 4; y++ )
    {
        for( x = 0; x < 4; x++ )
        {
            int pix;

            pix = dst[x] + diff[y][x];
            if( pix < 0 )
            {
                dst[x] = 0;
            }
            else if( pix > 255 )
            {
                dst[x] = 255;
            }
            else
            {
                dst[x] = pix;

            }
        }
        dst += i_dst;
    }
}
static void pixel_add_8x8( uint8_t *dst, int i_dst, int16_t diff[4][4][4] )
{
    pixel_add_4x4( &dst[0],         i_dst, diff[0] );
    pixel_add_4x4( &dst[4],         i_dst, diff[1] );
    pixel_add_4x4( &dst[0+4*i_dst], i_dst, diff[2] );
    pixel_add_4x4( &dst[4+4*i_dst], i_dst, diff[3] );
}
static void pixel_add_16x16( uint8_t *dst, int i_dst, int16_t diff[16][4][4] )
{
    pixel_add_8x8( &dst[0],         i_dst, &diff[ 0] );
    pixel_add_8x8( &dst[8],         i_dst, &diff[ 4] );
    pixel_add_8x8( &dst[0+8*i_dst], i_dst, &diff[ 8] );
    pixel_add_8x8( &dst[8+8*i_dst], i_dst, &diff[12] );
}

static int pixel_satd_wxh( uint8_t *pix1, int i_pix1, uint8_t *pix2, int i_pix2, int i_width, int i_height )
{
    int16_t tmp[4][4];
    int16_t diff[4][4];
    int x, y;
    int i_satd = 0;

    for( y = 0; y < i_height; y += 4 )
    {
        for( x = 0; x < i_width; x += 4 )
        {
            int d;

            pixel_sub_4x4( diff, &pix1[x], i_pix1, &pix2[x], i_pix2 );

            for( d = 0; d < 4; d++ )
            {
                int s01, s23;
                int d01, d23;

                s01 = diff[d][0] + diff[d][1]; s23 = diff[d][2] + diff[d][3];
                d01 = diff[d][0] - diff[d][1]; d23 = diff[d][2] - diff[d][3];

                tmp[d][0] = s01 + s23;
                tmp[d][1] = s01 - s23;
                tmp[d][2] = d01 - d23;
                tmp[d][3] = d01 + d23;
            }
            for( d = 0; d < 4; d++ )
            {
                int s01, s23;
                int d01, d23;

                s01 = tmp[0][d] + tmp[1][d]; s23 = tmp[2][d] + tmp[3][d];
                d01 = tmp[0][d] - tmp[1][d]; d23 = tmp[2][d] - tmp[3][d];

                i_satd += abs( s01 + s23 ) + abs( s01 - s23 ) + abs( d01 - d23 ) + abs( d01 + d23 );
            }

        }
        pix1 += 4 * i_pix1;
        pix2 += 4 * i_pix2;
    }

    return i_satd / 2;
}
#define PIXEL_SATD_C( name, width, height ) \
static int name( uint8_t *pix1, int i_stride_pix1, \
                 uint8_t *pix2, int i_stride_pix2 ) \
{ \
    return pixel_satd_wxh( pix1, i_stride_pix1, pix2, i_stride_pix2, width, height ); \
}
PIXEL_SATD_C( pixel_satd_16x16, 16, 16 )
PIXEL_SATD_C( pixel_satd_16x8,  16, 8 )
PIXEL_SATD_C( pixel_satd_8x16,  8, 16 )
PIXEL_SATD_C( pixel_satd_8x8,   8, 8 )
PIXEL_SATD_C( pixel_satd_8x4,   8, 4 )
PIXEL_SATD_C( pixel_satd_4x8,   4, 8 )
PIXEL_SATD_C( pixel_satd_4x4,   4, 4 )


static inline void pixel_avg_wxh( uint8_t *dst, int i_dst, uint8_t *src, int i_src, int width, int height )
{
    int x, y;
    for( y = 0; y < height; y++ )
    {
        for( x = 0; x < width; x++ )
        {
            dst[x] = ( dst[x] + src[x] + 1 ) >> 1;
        }
        dst += i_dst;
        src += i_src;
    }
}


#define PIXEL_AVG_C( name, width, height ) \
static void name( uint8_t *pix1, int i_stride_pix1, \
                  uint8_t *pix2, int i_stride_pix2 ) \
{ \
    pixel_avg_wxh( pix1, i_stride_pix1, pix2, i_stride_pix2, width, height ); \
}
PIXEL_AVG_C( pixel_avg_16x16, 16, 16 )
PIXEL_AVG_C( pixel_avg_16x8,  16, 8 )
PIXEL_AVG_C( pixel_avg_8x16,  8, 16 )
PIXEL_AVG_C( pixel_avg_8x8,   8, 8 )
PIXEL_AVG_C( pixel_avg_8x4,   8, 4 )
PIXEL_AVG_C( pixel_avg_4x8,   4, 8 )
PIXEL_AVG_C( pixel_avg_4x4,   4, 4 )

/****************************************************************************
 * x264_pixel_init:
 ****************************************************************************/
void x264_pixel_init( int cpu, x264_pixel_function_t *pixf )
{
    pixf->sad[PIXEL_16x16] = pixel_sad_16x16;
    pixf->sad[PIXEL_16x8]  = pixel_sad_16x8;
    pixf->sad[PIXEL_8x16]  = pixel_sad_8x16;
    pixf->sad[PIXEL_8x8]   = pixel_sad_8x8;
    pixf->sad[PIXEL_8x4]   = pixel_sad_8x4;
    pixf->sad[PIXEL_4x8]   = pixel_sad_4x8;
    pixf->sad[PIXEL_4x4]   = pixel_sad_4x4;

    pixf->satd[PIXEL_16x16]= pixel_satd_16x16;
    pixf->satd[PIXEL_16x8] = pixel_satd_16x8;
    pixf->satd[PIXEL_8x16] = pixel_satd_8x16;
    pixf->satd[PIXEL_8x8]  = pixel_satd_8x8;
    pixf->satd[PIXEL_8x4]  = pixel_satd_8x4;
    pixf->satd[PIXEL_4x8]  = pixel_satd_4x8;
    pixf->satd[PIXEL_4x4]  = pixel_satd_4x4;

    pixf->avg[PIXEL_16x16]= pixel_avg_16x16;
    pixf->avg[PIXEL_16x8] = pixel_avg_16x8;
    pixf->avg[PIXEL_8x16] = pixel_avg_8x16;
    pixf->avg[PIXEL_8x8]  = pixel_avg_8x8;
    pixf->avg[PIXEL_8x4]  = pixel_avg_8x4;
    pixf->avg[PIXEL_4x8]  = pixel_avg_4x8;
    pixf->avg[PIXEL_4x4]  = pixel_avg_4x4;

    pixf->sub4x4   = pixel_sub_4x4;
    pixf->sub8x8   = pixel_sub_8x8;
    pixf->sub16x16 = pixel_sub_16x16;

    pixf->add4x4   = pixel_add_4x4;
    pixf->add8x8   = pixel_add_8x8;
    pixf->add16x16 = pixel_add_16x16;
#ifdef HAVE_MMXEXT
    if( cpu&X264_CPU_MMXEXT )
    {
        x264_pixel_mmxext_init( pixf );
    }
#endif
#ifdef HAVE_ALTIVEC
    if( cpu&X264_CPU_ALTIVEC )
    {
        x264_pixel_altivec_init( pixf );
    }
#endif
}

