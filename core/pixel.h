
/*****************************************************************************
 * pixel.h: h264 encoder library

 *****************************************************************************/

#ifndef _PIXEL_H
#define _PIXEL_H 1

typedef int  (*x264_pixel_sad_t) ( uint8_t *, int, uint8_t *, int );
typedef int  (*x264_pixel_satd_t)( uint8_t *, int, uint8_t *, int );
typedef void (*x264_pixel_avg_t) ( uint8_t *, int, uint8_t *, int );

enum
{
    PIXEL_16x16 = 0,
    PIXEL_16x8  = 1,
    PIXEL_8x16  = 2,
    PIXEL_8x8   = 3,
    PIXEL_8x4   = 4,
    PIXEL_4x8   = 5,
    PIXEL_4x4   = 6,
};

typedef struct
{
    x264_pixel_sad_t  sad[7];
    x264_pixel_satd_t satd[7];
    x264_pixel_avg_t  avg[7];

    void (*sub4x4)  ( int16_t diff[4][4], uint8_t *, int, uint8_t *, int );
    void (*sub8x8)  ( int16_t diff[4][4][4], uint8_t *, int, uint8_t *, int );
    void (*sub16x16)( int16_t diff[16][4][4], uint8_t *, int, uint8_t *, int );

    void (*add4x4)  ( uint8_t *, int, int16_t diff[4][4] );
    void (*add8x8)  ( uint8_t *, int, int16_t diff[4][4][4] );
    void (*add16x16)( uint8_t *, int, int16_t diff[16][4][4] );

} x264_pixel_function_t;

void x264_pixel_init( int cpu, x264_pixel_function_t *pixf );

#endif
