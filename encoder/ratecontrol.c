
/*****************************************************************************
 * ratecontrol.c: h264 encoder library (Rate Control)
 *****************************************************************************

 *****************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "../core/common.h"
#include "ratecontrol.h"


x264_ratecontrol_t *x264_ratecontrol_new( x264_param_t *param )
{
    x264_ratecontrol_t *rc = (x264_ratecontrol_t *)x264_malloc( sizeof( x264_ratecontrol_t ) );

    rc->fps = param->f_fps > 0.1 ? param->f_fps : 25.0;
    rc->i_iframe = param->i_iframe;
    rc->i_bitrate = param->i_bitrate * 1000;

    rc->i_qp_last = 26;
    rc->i_qp      = param->i_qp_constant;

    rc->i_frames  = 0;
    rc->i_size    = 0;

    return rc;
}

void x264_ratecontrol_delete( x264_ratecontrol_t *rc )
{
    x264_free( rc );
}

void x264_ratecontrol_start( x264_ratecontrol_t *rc, int i_slice_type )
{
    rc->i_slice_type = i_slice_type;
}

int  x264_ratecontrol_qp( x264_ratecontrol_t *rc )
{
    return x264_clip3( rc->i_qp, 1, 51 );
}

void x264_ratecontrol_end( x264_ratecontrol_t *rc, int bits )
{
    return;
#if 0
    int i_avg;
    int i_target = rc->i_bitrate / rc->fps;
    int i_qp = rc->i_qp;

    rc->i_qp_last = rc->i_qp;
    rc->i_frames++;
    rc->i_size += bits / 8;

    i_avg = 8 * rc->i_size / rc->i_frames;

    if( rc->i_slice_type == SLICE_TYPE_I )
    {
        i_target = i_target * 20 / 10;
    }

    if( i_avg > i_target * 11 / 10 )
    {
        i_qp = rc->i_qp + ( i_avg / i_target - 1 );
    }
    else if( i_avg < i_target * 9 / 10 )
    {
        i_qp = rc->i_qp - ( i_target / i_avg - 1 );
    }

    rc->i_qp = x264_clip3( i_qp, rc->i_qp_last - 2, rc->i_qp_last + 2 );
#endif
}

