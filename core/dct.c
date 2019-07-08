
/*****************************************************************************
 * dct.c: h264 encoder library
 *****************************************************************************/

#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "dct.h"
/*
 * XXX For all dct input could be equal to output so ...
 */

static void dct2x2dc( int16_t dct[2][2], int16_t diff[2][2] )
{
    int tmp[2][2];

    tmp[0][0] = diff[0][0] + diff[0][1];
    tmp[1][0] = diff[0][0] - diff[0][1];
    tmp[0][1] = diff[1][0] + diff[1][1];
    tmp[1][1] = diff[1][0] - diff[1][1];

    dct[0][0] = tmp[0][0] + tmp[0][1];
    dct[0][1] = tmp[1][0] + tmp[1][1];
    dct[1][0] = tmp[0][0] - tmp[0][1];
    dct[1][1] = tmp[1][0] - tmp[1][1];
}

static void idct2x2dc( int16_t diff[2][2], int16_t dct[2][2] )
{
    dct2x2dc( dct, diff );
}

static void dct4x4dc( int16_t dct[4][4], int16_t diff[4][4] )
{
    int16_t tmp[4][4];
    int s01, s23;
    int d01, d23;
    int i;

    for( i = 0; i < 4; i++ )
    {
        s01 = diff[i][0] + diff[i][1];
        d01 = diff[i][0] - diff[i][1];
        s23 = diff[i][2] + diff[i][3];
        d23 = diff[i][2] - diff[i][3];

        tmp[0][i] = s01 + s23;
        tmp[1][i] = s01 - s23;
        tmp[2][i] = d01 - d23;
        tmp[3][i] = d01 + d23;
    }

    for( i = 0; i < 4; i++ )
    {
        s01 = tmp[i][0] + tmp[i][1];
        d01 = tmp[i][0] - tmp[i][1];
        s23 = tmp[i][2] + tmp[i][3];
        d23 = tmp[i][2] - tmp[i][3];

        dct[0][i] = ( s01 + s23 + 1 ) >> 1;
        dct[1][i] = ( s01 - s23 + 1 ) >> 1;
        dct[2][i] = ( d01 - d23 + 1 ) >> 1;
        dct[3][i] = ( d01 + d23 + 1 ) >> 1;
    }
}

static void idct4x4dc( int16_t diff[4][4], int16_t dct[4][4] )
{
    int16_t tmp[4][4];
    int s01, s23;
    int d01, d23;
    int i;

    for( i = 0; i < 4; i++ )
    {
        s01 = dct[0][i] + dct[1][i];
        d01 = dct[0][i] - dct[1][i];
        s23 = dct[2][i] + dct[3][i];
        d23 = dct[2][i] - dct[3][i];

        tmp[0][i] = s01 + s23;
        tmp[1][i] = s01 - s23;
        tmp[2][i] = d01 - d23;
        tmp[3][i] = d01 + d23;
    }

    for( i = 0; i < 4; i++ )
    {
        s01 = tmp[i][0] + tmp[i][1];
        d01 = tmp[i][0] - tmp[i][1];
        s23 = tmp[i][2] + tmp[i][3];
        d23 = tmp[i][2] - tmp[i][3];

        diff[i][0] = s01 + s23;
        diff[i][1] = s01 - s23;
        diff[i][2] = d01 - d23;
        diff[i][3] = d01 + d23;
    }
}


static void dct4x4( int16_t dct[4][4], int16_t diff[4][4] )
{
    int16_t tmp[4][4];
    int s03, s12;
    int d03, d12;
    int i;

    for( i = 0; i < 4; i++ )
    {
        s03 = diff[i][0] + diff[i][3];
        s12 = diff[i][1] + diff[i][2];
        d03 = diff[i][0] - diff[i][3];
        d12 = diff[i][1] - diff[i][2];

        tmp[0][i] =   s03 +   s12;
        tmp[1][i] = 2*d03 +   d12;
        tmp[2][i] =   s03 -   s12;
        tmp[3][i] =   d03 - 2*d12;
    }

    for( i = 0; i < 4; i++ )
    {
        s03 = tmp[i][0] + tmp[i][3];
        s12 = tmp[i][1] + tmp[i][2];
        d03 = tmp[i][0] - tmp[i][3];
        d12 = tmp[i][1] - tmp[i][2];

        dct[0][i] =   s03 +   s12;
        dct[1][i] = 2*d03 +   d12;
        dct[2][i] =   s03 -   s12;
        dct[3][i] =   d03 - 2*d12;
    }
}

static void idct4x4( int16_t diff[4][4], int16_t dct[4][4] )
{
    int16_t tmp[4][4];
    int s02, d02;
    int s13, d13;
    int i;

    for( i = 0; i < 4; i++ )
    {
        s02 = dct[0][i]      + dct[2][i];
        d02 = dct[0][i]      - dct[2][i];
        s13 = dct[1][i]      + (dct[3][i]>>1);
        d13 = (dct[1][i]>>1) -  dct[3][i];

        tmp[0][i] = s02 + s13;
        tmp[1][i] = d02 + d13;
        tmp[2][i] = d02 - d13;
        tmp[3][i] = s02 - s13;
    }

    for( i = 0; i < 4; i++ )
    {
        s02 =  tmp[i][0]     +  tmp[i][2];
        d02 =  tmp[i][0]     -  tmp[i][2];
        s13 =  tmp[i][1]     + (tmp[i][3]>>1);
        d13 = (tmp[i][1]>>1) -  tmp[i][3];

        diff[i][0] = ( s02 + s13 + 32 ) >> 6;
        diff[i][1] = ( d02 + d13 + 32 ) >> 6;
        diff[i][2] = ( d02 - d13 + 32 ) >> 6;
        diff[i][3] = ( s02 - s13 + 32 ) >> 6;
    }
}
void x264_dct_init( int cpu, x264_dct_function_t *dctf )
{
    dctf->dct4x4    = dct4x4;
    dctf->dct4x4dc  = dct4x4dc;
    dctf->idct4x4   = idct4x4;
    dctf->idct4x4dc = idct4x4dc;
    dctf->dct2x2dc  = dct2x2dc;
    dctf->idct2x2dc = idct2x2dc;
}

