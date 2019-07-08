
/*****************************************************************************
 * bs.h :
 *****************************************************************************
************/

#include "dump.h"


#ifdef _BS_H
#warning FIXME Multiple inclusion of bs.h
#else
#define _BS_H

typedef struct bs_s {
    uint8_t *p_start;
    uint8_t *p;
    uint8_t *p_end;

    int i_left; /* i_count number of available bits */
} bs_t;

static inline void
bs_init(bs_t *s, uint8_t *p_data, int i_data) {LOG_CALL;
    s->p_start = p_data;
    s->p = p_data;
    s->p_end = s->p + i_data;
    s->i_left = 8;
}

static inline int
bs_pos(bs_t *s) {LOG_CALL;
    return ( 8 * (s->p - s->p_start) + 8 - s->i_left);
}

static inline int
bs_eof(bs_t *s) {LOG_CALL;
    return ( s->p >= s->p_end ? 1 : 0);
}

static inline uint32_t
bs_read(bs_t *s, int i_count, const char str[]) {
    static uint32_t i_mask[33] = {0x00,
                                  0x01, 0x03, 0x07, 0x0f,
                                  0x1f, 0x3f, 0x7f, 0xff,
                                  0x1ff, 0x3ff, 0x7ff, 0xfff,
                                  0x1fff, 0x3fff, 0x7fff, 0xffff,
                                  0x1ffff, 0x3ffff, 0x7ffff, 0xfffff,
                                  0x1fffff, 0x3fffff, 0x7fffff, 0xffffff,
                                  0x1ffffff, 0x3ffffff, 0x7ffffff, 0xfffffff,
                                  0x1fffffff, 0x3fffffff, 0x7fffffff, 0xffffffff};
    int i_shr;
    uint32_t i_result = 0;

    while (i_count > 0)
    {
        if (s->p >= s->p_end)
        {
            break;
        }

        if ((i_shr = s->i_left - i_count) >= 0)
        {
            /* more in the buffer than requested */
            i_result |= (*s->p >> i_shr) & i_mask[i_count];
            s->i_left -= i_count;
            if (s->i_left == 0)
            {
                s->p++;
                s->i_left = 8;
            }
             myPrintf("bs_read %d, %u, %s \n", i_count, i_result, str);
            return ( i_result);
        }
        else
        {
            /* less in the buffer than requested */
            i_result |= (*s->p & i_mask[s->i_left]) << -i_shr;
            i_count -= s->i_left;
            s->p++;
            s->i_left = 8;
        }
    }
    myPrintf("bs_read %d, %u, %s\n", i_count, i_result, str);
    return ( i_result);
}


static inline uint32_t
_bs_read(bs_t *s, int i_count) {
    static uint32_t i_mask[33] = {0x00,
                                  0x01, 0x03, 0x07, 0x0f,
                                  0x1f, 0x3f, 0x7f, 0xff,
                                  0x1ff, 0x3ff, 0x7ff, 0xfff,
                                  0x1fff, 0x3fff, 0x7fff, 0xffff,
                                  0x1ffff, 0x3ffff, 0x7ffff, 0xfffff,
                                  0x1fffff, 0x3fffff, 0x7fffff, 0xffffff,
                                  0x1ffffff, 0x3ffffff, 0x7ffffff, 0xfffffff,
                                  0x1fffffff, 0x3fffffff, 0x7fffffff, 0xffffffff};
    int i_shr;
    uint32_t i_result = 0;

    while (i_count > 0)
    {
        if (s->p >= s->p_end)
        {
            break;
        }

        if ((i_shr = s->i_left - i_count) >= 0)
        {
            /* more in the buffer than requested */
            i_result |= (*s->p >> i_shr) & i_mask[i_count];
            s->i_left -= i_count;
            if (s->i_left == 0)
            {
                s->p++;
                s->i_left = 8;
            }
            return ( i_result);
        }
        else
        {
            /* less in the buffer than requested */
            i_result |= (*s->p & i_mask[s->i_left]) << -i_shr;
            i_count -= s->i_left;
            s->p++;
            s->i_left = 8;
        }
    }
    return ( i_result);
}
#if 0

/* Only > i386 */
static uint32_t
bswap32(uint32_t x) {LOG_CALL;
    asm( "bswap   %0" : "=r" (x) : "0" (x));
    return x;
}

/* work only for i_count <= 32 - 7 */
static inline uint32_t
bs_read(bs_t *s, int i_count) {LOG_CALL;
    if (s->p < s->p_end && i_count > 0)
    {
#if 0
        uint32_t i_cache = ((s->p[0] << 24)+(s->p[1] << 16)+(s->p[2] << 8) + s->p[3]) << (8 - s->i_left);
#else
        uint32_t i_cache = bswap32(*((uint32_t*) s->p)) << (8 - s->i_left);
#endif
        uint32_t i_ret = i_cache >> (32 - i_count);

        s->i_left -= i_count;
#if 0
        if (s->i_left <= 0)
        {
            int i_skip = (8 - s->i_left) >> 3;

            s->p += i_skip;

            s->i_left += i_skip << 3;
        }
#else
        while (s->i_left <= 0)
        {
            s->p++;
            s->i_left += 8;
        }
#endif
        return i_ret;
    }
    return 0;
}

#endif

static inline uint32_t
bs_read1(bs_t *s, const char str[]) {

    if (s->p < s->p_end)
    {
        unsigned int i_result;

        s->i_left--;
        i_result = (*s->p >> s->i_left)&0x01;
        if (s->i_left == 0)
        {
            s->p++;
            s->i_left = 8;
        }
        

        myPrintf("bs_read1 %u %s\n", i_result, str);
    
        return i_result;
    }
    myPrintf("bs_read1 %u %s\n", 0,str);
    return 0;
}


static inline uint32_t
_bs_read1(bs_t *s) {

    if (s->p < s->p_end)
    {
        unsigned int i_result;

        s->i_left--;
        i_result = (*s->p >> s->i_left)&0x01;
        if (s->i_left == 0)
        {
            s->p++;
            s->i_left = 8;
        }
        

    
        return i_result;
    }
    return 0;
}

static inline uint32_t
bs_show(bs_t *s, int i_count) {LOG_CALL;
#if 0
    bs_t s_tmp = *s;
    return bs_read(&s_tmp, i_count);
#else
    if (s->p < s->p_end && i_count > 0)
    {
        uint32_t i_cache = ((s->p[0] << 24)+(s->p[1] << 16)+(s->p[2] << 8) + s->p[3]) << (8 - s->i_left);
        return ( i_cache >> (32 - i_count));
    }
    return 0;
#endif
}

/* TODO optimize */
static inline void
bs_skip(bs_t *s, int i_count) {
    s->i_left -= i_count;

    while (s->i_left <= 0)
    {
        s->p++;
        s->i_left += 8;
    }
    
    myPrintf("bs_skip %d, %u \n", i_count, 0);
}

static inline int
bs_read_ue(bs_t *s, const char str[]) {
    int i = 0;

    while (_bs_read1(s) == 0 && s->p < s->p_end && i < 32)
    {
        i++;
    }
    int ret =( (1 << i) - 1 + _bs_read(s, i));
    myPrintf("bs_read_ue %d %s\n", ret, str);
    return ret;

}


static inline int
_bs_read_ue(bs_t *s) {
    int i = 0;

    while (_bs_read1(s) == 0 && s->p < s->p_end && i < 32)
    {
        i++;
    }
    int ret =( (1 << i) - 1 + _bs_read(s, i));
    return ret;

}



static inline int
bs_read_se(bs_t *s, const char str[]) {
    int val = _bs_read_ue(s);
    int ret = val & 0x01 ? (val + 1) / 2 : -(val / 2);
    myPrintf("bs_read_se %d %s \n", ret,str);
    return ret;
}

static inline int
bs_read_te(bs_t *s, int x, const char str[]) {LOG_CALL;
    if (x == 1)
    {
        return 1 - _bs_read1(s);
    }
    else if (x > 1)
    {
        return _bs_read_ue(s);
    }
    return 0;
}

/* TODO optimize (write x bits at once) */
static inline void
bs_write(bs_t *s, int i_count, uint32_t i_bits, const char str[]) {

    myPrintf( "bs_write %d , %u, %s\n",  i_count, i_bits, str);

    while (i_count > 0)
    {
        if (s->p >= s->p_end)
        {
            break;
        }

        i_count--;

        if ((i_bits >> i_count)&0x01)
        {
            *s->p |= 1 << (s->i_left - 1);
        }
        else
        {
            *s->p &= ~(1 << (s->i_left - 1));
        }
        s->i_left--;
        if (s->i_left == 0)
        {
            s->p++;
            s->i_left = 8;
        }
    }
}



static inline void
_bs_write(bs_t *s, int i_count, uint32_t i_bits) {

    while (i_count > 0)
    {
        if (s->p >= s->p_end)
        {
            break;
        }

        i_count--;

        if ((i_bits >> i_count)&0x01)
        {
            *s->p |= 1 << (s->i_left - 1);
        }
        else
        {
            *s->p &= ~(1 << (s->i_left - 1));
        }
        s->i_left--;
        if (s->i_left == 0)
        {
            s->p++;
            s->i_left = 8;
        }
    }
}


static inline void
bs_write1(bs_t *s, uint32_t i_bits, const char str[]) {

    myPrintf( "bs_write1 %d , %u ,%s\n",  1, i_bits, str);

    if (s->p < s->p_end)
    {
        s->i_left--;

        if (i_bits & 0x01)
        {
            *s->p |= 1 << s->i_left;
        }
        else
        {
            *s->p &= ~(1 << s->i_left);
        }
        if (s->i_left == 0)
        {
            s->p++;
            s->i_left = 8;
        }
    }
}

static inline void
_bs_write1(bs_t *s, uint32_t i_bits) {


    if (s->p < s->p_end)
    {
        s->i_left--;

        if (i_bits & 0x01)
        {
            *s->p |= 1 << s->i_left;
        }
        else
        {
            *s->p &= ~(1 << s->i_left);
        }
        if (s->i_left == 0)
        {
            s->p++;
            s->i_left = 8;
        }
    }
}

static inline void
bs_align(bs_t *s) {LOG_CALL;
    if (s->i_left != 8)
    {
        s->i_left = 8;
        s->p++;
    }
}

static inline void
bs_align_0(bs_t *s) {LOG_CALL;
    if (s->i_left != 8)
    {
        _bs_write(s, s->i_left, 0);
    }
}

static inline void
bs_align_1(bs_t *s) {LOG_CALL;
    if (s->i_left != 8)
    {
        _bs_write(s, s->i_left, 1);
    }
}

/* golomb functions */

static inline void
bs_write_ue(bs_t *s, unsigned int val, const char str[]) {
    myPrintf( "bs_write_ue %u %s\n", val, str);
    int i_size = 0;
    static const int i_size0_255[256] = {
                                         1, 1, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
                                         6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
                                         7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
                                         7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
                                         8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
                                         8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
                                         8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
                                         8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8
    };

    if (val == 0)
    {
        _bs_write(s, 1, 1);
    }
    else
    {
        unsigned int tmp = ++val;

        if (tmp >= 0x00010000)
        {
            i_size += 16;
            tmp >>= 16;
        }
        if (tmp >= 0x100)
        {
            i_size += 8;
            tmp >>= 8;
        }
        i_size += i_size0_255[tmp];

        _bs_write(s, 2 * i_size - 1, val);
    }
}
static inline void
_bs_write_ue(bs_t *s, unsigned int val) {;
    int i_size = 0;
    static const int i_size0_255[256] = {
                                         1, 1, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
                                         6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
                                         7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
                                         7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
                                         8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
                                         8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
                                         8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
                                         8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8
    };

    if (val == 0)
    {
        _bs_write(s, 1, 1);
    }
    else
    {
        unsigned int tmp = ++val;

        if (tmp >= 0x00010000)
        {
            i_size += 16;
            tmp >>= 16;
        }
        if (tmp >= 0x100)
        {
            i_size += 8;
            tmp >>= 8;
        }
        i_size += i_size0_255[tmp];

        _bs_write(s, 2 * i_size - 1, val);
    }
}

static inline void
bs_write_se(bs_t *s, int val , const char str[]) {
    myPrintf( "bs_write_se %d %s\n", val,str);
    _bs_write_ue(s, val <= 0 ? -val * 2 : val * 2 - 1 );
}

static inline void
bs_write_te(bs_t *s, int x, int val, const char str[]) {
    myPrintf( "bs_write_te %d %s\n", val,str);
    if (x == 1)
    {
        _bs_write(s, 1, ~val);
    }
    else if (x > 1)
    {
        _bs_write_ue(s, val);
    }
}

static inline void
bs_rbsp_trailing(bs_t *s) {LOG_CALL;
    _bs_write(s, 1, 1);
    if (s->i_left != 8)
    {
        _bs_write(s, s->i_left, 0x00);
    }
}

static inline int
bs_size_ue(unsigned int val) {LOG_CALL;
    static const int i_size0_254[255] = {
                                         1, 3, 3, 5, 5, 5, 5, 7, 7, 7, 7, 7, 7, 7, 7,
                                         9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
                                         11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11,
                                         11, 11, 11, 11, 11, 11, 11, 11, 11, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13,
                                         13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13,
                                         13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13,
                                         13, 13, 13, 13, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
                                         15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
                                         15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
                                         15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
                                         15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
                                         15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15
    };

    if (val < 255)
    {
        return i_size0_254[val];
    }
    else
    {
        int i_size = 0;

        val++;

        if (val >= 0x10000)
        {
            i_size += 32;
            val = (val >> 16) - 1;
        }
        if (val >= 0x100)
        {
            i_size += 16;
            val = (val >> 8) - 1;
        }
        return i_size0_254[val] + i_size;
    }
}

static inline int
bs_size_se(int val) {LOG_CALL;
    return bs_size_ue(val <= 0 ? -val * 2 : val * 2 - 1);
}

static inline int
bs_size_te(int x, int val) {LOG_CALL;
    if (x == 1)
    {
        return 1;
    }
    else if (x > 1)
    {
        return bs_size_ue(val);
    }
    return 0;
}



#endif
