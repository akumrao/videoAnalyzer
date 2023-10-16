
/*****************************************************************************
 * x264: h264 encoder/decoder testing program.
 * https://studylib.net/doc/5702305/fast-mode-decision-in-h264-avc-video-codec
 * http://lazybing.github.io/blog/2017/11/13/x264-macroblock-analyse/
 * 
 * https://zhuanlan.zhihu.com/p/640693117
 * 
 * http://wmnmtm.blog.163.com/blog/static/382457142011885411173/
 * 
 * 
https://huyunf.github.io/blogs/2017/11/20/x264_encoder_open/
 * 
 * https://github.com/cancan101/h.264-cuda/tree/master/encoder/pyramid
 * 
 * https://github.com/davidstutz/graph-based-image-segmentation/blob/master/lib/graph_segmentation.cpp
 * https://github.com/the-other-mariana/image-processing-algorithms/blob/master/006-camera/camera.cpp
 * https://github.com/cb1711/Image-Segmentation/blob/master/kmeans.cpp
 * ffmpeg -i video.mp4 -c:v rawvideo -pix_fmt yuv420p out.yuv
 * 
 * 
 * https://www.slideshare.net/vcodex/introduction-to-h264-advanced-video-compression
 * 
 * 
 * 
 * https://www.youtube.com/watch?v=wYZzpE1pPn0  Feature Extraction Methods for the classification of images

 * https://www.youtube.com/watch?v=fxfS0vSAsTA  fft vs wavelet    wavelet scattering transform Wavelets-based Feature Extraction
 * 
 * 
 * 
 * https://www.youtube.com/watch?v=AYzWnCHnfSc    /Euler apply thining algorithm to get humnoid structure

 * 
 * out.yuv 1280x720  -o final.264
 * 
 *****************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include <math.h>

#include <signal.h>
#define _GNU_SOURCE
#include <getopt.h>

#ifdef _MSC_VER
#include <io.h>     /* _setmode() */
#include <fcntl.h>  /* _O_BINARY */
#endif

#include "x264.h"
#include "core/common.h"

#define DATA_MAX 3000000
uint8_t data[DATA_MAX];

/* Ctrl-C handler */
static int     i_ctrl_c = 0;
static void    SigIntHandler( int a )
{
    i_ctrl_c = 1;
}

static void help( void )
{
    fprintf( stderr,
             "x264\n"
             "Syntax: x264 -x in.h26l > out.yuv (decoding)\n"
             "        x264 in.yuv widthxheigh > out.h26l (encoding)\n"
             "  -h, --help                  Print this help\n"
             "\n"
             "  -B, --bframe <integer>      Number of B-frames between I and P\n"
             "  -i, --iframe <integer>      Frequency of I frames\n"
             "  -I, --idrframe <integer>    Each 'number' I frames are IDR frames\n"
             "\n"
             "  -c, --cabac                 Enable CABAC\n"
             "  -r, --ref <integer>         Number of references\n"
             "  -n, --nf                    Disable loop filter\n"
             "  -q, --qp <integer>          Set QP\n"
             "  -m <integer>                ME algo (0: slow, 1:fast)\n"
             "  -b, --bitrate <integer>     Set bitrate [broken]\n"
             "\n"
             "  -s, --sar width:height      Specify Sample Aspect Ratio\n"
             "  -o, --output                Specify output file\n"
             "\n"
             "      --no-asm                Disable any CPU optims\n"
             "\n"
           );
}
void fillByte(BYTE *src, int dx, int dy)
{
    int j;
    int i;
    for (j = 0; j < dy; j++)
    {
        for ( i = 0; i < dx; i++)
        {
            src[j * dx + i]= 0;
        }

    }

} 
/****************************************************************************
 * main:
 ****************************************************************************/
int main( int argc, char **argv )
{
    x264_t *h;
    x264_param_t param;
    x264_picture_t *pic;

    FILE    *fyuv;
    FILE    *fh26l;
    FILE    *fout = stdout;

    char    *p;

    int     i_frame, i_frame_total;
    int64_t i_start, i_end;
    int64_t i_file;

    int     b_decompress = 0;

#ifdef _MSC_VER
    _setmode(_fileno(stdin), _O_BINARY);    /* thanks to Marcos Morais <morais at dee.ufcg.edu.br> */
    _setmode(_fileno(stdout), _O_BINARY);
#endif

    x264_param_default( &param );
    param.f_fps = 25.0;

    /* Parse command line options */
    opterr = 0; // no error message
    for( ;; )
    {
        int long_options_index;
        static struct option long_options[] =
        {
            { "help",    no_argument,       NULL, 'h' },
            { "bitrate", required_argument, NULL, 'b' },
            { "bframe",  required_argument, NULL, 'B' },
            { "iframe",  required_argument, NULL, 'i' },
            { "idrframe",  required_argument, NULL, 'I' },
            { "nf",      no_argument,       NULL, 'n' },
            { "cabac",   no_argument,       NULL, 'c' },
            { "qp",      required_argument, NULL, 'q' },
            { "ref",     required_argument, NULL, 'r' },
            { "no-asm",  no_argument,       NULL, 'C' },
            { "sar",     required_argument, NULL, 's' },
            { "output",  required_argument, NULL, 'o' },
            {0, 0, 0, 0}
        };

        int c;

        c = getopt_long( argc, argv, "hi:b:r:cxB:m:q:no:s:",
                         long_options, &long_options_index);

        if( c == -1 )
        {
            break;
        }

        switch( c )
        {
            case 0:
                break;
            case 'h':
                help();
                return 0;
            case 'b':
                param.i_bitrate = atol( optarg );
                break;
            case 'B':
                param.i_bframe = atol( optarg );
                break;
            case 'i':
                param.i_iframe = atol( optarg );
                break;
            case 'I':
                param.i_idrframe = atol( optarg );
                break;
            case 'n':
                param.b_deblocking_filter = 0;
                break;
            case 'q':
                param.i_qp_constant = atoi( optarg );
                break;
            case 'r':
                param.i_frame_reference = atoi( optarg );
                break;
            case 'c':
                param.b_cabac = 1;
                break;
            case 'm':
                param.i_me = atoi( optarg );
                break;
            case 'x':
                b_decompress = 1;
                break;
            case 'C':
                param.cpu = 0;
                break;
            case'o':
                if( ( fout = fopen( optarg, "wb" ) ) == NULL )
                {
                    fprintf( stderr, "cannot open output file `%s'\n", optarg );
                    return -1;
                }
                break;
            case 's':
            {
                char *p = strchr( optarg, ':' );
                if( p )
                {
                    param.vui.i_sar_width = atoi( optarg );
                    param.vui.i_sar_height = atoi( p + 1 );
                }
                break;
            }
            default:
                fprintf( stderr, "unknown option (%c)\n", optopt );
                return -1;
        }
    }

    /* Control-C handler */
    signal( SIGINT, SigIntHandler );

    if( b_decompress )
    {
        x264_nal_t nal;
        int i_data;
        int b_eof;

        if( optind > argc - 1 )
        {
            help();
            return -1;
        }

        if( !strcmp( argv[optind], "-" ) )
        {
            fh26l = stdin;
            optind++;
        }
        else if( ( fh26l = fopen( argv[optind++], "rb" ) ) == NULL )
        {
            fprintf( stderr, "could not open h26l file '%s'\n", argv[optind - 1] );
            return -1;
        }

        //param.cpu = 0;
        if( ( h = x264_decoder_open( &param ) ) == NULL )
        {
            fprintf( stderr, "x264_decoder_open failed\n" );
            return -1;
        }

        i_start = x264_mdate();
        b_eof = 0;
        i_frame = 0;
        i_data  = 0;
        nal.p_payload = (uint8_t*)x264_malloc( DATA_MAX );

        while( !i_ctrl_c )
        {
            uint8_t *p, *p_next, *end;
            int i_size;
            /* fill buffer */
            if( i_data < DATA_MAX && !b_eof )
            {
                int i_read = fread( &data[i_data], 1, DATA_MAX - i_data, fh26l );
                if( i_read <= 0 )
                {
                    b_eof = 1;
                }
                else
                {
                    i_data += i_read;
                }
            }

            if( i_data < 3 )
            {
                break;
            }

            end = &data[i_data];

            /* extract one nal */
            p = &data[0];
            while( p < end - 3 )
            {
                if( p[0] == 0x00 && p[1] == 0x00 && p[2] == 0x01 )
                {
                    break;
                }
                p++;
            }

            if( p >= end - 3 )
            {
                fprintf( stderr, "garbage (i_data = %d)\n", i_data );
                i_data = 0;
                continue;
            }

            p_next = p + 3;
            while( p_next < end - 3 )
            {
                if( p_next[0] == 0x00 && p_next[1] == 0x00 && p_next[2] == 0x01 )
                {
                    break;
                }
                p_next++;
            }

            if( p_next == end - 3 && i_data < DATA_MAX )
            {
                p_next = end;
            }

            /* decode this nal */
            i_size = p_next - p - 3;
            if( i_size <= 0 )
            {
                if( b_eof )
                {
                    break;
                }
                fprintf( stderr, "nal too large (FIXME) ?\n" );
                i_data = 0;
                continue;
            }

            x264_nal_decode( &nal, p +3, i_size );

            /* decode the content of the nal */
            x264_decoder_decode( h, &pic, &nal );

            if( pic != NULL )
            {
                int i;

                i_frame++;

                for( i = 0; i < pic->i_plane;i++ )
                {
                    int i_line;
                    int i_div;

                    i_div = i==0 ? 1 : 2;
                    for( i_line = 0; i_line < pic->i_height/i_div; i_line++ )
                    {
                        fwrite( pic->plane[i]+i_line*pic->i_stride[i], 1, pic->i_width/i_div, fout );
                        dumpByte( pic->plane[i]+i_line*pic->i_stride[i], pic->i_width/i_div, 1    );
                        
                    }
                }
            }

            memmove( &data[0], p_next, end - p_next );
            i_data -= p_next - &data[0];
        }

        i_end = x264_mdate();
        free( nal.p_payload );
        fprintf( stderr, "\n" );

        x264_decoder_close( h );

        fclose( fh26l );
        if( fout != stdout )
        {
            fclose( fout );
        }
        if( i_frame > 0 )
        {
            double fps = (double)i_frame * (double)1000000 /
                         (double)( i_end - i_start );
            fprintf( stderr, "decoded %d frames %ffps\n", i_frame, fps );
        }
    }
    else
    {
        if( optind > argc - 2 )
        {
            help();
            return -1;
        }

        /* init params */
        if( !strcmp( argv[optind], "-" ) )
        {
            fyuv = stdin;
            optind++;
        }
        else if( ( fyuv = fopen( argv[optind++], "rb" ) ) == NULL )
        {
            fprintf( stderr, "could not open yuv input file '%s'\n", argv[optind - 1] );
            return -1;
        }

        param.i_width           = strtol( argv[optind++], &p, 0 );
        param.i_height          = strtol( p+1, &p, 0 );

        i_frame_total = 0;
        if( !fseek( fyuv, 0, SEEK_END ) )
        {
            int64_t i_size = ftell( fyuv );
            fseek( fyuv, 0, SEEK_SET );
            i_frame_total = i_size / ( param.i_width * param.i_height * 3 / 2 );
        }

        if( ( h = x264_encoder_open( &param ) ) == NULL )
        {
            fprintf( stderr, "x264_encoder_open failed\n" );
            return -1;
        }

        pic = x264_picture_new( h );

        i_start = x264_mdate();
        for( i_frame = 0, i_file = 0; i_ctrl_c == 0 ; i_frame++ )
        {
            int         i_nal;
            x264_nal_t  *nal;

            int         i;

           /* i420  =   y = w*h
                      u = w*h/4
                      v = w*h/4  
            */
            
            //fillByte(pic->plane[0],  param.i_width , param.i_height);
            ///* read a frame
            if( fread( pic->plane[0], 1, param.i_width * param.i_height, fyuv ) <= 0 ||
                fread( pic->plane[1], 1, param.i_width * param.i_height / 4, fyuv ) <= 0 ||
                fread( pic->plane[2], 1, param.i_width * param.i_height / 4, fyuv ) <= 0 )
            {
                break;
            }
            
           // if(i_frame == 16 ) break; //arvind
            
            if( x264_encoder_encode( h, &nal, &i_nal, pic ) < 0 )
            {
                fprintf( stderr, "x264_encoder_encode failed\n" );
            }

            for( i = 0; i < i_nal; i++ )
            {
                int i_size;
                int i_data;

                i_data = DATA_MAX;
                if( ( i_size = x264_nal_encode( data, &i_data, 1, &nal[i] ) ) > 0 )
                {
                    i_file += fwrite( data, 1, i_size, fout );
                }
                else if( i_size < 0 )
                {
                    fprintf( stderr,
                             "need to increase buffer size (size=%d)\n", -i_size );
                }
            }
        }
        i_end = x264_mdate();
        x264_picture_delete( pic );
        x264_encoder_close( h );
        fprintf( stderr, "\n" );

        fclose( fyuv );
        if( fout != stdout )
        {
            fclose( fout );
        }

        if( i_frame > 0 )
        {
            double fps = (double)i_frame * (double)1000000 /
                         (double)( i_end - i_start );

            fprintf( stderr, "encoded %d frames %ffps %lld kb/s\n", i_frame, fps, i_file * 8 * 25 / i_frame / 1000 );
        }
    }

    return 0;
}

