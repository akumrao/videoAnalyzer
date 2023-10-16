
/*****************************************************************************
 * x264.h: h264 encoder library
 *****************************************************************************

 *****************************************************************************/

#ifndef _X264_H
#define _X264_H 1

#define X264_BUILD 0x0002

/* x264_t:
 *      opaque handler for decoder and encoder */
typedef struct x264_t x264_t;

/****************************************************************************
 * Initialisation structure and function.
 ****************************************************************************/
/* CPU flags
 */
#define X264_CPU_MMX        0x000001    /* mmx */
#define X264_CPU_MMXEXT     0x000002    /* mmx-ext*/
#define X264_CPU_SSE        0x000004    /* sse */
#define X264_CPU_SSE2       0x000008    /* sse 2 */
#define X264_CPU_3DNOW      0x000010    /* 3dnow! */
#define X264_CPU_3DNOWEXT   0x000020    /* 3dnow! ext */
#define X264_CPU_ALTIVEC    0x000040    /* altivec */

typedef struct
{
    /* CPU flags */
    int         cpu;

    /* Video Properties */
    int         i_width;
    int         i_height;

    struct
    {
        /* they will be reduced to be 0 &lt; x &lt;= 65535 */
        int         i_sar_height;
        int         i_sar_width;
    } vui;

    float       f_fps;  /* Used for rate control only */

    /* Encoder parameters */
    int         i_frame_reference;  /* Maximum number of reference frames */
    int         i_idrframe; /* every i_idrframe I frame are marked as IDR */
    int         i_iframe;   /* every i_iframe are intra */
    int         i_bframe;   /* how many b-frame between 2 references pictures */

    int         b_deblocking_filter;

    int         b_cabac;
    int         i_cabac_init_idc;

    int         i_qp_constant;  /* 1-51 */
    int         i_bitrate;      /* not working yet */

    int         i_me;           /* 0(slow), 1(faster) */

} x264_param_t;

/* x264_param_default:
 *      fill x264_param_t with default values and does CPU detection */
void    x264_param_default( x264_param_t * );

/****************************************************************************
 * Picture structures and functions.
 ****************************************************************************/
typedef struct
{
    int     i_width;
    int     i_height;

    int     i_plane;
    int     i_stride[4];
    uint8_t *plane[4];
} x264_picture_t;

/* x264_picture_new:
 *      create a picture for the encoder */
x264_picture_t *x264_picture_new( x264_t * );

/* x264_picture_delete:
 *      destroy a x264_picture_t */
void            x264_picture_delete( x264_picture_t * );

// arvind IDR (Instantaneous Decoder Refresh)
/****************************************************************************
 * NAL structure and functions:
 The coded data that makes up a slice is placed in three separate Data Partitions (A, B and C), each containing a subset of the coded slice. Partition A contains the slice header and header data for each macroblock in the slice, Partition B contains coded residual data for Intra and SI slice macroblocks and Partition C contains coded residual data for inter coded macroblocks (forward and bi-directional). Each Partition can be placed in a separate NAL unit and may therefore be transported separately.

If Partition A data is lost, it is likely to be difﬁcult or impossible to reconstruct the slice, hence Partition A is highly sensitive to transmission errors. Partitions B and C can (with careful choice of coding parameters) be made to be independently decodeable and so a decoder may (for example) decode A and B only, or A and C only, lending ﬂexibility in an error-prone environment.
 ****************************************************************************/
/* nal */
enum nal_unit_type_e
{
    NAL_UNKNOWN = 0,
    NAL_SLICE   = 1,
    NAL_SLICE_DPA   = 2,
    NAL_SLICE_DPB   = 3,
    NAL_SLICE_DPC   = 4,
    NAL_SLICE_IDR   = 5,    /* ref_idc != 0 */// 3  for slice i and p 1
    NAL_SEI         = 6,    /* ref_idc == 0 */
    NAL_SPS         = 7,     /* ref_idc == 3 */  // importand need to keep it for refrence 
    NAL_PPS         = 8      /* ref_idc == 3 */
    /* ref_idc == 0 for 6,9,10,11,12 */  // not important no need to keep it for reference
};
enum nal_priority_e
{
    NAL_PRIORITY_DISPOSABLE = 0,  
    NAL_PRIORITY_LOW        = 1,  
    NAL_PRIORITY_HIGH       = 2, // slice i and pi
    NAL_PRIORITY_HIGHEST    = 3, // IDR  sps and pps 
};

typedef struct
{
    int i_ref_idc;  /* nal_priority_e */
    int i_type;     /* nal_unit_type_e */

    /* This data are raw payload */
    int     i_payload;
    uint8_t *p_payload;
} x264_nal_t;

/* x264_nal_encode:
 *      encode a nal into a buffer, setting the size.
 *      if b_annexeb then a long synch work is added
 *      XXX: it currently doesn't check for overflow */
int x264_nal_encode( uint8_t *, int *, int b_annexeb, x264_nal_t *nal );

/* x264_nal_decode:
 *      decode a buffer nal into a x264_nal_t */
int x264_nal_decode( x264_nal_t *nal, uint8_t *, int );

/****************************************************************************
 * Encoder functions:
 ****************************************************************************/

/* x264_encoder_open:
 *      create a new encoder handler, all parameters from x264_param_t are copied */
x264_t *x264_encoder_open   ( x264_param_t * );
/* x264_encoder_headers:
 *      return the SPS and PPS that will be used for the complete stream */
int     x264_encoder_headers( x264_t *, x264_nal_t **, int * );
/* x264_encoder_encode:
 *      encode one picture.
 *      the picture *have* to be created by x264_picture_new */
int     x264_encoder_encode ( x264_t *, x264_nal_t **, int *, x264_picture_t * );
/* x264_encoder_close:
 *      close an encoder handler */
void    x264_encoder_close  ( x264_t * );

/****************************************************************************
 * Decoder functions:
 ****************************************************************************
 * XXX: Not yet working so do not try ...
 ****************************************************************************/
/* x264_decoder_open:
 */
x264_t *x264_decoder_open   ( x264_param_t * );
/* x264_decoder_decode:
 */
int     x264_decoder_decode ( x264_t *, x264_picture_t **, x264_nal_t * );
/* x264_decoder_close:
 */
void    x264_decoder_close  ( x264_t * );

/****************************************************************************
 * Private stuff for internal usage:
 ****************************************************************************/
#ifdef __X264__
#   ifdef _MSC_VER
#   define inline __inline
#   endif
#endif

#endif

