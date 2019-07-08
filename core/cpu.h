
/*****************************************************************************
 * cpu.h: h264 encoder library
 *****************************************************************************/

#ifndef _CPU_H
#define _CPU_H 1

uint32_t x264_cpu_detect( void );

/* probably MMX(EXT) centric but .... */
void     x264_cpu_restore( uint32_t cpu );

#endif
