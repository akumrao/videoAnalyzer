
/*****************************************************************************
 * frame.h: h264 encoder library
 *****************************************************************************/

#ifndef _FRAME_H
#define _FRAME_H 1

typedef struct
{
    /* for unrestricted mv we allocate more data than needed
     * allocated data are stored in buffer */
    void    *buffer[4];

    int     i_plane;
    int     i_stride[4];
    int     i_lines[4];
    uint8_t *plane[4];

    int     i_poc;
} x264_frame_t;

x264_frame_t *x264_frame_new( x264_t *h );
void          x264_frame_delete( x264_frame_t *frame );

void          x264_frame_copy_picture( x264_frame_t *dst, x264_picture_t *src );

void          x264_frame_expand_border( x264_frame_t *frame );

void          x264_frame_deblocking_filter( x264_t *h, int i_slice_type );

#endif

