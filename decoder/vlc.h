
/*****************************************************************************
 * vlc.h: h264 decoder

 *****************************************************************************/

#ifndef _DECODER_VLC_H
#define _DECODER_VLC_H 1

typedef struct
{
    int i_value;
    int i_size;
} vlc_lookup_t;

struct x264_vlc_table_t
{
    int          i_lookup_bits;

    int          i_lookup;
    vlc_lookup_t *lookup;
};

x264_vlc_table_t *x264_vlc_table_lookup_new( const vlc_t *vlc, int i_vlc, int i_lookup_bits );

void x264_vlc_table_lookup_delete( x264_vlc_table_t *table );

#endif

