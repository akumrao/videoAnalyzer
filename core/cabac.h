
/*****************************************************************************
 * cabac.h: h264 encoder library
 *****************************************************************************/

#ifndef _CABAC_H
#define _CABAC_H 1

typedef struct
{
    /* model */
    struct
    {
        int i_model;
        int i_cost;
    } slice[3];

    /* context */
    struct
    {
        int i_state;
        int i_mps;
        int i_count;
    } ctxstate[399];

    /* state */
    int i_low;
    int i_range;

    int i_sym_cnt;

    /* bit stream */
    int b_first_bit;
    int i_bits_outstanding;
    bs_t *s;

} x264_cabac_t;

/* encoder/decoder: init the contexts given i_slice_type, the quantif and the model */
void x264_cabac_context_init( x264_cabac_t *cb, int i_slice_type, int i_qp, int i_model );

/* decoder only: */
void x264_cabac_decode_init    ( x264_cabac_t *cb, bs_t *s );
int  x264_cabac_decode_decision( x264_cabac_t *cb, int i_ctx_idx );
int  x264_cabac_decode_bypass  ( x264_cabac_t *cb );
int  x264_cabac_decode_terminal( x264_cabac_t *cb );

/* encoder only: adaptive model init */
void x264_cabac_model_init( x264_cabac_t *cb );
int  x264_cabac_model_get ( x264_cabac_t *cb, int i_slice_type );
void x264_cabac_model_update( x264_cabac_t *cb, int i_slice_type, int i_qp );
/* encoder only: */
void x264_cabac_encode_init ( x264_cabac_t *cb, bs_t *s );
void x264_cabac_encode_decision( x264_cabac_t *cb, int i_ctx_idx, int b );
void x264_cabac_encode_bypass( x264_cabac_t *cb, int b );
void x264_cabac_encode_terminal( x264_cabac_t *cb, int b );
void x264_cabac_encode_flush( x264_cabac_t *cb );


#endif
