
/*****************************************************************************
 * macroblock.h: h264 encoder library

 *****************************************************************************/

#ifndef _MACROBLOCK_H
#define _MACROBLOCK_H 1

enum macroblock_position_e
{
    MB_LEFT     = 0x01,
    MB_TOP      = 0x02,
    MB_TOPRIGHT = 0x04,

    MB_PRIVATE  = 0x10,
};


/* XXX mb_type isn't the one written in the bitstream -> only internal usage */
#define IS_INTRA(type) ( (type) == I_4x4 || (type) == I_16x16 )
#define IS_SKIP(type)  ( (type) == P_SKIP || (type) == B_SKIP )
enum mb_class_e
{
    I_4x4           = 0,
    I_16x16         = 1,
    I_PCM           = 2,

    P_L0            = 3,
    P_8x8           = 4,
    P_SKIP          = 5,

    B_DIRECT        = 6,
    B_L0_L0         = 7,
    B_L0_L1         = 8,
    B_L0_BI         = 9,
    B_L1_L0         = 10,
    B_L1_L1         = 11,
    B_L1_BI         = 12,
    B_BI_L0         = 13,
    B_BI_L1         = 14,
    B_BI_BI         = 15,
    B_8x8           = 16,
    B_SKIP          = 17,
};
static const int x264_mb_type_list0_table[18][2] =
{
    {0,0}, {0,0}, {0,0},    /* INTRA */
    {1,1},                  /* P_L0 */
    {0,0},                  /* P_8x8 */
    {1,1},                  /* P_SKIP */
    {0,0},                  /* B_DIRECT */
    {1,1}, {1,0}, {1,1},    /* B_L0_* */
    {0,1}, {0,0}, {0,1},    /* B_L1_* */
    {1,1}, {1,0}, {1,1},    /* B_BI_* */
    {0,0},                  /* B_8x8 */
    {0,0}                   /* B_SKIP */
};
static const int x264_mb_type_list1_table[18][2] =
{
    {0,0}, {0,0}, {0,0},    /* INTRA */
    {0,0},                  /* P_L0 */
    {0,0},                  /* P_8x8 */
    {0,0},                  /* P_SKIP */
    {0,0},                  /* B_DIRECT */
    {0,0}, {0,1}, {0,1},    /* B_L0_* */
    {1,0}, {1,1}, {1,1},    /* B_L1_* */
    {1,0}, {1,1}, {1,1},    /* B_BI_* */
    {0,0},                  /* B_8x8 */
    {0,0}                   /* B_SKIP */
};

#define IS_SUB4x4(type) ( (type ==D_L0_4x4)||(type ==D_L1_4x4)||(type ==D_BI_4x4))
#define IS_SUB4x8(type) ( (type ==D_L0_4x8)||(type ==D_L1_4x8)||(type ==D_BI_4x8))
#define IS_SUB8x4(type) ( (type ==D_L0_8x4)||(type ==D_L1_8x4)||(type ==D_BI_8x4))
#define IS_SUB8x8(type) ( (type ==D_L0_8x8)||(type ==D_L1_8x8)||(type ==D_BI_8x8)||(type ==D_DIRECT_8x8))
enum mb_partition_e
{
    /* sub partition type for P_8x8 and B_8x8 */
    D_L0_4x4        = 0,
    D_L0_8x4        = 1,
    D_L0_4x8        = 2,
    D_L0_8x8        = 3,

    /* sub partition type for B_8x8 only */
    D_L1_4x4        = 4,
    D_L1_8x4        = 5,
    D_L1_4x8        = 6,
    D_L1_8x8        = 7,

    D_BI_4x4        = 8,
    D_BI_8x4        = 9,
    D_BI_4x8        = 10,
    D_BI_8x8        = 11,
    D_DIRECT_8x8    = 12,

    /* partition */
    D_8x8           = 13,
    D_16x8          = 14,
    D_8x16          = 15,
    D_16x16         = 16,
};

static const int x264_mb_partition_count_table[17] =
{
    /* sub L0 */
    4, 2, 2, 1,
    /* sub L1 */
    4, 2, 2, 1,
    /* sub BI */
    4, 2, 2, 1,
    /* Direct */
    1,
    /* Partition */
    4, 2, 2, 1
};

typedef struct x264_macroblock_t x264_macroblock_t;

typedef struct
{
    int i_intra4x4_pred_mode;
    int i_non_zero_count;
    int residual_ac[15];
    int luma4x4[16];
} x264_mb_block_t;

typedef struct
{
    int i_ref[2];
    int mv[2][2];   /* [L0/L1][x/y] when no sub-partitioning */

    /* XXX Only set and used by x264_macroblock_write_cabac XXX */
    int mvd[2][2];
} x264_mb_partition_t;

typedef struct
{
    x264_macroblock_t *mba;
    x264_macroblock_t *mbb;
    x264_macroblock_t *mbc;

    uint8_t *p_img[3];
    int     i_img[3];

    uint8_t *p_fdec[3];
    int     i_fdec[3];

    uint8_t *p_fref0[16][3];
    int     i_fref0[16][3];

    uint8_t *p_fref1[16][3];
    int     i_fref1[16][3];

    struct
    {
        x264_macroblock_t *mba;
        x264_macroblock_t *mbb;
        x264_mb_block_t   *bka;
        x264_mb_block_t   *bkb;
    } block[16+8]; //* luma:16, cb:4, cr:4 */

} x264_mb_context_t;


struct x264_macroblock_t
{
    /* position */
    int i_mb_x;
    int i_mb_y;

    /* neightboor a:left, b:top, c:topright */
    unsigned int       i_neighbour;
    x264_mb_context_t  *context;

    /* type */
    int i_type;

    /* partitioning */
    int i_partition;        /* type */
    int i_sub_partition[4]; /* type when i_partition is D_8x8 */
    x264_mb_partition_t partition[4][4];  /* to be easiest, we always represent a 4x4 matrix [x][y]*/

    /* qp for the current mb */
    int i_qp;

    /* residual data */
    int i_cbp_chroma;
    int i_cbp_luma;

    int i_intra16x16_pred_mode;
    int i_chroma_pred_mode;

    int luma16x16_dc[16];
    int chroma_dc[2][4];
    x264_mb_block_t block[16+8]; /* luma:16, cb:4, cr:4 */

};

static inline int x264_median( int a, int b, int c )
{
    int min = a, max =a;
    if( b < min )
        min = b;
    else
        max = b;    /* no need to do 'b > max' (more consuming than always doing affectation) */

    if( c < min )
        min = c;
    else if( c > max )
        max = c;

    return a + b + c - min - max;
}

static inline void x264_mb_partition_size( x264_macroblock_t *mb, int i_part, int i_sub, int *w, int *h )
{
    if( mb->i_partition == D_16x16 )
    {
        *w  = 4;
        *h  = 4;
    }
    else if( mb->i_partition == D_16x8 )
    {
        *w = 4;
        *h = 2;
    }
    else if( mb->i_partition == D_8x16 )
    {
        *w = 2;
        *h = 4;
    }
    else if( mb->i_partition == D_8x8 )
    {
        if( IS_SUB4x4( mb->i_sub_partition[i_part] ) )
        {
            *w = 1;
            *h = 1;
        }
        else if( IS_SUB4x8( mb->i_sub_partition[i_part] ) )
        {
            *w = 1;
            *h = 2;
        }
        else if( IS_SUB8x4( mb->i_sub_partition[i_part] ) )
        {
            *w = 2;
            *h = 1;
        }
        else
        {
            *w = 2;
            *h = 2;
        }
    }
}

static inline void x264_mb_partition_getxy( x264_macroblock_t *mb, int i_part, int i_sub, int *x, int *y )
{
    if( mb->i_partition == D_16x16 )
    {
        *x  = 0;
        *y  = 0;
    }
    else if( mb->i_partition == D_16x8 )
    {
        *x = 0;
        *y = 2*i_part;
    }
    else if( mb->i_partition == D_8x16 )
    {
        *x = 2*i_part;
        *y = 0;
    }
    else if( mb->i_partition == D_8x8 )
    {
        *x = 2 * (i_part%2);
        *y = 2 * (i_part/2);

        if( IS_SUB4x4( mb->i_sub_partition[i_part] ) )
        {
            (*x) += i_sub%2;
            (*y) += i_sub/2;
        }
        else if( IS_SUB4x8( mb->i_sub_partition[i_part] ) )
        {
            (*x) += i_sub;
        }
        else if( IS_SUB8x4( mb->i_sub_partition[i_part] ) )
        {
            (*y) += i_sub;
        }
    }
}

x264_macroblock_t *x264_macroblocks_new( int i_mb_width, int i_mb_height );
void x264_macroblock_context_load( x264_t *h, x264_macroblock_t *mb, x264_mb_context_t *neighbour );

void x264_mb_dequant_4x4_dc( int16_t dct[4][4], int i_qscale );
void x264_mb_dequant_2x2_dc( int16_t dct[2][2], int i_qscale );
void x264_mb_dequant_4x4( int16_t dct[4][4], int i_qscale );

void x264_mb_predict_mv( x264_macroblock_t *mb, int, int, int, int mv[2] );
void x264_mb_predict_mv_pskip( x264_macroblock_t *mb, int mv[2] );

void x264_mb_partition_set( x264_macroblock_t *mb, int i_list, int i_part, int i_sub, int i_ref, int mx, int my );
void x264_mb_partition_get( x264_macroblock_t *mb, int i_list, int i_part, int i_sub, int *pi_ref, int *pi_mx, int *pi_my );


int  x264_mb_predict_intra4x4_mode( x264_t *h, x264_macroblock_t *mb, int idx );
int  x264_mb_predict_non_zero_code( x264_t *h, x264_macroblock_t *mb, int idx );

void x264_mb_encode_i4x4( x264_t *h, x264_macroblock_t *mb, int idx, int i_qscale );

void x264_mb_mc( x264_t *h, x264_macroblock_t *mb );

#endif
