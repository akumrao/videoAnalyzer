/*****************************************************************************
 * dct.h: h264 encoder library
 *****************************************************************************/

#ifndef _DCT_H
#define _DCT_H 1

typedef struct
{
    void (*dct4x4)   ( int16_t dct[4][4],  int16_t diff[4][4] );
    void (*dct4x4dc) ( int16_t dct[4][4],  int16_t diff[4][4] );
    void (*idct4x4)  ( int16_t diff[4][4], int16_t dct[4][4] );
    void (*idct4x4dc)( int16_t diff[4][4], int16_t dct[4][4] );

    void (*dct2x2dc) ( int16_t dct[2][2],  int16_t diff[2][2] );
    void (*idct2x2dc)( int16_t diff[2][2], int16_t dct[2][2] );

} x264_dct_function_t;

void x264_dct_init( int cpu, x264_dct_function_t *dctf );

#endif

 
