
/***************************************************************************** 
 * set: h264 encoder (SPS and SPS init and write) 

 *****************************************************************************/

#include <stdlib.h>  
#include <stdio.h>  
#include <string.h>  
#include <stdint.h>  

#include "../core/common.h"    
//#include "../x264.h"  
//#include "../core/bs.h"  
//#include "../core/set.h"  

void x264_sps_init(x264_sps_t *sps, int i_id, x264_param_t *param) {LOG_CALL;
    sps->i_id = i_id;
    sps->i_profile_idc = PROFILE_EXTENTED;
    sps->i_level_idc = 21; /* FIXME ? */
    sps->b_constraint_set0 = 0;
    sps->b_constraint_set1 = 0;
    sps->b_constraint_set2 = 0;

    sps->i_log2_max_frame_num = 8;
    sps->i_poc_type = 0;
    if (sps->i_poc_type == 0)
    {
        sps->i_log2_max_poc_lsb = 8;
    }
    else if (sps->i_poc_type == 1)
    {
        int i;

        /* FIXME */
        sps->b_delta_pic_order_always_zero = 1;
        sps->i_offset_for_non_ref_pic = 0;
        sps->i_offset_for_top_to_bottom_field = 0;
        sps->i_num_ref_frames_in_poc_cycle = 0;

        for (i = 0; i < sps->i_num_ref_frames_in_poc_cycle; i++)
        {
            sps->i_offset_for_ref_frame[i] = 0;
        }
    }

    sps->i_num_ref_frames = param->i_frame_reference + 1; /* +1 for 2 ref in B */
    sps->b_gaps_in_frame_num_value_allowed = 0;
    sps->i_mb_width = (param->i_width + 15) / 16;
    sps->i_mb_height = (param->i_height + 15) / 16;
    sps->b_frame_mbs_only = 1;
    sps->b_mb_adaptive_frame_field = 0;
    sps->b_direct8x8_inference = 0;
    if (sps->b_frame_mbs_only == 0)
    {
        sps->b_direct8x8_inference = 1;
    }

    if (param->i_width % 16 != 0 || param->i_height % 16 != 0)
    {
        sps->b_crop = 1;
        sps->crop.i_left = 0;
        sps->crop.i_right = (16 - param->i_width % 16) / 2;
        sps->crop.i_top = 0;
        sps->crop.i_bottom = (16 - param->i_height % 16) / 2;
    }
    else
    {
        sps->b_crop = 0;
        sps->crop.i_left = 0;
        sps->crop.i_right = 0;
        sps->crop.i_top = 0;
        sps->crop.i_bottom = 0;
    }

    if (param->vui.i_sar_width > 0 && param->vui.i_sar_height > 0)
    {
        int w = param->vui.i_sar_width;
        int h = param->vui.i_sar_height;
        int a = w, b = h;

        while (b != 0)
        {
            int t = a;

            a = b;
            b = t % b;
        }

        w /= a;
        h /= a;
        while (w > 65535 || h > 65535)
        {
            w /= 2;
            h /= 2;
        }

        if (w == 0 || h == 0)
        {
            fprintf(stderr, "x264: cannot create valid sample aspect ratio\n");
            sps->b_vui = 0;
        }
        else if (w == h)
        {
            fprintf(stderr, "x264: no need for a SAR\n");
            sps->b_vui = 0;
        }
        else
        {
            fprintf(stderr, "x264: using SAR=%d/%d\n", w, h);
            sps->b_vui = 1;
            sps->vui.i_sar_width = w;
            sps->vui.i_sar_height = h;
        }
    }
    else
    {
        sps->b_vui = 0;
    }
}

void x264_sps_write(bs_t *s, x264_sps_t *sps) {LOG_CALL;
    bs_write(s, 8, sps->i_profile_idc, "i_profile_idc");
    bs_write(s, 1, sps->b_constraint_set0,"b_constraint_set0");
    bs_write(s, 1, sps->b_constraint_set1,"b_constraint_set1");
    bs_write(s, 1, sps->b_constraint_set2,"b_constraint_set2");

    bs_write(s, 5, 0,"reserved"); /* reserved */

    bs_write(s, 8, sps->i_level_idc,"i_level_idc");

    bs_write_ue(s, sps->i_id,"i_id");
    bs_write_ue(s, sps->i_log2_max_frame_num - 4,"i_log2_max_frame_num");
    bs_write_ue(s, sps->i_poc_type,"i_poc_type");
    if (sps->i_poc_type == 0)
    {
        bs_write_ue(s, sps->i_log2_max_poc_lsb - 4,"i_log2_max_poc_lsb");
    }
    else if (sps->i_poc_type == 1)
    {
        int i;

        bs_write(s, 1, sps->b_delta_pic_order_always_zero,"b_delta_pic_order_always_zero");
        bs_write_se(s, sps->i_offset_for_non_ref_pic,"i_offset_for_non_ref_pic");
        bs_write_se(s, sps->i_offset_for_top_to_bottom_field,"i_offset_for_top_to_bottom_field");
        bs_write_ue(s, sps->i_num_ref_frames_in_poc_cycle,"i_num_ref_frames_in_poc_cycle");

        for (i = 0; i < sps->i_num_ref_frames_in_poc_cycle; i++)
        {
            bs_write_se(s, sps->i_offset_for_ref_frame[i],"i_offset_for_ref_frame");
        }
    }
    bs_write_ue(s, sps->i_num_ref_frames,"i_num_ref_frames");
    bs_write(s, 1, sps->b_gaps_in_frame_num_value_allowed,"b_gaps_in_frame_num_value_allowed");
    bs_write_ue(s, sps->i_mb_width - 1,"i_mb_width");
    bs_write_ue(s, sps->i_mb_height - 1,"i_mb_height");
    bs_write(s, 1, sps->b_frame_mbs_only,"b_frame_mbs_only");
    if (!sps->b_frame_mbs_only)
    {
        bs_write(s, 1, sps->b_mb_adaptive_frame_field,"b_mb_adaptive_frame_field");
    }
    bs_write(s, 1, sps->b_direct8x8_inference,"b_direct8x8_inference");

    bs_write(s, 1, sps->b_crop, "b_crop");
    if (sps->b_crop)
    {
        bs_write_ue(s, sps->crop.i_left, "crop.i_left");
        bs_write_ue(s, sps->crop.i_right, "crop.i_right");
        bs_write_ue(s, sps->crop.i_top, "crop.i_top");
        bs_write_ue(s, sps->crop.i_bottom,"crop.i_bottom");
    }

    bs_write(s, 1, sps->b_vui,"b_vui");
    if (sps->b_vui)
    {
        int i;

        static const struct
        {
            int w, h;
            int sar;
        } sar[] ={
            { 1, 1, 1},
            { 12, 11, 2},
            { 10, 11, 3},
            { 16, 11, 4},
            { 40, 33, 5},
            { 24, 11, 6},
            { 20, 11, 7},
            { 32, 11, 8},
            { 80, 33, 9},
            { 18, 11, 10},
            { 15, 11, 11},
            { 64, 33, 12},
            { 160, 99, 13},
            { 0, 0, -1}
        };
        bs_write1(s, 1,"aspect_ratio_info_present_flag"); /* aspect_ratio_info_present_flag */
        for (i = 0; sar[i].sar != -1; i++)
        {
            if (sar[i].w == sps->vui.i_sar_width && sar[i].h == sps->vui.i_sar_height)
                break;
        }
        if (sar[i].sar != -1)
        {
            bs_write(s, 8, sar[i].sar, "sar");
        }
        else
        {
            bs_write(s, 8, 255,"aspect_ration_idc (extented)"); /* aspect_ration_idc (extented) */
            bs_write(s, 16, sps->vui.i_sar_width,"vui.i_sar_width");
            bs_write(s, 16, sps->vui.i_sar_height,"vui.i_sar_height");
        }

        bs_write1(s, 0,"overscan_info_present_flag"); /* overscan_info_present_flag */

        bs_write1(s, 0,"video_signal_type_present_flag"); /* video_signal_type_present_flag */
#if 0  
        bs_write(s, 3, 5); /* unspecified video format */
        bs_write1(s, 1); /* video full range flag */
        bs_write1(s, 0); /* colour description present flag */
#endif  
        bs_write1(s, 0,"chroma_loc_info_present_flag"); /* chroma_loc_info_present_flag */
        bs_write1(s, 0,"timing_info_present_flag"); /* timing_info_present_flag */
        bs_write1(s, 0,"nal_hrd_parameters_present_flag"); /* nal_hrd_parameters_present_flag */
        bs_write1(s, 0,"vcl_hrd_parameters_present_flag"); /* vcl_hrd_parameters_present_flag */
        bs_write1(s, 0,"pic_struct_present_flag"); /* pic_struct_present_flag */
        bs_write1(s, 0,"bitstream_restriction_flag"); /* bitstream_restriction_flag */
    }

    bs_rbsp_trailing(s);
}

void x264_pps_init(x264_pps_t *pps, int i_id, x264_param_t *param, x264_sps_t *sps) {LOG_CALL;
    pps->i_id = i_id;
    pps->i_sps_id = sps->i_id;
    pps->b_cabac = param->b_cabac;

    pps->b_pic_order = 0;
    pps->i_num_slice_groups = 1;

    if (pps->i_num_slice_groups > 1)
    {
        int i;

        pps->i_slice_group_map_type = 0;
        if (pps->i_slice_group_map_type == 0)
        {
            for (i = 0; i < pps->i_num_slice_groups; i++)
            {
                pps->i_run_length[i] = 1;
            }
        }
        else if (pps->i_slice_group_map_type == 2)
        {
            for (i = 0; i < pps->i_num_slice_groups; i++)
            {
                pps->i_top_left[i] = 0;
                pps->i_bottom_right[i] = 0;
            }
        }
        else if (pps->i_slice_group_map_type >= 3 &&
                pps->i_slice_group_map_type <= 5)
        {
            pps->b_slice_group_change_direction = 0;
            pps->i_slice_group_change_rate = 0;
        }
        else if (pps->i_slice_group_map_type == 6)
        {
            pps->i_pic_size_in_map_units = 1;
            for (i = 0; i < pps->i_pic_size_in_map_units; i++)
            {
                pps->i_slice_group_id[i] = 0;
            }
        }
    }
    pps->i_num_ref_idx_l0_active = 1;
    pps->i_num_ref_idx_l1_active = 1;

    pps->b_weighted_pred = 0;
    pps->b_weighted_bipred = 0;

    pps->i_pic_init_qp = 26;
    pps->i_pic_init_qs = 26;

    pps->i_chroma_qp_index_offset = 0;
#if 0  
    if (!param->b_deblocking_filter)
    {
        pps->b_deblocking_filter_control = 1;
    }
    else
    {
        pps->b_deblocking_filter_control = 1;
    }
#endif  
    pps->b_deblocking_filter_control = 1;
    pps->b_constrained_intra_pred = 0;
    pps->b_redundant_pic_cnt = 0;
}

void x264_pps_write(bs_t *s, x264_pps_t *pps) {LOG_CALL;
    bs_write_ue(s, pps->i_id,"i_id");
    bs_write_ue(s, pps->i_sps_id,"i_sps_id");

    bs_write(s, 1, pps->b_cabac,"b_cabac");
    bs_write(s, 1, pps->b_pic_order,"b_pic_order");
    bs_write_ue(s, pps->i_num_slice_groups - 1,"i_num_slice_groups");

    if (pps->i_num_slice_groups > 1)
    {
        int i;

        bs_write_ue(s, pps->i_slice_group_map_type,"i_slice_group_map_type");
        if (pps->i_slice_group_map_type == 0)
        {
            for (i = 0; i < pps->i_num_slice_groups; i++)
            {
                bs_write_ue(s, pps->i_run_length[i] - 1,"i_run_length");
            }
        }
        else if (pps->i_slice_group_map_type == 2)
        {
            for (i = 0; i < pps->i_num_slice_groups; i++)
            {
                bs_write_ue(s, pps->i_top_left[i],"i_top_left[i]");
                bs_write_ue(s, pps->i_bottom_right[i],"i_bottom_right[i]");
            }
        }
        else if (pps->i_slice_group_map_type >= 3 &&
                pps->i_slice_group_map_type <= 5)
        {
            bs_write(s, 1, pps->b_slice_group_change_direction,"b_slice_group_change_direction");
            bs_write_ue(s, pps->b_slice_group_change_direction - 1,"b_slice_group_change_direction");
        }
        else if (pps->i_slice_group_map_type == 6)
        {
            bs_write_ue(s, pps->i_pic_size_in_map_units - 1,"i_pic_size_in_map_units");
            for (i = 0; i < pps->i_pic_size_in_map_units; i++)
            {
                /* FIXME */
                /* bs_write( s, ceil( log2( pps->i_pic_size_in_map_units +1 ) ), 
                 *              pps->i_slice_group_id[i] ); 
                 */
            }
        }
    }

    bs_write_ue(s, pps->i_num_ref_idx_l0_active - 1,"i_num_ref_idx_l0_active");
    bs_write_ue(s, pps->i_num_ref_idx_l1_active - 1,"i_num_ref_idx_l1_active");
    bs_write(s, 1, pps->b_weighted_pred,"b_weighted_pred");
    bs_write(s, 2, pps->b_weighted_bipred,"b_weighted_bipred");

    bs_write_se(s, pps->i_pic_init_qp - 26,"i_pic_init_qp");
    bs_write_se(s, pps->i_pic_init_qs - 26,"i_pic_init_qs");
    bs_write_se(s, pps->i_chroma_qp_index_offset,"i_chroma_qp_index_offset");

    bs_write(s, 1, pps->b_deblocking_filter_control,"b_deblocking_filter_control");
    bs_write(s, 1, pps->b_constrained_intra_pred,"b_constrained_intra_pred");
    bs_write(s, 1, pps->b_redundant_pic_cnt,"b_redundant_pic_cnt");

    bs_rbsp_trailing(s);
}



