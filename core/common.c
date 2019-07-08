
/*****************************************************************************
 * common.c: h264 library
 *****************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#ifdef HAVE_MALLOC_H
#include <malloc.h>
#endif

#include "common.h"
#include "cpu.h"


/****************************************************************************
 * x264_picture_new/x264_picture_delete:
 ****************************************************************************/
x264_picture_t *x264_picture_new( x264_t *h )
{
    x264_picture_t *pic =(x264_picture_t *) x264_aligned_alloc( sizeof( x264_picture_t ) );
    int i_stride;
    int i_lines;

    pic->i_width = h->param.i_width;
    pic->i_height= h->param.i_height;

    i_stride = ( h->param.i_width + 15 )&0xfffff0;
    i_lines  = ( h->param.i_height + 15 )&0xfffff0;

    pic->i_plane = 3;

    pic->i_stride[0] = i_stride;
    pic->i_stride[1] = i_stride / 2;
    pic->i_stride[2] = i_stride / 2;
    pic->i_stride[3] = 0;

    pic->plane[0] =  (uint8_t *)x264_aligned_alloc( i_lines * pic->i_stride[0] );
    pic->plane[1] = (uint8_t *)x264_aligned_alloc( i_lines / 2 * pic->i_stride[1] );
    pic->plane[2] = (uint8_t *)x264_aligned_alloc( i_lines / 2 * pic->i_stride[2] );
    pic->plane[3] = NULL;

    memset( pic->plane[0],  128, i_lines * pic->i_stride[0] );
    memset( pic->plane[1], 128, i_lines / 2 * pic->i_stride[1] );
    memset( pic->plane[2], 128, i_lines / 2 * pic->i_stride[2] );

    return pic;
}
void x264_picture_delete( x264_picture_t *pic )
{
    int i;

    for( i = 0; i < pic->i_plane; i++ )
    {
        x264_free( pic->plane[i] );
    }
    x264_free( pic );
}

/****************************************************************************
 * x264_param_default:
 ****************************************************************************/
void    x264_param_default( x264_param_t *param )
{
    /* */
    memset( param, 0, sizeof( x264_param_t ) );

    /* CPU autodetect */
    param->cpu = x264_cpu_detect();
    fprintf( stderr, "x264: cpu capabilities: %s%s%s%s%s%s\n",
             param->cpu&X264_CPU_MMX ? "MMX " : "",
             param->cpu&X264_CPU_MMXEXT ? "MMXEXT " : "",
             param->cpu&X264_CPU_SSE ? "SSE " : "",
             param->cpu&X264_CPU_SSE2 ? "SSE2 " : "",
             param->cpu&X264_CPU_3DNOW ? "3DNow! " : "",
             param->cpu&X264_CPU_ALTIVEC ? "Altivec " : "" );


    /* Video properties */
    param->i_width         = 0;
    param->i_height        = 0;
    param->vui.i_sar_width = 0;
    param->vui.i_sar_height= 0;
    param->f_fps           = 25.0;

    /* Encoder parameters */
    param->i_frame_reference = 1;
    param->i_idrframe = 2;
    param->i_iframe = 60;
    param->i_bframe = 0;

    param->b_deblocking_filter = 1;

    param->b_cabac = 0;
    param->i_cabac_init_idc = -1;

    param->i_bitrate = 3000;
    param->i_qp_constant = 26;

    param->i_me = X264_ME_DIAMOND;
}





/****************************************************************************
 * x264_aligned_alloc:
 ****************************************************************************/
void *x264_aligned_alloc( int i_size )
{
#ifdef HAVE_MALLOC_H
    return memalign( 16, i_size );
#else
   return std::aligned_alloc( 16, i_size );
#endif
}

/****************************************************************************
 * x264_malloc:
 ****************************************************************************/
void *x264_malloc( int i_size )
{
#ifdef HAVE_MALLOC_H
    return malloc( i_size );
#else
   return std::malloc( i_size );
#endif
}
/****************************************************************************
 * x264_free:
 ****************************************************************************/
void x264_free( void *p )
{
    if( p )
    {
#ifdef HAVE_MALLOC_H
        free( p );
#else
        return std::free( p );
#endif
    }
}

/****************************************************************************
 * x264_realloc:
 ****************************************************************************/
void *x264_realloc( void *p, int i_size )
{
#ifdef HAVE_MALLOC_H
    return realloc( p, i_size );
#else
    return std::realloc( p, i_size );
#endif
}

