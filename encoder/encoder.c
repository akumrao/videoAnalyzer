
/*****************************************************************************
 * x264: h264 encoder

 *****************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include <math.h>

#include "../core/common.h"
#include "../core/cpu.h"

#include "set.h"
#include "analyse.h"
#include "ratecontrol.h"
#include "macroblock.h"

/****************************************************************************
 *
 ******************************* x264 libs **********************************
 *
 ****************************************************************************/


static float x264_psnr(uint8_t *pix1, int i_pix_stride, uint8_t *pix2, int i_pix2_stride, int i_width, int i_height) {LOG_CALL;
    int64_t i_sqe = 0;

    int x, y;

    for (y = 0; y < i_height; y++)
    {
        for (x = 0; x < i_width; x++)
        {
            int tmp;

            tmp = pix1[y * i_pix_stride + x] - pix2[y * i_pix2_stride + x];

            i_sqe += tmp * tmp;
        }
    }

    if (i_sqe == 0)
    {
        return -1.0;
    }
    return (float) (10.0 * log((double) 65025.0 * (double) i_height * (double) i_width / (double) i_sqe) / log(10.0));
}

/* Fill "default" values */
static void x264_slice_header_init(x264_slice_header_t *sh, x264_param_t *param,
        x264_sps_t *sps, x264_pps_t *pps,
        int i_type, int i_idr_pic_id, int i_frame) {LOG_CALL;
    /* First we fill all field */
    sh->sps = sps;
    sh->pps = pps;

    sh->i_type = i_type;
    sh->i_first_mb = 0;
    sh->i_pps_id = pps->i_id;

    sh->i_frame_num = i_frame;

    sh->b_field_pic = 0; /* Not field support for now */
    sh->b_bottom_field = 1; /* not yet used */

    sh->i_idr_pic_id = i_idr_pic_id;

    /* poc stuff, fixed later */
    sh->i_poc_lsb = 0;
    sh->i_delta_poc_bottom = 0;
    sh->i_delta_poc[0] = 0;
    sh->i_delta_poc[1] = 0;

    sh->i_redundant_pic_cnt = 0;

    sh->b_direct_spatial_mv_pred = 0;

    sh->b_num_ref_idx_override = 0;
    sh->i_num_ref_idx_l0_active = 1;
    sh->i_num_ref_idx_l1_active = 1;

    sh->i_cabac_init_idc = param->i_cabac_init_idc;

    sh->i_qp_delta = 0;
    sh->b_sp_for_swidth = 0;
    sh->i_qs_delta = 0;

    if (param->b_deblocking_filter)
    {
        sh->i_disable_deblocking_filter_idc = 0;
    } else
    {
        sh->i_disable_deblocking_filter_idc = 1;
    }
    sh->i_alpha_c0_offset = 0;
    sh->i_beta_offset = 0;
}

static void x264_slice_header_write(bs_t *s, x264_slice_header_t *sh, int i_nal_ref_idc) {LOG_CALL;
    bs_write_ue(s, sh->i_first_mb,"i_first_mb");
    bs_write_ue(s, sh->i_type + 5,"i_type"); /* same type things */
    bs_write_ue(s, sh->i_pps_id,"i_pps_id");
    bs_write(s, sh->sps->i_log2_max_frame_num, sh->i_frame_num,"i_log2_max_frame_num");

    if (sh->i_idr_pic_id >= 0) /* NAL IDR */
    {
        bs_write_ue(s, sh->i_idr_pic_id,"i_idr_pic_id");
    }

    if (sh->sps->i_poc_type == 0)
    {
        bs_write(s, sh->sps->i_log2_max_poc_lsb, sh->i_poc_lsb,"i_log2_max_poc_lsb");
        if (sh->pps->b_pic_order && !sh->b_field_pic)
        {
            bs_write_se(s, sh->i_delta_poc_bottom,"i_delta_poc_bottom");
        }
    } else if (sh->sps->i_poc_type == 1 && !sh->sps->b_delta_pic_order_always_zero)
    {
        bs_write_se(s, sh->i_delta_poc[0],"i_delta_poc[0]");
        if (sh->pps->b_pic_order && !sh->b_field_pic)
        {
            bs_write_se(s, sh->i_delta_poc[1],"i_delta_poc[1]");
        }
    }

    if (sh->pps->b_redundant_pic_cnt)
    {
        bs_write_ue(s, sh->i_redundant_pic_cnt,"i_redundant_pic_cnt");
    }

    if (sh->i_type == SLICE_TYPE_B)
    {
        bs_write1(s, sh->b_direct_spatial_mv_pred, "b_direct_spatial_mv_pred");
    }
    if (sh->i_type == SLICE_TYPE_P || sh->i_type == SLICE_TYPE_SP || sh->i_type == SLICE_TYPE_B)
    {
        bs_write1(s, sh->b_num_ref_idx_override,"b_num_ref_idx_override");
        if (sh->b_num_ref_idx_override)
        {
            bs_write_ue(s, sh->i_num_ref_idx_l0_active - 1, "i_num_ref_idx_l0_active");
            if (sh->i_type == SLICE_TYPE_B)
            {
                bs_write_ue(s, sh->i_num_ref_idx_l1_active - 1,"i_num_ref_idx_l1_active");
            }
        }
    }

    /* ref pic list reordering */
    if (sh->i_type != SLICE_TYPE_I)
    {
        int b_ref_pic_list_reordering_l0 = 0;
        bs_write1(s, b_ref_pic_list_reordering_l0,"b_ref_pic_list_reordering_l0");
        if (b_ref_pic_list_reordering_l0)
        {
            /* FIXME */
        }
    }
    if (sh->i_type == SLICE_TYPE_B)
    {
        int b_ref_pic_list_reordering_l1 = 0;
        bs_write1(s, b_ref_pic_list_reordering_l1,"b_ref_pic_list_reordering_l1");
        if (b_ref_pic_list_reordering_l1)
        {
            /* FIXME */
        }
    }

    if ((sh->pps->b_weighted_pred && (sh->i_type == SLICE_TYPE_P || sh->i_type == SLICE_TYPE_SP)) ||
            (sh->pps->b_weighted_bipred == 1 && sh->i_type == SLICE_TYPE_B))
    {
        /* FIXME */
    }

    if (i_nal_ref_idc != 0)
    {
        if (sh->i_idr_pic_id >= 0)
        {
            bs_write1(s, 0, " no output of prior pics flag "); /* no output of prior pics flag */
            bs_write1(s, 0,"long term reference flag "); /* long term reference flag */
        } else
        {
            bs_write1(s, 0,"adaptive_ref_pic_marking_mode_flag"); /* adaptive_ref_pic_marking_mode_flag */
            /* FIXME */
        }
    }

    if (sh->pps->b_cabac && sh->i_type != SLICE_TYPE_I)
    {
        bs_write_ue(s, sh->i_cabac_init_idc,"i_cabac_init_idc");
    }
    bs_write_se(s, sh->i_qp_delta,"i_qp_delta"); /* slice qp delta */
#if 0
    if (sh->i_type == SLICE_TYPE_SP || sh->i_type == SLICE_TYPE_SI)
    {
        if (sh->i_type == SLICE_TYPE_SP)
        {
            bs_write1(s, sh->b_sp_for_swidth);
        }
        bs_write_se(s, sh->i_qs_delta);
    }
#endif

    if (sh->pps->b_deblocking_filter_control)
    {
        bs_write_ue(s, sh->i_disable_deblocking_filter_idc,"i_disable_deblocking_filter_idc");
        if (sh->i_disable_deblocking_filter_idc != 1)
        {
            bs_write_se(s, sh->i_alpha_c0_offset >> 1,"i_alpha_c0_offset");
            bs_write_se(s, sh->i_beta_offset >> 1,"i_beta_offset");
        }
    }
}
#if 0

static void x264_frame_dump(x264_t *h, x264_frame_t *fr, char *name) {LOG_CALL;
    FILE * f = fopen(name, "a");
    int i, y;

    fseek(f, 0, SEEK_END);

    for (i = 0; i < fr->i_plane; i++)
    {
        for (y = 0; y < h->param.i_height / (i == 0 ? 1 : 2); y++)
        {
            fwrite(&fr->plane[i][y * fr->i_stride[i]], 1, h->param.i_width / (i == 0 ? 1 : 2), f);
        }
    }
    fclose(f);
}
#endif

/****************************************************************************
 *
 ****************************************************************************
 ****************************** External API*********************************
 ****************************************************************************
 *
 ****************************************************************************/

/****************************************************************************
 * x264_encoder_open:
 ****************************************************************************/
x264_t *x264_encoder_open(x264_param_t *param) {LOG_CALL;
    x264_t *h = (x264_t *)x264_aligned_alloc(sizeof ( x264_t));
    int i;
    x264_me_t me[2];

    if (param->i_width <= 0 || param->i_height <= 0)
    {
        fprintf(stderr, "invalid width x height (%dx%d)\n",
                param->i_width, param->i_height);
        free(h);
        return NULL;
    }

    if (param->i_width % 16 != 0 || param->i_height % 16 != 0)
    {
        fprintf(stderr, "width %% 16 != 0 pr height %% 16 != 0 (%dx%d)\n",
                param->i_width, param->i_height);
        free(h);
        return NULL;
    }

    memcpy(&h->param, param, sizeof ( x264_param_t));
    if (h->param.i_frame_reference <= 0)
    {
        h->param.i_frame_reference = 1;
    } else if (h->param.i_frame_reference > 15)
    {
        h->param.i_frame_reference = 15;
    }
    if (h->param.i_idrframe <= 0)
    {
        h->param.i_idrframe = 1;
    }
    if (h->param.i_iframe <= 0)
    {
        h->param.i_iframe = 1;
    }
    if (h->param.i_bframe < 0)
    {
        h->param.i_bframe = 0;
    } else if (h->param.i_bframe > X264_BFRAME_MAX)
    {
        h->param.i_bframe = X264_BFRAME_MAX;
    }
    /* XXX for now we use fixed patern IB...BPB...BPB...BP */
    if (h->param.i_bframe > 0)
    {
        h->param.i_iframe = ((h->param.i_iframe - 1) / (h->param.i_bframe + 1))*(h->param.i_bframe + 1) + 1;

        if (h->param.b_cabac)
        {
            fprintf(stderr, "cabac not supported with B frame (cabac disabled)\n");
            h->param.b_cabac = 0;
        }
    }
    if (h->param.i_cabac_init_idc < -1)
    {
        h->param.i_cabac_init_idc = -1;
    } else if (h->param.i_cabac_init_idc > 2)
    {
        h->param.i_cabac_init_idc = 2;
    }

    h->cpu = param->cpu;

    h->i_nal = 0;
    h->i_bitstream = 1000000; /* FIXME calculate max size from width/height */
    h->p_bitstream = (uint8_t*)x264_aligned_alloc(h->i_bitstream);

    h->i_frame = 0;
    h->i_frame_num = 0;
    h->i_poc = 0;
    h->i_idr_pic_id = 0;

    h->sps = &h->sps_array[0];
    x264_sps_init(h->sps, 0, &h->param);

    h->pps = &h->pps_array[0];
    x264_pps_init(h->pps, 0, &h->param, h->sps);

    /* init picture and bframe context */
    h->picture = NULL;
    for (i = 0; i < X264_BFRAME_MAX; i++)
    {
        h->bframe_current[i] = NULL;
        h->frame_next[i] = NULL;
        h->frame_unused[i] = NULL;
    }
    h->frame_next[X264_BFRAME_MAX] = NULL;
    h->frame_unused[X264_BFRAME_MAX] = NULL;
    for (i = 0; i < h->param.i_bframe + 1; i++)
    {
        h->frame_unused[i] = x264_frame_new(h);
    }

    h->mb = x264_macroblocks_new(h->sps->i_mb_width, h->sps->i_mb_height);

    /* reference frame (2 for B frame) and current one */
    for (i = 0; i < h->param.i_frame_reference + 2; i++)
    {
        h->freference[i] = x264_frame_new(h);
    }
    h->fdec = h->freference[0];
    h->i_ref0 = 0;
    h->i_ref1 = 0;

    /* init cabac adaptive model */
    x264_cabac_model_init(&h->cabac);

    /* init predict_XxX */
    x264_predict_16x16_init(h->cpu, h->predict_16x16);
    x264_predict_8x8_init(h->cpu, h->predict_8x8);
    x264_predict_4x4_init(h->cpu, h->predict_4x4);

    x264_pixel_init(h->cpu, &h->pixf);
    x264_dct_init(h->cpu, &h->dctf);

    x264_mc_init(h->cpu, h->mc);

    x264_me_init(h->cpu, me);
    h->me = me[h->param.i_me];

    /* rate control */
    h->rc = x264_ratecontrol_new(&h->param);

    return h;
}

/****************************************************************************
 * x264_nal_encode:
 ****************************************************************************/
int x264_nal_encode(uint8_t *p_data, int *pi_data, int b_annexeb, x264_nal_t *nal) {LOG_CALL;
    uint8_t *dst = p_data;
    uint8_t *src = nal->p_payload;
    uint8_t *end = &nal->p_payload[nal->i_payload];

    int i_count = 0;

    /* FIXME this code doesn't check overflow */

    if (b_annexeb)
    {
        /* long nal start code (we always use long ones)*/
        *dst++ = 0x00;
        *dst++ = 0x00;
        *dst++ = 0x00;
        *dst++ = 0x01;
    }

    /* nal header */
    *dst++ = (0x00 << 7) | (nal->i_ref_idc << 5) | nal->i_type;

    while (src < end)
    {
        if (i_count == 2 && *src <= 0x03)
        {
            *dst++ = 0x03;
            i_count = 0;
        }
        if (*src == 0)
        {
            i_count++;
        } else
        {
            i_count = 0;
        }
        *dst++ = *src++;
    }
    *pi_data = dst - (uint8_t*) p_data;

    return *pi_data;
}

/* internal usage */
static void x264_nal_start(x264_t *h, int i_type, int i_ref_idc) {LOG_CALL;
    x264_nal_t *nal = &h->nal[h->i_nal];

    nal->i_ref_idc = i_ref_idc;
    nal->i_type = i_type;

    bs_align_0(&h->bs); /* not needed */

    nal->i_payload = 0;
    nal->p_payload = &h->p_bitstream[bs_pos(&h->bs) / 8];
}

static void x264_nal_end(x264_t *h) {LOG_CALL;
    x264_nal_t *nal = &h->nal[h->i_nal];

    bs_align_0(&h->bs); /* not needed */

    nal->i_payload = &h->p_bitstream[bs_pos(&h->bs) / 8] - nal->p_payload;

    h->i_nal++;
}

/****************************************************************************
 * x264_encoder_headers:
 ****************************************************************************/
int x264_encoder_headers(x264_t *h, x264_nal_t **pp_nal, int *pi_nal) {LOG_CALL;
    /* init bitstream context */
    h->i_nal = 0;
    bs_init(&h->bs, h->p_bitstream, h->i_bitstream);

    /* Put SPS and PPS */
    if (h->i_frame == 0)
    {
        /* generate sequence parameters */
        x264_nal_start(h, NAL_SPS, NAL_PRIORITY_HIGHEST);
        x264_sps_write(&h->bs, h->sps);
        x264_nal_end(h);

        /* generate picture parameters */
        x264_nal_start(h, NAL_PPS, NAL_PRIORITY_HIGHEST);
        x264_pps_write(&h->bs, h->pps);
        x264_nal_end(h);
    }
    /* now set output*/
    *pi_nal = h->i_nal;
    *pp_nal = &h->nal[0];

    return 0;
}

static void x264_encoder_frame_put(x264_frame_t *buffer[X264_BFRAME_MAX], x264_frame_t *frame) {LOG_CALL;
    int i;

    /* put the frame in buffer */
    for (i = 0; buffer[i] != NULL; i++)
    {
        if (i >= X264_BFRAME_MAX)
        {
            fprintf(stderr, "not enough room\n");
            return;
        }
    }
    buffer[i] = frame;
}

static x264_frame_t *x264_encoder_frame_put_from_picture(x264_t *h, x264_frame_t *buffer[X264_BFRAME_MAX], x264_picture_t *picture) {LOG_CALL;
    int i;
    x264_frame_t *frame;

    /* get an unused bframe */
    frame = h->frame_unused[0];
    for (i = 0; i < X264_BFRAME_MAX; i++)
    {
        h->frame_unused[i] = h->frame_unused[i + 1];
    }
    h->frame_unused[X264_BFRAME_MAX] = NULL;

    /* copy the picture in it */
    x264_frame_copy_picture(frame, picture);

    /* put the frame in buffer */
    x264_encoder_frame_put(buffer, frame);

    buffer[i] = frame;
    return frame;
}

static x264_frame_t *x264_encoder_frame_get(x264_frame_t *buffer[X264_BFRAME_MAX]) {LOG_CALL;
    int i;
    x264_frame_t *frame = buffer[0];

    for (i = 0; i < X264_BFRAME_MAX - 1; i++)
    {
        buffer[i] = buffer[i + 1];
    }
    buffer[X264_BFRAME_MAX - 1] = NULL;

    return frame;
}

static int64_t i_mtime_analyse = 0;
static int64_t i_mtime_encode = 0;
static int64_t i_mtime_write = 0;
static int64_t i_mtime_filter = 0;
#define TIMER_START( d ) \
    { \
        int64_t d##start = x264_mdate();

#define TIMER_STOP( d ) \
        d += x264_mdate() - d##start;\
    }

/****************************************************************************
 * x264_encoder_encode:
 *  XXX: i_poc   : is the poc of the current given picture
 *       i_frame : is the number of the frame being coded
 *  ex:  type frame poc
 *       I      0   2*0
 *       P      1   2*3
 *       B      2   2*1
 *       B      3   2*2
 *       P      4   2*6
 *       B      5   2*4
 *       B      6   2*5
 ****************************************************************************/
int x264_encoder_encode(x264_t *h,
        x264_nal_t **pp_nal, int *pi_nal,
        x264_picture_t *pic) {LOG_CALL;
    x264_picture_t picture_tmp;
    x264_frame_t *frame_psnr = h->fdec; /* just to kept the current decoded frame for psnr calculation */
    int i_nal_type;
    int i_nal_ref_idc;
    int i_slice_type;

    int mb_xy;

    int i;
    int i_skip;

    int b_ok;

    float psnr_y, psnr_u, psnr_v;

    /* no data out */
    *pp_nal = NULL;
    *pi_nal = 0;

    /* ------------------- Select slice type and frame --------------------- */
    if (h->i_frame % (h->param.i_iframe * h->param.i_idrframe) == 0)
    {
        i_nal_type = NAL_SLICE_IDR;
        i_nal_ref_idc = NAL_PRIORITY_HIGHEST;
        i_slice_type = SLICE_TYPE_I;

        /* we encode the given frame */
        h->picture = pic;

        /* null poc */
        h->i_poc = 0;
        h->fdec->i_poc = 0;

        /* null frame */
        h->i_frame_num = 0;

        /* reset ref pictures */
        for (i = 1; i < h->param.i_frame_reference + 2; i++)
        {
            h->freference[i]->i_poc = -1;
        }
        h->freference[0]->i_poc = 0;
    } else
    {
        /* TODO detect scene changes and switch to I slice */

        if (h->param.i_bframe > 0)
        {
            x264_frame_t *frame;

            /* TODO avoid always doing x264_encoder_frame_put_from_picture */
            /* TODO implement adaptive B frame patern (not fixed one)*/

            /* put the current picture in frame_next */
            frame = x264_encoder_frame_put_from_picture(h, h->frame_next, pic);
            frame->i_poc = h->i_poc;

            /* get the frame to be encoded */
            if (h->bframe_current[0])
            {
                frame = x264_encoder_frame_get(h->bframe_current);
                i_slice_type = SLICE_TYPE_B;
            } else if (h->frame_next[h->param.i_bframe] == NULL)
            {
                /* not enough b-frame yet */
                frame = NULL;
                i_slice_type = -1;
            } else
            {
                int i_mod = h->i_frame % h->param.i_iframe;

                if (i_mod == 0)
                {
                    i_slice_type = SLICE_TYPE_I;
                    frame = x264_encoder_frame_get(h->frame_next);
                } else
                {
                    int i;

                    for (i = 0; i < h->param.i_bframe; i++)
                    {
                        h->bframe_current[i] = x264_encoder_frame_get(h->frame_next);
                    }
                    frame = x264_encoder_frame_get(h->frame_next);

                    i_slice_type = SLICE_TYPE_P;
                }
            }

            if (frame)
            {
                h->picture = &picture_tmp;
                for (i = 0; i < frame->i_plane; i++)
                {
                    h->picture->i_stride[i] = frame->i_stride[i];
                    h->picture->plane[i] = frame->plane[i];
                }
                h->fdec->i_poc = frame->i_poc;

                x264_encoder_frame_put(h->frame_unused, frame); /* it's ok to do it now */
            } else
            {
                h->picture = NULL;
            }
        } else
        {
            i_slice_type = h->i_frame % h->param.i_iframe == 0 ? SLICE_TYPE_I : SLICE_TYPE_P;

            /* we encode the given frame */
            h->picture = pic;
            h->fdec->i_poc = h->i_poc; /* low delay */
        }

        i_nal_type = NAL_SLICE;
        if (i_slice_type == SLICE_TYPE_I || i_slice_type == SLICE_TYPE_P)
        {
            i_nal_ref_idc = NAL_PRIORITY_HIGH; /* Not completely true but for now it is (as all I/P are kept as ref)*/
        } else /* if( i_slice_type == SLICE_TYPE_B ) */
        {
            i_nal_ref_idc = NAL_PRIORITY_DISPOSABLE;
        }
    }

    /* increase poc */
    h->i_poc += 2;

    if (h->picture == NULL)
    {
        /* waiting for filling bframe buffer */
        return 0;
    }

    /* increase frame num but only once for B frame */
    if (i_slice_type != SLICE_TYPE_B || h->sh.i_type != SLICE_TYPE_B)
    {
        h->i_frame_num++;
    }

    /* build ref list 0/1 */
    h->i_ref0 = 0;
    h->i_ref1 = 0;
    for (i = 1; i < h->param.i_frame_reference + 2; i++)
    {
        if (h->freference[i]->i_poc >= 0)
        {
            if (h->freference[i]->i_poc < h->fdec->i_poc)
            {
                h->fref0[h->i_ref0++] = h->freference[i];
            } else if (h->freference[i]->i_poc > h->fdec->i_poc)
            {
                h->fref1[h->i_ref1++] = h->freference[i];
            }
        }
    }
    /* Order ref0 from higher to lower poc */
    do
    {
        b_ok = 1;
        for (i = 0; i < h->i_ref0 - 1; i++)
        {
            if (h->fref0[i]->i_poc < h->fref0[i + 1]->i_poc)
            {
                x264_frame_t *tmp = h->fref0[i + 1];

                h->fref0[i + 1] = h->fref0[i];
                h->fref0[i] = tmp;
                b_ok = 0;
                break;
            }
        }
    } while (!b_ok);
    /* Order ref1 from lower to higher poc (bubble sort) for B-frame */
    do
    {
        b_ok = 1;
        for (i = 0; i < h->i_ref1 - 1; i++)
        {
            if (h->fref1[i]->i_poc > h->fref1[i + 1]->i_poc)
            {
                x264_frame_t *tmp = h->fref1[i + 1];

                h->fref1[i + 1] = h->fref1[i];
                h->fref1[i] = tmp;
                b_ok = 0;
                break;
            }
        }
    } while (!b_ok);

    if (h->i_ref0 > h->param.i_frame_reference)
    {
        h->i_ref0 = h->param.i_frame_reference;
    }
    if (h->i_ref1 > 1)
    {
        h->i_ref1 = 1;
    }


    /* Init the rate control */
    x264_ratecontrol_start(h->rc, i_slice_type);

    /* ------------------------ Create slice header  ----------------------- */
    if (i_nal_type == NAL_SLICE_IDR)
    {
        x264_slice_header_init(&h->sh, &h->param, h->sps, h->pps, i_slice_type, h->i_idr_pic_id, h->i_frame_num - 1);

        /* increment id */
        h->i_idr_pic_id = (h->i_idr_pic_id + 1) % 65535;
    } else
    {
        x264_slice_header_init(&h->sh, &h->param, h->sps, h->pps, i_slice_type, -1, h->i_frame_num - 1);

        /* always set the real higher num of ref frame used */
        h->sh.b_num_ref_idx_override = 1;
        h->sh.i_num_ref_idx_l0_active = h->i_ref0 <= 0 ? 1 : h->i_ref0;
        h->sh.i_num_ref_idx_l1_active = h->i_ref1 <= 0 ? 1 : h->i_ref1;
    }

    /*Then set i_poc_type to 0. The POC type specifies the encoding method of POC. The POC identifies the playback order of the image. Since H.264 uses B frames, the decoding order of images is not necessarily equal to the playback order, but they have a certain mapping relationship. The POC type can be calculated from Frame_num through the mapping relationship, or it can simply be transmitted to the decoder through display transmission by the encoder. There are three solutions for the POC type.*/
    if (h->sps->i_poc_type == 0) 
    {
        //    int i_poc_lsb; /* decoding only */    // int i_log2_max_poc_lsb; //Indicates the max value of the variable i_poc_lsb

        h->sh.i_poc_lsb = h->fdec->i_poc & ((1 << h->sps->i_log2_max_poc_lsb) - 1);
        h->sh.i_delta_poc_bottom = 0; /* XXX won't work for field */
    } else if (h->sps->i_poc_type == 1)
    {
        /* FIXME TODO FIXME */
    } else
    {
        /* Nothing to do ? */  //error 
    }

    /* global qp */
    h->sh.i_qp_delta = x264_ratecontrol_qp(h->rc) - h->pps->i_pic_init_qp;

    /* get adapative cabac model if needed */
    if (h->param.b_cabac)
    {
        if (h->param.i_cabac_init_idc == -1)
        {
            h->sh.i_cabac_init_idc = x264_cabac_model_get(&h->cabac, i_slice_type);
        }
    }


    /* ---------------------- Write the bitstream -------------------------- */

    /* init bitstream context */
    h->i_nal = 0;
    bs_init(&h->bs, h->p_bitstream, h->i_bitstream);

    /* Put SPS and PPS */
    if (h->i_frame == 0)
    {
        /* generate sequence parameters */
        x264_nal_start(h, NAL_SPS, NAL_PRIORITY_HIGHEST);
        x264_sps_write(&h->bs, h->sps);

        x264_sps_t *sps = h->sps;
        fprintf(stderr, "x264_sps_read: sps:0x%x profile:%d/%d poc:%d ref:%d %xx%d crop:%d-%d-%d-%d\n",
                sps->i_id,
                sps->i_profile_idc, sps->i_level_idc,
                sps->i_poc_type,
                sps->i_num_ref_frames,
                sps->i_mb_width, sps->i_mb_height,
                sps->crop.i_left, sps->crop.i_right,
                sps->crop.i_top, sps->crop.i_bottom);

        x264_nal_end(h);

        /* generate picture parameters */
        x264_nal_start(h, NAL_PPS, NAL_PRIORITY_HIGHEST);
        x264_pps_write(&h->bs, h->pps);

        x264_pps_t *pps = h->pps;
        fprintf(stderr, "x264_sps_read: pps:0x%x sps:0x%x %s slice_groups=%d ref0:%d ref1:%d QP:%d QS:%d QC=%d DFC:%d CIP:%d RPC:%d\n",
                pps->i_id, pps->i_sps_id,
                pps->b_cabac ? "CABAC" : "CAVLC",
                pps->i_num_slice_groups,
                pps->i_num_ref_idx_l0_active,
                pps->i_num_ref_idx_l1_active,
                pps->i_pic_init_qp, pps->i_pic_init_qs, pps->i_chroma_qp_index_offset,
                pps->b_deblocking_filter_control,
                pps->b_constrained_intra_pred,
                pps->b_redundant_pic_cnt);


        x264_nal_end(h);
    }

    /* Reset stats */
    for (i = 0; i < 18; i++) h->stat.i_mb_count[i] = 0;

    /* Slice */
    x264_nal_start(h, i_nal_type, i_nal_ref_idc);

    /* Slice header */
    x264_slice_header_write(&h->bs, &h->sh, i_nal_ref_idc);
    if (h->param.b_cabac)
    {
        /* alignement needed */
        bs_align_1(&h->bs);

        /* init cabac */
        x264_cabac_context_init(&h->cabac, h->sh.i_type, h->sh.pps->i_pic_init_qp + h->sh.i_qp_delta, h->sh.i_cabac_init_idc);
        x264_cabac_encode_init(&h->cabac, &h->bs);
    }

    for (mb_xy = 0, i_skip = 0; mb_xy < h->sps->i_mb_width * h->sps->i_mb_height; mb_xy++)
    {
        myPrintf( "mb_xy %d \n", mb_xy);
        
        x264_mb_context_t context;
        x264_macroblock_t *mb;

        mb = &h->mb[mb_xy];

        /* load neighbour */
        x264_macroblock_context_load(h, mb, &context);

        /* analyse parameters
         * Slice I: choose I_4x4 or I_16x16 mode
         * Slice P: choose between using P mode or intra (4x4 or 16x16)
         * */
        TIMER_START(i_mtime_analyse);
        x264_macroblock_analyse(h, &h->mb[mb_xy]);
        TIMER_STOP(i_mtime_analyse);

        /* encode this macrobock -> be carefull it can change the mb type to P_SKIP if needed */
        TIMER_START(i_mtime_encode);
        x264_macroblock_encode(h, mb);
        TIMER_STOP(i_mtime_encode);

        TIMER_START(i_mtime_write);
        if (IS_SKIP(mb->i_type))
        {
            if (h->param.b_cabac)
            {
                if (mb_xy > 0)
                {
                    /* not end_of_slice_flag */
                    x264_cabac_encode_terminal(&h->cabac, 0);
                }

                x264_cabac_mb_skip(h, mb, 1);
            } else
            {
                i_skip++;
            }
        } else
        {
            if (h->param.b_cabac)
            {
                if (mb_xy > 0)
                {
                    /* not end_of_slice_flag */
                    x264_cabac_encode_terminal(&h->cabac, 0);
                }
                if (i_slice_type != SLICE_TYPE_I)
                {
                    x264_cabac_mb_skip(h, mb, 0);
                }
                x264_macroblock_write_cabac(h, &h->bs, mb);
            } else
            {
                if (i_slice_type != SLICE_TYPE_I)
                {
                    bs_write_ue(&h->bs, i_skip,"i_skip"); /* skip run */
                    i_skip = 0;
                }
                x264_macroblock_write_cavlc(h, &h->bs, mb);
            }
        }
        TIMER_STOP(i_mtime_write);
        h->stat.i_mb_count[mb->i_type]++;
    }

    if (h->param.b_cabac)
    {
        /* end of slice */
        x264_cabac_encode_terminal(&h->cabac, 1);
    } else if (i_skip > 0)
    {
        bs_write_ue(&h->bs, i_skip,"i_skip"); /* last skip run */
    }

    if (h->param.b_cabac)
    {
        int i_cabac_word;
        x264_cabac_encode_flush(&h->cabac);
        /* TODO cabac stuffing things (p209) */
        i_cabac_word = (((3 * h->cabac.i_sym_cnt - 3 * 96 * h->sps->i_mb_width * h->sps->i_mb_height) / 32) - bs_pos(&h->bs) / 8) / 3;

        while (i_cabac_word > 0)
        {
            bs_write(&h->bs, 16, 0x0000,"i_cabac_word");
            i_cabac_word--;
        }
    } else
    {
        /* rbsp_slice_trailing_bits */
        bs_rbsp_trailing(&h->bs);
    }

    x264_nal_end(h);

    /* now set output*/
    *pi_nal = h->i_nal;
    *pp_nal = &h->nal[0];

    /* update cabac */
    if (h->param.b_cabac)
    {
        x264_cabac_model_update(&h->cabac, i_slice_type, h->sh.pps->i_pic_init_qp + h->sh.i_qp_delta);
    }
    /* update rc */
    x264_ratecontrol_end(h->rc, h->nal[h->i_nal - 1].i_payload * 8);

    /* ----------------------- handle frame reference ---------------------- */
    if (i_nal_ref_idc != NAL_PRIORITY_DISPOSABLE)
    {
        /* apply deblocking filter to the current decoded picture */
        if (h->param.b_deblocking_filter)
        {
            TIMER_START(i_mtime_filter);
            x264_frame_deblocking_filter(h, i_slice_type);
            TIMER_STOP(i_mtime_filter);
        }
        /* expand border */
        x264_frame_expand_border(h->fdec);

        /* move frame in the buffer */
        h->fdec = h->freference[h->param.i_frame_reference + 1];
        for (i = h->param.i_frame_reference + 1; i > 0; i--)
        {
            h->freference[i] = h->freference[i - 1];
        }
        h->freference[0] = h->fdec;
    }

    /* increase frame count */
    h->i_frame++;

    /* restore CPU state (before using float again) */
    x264_cpu_restore(h->cpu);

    /* PSNR */
    psnr_y = x264_psnr(frame_psnr->plane[0], frame_psnr->i_stride[0], h->picture->plane[0], h->picture->i_stride[0], h->param.i_width, h->param.i_height);
    psnr_u = x264_psnr(frame_psnr->plane[1], frame_psnr->i_stride[1], h->picture->plane[1], h->picture->i_stride[1], h->param.i_width / 2, h->param.i_height / 2);
    psnr_v = x264_psnr(frame_psnr->plane[2], frame_psnr->i_stride[2], h->picture->plane[2], h->picture->i_stride[2], h->param.i_width / 2, h->param.i_height / 2);

    /* print stat */
    fprintf(stderr, "frame=%4d NAL=%d Slice:%c Poc:%-3d I4x4:%-5d I16x16:%-5d P:%-5d SKIP:%-3d size=%d bytes PSNR Y:%2.2f U:%2.2f V:%2.2f\n",
            h->i_frame - 1,
            i_nal_ref_idc,
            i_slice_type == SLICE_TYPE_I ? 'I' : (i_slice_type == SLICE_TYPE_P ? 'P' : 'B'),
            frame_psnr->i_poc,
            h->stat.i_mb_count[I_4x4],
            h->stat.i_mb_count[I_16x16],
            h->stat.i_mb_count[P_L0] + h->stat.i_mb_count[P_8x8],
            h->stat.i_mb_count[P_SKIP],
            h->nal[h->i_nal - 1].i_payload,
            psnr_y, psnr_u, psnr_v);

#if 0
    /* Dump reconstructed frame */
    x264_frame_dump(h, frame_psnr, "fdec.yuv");
#endif
#if 0
    if (h->i_ref0 > 0)
    {
        x264_frame_dump(h, h->fref0[0], "ref0.yuv");
    }
    if (h->i_ref1 > 0)
    {
        x264_frame_dump(h, h->fref1[0], "ref1.yuv");
    }
#endif
    return 0;
}

/****************************************************************************
 * x264_encoder_close:
 ****************************************************************************/
void x264_encoder_close(x264_t *h) {LOG_CALL;
    int64_t i_mtime_total = i_mtime_analyse + i_mtime_encode + i_mtime_write + i_mtime_filter + 1;
    int i;

    fprintf(stderr, "x264: analyse=%d(%lldms) encode=%d(%lldms) write=%d(%lldms) filter=%d(%lldms)",
            (int) (100 * i_mtime_analyse / i_mtime_total), i_mtime_analyse / 1000,
            (int) (100 * i_mtime_encode / i_mtime_total), i_mtime_encode / 1000,
            (int) (100 * i_mtime_write / i_mtime_total), i_mtime_write / 1000,
            (int) (100 * i_mtime_filter / i_mtime_total), i_mtime_filter / 1000);

    /* ref frames */
    for (i = 0; i < h->param.i_frame_reference + 2; i++)
    {
        x264_frame_delete(h->freference[i]);
    }

    /* empty all allocated picture for bframes */
    for (i = 0; i < h->param.i_bframe; i++)
    {
        if (h->frame_unused[i])
        {
            x264_frame_delete(h->frame_unused[i]);
        }
        if (h->bframe_current[i])
        {
            x264_frame_delete(h->bframe_current[i]);
        }
        if (h->frame_next[i])
        {
            x264_frame_delete(h->frame_next[i]);
        }
    }

    /* rc */
    x264_ratecontrol_delete(h->rc);

    x264_free(h->mb);
    x264_free(h->p_bitstream);
    x264_free(h);
}

