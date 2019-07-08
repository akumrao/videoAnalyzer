
/*****************************************************************************
 * set.h: h264 decoder
 *****************************************************************************

 *****************************************************************************/

#ifndef _DECODER_SET_H
#define _DECODER_SET_H 1

/* return -1 if invalid, else the id */
int x264_sps_read( bs_t *s, x264_sps_t sps_array[32] );

/* return -1 if invalid, else the id */
int x264_pps_read( bs_t *s, x264_pps_t pps_array[256] );

#endif
