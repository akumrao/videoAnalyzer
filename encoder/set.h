
/*****************************************************************************
 * set.h: h264 encoder
 *****************************************************************************
 *****************************************************************************/

#ifndef _ENCODER_SET_H
#define _ENCODER_SET_H 1

void x264_sps_init( x264_sps_t *sps, int i_id, x264_param_t *param );
void x264_sps_write( bs_t *s, x264_sps_t *sps );
void x264_pps_init( x264_pps_t *pps, int i_id, x264_param_t *param, x264_sps_t *sps );
void x264_pps_write( bs_t *s, x264_pps_t *pps );

#endif
