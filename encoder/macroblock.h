
/*****************************************************************************
 * macroblock.h: h264 encoder library
 *****************************************************************************/

#ifndef _ENCODER_MACROBLOCK_H
#define _ENCODER_MACROBLOCK_H 1

void x264_macroblock_encode      ( x264_t *h, x264_macroblock_t *mb );
void x264_macroblock_write_cabac ( x264_t *h, bs_t *s, x264_macroblock_t *mb );
void x264_macroblock_write_cavlc ( x264_t *h, bs_t *s, x264_macroblock_t *mb );

void x264_cabac_mb_skip( x264_t *h, x264_macroblock_t *mb, int b_skip );

#endif

