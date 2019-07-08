
/*****************************************************************************
 * me.h: h264 encoder library (Motion Estimation)
 *****************************************************************************
 * Copyright (C) 2003 Laurent Aimar
 * $Id: me.h,v 1.3 2003/11/10 05:09:06 fenrir Exp $
 *
 * Authors: Laurent Aimar <fenrir@via.ecp.fr>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111, USA.
 *****************************************************************************/

#ifndef _ME_H
#define _ME_H 1

typedef int (*x264_me_t)( x264_t *h,
                          uint8_t *p_ref_y, int i_ref_y_stride,
                          uint8_t *p_img_y, int i_img_y_stride,
                          int i_sad_mode,
                          int i_lambda_motion,
                          int *pi_mvx, int *pi_mvy );
enum
{
    X264_ME_UMHEXAGONS = 0,
    X264_ME_DIAMOND    = 1,
};

void x264_me_init( int cpu, x264_me_t pf[2] );

#endif
