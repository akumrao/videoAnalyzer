
/*****************************************************************************
 * cpu.c: h264 encoder library
 *****************************************************************************/

#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "../x264.h"
#include "cpu.h"

#ifdef ARCH_X86
/*
 * XXX: adapted from libmpeg2 */

#define cpuid(op,eax,ebx,ecx,edx)   \
    __asm__ ("push %%ebx\n\t"       \
             "cpuid\n\t"            \
             "movl %%ebx,%1\n\t"    \
             "pop %%ebx"        \
             : "=a" (eax),      \
               "=r" (ebx),      \
               "=c" (ecx),      \
               "=d" (edx)       \
             : "a" (op)         \
             : "cc")

uint32_t x264_cpu_detect( void )
{
    uint32_t cpu = 0;

    uint32_t eax, ebx, ecx, edx;
    int      b_amd;

    /* Test if cpuid is supported */
    asm volatile(
        "pushf\n"
        "pushf\n"
        "pop %0\n"
        "movl %0,%1\n"
        "xorl $0x200000,%0\n"
        "push %0\n"
        "popf\n"
        "pushf\n"
        "pop %0\n"
        "popf\n"
         : "=r" (eax), "=r" (ebx) : : "cc");

    if( eax == ebx )
    {
        /* No cpuid */
        return 0;
    }

    cpuid( 0, eax, ebx, ecx, edx);
    if( eax == 0 )
    {
        return 0;
    }
    b_amd   = (ebx == 0x68747541) && (ecx == 0x444d4163) && (edx == 0x69746e65);

    cpuid( 1, eax, ebx, ecx, edx );
    if( (edx&0x00800000) == 0 )
    {
        /* No MMX */
        return 0;
    }
    cpu = X264_CPU_MMX;
    if( (edx&0x02000000) )
    {
        /* SSE - identical to AMD MMX extensions */
        cpu |= X264_CPU_MMXEXT|X264_CPU_SSE;
    }
    if( (edx&0x04000000) )
    {
        /* Is it OK ? */
        cpu |= X264_CPU_SSE2;
    }

    cpuid( 0x80000000, eax, ebx, ecx, edx );
    if( eax < 0x80000001 )
    {
        /* no extended capabilities */
        return cpu;
    }

    cpuid( 0x80000001, eax, ebx, ecx, edx );
    if( edx&0x80000000 )
    {
        cpu |= X264_CPU_3DNOW;
    }
    if( b_amd && (edx&0x00400000) )
    {
        /* AMD MMX extensions */
        cpu |= X264_CPU_MMXEXT;
    }

    return cpu;
}

void     x264_cpu_restore( uint32_t cpu )
{
    if( cpu&(X264_CPU_MMX|X264_CPU_MMXEXT|X264_CPU_3DNOW|X264_CPU_3DNOWEXT) )
    {
        asm volatile( "emms" : : );
    }
}

#elif defined( HAVE_ALTIVEC )
#include <sys/sysctl.h>

uint32_t x264_cpu_detect( void )
{
    /* Thx VLC */
    uint32_t cpu = 0;
    int      selectors[2] = { CTL_HW, HW_VECTORUNIT };
    int      has_altivec = 0;
    size_t   length = sizeof( has_altivec );
    int      error = sysctl( selectors, 2, &has_altivec, &length, NULL, 0 );

    if( error == 0 && has_altivec != 0 )
    {
        cpu |= X264_CPU_ALTIVEC;
    }

    return cpu;
}

void     x264_cpu_restore( uint32_t cpu )
{
}

#else

uint32_t x264_cpu_detect( void )
{
    return 0;
}

void     x264_cpu_restore( uint32_t cpu )
{
}

#endif

