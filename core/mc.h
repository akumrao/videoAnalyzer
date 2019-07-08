
/*****************************************************************************
 * mc.h: h264 encoder library (Motion Compensation)

 *****************************************************************************/

#ifndef _MC_H
#define _MC_H 1

/* Do the MC
 * XXX: Only width = 4, 8 or 16 are valid
 * width == 4 -> height == 4 or 8
 * width == 8 -> height == 4 or 8 or 16
 * width == 16-> height == 8 or 16
 * */

typedef void (*x264_mc_t)(uint8_t *, int, uint8_t *, int,
                          int mvx, int mvy,
                          int i_width, int i_height );
enum
{
    MC_LUMA   = 0,
    MC_CHROMA = 1,
};

void x264_mc_init( int cpu, x264_mc_t pf[2] );

#endif
