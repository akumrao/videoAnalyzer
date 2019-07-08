
/*****************************************************************************
 * macroblock.h: h264 decoder library
 *****************************************************************************/

#ifndef _DECODER_MACROBLOCK_H
#define _DECODER_MACROBLOCK_H 1

int  x264_macroblock_read_cabac( x264_t *h, bs_t *s, x264_macroblock_t *mb );
int  x264_macroblock_read_cavlc( x264_t *h, bs_t *s, x264_macroblock_t *mb );

int  x264_macroblock_decode( x264_t *h, x264_macroblock_t *mb );
void x264_macroblock_decode_skip( x264_t *h, x264_macroblock_t *mb );

#endif

