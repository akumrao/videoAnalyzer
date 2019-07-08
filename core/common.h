
/*****************************************************************************
 * common.h: h264 encoder
  ********************************************************************/

#ifndef _COMMON_H
#define _COMMON_H 1

#include <stdint.h>
#include "dump.h"
#include "../x264.h"
#include "bs.h"
#include "macroblock.h"
#include "set.h"
#include "predict.h"
#include "pixel.h"
#include "mc.h"
#include "frame.h"
#include "dct.h"
#include "../encoder/me.h"


/* FIXME */
typedef struct x264_slice_header_t x264_slice_header_t;
#include "cabac.h"

typedef struct x264_ratecontrol_t   x264_ratecontrol_t;
typedef struct x264_vlc_table_t     x264_vlc_table_t;

#define X264_MIN(a,b) ( (a)<(b) ? (a) : (b) )
#define X264_MAX(a,b) ( (a)>(b) ? (a) : (b) )
#define X264_ABS(a)   ( (a)< 0 ? -(a) : (a) )

/* x264_aligned_alloc : will do or emulate a memalign
 * XXX you HAVE TO use x264_free for buffer allocated
 * with x264_aligned_alloc
 */
void *x264_aligned_alloc( int );
void *x264_malloc( int );
void *x264_realloc( void *p, int i_size );
void  x264_free( void * );

/* mdate: return the current date in microsecond */
int64_t x264_mdate( void );

static inline int x264_clip3( int v, int i_min, int i_max )
{
    if( v < i_min )
    {
        return i_min;
    }
    else if( v > i_max )
    {
        return i_max;
    }
    else
    {
        return v;
    }
}

enum slice_type_e
{
    SLICE_TYPE_P  = 0,
    SLICE_TYPE_B  = 1,
    SLICE_TYPE_I  = 2,
    SLICE_TYPE_SP = 3,
    SLICE_TYPE_SI = 4
};

struct x264_slice_header_t
{
    x264_sps_t *sps;
    x264_pps_t *pps;

    int i_type;
    int i_first_mb;

    int i_pps_id;

    int i_frame_num;

    int b_field_pic;
    int b_bottom_field;

    int i_idr_pic_id;   /* -1 if nal_type != 5 */

    int i_poc_lsb;
    int i_delta_poc_bottom;

    int i_delta_poc[2];
    int i_redundant_pic_cnt;

    int b_direct_spatial_mv_pred;

    int b_num_ref_idx_override;
    int i_num_ref_idx_l0_active;
    int i_num_ref_idx_l1_active;

    int i_cabac_init_idc;

    int i_qp_delta;
    int b_sp_for_swidth;
    int i_qs_delta;

    /* deblocking filter */
    int i_disable_deblocking_filter_idc;
    int i_alpha_c0_offset;
    int i_beta_offset;

};

#define X264_BFRAME_MAX 16
struct x264_t
{
    /* cpu capabilities (not yet used) */
    unsigned int    cpu;

    /* bitstream output */
    int             i_nal;
    x264_nal_t      nal[3];         /* for now 3 is enought */
    int             i_bitstream;    /* size of p_bitstream */
    uint8_t         *p_bitstream;   /* will hold data for all nal */
    bs_t            bs;

    /* encoder parameters */
    x264_param_t    param;

    /* frame number/poc (TODO: rework that for B-frame) */
    int             i_frame;
    int             i_poc;

    int             i_frame_offset; /* decoding only */
    int             i_frame_num;    /* decoding only */
    int             i_poc_msb;      /* decoding only */
    int             i_poc_lsb;      /* decoding only */

    /* We use only one SPS and one PPS */
    x264_sps_t      sps_array[32];
    x264_sps_t      *sps;
    x264_pps_t      pps_array[256];
    x264_pps_t      *pps;
    int             i_idr_pic_id;

    /* Slice header */
    x264_slice_header_t sh;

    /* cabac context */
    x264_cabac_t cabac;

    /* current picture being encoded */
    x264_picture_t    *picture;

    /* bframe handling (only encoding for now) */
    x264_frame_t  *bframe_current[X264_BFRAME_MAX]; /* store the sequence of b frame being encoded */
    x264_frame_t  *frame_next[X264_BFRAME_MAX+1];   /* store the next sequence of frames to be encoded */
    x264_frame_t  *frame_unused[X264_BFRAME_MAX+1]; /* store unused frames */

    /* frame being reconstructed */
    x264_frame_t      *fdec;

    /* macroblock status (for current frame) */
    x264_macroblock_t *mb;

    /* frames used for reference */
    x264_frame_t      *freference[16+1];  /* all references frames plus current */
    int               i_ref0;
    x264_frame_t      *fref0[16];       /* ref list 0 */
    int               i_ref1;
    x264_frame_t      *fref1[16];       /* ref list 1 */

    /* rate control encoding only */
    x264_ratecontrol_t *rc;

    /* stats */
    struct
    {
        int i_mb_count[18];
    } stat;

    /* CPU functions dependants */
    x264_predict_t      predict_16x16[4+3];
    x264_predict_t      predict_8x8[4+3];
    x264_predict_t      predict_4x4[9+3];

    x264_pixel_function_t pixf;

    x264_mc_t           mc[2];
    x264_me_t           me;

    x264_dct_function_t dctf;

    /* vlc table for decoding purpose only */
    x264_vlc_table_t *x264_coeff_token_lookup[5];
    x264_vlc_table_t *x264_level_prefix_lookup;
    x264_vlc_table_t *x264_total_zeros_lookup[15];
    x264_vlc_table_t *x264_total_zeros_dc_lookup[3];
    x264_vlc_table_t *x264_run_before_lookup[7];
};

#endif

