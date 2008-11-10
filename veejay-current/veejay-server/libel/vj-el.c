/* veejay - Linux VeeJay
 * 	     (C) 2002-2004 Niels Elburg <nwelburg@gmail.com> 
 *
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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/*


	This file contains code-snippets from the mjpegtools' EditList
	(C) The Mjpegtools project

	http://mjpeg.sourceforge.net
*/
#include <config.h>
#include <string.h>
#include <stdio.h>
#include <libvjmsg/vj-msg.h>
#include <veejay/vims.h>
#include <libel/lav_io.h>
#include <libel/vj-el.h>
#include <libvje/vje.h>
#include <libel/vj-avcodec.h>
#include <libel/elcache.h>
#include <libel/pixbuf.h>
#include <limits.h>
#include <mjpegtools/mpegconsts.h>
#include <mjpegtools/mpegtimecode.h>
#include <libvjmem/vjmem.h>
#include <libyuv/yuvconv.h>
#include AVCODEC_INC
#include AVFORMAT_INC
#include <liblzo/lzo.h>
#include <math.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#ifdef SUPPORT_READ_DV2
#include "rawdv.h"
#include "vj-dv.h"
#endif
#define MAX_CODECS 50
#define CODEC_ID_YUV420 999
#define CODEC_ID_YUV422 998
#define CODEC_ID_YUVLZO 900
#define DUMMY_FRAMES 2

//@@ !
#ifdef STRICT_CHECKING
#include <assert.h>
#endif
static struct
{
	const char *name;
} _chroma_str[] = 
{
	{	"Unknown"	}, // CHROMAUNKNOWN
	{	"4:2:0"		},
	{	"4:2:2"		},
	{	"4:4:4"		},
	{	"4:1:1"		},
};


static struct
{
        const char *name;
        int  id;
	int	pf;
} _supported_codecs[] = 
{
        { "mjpg" , CODEC_ID_MJPEG ,		0 },
	{ "mjpg" , CODEC_ID_MJPEG , 		1 },
	{ "mjpb", CODEC_ID_MJPEGB,		0 },
	{ "mjpb", CODEC_ID_MJPEGB,		1 },
        { "msmpeg4",CODEC_ID_MPEG4,		-1},
	{ "fmp4", CODEC_ID_MPEG4,		1},
	{ "fmp4", CODEC_ID_MPEG4,		0},
        { "divx"   ,CODEC_ID_MSMPEG4V3,		-1 },
        { "i420",   CODEC_ID_YUV420,		-1 },
        { "i422",   CODEC_ID_YUV422,		-1 },
	{ "dmb1",	CODEC_ID_MJPEG,		0  },
	{ "dmb1",	CODEC_ID_MJPEG,		1 },
	{ "jpeg",	CODEC_ID_MJPEG,		0 },
	{ "jpeg",	CODEC_ID_MJPEG,		1 },
	{ "mjpa",	CODEC_ID_MJPEG,		0  },
	{ "mjpb",	CODEC_ID_MJPEG,		1 },
	{ "jfif",	CODEC_ID_MJPEG,		0  },
	{ "jfif",	CODEC_ID_MJPEG,		1 },
	{ "png",	CODEC_ID_PNG,		-1 },
	{ "mpng",	CODEC_ID_PNG, 		-1 },
#if LIBAVCODEC_BUILD > 4680
	{ "sp5x",   	CODEC_ID_SP5X,		-1 }, /* sunplus motion jpeg video */
#endif
	{ "jpgl",	CODEC_ID_MJPEG, 	0},
	{ "jpgl",	CODEC_ID_MJPEG,		1},
	{ "dvsd",	CODEC_ID_DVVIDEO,	-1},
	{ "dvcp",	CODEC_ID_DVVIDEO,	-1},
	{ "dv",		CODEC_ID_DVVIDEO,	-1},
	{ "dvhd",	CODEC_ID_DVVIDEO,	-1},
	{ "dvp",	CODEC_ID_DVVIDEO,	-1},
	{ "mp4v",	CODEC_ID_MPEG4,		-1},
	{ "xvid",	CODEC_ID_MPEG4,		-1},
	{ "divx",	CODEC_ID_MPEG4,		-1},
	{ "dxsd",	CODEC_ID_MPEG4,		-1},
	{ "mp4s",	CODEC_ID_MPEG4,		-1},
	{ "m4s2",	CODEC_ID_MPEG4,		-1},	
	{ "avc1",	CODEC_ID_H264,		-1},
	{ "h264",	CODEC_ID_H264,		-1},
	{ "x264",	CODEC_ID_H264,		-1},
	{ "davc",	CODEC_ID_H264,		-1},
	{ "div3",	CODEC_ID_MSMPEG4V3,	-1},
	{ "mp43",	CODEC_ID_MSMPEG4V3,	-1},
	{ "mp42",	CODEC_ID_MSMPEG4V2,	-1},
	{ "mpg4",	CODEC_ID_MSMPEG4V1,	-1},
	{ "yuv",	CODEC_ID_YUV420,	-1},
	{ "iyuv",	CODEC_ID_YUV420,	-1},
	{ "i420",	CODEC_ID_YUV420,	-1},
	{ "yv16",	CODEC_ID_YUV422,	-1},
	{ "mlzo",	CODEC_ID_YUVLZO,	-1},
	{ "pict",	0xffff,			-1}, 
	{ "hfyu",	CODEC_ID_HUFFYUV,	-1},
	{ "cyuv",	CODEC_ID_CYUV,		-1},
	{ "svq1",	CODEC_ID_SVQ1,		-1},
	{ "svq3",	CODEC_ID_SVQ3,		-1},
	{ "rpza",	CODEC_ID_RPZA,		-1},
	{ NULL  , 0,				-1},
};

static struct
{
	const char *name;
	int id;
} _supported_fourcc[] =
{
	{ "mjpg",	CODEC_ID_MJPEG	},
	{ "mjpb",	CODEC_ID_MJPEGB },
	{ "dmb1",	CODEC_ID_MJPEG	},
	{ "jpeg",	CODEC_ID_MJPEG	},
	{ "mjpa",	CODEC_ID_MJPEG  },
	{ "jfif",	CODEC_ID_MJPEG  },
	{ "png",	CODEC_ID_PNG	},
	{ "mpng",	CODEC_ID_PNG	},
#if LIBAVCODEC_BUILD > 4680
	{ "sp5x",   	CODEC_ID_SP5X }, /* sunplus motion jpeg video */
#endif
	{ "jpgl",	CODEC_ID_MJPEG  },
	{ "dvsd",	CODEC_ID_DVVIDEO},
	{ "dv",		CODEC_ID_DVVIDEO},
	{ "dvhd",	CODEC_ID_DVVIDEO},
	{ "dvp",	CODEC_ID_DVVIDEO},
	{ "mp4v",	CODEC_ID_MPEG4  },
	{ "xvid",	CODEC_ID_MPEG4	},
	{ "divx",	CODEC_ID_MPEG4  },
	{ "dxsd",	CODEC_ID_MPEG4	},
	{ "mp4s",	CODEC_ID_MPEG4	},
	{ "m4s2",	CODEC_ID_MPEG4	},	
	{ "fmp4",	CODEC_ID_MPEG4 },
	{ "fmp4",	CODEC_ID_MPEG4 },
	{ "avc1",	CODEC_ID_H264	},
	{ "h264",	CODEC_ID_H264	},
	{ "x264",	CODEC_ID_H264 	},
	{ "davc",	CODEC_ID_H264 	},
	{ "div3",	CODEC_ID_MSMPEG4V3 },
	{ "divx",	CODEC_ID_MPEG4 },
	{ "mp43",	CODEC_ID_MSMPEG4V3 },
	{ "mp42",	CODEC_ID_MSMPEG4V2 },
	{ "mpg4",	CODEC_ID_MSMPEG4V1 },
	{ "yuv",	CODEC_ID_YUV420 },
	{ "iyuv",	CODEC_ID_YUV420 },
	{ "i420",	CODEC_ID_YUV420 },
	{ "mlzo",	CODEC_ID_YUVLZO },
	{ "yv16",	CODEC_ID_YUV422 },
	{ "pict",	0xffff	}, /* invalid fourcc */
	{ "hfyu",	CODEC_ID_HUFFYUV},
	{ "cyuv",	CODEC_ID_CYUV   },
	{ "svq1",	CODEC_ID_SVQ1	},
	{ "svq3",	CODEC_ID_SVQ3	},
	{ "rpza",	CODEC_ID_RPZA	},
	{ NULL, 0 }
};

static	int mmap_size = 0;

typedef struct
{
        AVCodec *codec; // veejay supports only 2 yuv formats internally
        AVFrame *frame;
        AVCodecContext  *context;
        uint8_t *tmp_buffer;
        uint8_t *deinterlace_buffer[3];
	VJFrame *img;
	int fmt;
        int ref;
	void *sampler;
} vj_decoder;

#ifdef SUPPORT_READ_DV2
static	vj_dv_decoder *dv_decoder_ = NULL;
#endif

static	void		*lzo_decoder_ = NULL;

static	vj_decoder *el_codecs[MAX_CODECS];

static	int _el_get_codec(int id, int in_pix_fmt )
{
	int i;
	for( i = 0; _supported_codecs[i].name != NULL ; i ++ )
	{
		if( _supported_codecs[i].id == id)
		{
			if( _supported_codecs[i].pf == -1) {
				return i;
			} else if ( _supported_codecs[i].pf == 0 && (in_pix_fmt == FMT_420F || in_pix_fmt == FMT_420)) {
				return i;
			} else if ( _supported_codecs[i].pf == 1 && (in_pix_fmt == FMT_422F || in_pix_fmt == FMT_422)) {
				return i;		
			}	
		}
	}
	return -1;
}
static	int	_el_get_codec_id( const char *fourcc )
{
	int i;
	for( i = 0; _supported_fourcc[i].name != NULL ; i ++ )
		if( strncasecmp( fourcc, _supported_fourcc[i].name, strlen(_supported_fourcc[i].name) ) == 0 )
			return _supported_fourcc[i].id;
	return -1;
}

int		vj_el_get_decoder_from_fourcc( const char *fourcc )
{
	return _el_get_codec_id( fourcc );
}

static void	_el_free_decoder( vj_decoder *d )
{
	if(d)
	{
		if(d->tmp_buffer)
			free( d->tmp_buffer );
		if(d->deinterlace_buffer[0])
			free(d->deinterlace_buffer[0]);

		if(d->context)
		{
			avcodec_close( d->context ); 
			free( d->context );
			d->context = NULL;
		}
		if(d->frame) 
			free(d->frame);

		if(d->sampler)
			subsample_free(d->sampler);
	
		if(d->img)
			free(d->img);
	
		free(d);
	}
	d = NULL;
}
#define LARGE_NUM (256*256*256*64)
/*
static int get_buffer(AVCodecContext *context, AVFrame *av_frame){
	vj_decoder *this = (vj_decoder *)context->opaque;
	int width  = context->width;
	int height = context->height;
	VJFrame *img = this->img;	
	avcodec_align_dimensions(context, &width, &height);

	av_frame->opaque = img;

	av_frame->data[0]= img->data[0];
	av_frame->data[1]= img->data[1];
	av_frame->data[2]= img->data[2];

	av_frame->linesize[0] = img->width;
	av_frame->linesize[1] = img->uv_width;
	av_frame->linesize[2] = img->uv_width;

	av_frame->age = LARGE_NUM;

	av_frame->type= FF_BUFFER_TYPE_USER;

	return 0;
}
static void release_buffer(struct AVCodecContext *context, AVFrame *av_frame){
 VJFrame *img = (VJFrame*)av_frame->opaque;
  av_frame->opaque = NULL;
}*/

static int el_pixel_format_ = 1;
static int el_len_ = 0;
static int el_uv_len_ = 0;
static int el_uv_wid_ = 0;
static int mem_chunk_ = 0;
void	vj_el_init_chunk(int size)
{
//@@ chunk size per editlist
	mem_chunk_ = 1024 * size;
}
void	vj_el_init(int pf)
{
	int i;
	for( i = 0; i < MAX_CODECS ;i ++ )
		el_codecs[i] = NULL;
	el_pixel_format_ =pf;
#ifdef STRICT_CHECKING
	assert( pf == FMT_420 || pf == FMT_420F || pf == FMT_422 || pf == FMT_422F );
#endif
}

int	vj_el_is_dv(editlist *el)
{
#ifdef SUPPORT_READ_DV2
	return is_dv_resolution(el->video_width, el->video_height);
#else
	return 0;
#endif
}


void	vj_el_prepare()
{
//	reset_cache( el->cache );
}

void	vj_el_break_cache( editlist *el )
{
	if(el) {
		if( el->cache )
			free_cache( el->cache );
		el->cache = NULL;
	}
}

static int never_cache_ = 0;
void	vj_el_set_caching(int status)
{
	never_cache_ = status;
}

//@ iterateovers over sample fx chain
void	vj_el_setup_cache( editlist *el )
{
//@ FIXME: user setting to prevent cache setup!
	if(!el->cache && !never_cache_)
	{
		int n_slots = mem_chunk_ / el->max_frame_size;
//		if( n_slots < (el->video_frames - 1) )
		if( n_slots < el->total_frames )
		{
			veejay_msg(VEEJAY_MSG_DEBUG, "Not caching this EDL to memory (Cachesize too small)");
			veejay_msg(VEEJAY_MSG_DEBUG, "try increasing cache size with -m commandline parameter");
		}
		else if( el->max_frame_size > 1024 )
		{
			veejay_msg(VEEJAY_MSG_DEBUG, "EditList caches at most %d slots (chunk=%d, framesize=%d)", n_slots, mem_chunk_, el->max_frame_size ); 
			el->cache = init_cache( n_slots );
		}
	}
}

void	vj_el_clear_cache( editlist *el )
{
	if(el->cache)
		reset_cache(el->cache);
}

void	vj_el_deinit()
{
	int i;
	for( i = 0; i < MAX_CODECS ;i ++ )
	{
		if( el_codecs[i] )
			_el_free_decoder( el_codecs[i] );
	}
}

int	vj_el_cache_size()
{
	return cache_avail_mb();
}

//@ pixel_format = input pixel format FIXME
vj_decoder *_el_new_decoder( int id , int width, int height, float fps, int pixel_format, int out_fmt)
{
        vj_decoder *d = (vj_decoder*) vj_calloc(sizeof(vj_decoder));
        if(!d)
	  return NULL;

#ifdef SUPPORT_READ_DV2
	if( id == CODEC_ID_DVVIDEO )
		dv_decoder_ = vj_dv_decoder_init(1, width, height, pixel_format );
#endif	
	if( id == CODEC_ID_YUVLZO )
	{
		lzo_decoder_ = lzo_new();
		d->sampler = subsample_init( width );
	} else  if( id != CODEC_ID_YUV422 && id != CODEC_ID_YUV420 )
        {
		d->codec = avcodec_find_decoder( id );
		d->context = avcodec_alloc_context();
		d->context->width = width;
		d->context->height = height;
		d->context->opaque = d;
		d->context->palctrl = NULL;
		d->frame = avcodec_alloc_frame();
		d->img = (VJFrame*) vj_calloc(sizeof(VJFrame));
		d->img->width = width;	
		if ( avcodec_open( d->context, d->codec ) < 0 )
       		{
      		       veejay_msg(VEEJAY_MSG_ERROR, "Error initializing decoder %d",id); 
		       _el_free_decoder( d );
       		       return NULL;
      		}
	}
		
        d->tmp_buffer = (uint8_t*) vj_malloc(sizeof(uint8_t) * width * height * 4 );
        if(!d->tmp_buffer)
	{
		free(d);
                return NULL;
	}
        d->fmt = id;
        veejay_memset( d->tmp_buffer, 0, width * height * 4 );

        d->deinterlace_buffer[0] = (uint8_t*) vj_malloc(sizeof(uint8_t) * width * height * 3);
        if(!d->deinterlace_buffer[0]) { if(d) free(d); return NULL; }

		d->deinterlace_buffer[1] = d->deinterlace_buffer[0] + (width * height );
		d->deinterlace_buffer[2] = d->deinterlace_buffer[0] + (2 * width * height );
		
        veejay_memset( d->deinterlace_buffer[0], 0, width * height * 3 );

        return d;
}

void	vj_el_set_image_output_size(editlist *el, int dw, int dh, float fps, int pf)
{
	if( el->video_width <= 0 || el->video_height <= 0 )
		lav_set_project( dw,dh, fps, pf );
	else
		lav_set_project(
			el->video_width, el->video_height, el->video_fps , el_pixel_format_ );

}

static int _el_probe_for_pixel_fmt( lav_file_t *fd )
{
//	int old = lav_video_cmodel( fd );

	int new = test_video_frame( fd, el_pixel_format_ );

	switch(new)
	{
		case FMT_420:
				veejay_msg(VEEJAY_MSG_DEBUG,"\tPixel format: YUV Planar 4:2:0 [16-235][16-240]");
				break;
		case FMT_422:
				veejay_msg(VEEJAY_MSG_DEBUG,"\tPixel format: YUV Planar 4:2:2 [16-235][16-240]");
				break;
		case FMT_420F:
				veejay_msg(VEEJAY_MSG_DEBUG,"\tPixel format: YUV Planar 4:2:0 [JPEG full range]");
				break;
		case FMT_422F:
				veejay_msg(VEEJAY_MSG_DEBUG,"\tPixel format: YUV Planar 4:2:2 [JPEG full range]");
				break;
	}
	
	return new;
}

int	get_ffmpeg_pixfmt( int pf )
{
	switch( pf )
	{
		case FMT_420:
			return PIX_FMT_YUV420P;
		case FMT_422:
			return PIX_FMT_YUV422P;
		case FMT_420F:
			return PIX_FMT_YUVJ420P;
		case FMT_422F:
			return PIX_FMT_YUVJ422P;
		case 4:
			return PIX_FMT_YUV444P;
		
	}
	return PIX_FMT_YUV422P;
}
int       get_ffmpeg_shift_size(int fmt)
{
        switch(fmt)
        {
                case FMT_420:
                case FMT_420F:
                        return 1;
                case FMT_422:
                case FMT_422F:
                        return 0;
		case 4:
			return 0;
                default:
                        break;
        }
        return 0;
}


int open_video_file(char *filename, editlist * el, int preserve_pathname, int deinter, int force, char override_norm)
{
	int i, n, nerr;
	int chroma=0;
	int _fc;
	int decoder_id = 0;
	const char *compr_type;
	int pix_fmt = -1;
	char *realname = NULL;	

 	if( filename == NULL ) 
	{
		veejay_msg(VEEJAY_MSG_ERROR, "No files to open!");
		return -1;
	}

	if (preserve_pathname)
		realname = strdup(filename);
	else
		realname = canonicalize_file_name( filename );

	if(realname == NULL )
	{
		veejay_msg(VEEJAY_MSG_ERROR, "Cannot get full path of '%s'", filename);
		return -1;
	}

	for (i = 0; i < el->num_video_files; i++)
	{
		if (strncmp(realname, el->video_file_list[i], strlen( el->video_file_list[i])) == 0)
		{
		    veejay_msg(VEEJAY_MSG_ERROR, "File %s already in editlist", realname);
		    if(realname) free(realname);
		    return i;
		}
	}

	if (el->num_video_files >= MAX_EDIT_LIST_FILES)
	{
		// mjpeg_error_exit1("Maximum number of video files exceeded");
		veejay_msg(VEEJAY_MSG_ERROR,"Maximum number of video files exceeded\n");
        	if(realname) free(realname);
		return -1; 
    	}

    	if (el->num_video_files >= 1)
		chroma = el->MJPG_chroma;
        
	int in_pixel_format = detect_pixel_format_with_ffmpeg( filename );
	
 
    	n = el->num_video_files;
	lav_file_t	*elfd = lav_open_input_file(filename,mmap_size );

	el->lav_fd[n] = NULL;

	if (elfd == NULL)
	{
		veejay_msg(VEEJAY_MSG_ERROR,"Error loading videofile '%s'", realname);
	        veejay_msg(VEEJAY_MSG_ERROR,"%s",lav_strerror());
	 	if(realname) free(realname);
		return -1;
	}

	if(lav_video_frames(elfd) < 2)
	{
		veejay_msg(VEEJAY_MSG_ERROR, "Cowardly refusing to load video files that contain less than 2 frames");
		if(realname) free(realname);
		lav_close(elfd);
		return -1;
	}

	
	_fc = lav_video_MJPG_chroma(elfd);
#ifdef STRICT_CHECKING
	if( in_pixel_format >= 0 )
	{
		if ( _fc == CHROMA422 )
			assert( (in_pixel_format == FMT_422 || in_pixel_format == FMT_422F ) );
		if ( _fc == CHROMA420 )
			assert( (in_pixel_format == FMT_420 || in_pixel_format == FMT_420F ) );
	}
#endif

	if( !(_fc == CHROMA422 || _fc == CHROMA420 || _fc == CHROMA444 || _fc == CHROMAUNKNOWN || _fc == CHROMA411 ))
	{
		veejay_msg(VEEJAY_MSG_ERROR,"Input file %s is not in a valid format (%d)",filename,_fc);
	    	if(realname) free(realname);
		lav_close( elfd );
		return -1;

	}

	if(chroma == CHROMAUNKNOWN)
	{ /* set chroma */
  	  el->MJPG_chroma = _fc;
	  chroma = _fc;
	}

	pix_fmt = _el_probe_for_pixel_fmt( elfd );
#ifdef STRICT_CHECKING
	if( in_pixel_format >= 0 )
		assert( pix_fmt == in_pixel_format );
#endif
	
	if(pix_fmt < 0 && in_pixel_format < 0)
	{
		veejay_msg(VEEJAY_MSG_ERROR, "Unable to determine pixel format");
		if(elfd) lav_close( elfd );
		if(realname) free(realname);
		return -1;
	}

	if( pix_fmt < 0 )
	{
		veejay_msg(VEEJAY_MSG_WARNING, "(!) Using pixelformat detected by FFmpeg (fallback)");
		pix_fmt = in_pixel_format;
	}

	el->yuv_taste[n] = pix_fmt;
	el->lav_fd[n] = elfd;
    	el->num_frames[n] = lav_video_frames(el->lav_fd[n]);
    	el->video_file_list[n] = strndup(realname, strlen(realname));
	
	    /* Debug Output */
	if(n == 0 )
	{
	    veejay_msg(VEEJAY_MSG_DEBUG,"\tFull name:       %s", filename, realname);
	    veejay_msg(VEEJAY_MSG_DEBUG,"\tFrames:          %ld", lav_video_frames(el->lav_fd[n]));
	    veejay_msg(VEEJAY_MSG_DEBUG,"\tWidth:           %d", lav_video_width(el->lav_fd[n]));
	    veejay_msg(VEEJAY_MSG_DEBUG,"\tHeight:          %d", lav_video_height(el->lav_fd[n]));
    	
		const char *int_msg;
		switch (lav_video_interlacing(el->lav_fd[n]))
		{
			case LAV_NOT_INTERLACED:
			    int_msg = "Not interlaced";
			    break;
			case LAV_INTER_TOP_FIRST:
			    int_msg = "Top field first";
			    break;
			case LAV_INTER_BOTTOM_FIRST:
			    int_msg = "Bottom field first";
			    break;
			default:
			    int_msg = "Unknown!";
			    break;
		}

		 if( deinter == 1 && (lav_video_interlacing(el->lav_fd[n]) != LAV_NOT_INTERLACED))
			el->auto_deinter = 1;

 	   	veejay_msg(VEEJAY_MSG_DEBUG,"\tInterlacing:      %s", int_msg);
		veejay_msg(VEEJAY_MSG_DEBUG,"\tFrames/sec:       %f", lav_frame_rate(el->lav_fd[n]));
		veejay_msg(VEEJAY_MSG_DEBUG,"\tSampling format:  %s", _chroma_str[ lav_video_MJPG_chroma(el->lav_fd[n])].name);
		veejay_msg(VEEJAY_MSG_DEBUG,"\tFOURCC:           %s",lav_video_compressor(el->lav_fd[n]));
		veejay_msg(VEEJAY_MSG_DEBUG,"\tAudio samps:      %ld", lav_audio_clips(el->lav_fd[n]));
		veejay_msg(VEEJAY_MSG_DEBUG,"\tAudio chans:      %d", lav_audio_channels(el->lav_fd[n]));
		veejay_msg(VEEJAY_MSG_DEBUG,"\tAudio bits:       %d", lav_audio_bits(el->lav_fd[n]));
		veejay_msg(VEEJAY_MSG_DEBUG,"\tAudio rate:       %ld", lav_audio_rate(el->lav_fd[n]));
	}
	else
	{
		veejay_msg(VEEJAY_MSG_DEBUG, "\tFull name	%s",realname);	
		veejay_msg(VEEJAY_MSG_DEBUG, "\tFrames	        %d", lav_video_frames(el->lav_fd[n]));
		veejay_msg(VEEJAY_MSG_DEBUG, "\tDecodes into    %s", _chroma_str[ lav_video_MJPG_chroma( el->lav_fd[n]) ]);
	}

    nerr = 0;
    if (n == 0) {
	/* First file determines parameters */
		if(el->is_empty)
		{	/* Dummy determines parameters */
			if(el->video_height != lav_video_height(el->lav_fd[n]))
				nerr++;
			if(el->video_width != lav_video_width(el->lav_fd[n]))
				nerr++;
		}
		else
		{
			el->video_height = lav_video_height(el->lav_fd[n]);
			el->video_width = lav_video_width(el->lav_fd[n]);
			el->video_inter = lav_video_interlacing(el->lav_fd[n]);
			el->video_fps = lav_frame_rate(el->lav_fd[n]);
		}
		lav_video_clipaspect(el->lav_fd[n],
				       &el->video_sar_width,
				       &el->video_sar_height);

		if (!el->video_norm)
		{
		    /* TODO: This guessing here is a bit dubious but it can be over-ridden */
		    if (el->video_fps > 24.95 && el->video_fps < 25.05)
			el->video_norm = 'p';
		    else if (el->video_fps > 29.92 && el->video_fps <= 30.02)
			el->video_norm = 'n';
		}

		if (!el->video_norm)
		{
			if(override_norm == 'p' || override_norm == 'n')
				el->video_norm = override_norm;
			else
			{
				veejay_msg(VEEJAY_MSG_ERROR,
				"Invalid video norm - override with -N / --norm");
				nerr++;
			}
		}
	
		if(!el->is_empty)
		{
			el->audio_chans = lav_audio_channels(el->lav_fd[n]);
			if (el->audio_chans > 2) {
		  	  veejay_msg(VEEJAY_MSG_ERROR, "File %s has %d audio channels - cant play that!",
			              filename,el->audio_chans);
			   nerr++;
			}
	
			el->has_audio = (el->audio_chans == 0 ? 0: 1);
			el->audio_bits = lav_audio_bits(el->lav_fd[n]);
			el->audio_rate = lav_audio_rate(el->lav_fd[n]);
			el->audio_bps = (el->audio_bits * el->audio_chans + 7) / 8;
		}
		else
		{
			if(lav_audio_channels(el->lav_fd[n]) != el->audio_chans ||
			 lav_audio_rate(el->lav_fd[n]) != el->audio_rate ||
			 lav_audio_bits(el->lav_fd[n]) != el->audio_bits )
				nerr++;
			else
				el->has_audio = 1;
		}
   	 } else {
		/* All files after first have to match the paramters of the first */
	
		if (el->video_height != lav_video_height(el->lav_fd[n]) ||
		    el->video_width != lav_video_width(el->lav_fd[n])) {
		    veejay_msg(VEEJAY_MSG_ERROR,"File %s: Geometry %dx%d does not match %dx%d.",
				filename, lav_video_width(el->lav_fd[n]),
				lav_video_height(el->lav_fd[n]), el->video_width,
				el->video_height);
		    nerr++;

	}
	if (el->video_inter != lav_video_interlacing(el->lav_fd[n])) {
	    if(force)
	    veejay_msg(VEEJAY_MSG_WARNING,"File %s: Interlacing is %d should be %d",
			filename, lav_video_interlacing(el->lav_fd[n]),
			el->video_inter);
		else
		veejay_msg(VEEJAY_MSG_ERROR, "File %s: Interlacing is %d should be %d",
			filename, lav_video_interlacing(el->lav_fd[n]),
			el->video_inter);


	    if(!el->auto_deinter)
		{
			if(force)
			{
				veejay_msg(VEEJAY_MSG_WARNING, "(Force loading video) Auto deinterlacing enabled");
				el->auto_deinter = 1;  
			}
			else
			{
				nerr++;
			}
		}
	}
	/* give a warning on different fps instead of error , this is better 
	   for live performances */
	if (fabs(el->video_fps - lav_frame_rate(el->lav_fd[n])) >
	    0.0000001) {
	    veejay_msg(VEEJAY_MSG_WARNING,"File %s: fps is %3.2f , but playing at %3.2f", filename,
		       lav_frame_rate(el->lav_fd[n]), el->video_fps);
	}
	/* If first file has no audio, we don't care about audio */

	if (el->has_audio) {
	    if( el->audio_rate < 44000 )
	    {
		veejay_msg(VEEJAY_MSG_ERROR, "File %s: Cannot play %d Hz audio. Use at least 44100 Hz or start with -a0", filename, el->audio_rate);
		nerr++;
	    }
            else {
	    if (el->audio_chans != lav_audio_channels(el->lav_fd[n]) ||
		el->audio_bits != lav_audio_bits(el->lav_fd[n]) ||
		el->audio_rate != lav_audio_rate(el->lav_fd[n])) {
		veejay_msg(VEEJAY_MSG_ERROR,"File %s: Mismatched audio properties: %d channels , %d bit %ld Hz",
			   filename, lav_audio_channels(el->lav_fd[n]),
			   lav_audio_bits(el->lav_fd[n]),
			   lav_audio_rate(el->lav_fd[n]) );
		nerr++;
	    }
            }
	}




	if (nerr) {
	    if(el->lav_fd[n]) 
			lav_close( el->lav_fd[n] );
	    el->lav_fd[n] = NULL;
	    if(realname) free(realname);
	    if(el->video_file_list[n]) 
		free(el->video_file_list[n]);
	    el->video_file_list[n] = NULL;
	    return -1;
        }
    }

	compr_type = (const char*) lav_video_compressor(el->lav_fd[n]);

	if(!compr_type)
	{
		veejay_msg(VEEJAY_MSG_ERROR, "Cannot get codec information from lav file");
		if(el->lav_fd[n])
		 lav_close( el->lav_fd[n] );
		el->lav_fd[n] = NULL;
		if(realname) free(realname);
		if(el->video_file_list[n]) 
			free(el->video_file_list[n]);
		el->video_file_list[n] = NULL;
		return -1;
	}
     // initialze a decoder if needed
	decoder_id = _el_get_codec_id( compr_type );
	if(decoder_id > 0 && decoder_id != 0xffff)
	{
		int c_i = _el_get_codec(decoder_id, el->yuv_taste[n] );
		if(c_i == -1)
		{
			veejay_msg(VEEJAY_MSG_ERROR, "Unsupported codec %s",compr_type);
			if( el->lav_fd[n] ) 
				lav_close( el->lav_fd[n] );
			el->lav_fd[n] = NULL;
			if( realname ) free(realname );
			if( el->video_file_list[n]) 
				free(el->video_file_list[n]);
			el->video_file_list[n] = NULL;
			return -1;
		}
		if( el_codecs[c_i] == NULL )
		{
		//	el_codecs[c_i] = _el_new_decoder( decoder_id, el->video_width, el->video_height, el->video_fps, pix_fmt );
			int ff_pf = get_ffmpeg_pixfmt( el_pixel_format_ );
			el_codecs[c_i] = _el_new_decoder( decoder_id, el->video_width, el->video_height, el->video_fps, el->yuv_taste[ n ],ff_pf );
			if(!el_codecs[c_i])
			{
				veejay_msg(VEEJAY_MSG_ERROR,"Cannot initialize %s codec", compr_type);
				if( el->lav_fd[n] ) 
					lav_close( el->lav_fd[n] );
				el->lav_fd[n] = NULL;
			    	if(realname) free(realname);
				if( el->video_file_list[n]) 
					free(el->video_file_list[n]);
				el->video_file_list[n] = NULL;
				return -1;
			}
		}
	}

	if(decoder_id <= 0)	
	{
		veejay_msg(VEEJAY_MSG_ERROR, "Dont know how to handle %s (fmt %d) %x", compr_type, pix_fmt,decoder_id);
		if(realname) free(realname);
		if( el->video_file_list[n]) free( el->video_file_list[n] );
		if( el->lav_fd[n] ) 
			lav_close( el->lav_fd[n]);
		el->lav_fd[n] = NULL;
		el->video_file_list[n] = NULL;
		return -1;
	}

	if(realname)
		free(realname);

	if(el->is_empty)
	{
		el->video_frames = el->num_frames[0];
		el->video_frames -= DUMMY_FRAMES;
	}
	el->is_empty = 0;	
	el->has_video = 1;
	el->num_video_files ++;
	el->total_frames = el->video_frames - 1;
    return n;
}

void		vj_el_show_formats(void)
{
#ifdef SUPPORT_READ_DV2
		veejay_msg(VEEJAY_MSG_INFO,
			"Video containers: AVI (up to 32gb), RAW DV and Quicktime");
#else
		veejay_msg(VEEJAY_MSG_INFO,
			"Video containers: AVI (up to 32gb) and  Quicktime");
#endif
		veejay_msg(VEEJAY_MSG_INFO,
			"Video fourcc (preferred): mjpg, mjpb, mjpa, dv, dvsd,sp5x,dmb1,dvcp,dvhd, yv16,i420");
		veejay_msg(VEEJAY_MSG_INFO,
			"Video codecs (preferred): YV16, I420, Motion Jpeg or Digital Video");
		veejay_msg(VEEJAY_MSG_INFO,
			"If the video file is made up out of only I-frames (whole images), you can also decode:");
		veejay_msg(VEEJAY_MSG_INFO,
			" mpg4,mp4v,svq3,svq1,rpza,hfyu,mp42,mpg43,davc,div3,x264,h264,avc1,m4s2,divx,xvid");
		veejay_msg(VEEJAY_MSG_INFO,
			"Use veejay's internal format YV16 to reduce CPU usage");
		
#ifdef USE_GDK_PIXBUF
		veejay_msg(VEEJAY_MSG_INFO,
			"Image types supported:");
		vj_picture_display_formats();
#endif


		
}


static int	vj_el_dummy_frame( uint8_t *dst[3], editlist *el ,int pix_fmt)
{
	const int uv_len = (el->video_width * el->video_height) / ( ( (pix_fmt==FMT_422||pix_fmt==FMT_422F) ? 2 : 4));
	const int len = el->video_width * el->video_height;
	veejay_memset( dst[0], 16, len );
	veejay_memset( dst[1],128, uv_len );
	veejay_memset( dst[2],128, uv_len );
	return 1;
}

void	vj_el_get_video_fourcc(editlist *el, int num, char *fourcc)
{
	uint64_t n;
//	if( num < 0 ) num = 0; else if (num >= el->video_frames ) num = el->video_frames - 1;
#ifdef STRICT_CHECKING
	assert( num >= 0 && num <= el->total_frames );
#else
	if (num < 0)
		num = 0;
	else if (num >= el->video_frames)
		num = el->video_frames-1;
#endif
	n = el->frame_list[ num ];

	const char *compr = lav_video_compressor( el->lav_fd[ N_EL_FILE(n) ] );
	snprintf(fourcc,4,"%s", compr );
}


int	vj_el_get_file_fourcc(editlist *el, int num, char *fourcc)
{
	if(num >= el->num_video_files)
		return 0;
	if( fourcc == NULL)
		return 0;

	const char *compr = lav_video_compressor( el->lav_fd[num] );
	if(compr == NULL)
		return 0;
	snprintf(fourcc,4,"%s", compr );
	fourcc[5] = '\0';
	return 1;
}


int	vj_el_bogus_length( editlist *el, long nframe )
{
	uint64_t n = 0;
	
	if( !el->has_video || el->is_empty )
		return 0;

	if( nframe < 0 )
		nframe = 0;
	else if (nframe >= el->video_frames )
		nframe = el->video_frames - 1;

	n = el->frame_list[nframe];

	return lav_bogus_video_length( el->lav_fd[ N_EL_FILE(n) ] );
}

int	vj_el_set_bogus_length( editlist *el, long nframe, int len )
{
	uint64_t n = 0;
	
	if( len <= 0 )
		return 0;	

	if( !el->has_video || el->is_empty )
		return 0;
#ifdef STRICT_CHECKING
	assert( nframe >= 0 && nframe <= el->total_frames );
#else
	if (nframe < 0)
		nframe = 0;

	if (nframe >= el->video_frames)
		nframe = el->video_frames-1;

#endif

	n = el->frame_list[nframe];

	if( !lav_bogus_video_length( el->lav_fd[N_EL_FILE(n)] ) )
		return 0;

	lav_bogus_set_length( el->lav_fd[N_EL_FILE(n)], len );
	
	return 1;
}

int	vj_el_get_video_frame(editlist *el, long nframe, uint8_t *dst[3])
{
	if( el->has_video == 0 || el->is_empty )
	{
		vj_el_dummy_frame( dst, el, el->pixel_format );
		return 2;
	}

	int res = 0;
   	uint64_t n;
	int decoder_id =0;
	int c_i = 0;
	vj_decoder *d = NULL;
	int out_pix_fmt = el->pixel_format;
	int in_pix_fmt  = out_pix_fmt;

#ifdef STRICT_CHECKING
	if( nframe > el->total_frames  )
		veejay_msg(0, "Fatal: %ld requested, have only %ld",
			nframe, el->total_frames);
	assert( nframe >= 0 && nframe < el->video_frames );
#else
	if (nframe < 0)
		nframe = 0;

	if (nframe >= el->video_frames)
		nframe = el->video_frames-1;

#endif
	n = el->frame_list[nframe];

	in_pix_fmt = el->yuv_taste[N_EL_FILE(n)];

	uint8_t *in_cache = NULL;
	if(el->cache)
		in_cache = get_cached_frame( el->cache, nframe, &res, &decoder_id );
	
	if(! in_cache )	
	{
		res = lav_set_video_position(el->lav_fd[N_EL_FILE(n)], N_EL_FRAME(n));
       		decoder_id = lav_video_compressor_type( el->lav_fd[N_EL_FILE(n)] );

		if (res < 0)
		{
			veejay_msg(VEEJAY_MSG_ERROR,"Error setting video position: %s",
				  lav_strerror());
			return -1;
 		}
	}

	if( decoder_id == 0xffff )
	{
		VJFrame *srci  = lav_get_frame_ptr( el->lav_fd[ N_EL_FILE(n) ] );
		if( srci == NULL )
		{
			veejay_msg(VEEJAY_MSG_ERROR, "Error decoding Image %ld",
				N_EL_FRAME(n));
			return -1;
		}
#ifdef STRICT_CHECKING
		assert( dst[0] != NULL  && dst[1] != NULL && dst[2] != NULL );
#endif	
		veejay_memcpy( dst[0], srci->data[0], el_len_ );
                veejay_memcpy( dst[1], srci->data[1], el_uv_len_ );
                veejay_memcpy( dst[2], srci->data[2], el_uv_len_ );
                return 1;     
	}

	c_i = _el_get_codec( decoder_id , in_pix_fmt);
	if(c_i >= 0 && c_i < MAX_CODECS && el_codecs[c_i] != NULL)
       	       	d = el_codecs[c_i];
	else
	{
		veejay_msg(VEEJAY_MSG_ERROR, "Choked on decoder %x (%d), slot %d",decoder_id,decoder_id, c_i );
		return -1;
	}

	if(!in_cache)
	{
		if( d == NULL )
		{
			veejay_msg(VEEJAY_MSG_ERROR, "Codec not initlaized");
			return -1;
		}
		if(lav_filetype( el->lav_fd[N_EL_FILE(n)] ) != 'x')
		{
		    res = lav_read_frame(el->lav_fd[N_EL_FILE(n)], d->tmp_buffer);
		    if(res > 0 && el->cache)
			cache_frame( el->cache, d->tmp_buffer, res, nframe, decoder_id );
		}
	}

	uint8_t *data = ( in_cache == NULL ? d->tmp_buffer: in_cache );
	int inter = 0;
	int got_picture = 0;

	switch( decoder_id )
	{
		case CODEC_ID_YUV420:
			if(out_pix_fmt == FMT_420 || out_pix_fmt == FMT_420F)
                	{
                	        veejay_memcpy( dst[0], data, el_len_);
               		        veejay_memcpy( dst[1], data+el_len_,el_uv_len_);
                		veejay_memcpy( dst[2], data+(el_len_+el_uv_len_), el_uv_len_);
                	}
               		else
                	{
				VJFrame *srci = yuv_yuv_template( data,data+el_len_,data+el_len_+el_uv_len_,el->video_width,
							el->video_height, PIX_FMT_YUV420P );
				VJFrame *dsti = yuv_yuv_template( dst[0],dst[1],dst[2], el->video_width,
							el->video_height, get_ffmpeg_pixfmt( out_pix_fmt));
				yuv_convert_any_ac( srci, dsti, srci->format, dsti->format );

				free(srci);
				free(dsti);
                	}
			return 1;
			break;
		case CODEC_ID_YUV422:
			if(out_pix_fmt == FMT_420 || out_pix_fmt == FMT_420F)
			{
				VJFrame *srci = yuv_yuv_template( data,data+el_len_,data+el_len_+el_uv_len_, el->video_width,
							el->video_height, PIX_FMT_YUV422P );
				VJFrame *dsti = yuv_yuv_template( dst[0],dst[1], dst[2], el->video_width,
							el->video_height, get_ffmpeg_pixfmt( out_pix_fmt));
				yuv_convert_any_ac( srci,dsti, srci->format, dsti->format );
				free(srci);
				free(dsti);
			}
			else
			{	
				veejay_memcpy( dst[0], data, el_len_);
            			veejay_memcpy( dst[1], data+el_len_,el_uv_len_);
            			veejay_memcpy( dst[2], data+el_len_+el_uv_len_, el_uv_len_);
			}
			return 1;
			break;
		case CODEC_ID_DVVIDEO:
#ifdef SUPPORT_READ_DV2
			return vj_dv_decode_frame( dv_decoder_,  data, dst[0], dst[1], dst[2], el->video_width,el->video_height,
					out_pix_fmt);
#else
			return 0;
#endif			
			break;
		case CODEC_ID_YUVLZO:

			inter = lzo_decompress( lzo_decoder_, data,res, dst );
			if( inter == 0 )
				return 0;

			if( in_pix_fmt != out_pix_fmt )
			{
				if( out_pix_fmt > in_pix_fmt )
					chroma_supersample( SSM_420_422, d->sampler, dst, el->video_width,el->video_height );
				else
					chroma_subsample( SSM_420_422, d->sampler, dst, el->video_width,el->video_height );
			}
			return inter;

			break;			
		default:
			inter = lav_video_interlacing(el->lav_fd[N_EL_FILE(n)]);
			d->img->width = el->video_width;
			d->img->uv_width = el->video_width / 2;
			d->img->data[0] = dst[0];
			d->img->data[1] = dst[1];
			d->img->data[2] = dst[2];
			
			int decode_len = avcodec_decode_video(
				d->context,
				d->frame,
				&got_picture,
				data,
				res
			);

			if(!got_picture)
			{
				veejay_msg(0, "Cannot decode frame ,unable to get whole picture");
				return 0;
			}
		
			if( decode_len <= 0 )
			{
				veejay_msg(VEEJAY_MSG_ERROR, "Cannot decode frame");
				return 0;
			}

			int dst_fmt = get_ffmpeg_pixfmt( el_pixel_format_ );
			int src_fmt = d->context->pix_fmt;
			if(!d->frame->opaque)
			{
				if( el->auto_deinter && inter != LAV_NOT_INTERLACED)
				{
					AVPicture pict2;
					veejay_memset(&pict2,0,sizeof(AVPicture));
					pict2.data[0] = d->deinterlace_buffer[0];
					pict2.data[1] = d->deinterlace_buffer[1];
					pict2.data[2] = d->deinterlace_buffer[2];
					pict2.linesize[1] = el->video_width >> 1;
					pict2.linesize[2] = el->video_width >> 1;
					pict2.linesize[0] = el->video_width;
					avpicture_deinterlace(
						&pict2,
						(const AVPicture*) d->frame,
						src_fmt,
						el->video_width,
						el->video_height);

					VJFrame *src1 = yuv_yuv_template( d->deinterlace_buffer[0],
								d->deinterlace_buffer[1], d->deinterlace_buffer[2],
								el->video_width, el->video_height,	
								src_fmt );
					VJFrame *dst1 = yuv_yuv_template( dst[0],dst[1],dst[2],
								el->video_width, el->video_height,
								dst_fmt );


					yuv_convert_any3( src1,d->frame->linesize,dst1,src1->format,dst1->format);

					free(src1);
					free(dst1);

				}
				else
				{
					VJFrame *src1 = yuv_yuv_template( d->frame->data[0],
								d->frame->data[1], d->frame->data[2],
								el->video_width,el->video_height,
								src_fmt );
					VJFrame *dst1 = yuv_yuv_template( dst[0],dst[1],dst[2],
								el->video_width,el->video_height,
								dst_fmt );
		
					yuv_convert_any3( src1,d->frame->linesize,dst1,src1->format,dst1->format);
					free(src1);
					free(dst1);
				}
			}
			else
			{
				dst[0] = d->frame->data[0];
				dst[1] = d->frame->data[1];
				dst[2] = d->frame->data[2];
			}
			return 1;
			break;
	}

	veejay_msg(VEEJAY_MSG_ERROR, "Error decoding frame %ld", nframe);
	return 0;  
}

int	detect_pixel_format_with_ffmpeg( const char *filename )
{
	AVCodec *codec = NULL;
	AVCodecContext *codec_ctx = NULL;
	AVFormatContext *avformat_ctx = NULL;
	AVStream *avformat_stream = NULL;	
	AVInputFormat *av_input_format = NULL;
	AVFormatParameters avf;
	AVFrame *av_frame = NULL;
	AVPacket pkt;

//	int err = av_open_input_file( &avformat_ctx,filename, av_input_format,0,&avf);	
	int err = av_open_input_file( &avformat_ctx,filename,NULL,0,NULL );
	if(err < 0 ) {
		veejay_msg(VEEJAY_MSG_DEBUG, "FFmpeg: Unable to open %s: %d",filename,err );
		return -1;
	}
	err = av_find_stream_info( avformat_ctx );
	if(err < 0 )
	{
		veejay_msg(VEEJAY_MSG_DEBUG, "FFmpeg: Stream information found in %s",filename);
		av_close_input_file( avformat_ctx );
		return -1;
	}
	av_read_play(avformat_ctx);

	int i,j;
	int n = avformat_ctx->nb_streams;
	int vi = -1;

	veejay_msg(VEEJAY_MSG_DEBUG, "FFmpeg: File has %d %s", n, ( n == 1 ? "stream" : "streams") );

	for( i=0; i < n; i ++ )
	{
		if( avformat_ctx->streams[i]->codec )
		{
			if( avformat_ctx->streams[i]->codec->codec_type == CODEC_TYPE_VIDEO )
			{
				int sup_codec = 0;
				for( j = 0; _supported_codecs[j].name != NULL; j ++ ) {
					if( avformat_ctx->streams[i]->codec->codec_id == _supported_codecs[j].id ) {
						sup_codec = 1;
						goto further;
					}
				}	
further:
				if( !sup_codec ) {
					veejay_msg(VEEJAY_MSG_DEBUG, "FFmpeg: Unrecognized file %s",		
						avformat_ctx->streams[i]->codec->codec_name );
					av_close_input_file( avformat_ctx );
					return -1;
				}
				codec = avcodec_find_decoder( avformat_ctx->streams[i]->codec->codec_id );
				if( codec == NULL ) 
				{
					veejay_msg(VEEJAY_MSG_DEBUG, "FFmpeg: Unable to find decoder for codec %s", 
						avformat_ctx->streams[i]->codec->codec_name);	
					av_close_input_file( avformat_ctx );
					return -1;
				}
				vi = i;
				break;
			}
		}
	}

	if( vi == -1 ) {
		veejay_msg(VEEJAY_MSG_DEBUG, "FFmpeg: No video streams found");
		av_close_input_file( avformat_ctx );
		return -1;
	}

	codec_ctx = avformat_ctx->streams[vi]->codec;
	avformat_stream=avformat_ctx->streams[vi];
	if ( avcodec_open( codec_ctx, codec ) < 0 ) {
		veejay_msg(VEEJAY_MSG_DEBUG, "FFmpeg: Unable to open %s decoder (codec %x)",
				codec_ctx->codec_name, codec_ctx->codec_id);
		return -1;
	}

	veejay_memset( &pkt, 0, sizeof(AVPacket));
	AVFrame *f = avcodec_alloc_frame();
	int input_pix_fmt = -1;

	int got_picture = 0;
	while( (av_read_frame(avformat_ctx, &pkt) >= 0 ) ) {
		avcodec_decode_video( codec_ctx,f,&got_picture, pkt.data, pkt.size );

		if( got_picture ) {
			break;
		}
	}
	
	if(!got_picture) {
		veejay_msg(VEEJAY_MSG_DEBUG, "FFmpeg: Error while reading %s", filename );
		av_free(f);
		av_free_packet(&pkt); 
		avcodec_close( codec_ctx );
		av_close_input_file( avformat_ctx );
		return -1;
	}
		
	veejay_msg(VEEJAY_MSG_DEBUG, "FFmpeg reports Video [%s] %dx%d. Pixel format: %x Has B frames: %s (%s)",
		codec_ctx->codec_name, codec_ctx->width,codec_ctx->height, codec_ctx->pix_fmt, 
		(codec_ctx->has_b_frames ? "Yes" : "No"), filename );

	av_free_packet(&pkt); 
	avcodec_close( codec_ctx );
	av_close_input_file( avformat_ctx );
	av_free(f);

	switch( input_pix_fmt ) {	
		case PIX_FMT_YUV420P:	input_pix_fmt = FMT_420;	break;	
		case PIX_FMT_YUV422P:	input_pix_fmt = FMT_422;	break;
		case PIX_FMT_YUVJ420P:	input_pix_fmt = FMT_420F;	break;
		case PIX_FMT_YUVJ422P:	input_pix_fmt = FMT_422F;	break;
	}
	
	return input_pix_fmt;	
}
 
int	test_video_frame( lav_file_t *lav,int out_pix_fmt)
{
	int in_pix_fmt  = 0;

	int res = lav_set_video_position( lav,  0);
	if( res < 0 )
	{
		veejay_msg(VEEJAY_MSG_ERROR, "Error setting frame 0: %s", lav_strerror());
		return -1;
	}
	
    	int decoder_id = lav_video_compressor_type( lav );

	if( decoder_id < 0 )
	{
		veejay_msg(VEEJAY_MSG_ERROR, "Cannot play that file, unsupported codec");
		return -1;
	}

	if(lav_filetype( lav ) == 'x')
	{
		veejay_msg(VEEJAY_MSG_INFO,"\tFile is an image");
		return out_pix_fmt;
	}

	switch( lav->MJPG_chroma )
	{
		case CHROMA420:
			in_pix_fmt = FMT_420; break;
		case CHROMA422:
		case CHROMA411:
			in_pix_fmt = FMT_422; break;
		default:
			veejay_msg(0 ,"Unsupported pixel format");
			break;			
	}
	vj_decoder *d = _el_new_decoder(
					decoder_id,
					lav_video_width( lav),
					lav_video_height( lav),
				   	(float) lav_frame_rate( lav ),
					in_pix_fmt,
					out_pix_fmt );

	if(!d)
	{
		veejay_msg(VEEJAY_MSG_ERROR, "Choked on decoder %x", decoder_id);
		return -1;
	}

	res = lav_read_frame( lav, d->tmp_buffer);

	if( res <= 0 )
	{
		veejay_msg(VEEJAY_MSG_ERROR, "Error reading frame: %s", lav_strerror());
		_el_free_decoder( d );
		return -1;
	}
	
	int got_picture = 0;
	int ret = -1;
	int len = 0;
	switch( decoder_id )
	{
		case CODEC_ID_YUV420:
			if( out_pix_fmt == FMT_422 ) {
				ret = FMT_420;
			} else if( out_pix_fmt == FMT_420 ) {
				ret = FMT_420;
			} else if( out_pix_fmt == FMT_422F ) {
				ret = FMT_420F;
			} else
			   	ret = FMT_420F;
			break;
		case CODEC_ID_YUV422:
			if( out_pix_fmt == FMT_422 ) {
				ret = FMT_422;
			} else if( out_pix_fmt == FMT_420 ) {
				ret = FMT_422;
			} else if( out_pix_fmt == FMT_420F ) {
				ret = FMT_422F;
			} else 
				ret = FMT_422F;
			break;
		case CODEC_ID_DVVIDEO:
#ifdef SUPPORT_READ_DV2
			ret = vj_dv_scan_frame( dv_decoder_, d->tmp_buffer );
			if( ret == FMT_420 || ret == FMT_420F )
				lav->MJPG_chroma = CHROMA420;
			else
				lav->MJPG_chroma = CHROMA422;
#endif
			break;
		case CODEC_ID_YUVLZO:
			ret = FMT_422;
			if ( in_pix_fmt != ret )	
			{
				//@ correct chroma 
				if( ret == FMT_420 || ret == FMT_420F )
					lav->MJPG_chroma = CHROMA420;
				else
					lav->MJPG_chroma = CHROMA422;
			}

			break;
		default:

			len = avcodec_decode_video(
				d->context,
				d->frame,
				&got_picture,
				d->tmp_buffer,
				res
			);

			if(!got_picture || len <= 0 )
			{
				veejay_msg(VEEJAY_MSG_ERROR, "Unable to get whole picture");
				ret = -1;
			}
			else
				switch( d->context->pix_fmt )
				{
					case PIX_FMT_YUV420P: ret = FMT_420; break;
					case PIX_FMT_YUV422P: ret = FMT_422; break;
					case PIX_FMT_YUVJ420P:ret = FMT_420F;break;
					case PIX_FMT_YUVJ422P:ret = FMT_422F;break;
					default:
						ret= d->context->pix_fmt;
						break;
				}
			
				break;	

	}

	_el_free_decoder( d );
	
	return ret;  
}



int	vj_el_get_audio_frame(editlist *el, uint32_t nframe, uint8_t *dst)
{
    long pos, asize;
    int ret = 0;
    uint64_t n;	
	int ns0, ns1;

	if(el->is_empty)
	{
		int ns = el->audio_rate / el->video_fps;
		veejay_memset( dst, 0, sizeof(uint8_t) * ns * el->audio_bps );
		return 1;
	}

    	if (!el->has_audio)
		return 0;
#ifdef STRICT_CHECKING
	assert( nframe >= 0 && nframe < el->video_frames );
#else
    	if (nframe < 0)
		nframe = 0;

	if (nframe >= el->video_frames)
		nframe = el->video_frames-1;

#endif

    n = el->frame_list[nframe];

    /*if( lav_is_DV( el->lav_fd[N_EL_FILE(n)] ) )
    {
	lav_set_video_position( el->lav_fd[N_EL_FILE(n)] , nframe );
	return lav_read_audio( el->lav_fd[N_EL_FILE(n)], dst, 0  );
    }*/

    ns1 = (double) (N_EL_FRAME(n) + 1) * el->audio_rate / el->video_fps;
    ns0 = (double) N_EL_FRAME(n) * el->audio_rate / el->video_fps;

    //asize = el->audio_rate / el->video_fps;
    pos = nframe * asize;

    ret = lav_set_audio_position(el->lav_fd[N_EL_FILE(n)], ns0);

    if (ret < 0)
    {
	    veejay_msg(0,"Unable to seek to frame position %ld", ns0);
		return -1;
	}
    //mlt need int16_t
    ret = lav_read_audio(el->lav_fd[N_EL_FILE(n)], dst, (ns1 - ns0));
    if (ret < 0)
 	return -1;
    return (ns1 - ns0);

}

int	vj_el_init_420_frame(editlist *el, VJFrame *frame)
{
	if(!el) return 0;
	frame->data[0] = NULL;
	frame->data[1] = NULL;
	frame->data[2] = NULL;
	frame->uv_len = (el->video_width>>1) * (el->video_height>>1);
	frame->uv_width = el->video_width >> 1;
	frame->uv_height = el->video_height >> 1;
	frame->len = el->video_width * el->video_height;
	frame->shift_v = 1;
	frame->shift_h = 1;
	frame->width = el->video_width;
	frame->height = el->video_height;
	frame->ssm = 0;
	frame->stride[0] = el->video_width;
	frame->stride[1] = frame->stride[2] = frame->stride[0]/2;
	frame->format = el_pixel_format_;
	return 1;
}


int	vj_el_init_422_frame(editlist *el, VJFrame *frame)
{
	if(!el) return 0;
	frame->data[0] = NULL;
	frame->data[1] = NULL;
	frame->data[2] = NULL;
	frame->uv_len = (el->video_width>>1) * (el->video_height);
	frame->uv_width = el->video_width >> 1;
	frame->uv_height = el->video_height;
	frame->len = el->video_width * el->video_height;
	frame->shift_v = 0;
	frame->shift_h = 1;
	frame->width = el->video_width;
	frame->height = el->video_height;
	frame->ssm = 0;
	frame->stride[0] = el->video_width;
	frame->stride[1] = frame->stride[2] = frame->stride[0]/2;
	frame->format = el_pixel_format_;
	return 1;
}

int	vj_el_get_audio_frame_at(editlist *el, uint32_t nframe, uint8_t *dst, int num )
{
	// get audio from current frame + n frames
    long pos, asize;
    int ret = 0;
    uint64_t n;	
    int ns0, ns1;

    if (!el->has_audio)
	return 0;

    if  (!el->has_video)
	{
		int size = el->audio_rate / el->video_fps * el->audio_bps;
		veejay_memset(dst,0,size);
		return size;
	}
#ifdef STRICT_CHECKING
	assert( nframe >= 0 && nframe < el->video_frames );
#else
	if (nframe < 0)
		nframe = 0;

	if (nframe >= el->video_frames)
		nframe = el->video_frames-1;

#endif

    n = el->frame_list[nframe];

    ns1 = (double) (N_EL_FRAME(n) + num) * el->audio_rate / el->video_fps;
    ns0 = (double) N_EL_FRAME(n) * el->audio_rate / el->video_fps;

    //asize = el->audio_rate / el->video_fps;
    pos = nframe * asize;
    ret = lav_set_audio_position(el->lav_fd[N_EL_FILE(n)], ns0);

    if (ret < 0)
		return -1;

    //mlt need int16_t
    ret = lav_read_audio(el->lav_fd[N_EL_FILE(n)], dst, (ns1 - ns0));
    if (ret < 0)
		return -1;

    return (ns1 - ns0);

}


editlist *vj_el_dummy(int flags, int deinterlace, int chroma, char norm, int width, int height, float fps, int fmt)
{
	editlist *el = vj_calloc(sizeof(editlist));
	if(!el) return NULL;
	el->MJPG_chroma = chroma;
	el->video_norm = norm;
	el->is_empty = 1;
	el->is_clone = 1;
	el->has_audio = 0;
	el->audio_rate = 0;
	el->audio_bits = 0;
	el->audio_bps = 0;
	el->audio_chans = 0;
	el->num_video_files = 0;
	el->video_width = width;
	el->video_height = height;
	el->video_frames = DUMMY_FRAMES; /* veejay needs at least 2 frames (play and queue next)*/
	el->total_frames = DUMMY_FRAMES-1;
	el->video_fps = fps;
	el->video_inter = LAV_NOT_INTERLACED;

	/* output pixel format */
	if( fmt == -1 )
		el->pixel_format = el_pixel_format_;
	
	el->pixel_format = fmt;

	el->auto_deinter = deinterlace;
	el->max_frame_size = width*height*3;
	el->last_afile = -1;
	el->last_apos = 0;
	el->frame_list = NULL;
	el->has_video = 0;
	el->cache = NULL;
	el_len_ = el->video_width * el->video_height;
	el_uv_len_ = el_len_;

	switch(fmt)
	{
		case FMT_420:
		case FMT_420F:
			el_uv_len_ = el_len_ / 4;
			el_uv_wid_ = el->video_width/2;
			break;
		case FMT_422:
		case FMT_422F:
			el_uv_len_ = el_len_ / 2;
			el_uv_wid_ = el->video_width;
			break;
	}

	return el;
}

editlist *vj_el_init_with_args(char **filename, int num_files, int flags, int deinterlace, int force	,char norm , int fmt)
{
	editlist *el = vj_calloc(sizeof(editlist));
	FILE *fd;
	char line[1024];
	uint64_t	index_list[MAX_EDIT_LIST_FILES];
	int	num_list_files;
	long i,nf=0;
	int n1=0;
	int n2=0;
	long nl=0;
	uint64_t n =0;
	bzero(line,1024);
	if(!el) return NULL;
	veejay_memset( el, 0, sizeof(editlist) );  

	el->has_video = 1; //assume we get it   
	el->MJPG_chroma = CHROMA420;
    /* Check if a norm parameter is present */
	if(!filename[0] || filename == NULL)
	{
		veejay_msg(VEEJAY_MSG_ERROR,"\tInvalid filename given");
		vj_el_free(el);
		return NULL;	
	}

    if (strcmp(filename[0], "+p") == 0 || strcmp(filename[0], "+n") == 0)
	{
		el->video_norm = filename[0][1];
		nf = 1;
		veejay_msg(VEEJAY_MSG_DEBUG,"Norm set to %s",  el->video_norm == 'n' ? "NTSC" : "PAL");
    }

	if(force)
	{
		veejay_msg(VEEJAY_MSG_WARNING, "Forcing load on interlacing and gop_size");
	}

	for (; nf < num_files; nf++)
	{
		/* Check if file really exists, is mounted etc... */
		struct stat fileinfo;
		if(stat( filename[nf], &fileinfo)!= 0)
		{
			veejay_msg(VEEJAY_MSG_ERROR, "Unable to access file '%s'",filename[nf] );
			vj_el_free(el);
			return NULL;
		} 
			fd = fopen(filename[nf], "r");
			if (fd <= 0)
			{
			   	 veejay_msg(VEEJAY_MSG_ERROR,"Error opening %s:", filename[nf]);
			 	 vj_el_free(el);
	 			 return NULL;
			}
		
			fgets(line, 1024, fd);
			if (strcmp(line, "LAV Edit List\n") == 0)
			{
				   	/* Ok, it is a edit list */
			    	veejay_msg(VEEJAY_MSG_DEBUG, "Edit list %s opened", filename[nf]);
			    	/* Read second line: Video norm */
			    	fgets(line, 1024, fd);
			    	if (line[0] != 'N' && line[0] != 'n' && line[0] != 'P' && line[0] != 'p')
				{
					veejay_msg(VEEJAY_MSG_ERROR,"Edit list second line is not NTSC/PAL");
					vj_el_free(el);
					return NULL;
				}
				veejay_msg(VEEJAY_MSG_DEBUG,"Edit list norm is %s", line[0] =='N' || line[0] == 'n' ? "NTSC" : "PAL" );
			    	if (line[0] == 'N' || line[0] == 'n')
					{
						if (el->video_norm == 'p')
						{	
							veejay_msg(VEEJAY_MSG_WARNING, "Norm already set to PAL, ignoring new norm 'NTSC'");
						}
						else el->video_norm = 'n';
					}
		    		else
				{
					if (el->video_norm == 'n')
					{
					    	veejay_msg(VEEJAY_MSG_WARNING,"Norm allready set to NTSC, ignoring new norm PAL");
					}
					else
						el->video_norm = 'p';
			    	}
		   	 	/* read third line: Number of files */
		    		fgets(line, 1024, fd);
		    		sscanf(line, "%d", &num_list_files);

		   	 	veejay_msg(VEEJAY_MSG_DEBUG, "Edit list contains %d files", num_list_files);
		   		/* read files */

			    	for (i = 0; i < num_list_files; i++)
				{
					fgets(line, 1024, fd);
					n = strlen(line);

					if (line[n - 1] != '\n')
					{
					    veejay_msg(VEEJAY_MSG_ERROR, "Filename in edit list too long");
						vj_el_free(el);
						return NULL;
					}

					line[n - 1] = 0;	/* Get rid of \n at end */
	
					index_list[i] =
					    open_video_file(line, el, flags, deinterlace,force,norm);
	
					if(index_list[i]< 0)
					{
						vj_el_free(el);
						return NULL;
					}

				/*	el->frame_list = (uint64_t *) realloc(el->frame_list,
					      (el->video_frames +
					       el->num_frames[i]) *
					      sizeof(uint64_t));
					if (el->frame_list==NULL)
					{
						veejay_msg(VEEJAY_MSG_ERROR, "Insufficient memory to allocate frame_list");
						vj_el_free(el);
						return NULL;
					}
					
					long x = el->num_frames[i] + el->total_frames;	
					long j;
	    				for (j = el->video_frames; j < x; j++)
					{
						el->frame_list[el->video_frames] = EL_ENTRY(n, j);
						el->video_frames++;
					}*/


		   		 }
	
				    /* Read edit list entries */
			
				    while (fgets(line, 1024, fd))
				   {
						if (line[0] != ':')
						{	/* ignore lines starting with a : */
						    sscanf(line, "%ld %d %d", &nl, &n1, &n2);
			   
						    if (nl < 0 || nl >= num_list_files)
							{
					    		veejay_msg(VEEJAY_MSG_ERROR,"Wrong file number in edit list entry");
								vj_el_free(el);
								return NULL;
			    				}
						    if (n1 < 0)
								n1 = 0;
			  	    		    if (n2 >= el->num_frames[index_list[nl]])
							n2 = el->num_frames[index_list[nl]];
			    			    if (n2 < n1)
							continue;
	
					 el->frame_list = (uint64_t *) realloc(el->frame_list,
								      (el->video_frames +
								       n2 - n1 +
								       1) * sizeof(uint64_t));
	
				   			if (el->frame_list==NULL)
							{
								veejay_msg(VEEJAY_MSG_ERROR, "Insufficient memory to allocate frame_list");
								vj_el_free(el);
								return NULL;
							}
	
	
               		        for (i = n1; i <= n2; i++)
							{
								el->frame_list[el->video_frames] =  EL_ENTRY( index_list[ nl], i);

								el->video_frames++;
							}
						}
		    		} /* done reading editlist entries */
			fclose(fd);
			}
			else
			{
	    		/* Not an edit list - should be a ordinary video file */
	    			fclose(fd);

		     		n = open_video_file(filename[nf], el, flags, deinterlace,force,norm);
				if(n >= 0 )
				{
			       		el->frame_list = (uint64_t *) realloc(el->frame_list,
					      (el->video_frames +
					       el->num_frames[n]) *
					      sizeof(uint64_t));
					if (el->frame_list==NULL)
					{
						veejay_msg(VEEJAY_MSG_ERROR, "Insufficient memory to allocate frame_list");
						vj_el_free(el);
						return NULL;
					}

	    				for (i = 0; i < el->num_frames[n]; i++)
					{
						el->frame_list[el->video_frames] = EL_ENTRY(n, i);
						el->video_frames++;
					}
				}
			}
	}

	if( el->num_video_files == 0 || 
		el->video_width == 0 || el->video_height == 0 || el->video_frames < 2)
	{
		if( el->video_frames < 2 ) {
			veejay_msg(VEEJAY_MSG_ERROR, "\tFile has only %d frame(s)", el->video_frames );
			vj_el_free(el); return NULL;
		}
		if( el->num_video_files == 0 ) {
			veejay_msg(VEEJAY_MSG_ERROR, "\tNo videofiles in EDL");
			vj_el_free(el); return NULL;
		}
		if( el->video_height == 0 || el->video_width == 0 ) {
			veejay_msg(VEEJAY_MSG_ERROR, "\tImage dimensions unknown");
			vj_el_free(el); return NULL;
		}
		vj_el_free(el);
		return NULL;
	}

	/* do we have anything? */

	/* Calculate maximum frame size */

	for (i = 0; i < el->video_frames; i++)
	{
		n = el->frame_list[i];
		if(!el->lav_fd[N_EL_FILE(n)] )
		{
			veejay_msg(VEEJAY_MSG_ERROR, "\tUnable to read file");
			vj_el_free(el);
			return NULL;
		}
		if (lav_frame_size(el->lav_fd[N_EL_FILE(n)], N_EL_FRAME(n)) >
		    el->max_frame_size)
		    el->max_frame_size =
			lav_frame_size(el->lav_fd[N_EL_FILE(n)], N_EL_FRAME(n));


   	}

	/* Pick a pixel format */
	
	el->pixel_format = el_pixel_format_;

	/* Help for audio positioning */

	el->last_afile = -1;


    //el->auto_deinter = auto_deinter;
    //if(el->video_inter != 0 ) el->auto_deinter = 0;
	el->auto_deinter = 0;
	el->total_frames = el->video_frames - 1;

	el_len_ = el->video_width * el->video_height;
	el_uv_len_ = el_len_;

	switch( el_pixel_format_ )
	{
		case FMT_420:
		case FMT_420F:
			el_uv_len_ = el_len_ / 4;
			el_uv_wid_ = el->video_width/2;
			break;
		case FMT_422:
		case FMT_422F:
			el_uv_len_ = el_len_ / 2;
			el_uv_wid_ = el->video_width;
			break;
	}

	return el;
}


void	vj_el_free(editlist *el)
{
	if(!el)
		return;

	int i;
	if(el->is_clone)
	{
		for( i = 0; i < el->num_video_files; i ++ )
		{
			if( el->video_file_list[i])
				free(el->video_file_list[i] );
		}
	}
	else
	{
		for ( i = 0; i < el->num_video_files; i ++ )
		{
			if( el->lav_fd[i] ) 
			{
				lav_close( el->lav_fd[i] );
				el->lav_fd[i] = NULL;
			}
			if( el->video_file_list[i]) {
				free(el->video_file_list[i]);
				el->video_file_list[i] = NULL;
			}
		}
	}

	if( el->cache )
		free_cache( el->cache );
	if( el->frame_list )
		free(el->frame_list );
	free(el);

	el = NULL;
}

void	vj_el_print(editlist *el)
{
	int i;
	char timecode[64];
	char interlacing[64];
	MPEG_timecode_t ttc;
	veejay_msg(VEEJAY_MSG_INFO,"EditList settings: Video:%dx%d@%2.2f %s\tAudio:%d Hz/%d channels/%d bits",
		el->video_width,el->video_height,el->video_fps,(el->video_norm=='p' ? "PAL" :"NTSC"),
		el->audio_rate, el->audio_chans, el->audio_bits);
	for(i=0; i < el->num_video_files ; i++)
	{
		long num_frames = lav_video_frames(el->lav_fd[i]);
		MPEG_timecode_t tc;
		switch( lav_video_interlacing(el->lav_fd[i]))
		{
			case LAV_NOT_INTERLACED:
				sprintf(interlacing, "Not interlaced"); break;
			case LAV_INTER_TOP_FIRST:
				sprintf(interlacing,"Top field first"); break;
			case LAV_INTER_BOTTOM_FIRST:
				sprintf(interlacing, "Bottom field first"); break;
			default:
				sprintf(interlacing, "Unknown !"); break;
		} 

		mpeg_timecode(&tc, num_frames,
				mpeg_framerate_code( mpeg_conform_framerate( el->video_fps )),
				el->video_fps );

		sprintf( timecode, "%2d:%2.2d:%2.2d:%2.2d", tc.h, tc.m, tc.s, tc.f );

		veejay_msg(VEEJAY_MSG_INFO, "\tFile %s (%s) with %ld frames (total duration %s)",
			el->video_file_list[i],
			interlacing,
			num_frames,
			timecode );
			
	}

	mpeg_timecode(&ttc, el->video_frames,
			mpeg_framerate_code( mpeg_conform_framerate( el->video_fps )),
			el->video_fps );

	sprintf( timecode, "%2d:%2.2d:%2.2d:%2.2d", ttc.h, ttc.m, ttc.s, ttc.f );


	veejay_msg(VEEJAY_MSG_INFO, "\tDuration: %s (%2d hours, %2d minutes)(%ld frames)", timecode,ttc.h,ttc.m,el->video_frames);




}

MPEG_timecode_t get_timecode(editlist *el, long num_frames)
{
	MPEG_timecode_t tc;
	veejay_memset(&tc,0,sizeof(tc));
	mpeg_timecode(&tc, num_frames,
			mpeg_framerate_code( mpeg_conform_framerate( el->video_fps )),
			el->video_fps );
	return tc;
}

int	vj_el_get_file_entry(editlist *el, long *start_pos, long *end_pos, long entry )
{
	if(entry >= el->num_video_files)
		return 0;

	int64_t	n = (int64_t) entry;
	int64_t i = 0;

	if( el->video_file_list[ n ] == NULL )
		return 0;

	*start_pos = 0;

	for( i = 0;i < n ; i ++ )
		*start_pos += el->num_frames[i];

	*end_pos = (*start_pos + el->num_frames[n] - 1);

	return 1;
}



char *vj_el_write_line_ascii( editlist *el, int *bytes_written )
{
	if(el->is_empty)
		return NULL;

	int num_files;
	int64_t oldfile, oldframe;
	int64_t index[MAX_EDIT_LIST_FILES];
	int64_t n;
	char *result = NULL;
	int64_t n1 = 0;
	int64_t j = 0;
	int64_t n2 = el->video_frames-1;
	/* get which files are actually referenced in the edit list entries */
	int est_len = 0;
   	for (j = 0; j < MAX_EDIT_LIST_FILES; j++)
		index[j] = -1;

   	for (j = n1; j <= n2; j++)
	{
		n = el->frame_list[j];
		index[N_EL_FILE(n)] = 1;
	}
   
	num_files = 0;
   	for (j = 0; j < MAX_EDIT_LIST_FILES; j++)
	{
		if (index[j] > 0 )
			index[j] = (int64_t)num_files++;
	}
	int nnf = 0;
	for ( j = 0; j < MAX_EDIT_LIST_FILES ; j ++ )
		if(index[j] >= 0 && el->video_file_list[j] != NULL)
		{
			nnf ++;
		}
	n = el->frame_list[n1];
	oldfile = index[ N_EL_FILE(n) ];
   	oldframe = N_EL_FRAME(n);
 
	
	est_len = nnf * 1024;
	result = (char*) vj_calloc(sizeof(char) * est_len );
	sprintf(result, "%04d",nnf );

	for (j = 0; j < MAX_EDIT_LIST_FILES; j++)
	{
		if (index[j] >= 0 && el->video_file_list[j] != NULL)
		{
			char filename[400];
			char fourcc[6];
			sprintf(fourcc, "%s", "????");
			vj_el_get_file_fourcc( el, j, fourcc );
			sprintf(filename ,"%03d%s%04d%010ld%02d%s",
				strlen( el->video_file_list[j]  ),
				el->video_file_list[j],
				(unsigned long) j,
				el->num_frames[j],
				strlen(fourcc),
				fourcc 
			);
			sprintf(fourcc, "%04d", strlen( filename ));
			veejay_strncat( result, fourcc, strlen(fourcc ));
			veejay_strncat ( result, filename, strlen(filename));
		}
	}

	char first[33];
	sprintf(first, "%016lld%016lld",oldfile, oldframe);
	veejay_strncat( result, first, strlen(first) );

  	for (j = n1+1; j <= n2; j++)
	{
		n = el->frame_list[j];
		if ( index[ N_EL_FILE(n) ] != oldfile ||
			N_EL_FRAME(n) != oldframe + 1 )	
		{
			int len = 20 + (16 * 3 ) + strlen( el->video_file_list[ index[N_EL_FILE(n)] ] );
			char *tmp = (char*) vj_calloc(sizeof(char) * len );
			sprintf( tmp, "%016lld%016lld%016lld",
				 oldframe,
				 index[N_EL_FILE(n)],
				 N_EL_FRAME(n) );
			veejay_strncat( result, tmp, strlen(tmp) );
			free(tmp);
		}
		oldfile = index[N_EL_FILE(n)];
		oldframe = N_EL_FRAME(n);
    	}

	char last_word[16];
	sprintf(last_word,"%016lld", oldframe);
	veejay_strncat( result, last_word, 16 );
	*bytes_written = strlen( result );
	el->total_frames = el->video_frames - 1;
	return result;
}

int	vj_el_write_editlist( char *name, long _n1, long _n2, editlist *el )
{
	FILE *fd;
    	int num_files;
	int64_t oldfile, oldframe;
	int64_t index[MAX_EDIT_LIST_FILES];
	int64_t n;
	int64_t n1 = (uint64_t) _n1;
	int64_t n2 = (uint64_t) _n2;
	int64_t i;

	if(!el) 
		return 0;

	if(el->is_empty)
	{
		veejay_msg(VEEJAY_MSG_ERROR, "No frames in EditList" );
		return 0;
	}

    	if (n1 < 0)
		n1 = 0;

    	if (n2 >= el->video_frames || n2 < n1)
		n2 = el->video_frames - 1;

    	fd = fopen(name, "w");

	if (!fd)
	{
		veejay_msg(VEEJAY_MSG_ERROR,"Can not open %s - no edit list written!", name);
		return 0;
    	}

	fprintf(fd, "LAV Edit List\n");
	fprintf(fd, "%s\n", el->video_norm == 'n' ? "NTSC" : "PAL");

	/* get which files are actually referenced in the edit list entries */

	for (i = 0; i < MAX_EDIT_LIST_FILES; i++)
		index[i] = -1;

	for (i = n1; i <= n2; i++)
		index[N_EL_FILE(el->frame_list[i])] = 1;

	num_files = 0;
	for (i = 0; i < MAX_EDIT_LIST_FILES; i++)
		if (index[i] > 0) index[i] = (int64_t)num_files++;

	fprintf(fd, "%d\n", num_files);
	for (i = 0; i < MAX_EDIT_LIST_FILES; i++)
		if (index[i] >= 0 && el->video_file_list[i] != NULL)
		{
			 fprintf(fd, "%s\n", el->video_file_list[i]);
		}

	n = el->frame_list[ n1 ];
	oldfile = index[N_EL_FILE(n)];
	oldframe = N_EL_FRAME(n);

    	fprintf(fd, "%lld %lld ", oldfile, oldframe);
    	for (i = n1 + 1; i <= n2; i++)
	{
		n = el->frame_list[i];
		if (index[N_EL_FILE(n)] != oldfile
		    || N_EL_FRAME(n) != oldframe + 1)
		{
		    fprintf(fd, "%lld\n", oldframe);
	    	fprintf(fd, "%lld %lld ",index[N_EL_FILE(n)], N_EL_FRAME(n));
		}
		oldfile = index[N_EL_FILE(n)];
		oldframe = N_EL_FRAME(n);
    	}
    	n = fprintf(fd, "%lld\n", oldframe);

    	/* We did not check if all our prints succeeded, so check at least the last one */
    	if (n <= 0)
	{
		veejay_msg(VEEJAY_MSG_ERROR,"Error writing edit list: ");
		return 0;
    	}
	el->total_frames = el->video_frames - 1;
    	fclose(fd);

	return 1;
}

void	vj_el_frame_cache(int n )
{
	if(n > 1  || n < 1000)
	{
		mmap_size = n;
	}
}

editlist	*vj_el_soft_clone(editlist *el)
{
	editlist *clone = (editlist*) vj_calloc(sizeof(editlist));
	if(!clone)
		return NULL;
	clone->is_empty = el->is_empty;
	clone->has_video = el->has_video;
	clone->video_width = el->video_width;
	clone->video_height = el->video_height;
	clone->video_fps = el->video_fps;
	clone->video_sar_width = el->video_sar_width;
	clone->video_sar_height = el->video_sar_height;
	clone->video_norm = el->video_norm;
	clone->has_audio = el->has_audio;
	clone->audio_rate = el->audio_rate;
	clone->audio_chans = el->audio_chans;
	clone->audio_bits = el->audio_bits;
	clone->audio_bps = el->audio_bps;
	clone->video_frames = el->video_frames;
	clone->total_frames = el->video_frames-1;
	clone->num_video_files = el->num_video_files;
	clone->max_frame_size = el->max_frame_size;
	clone->MJPG_chroma = el->MJPG_chroma;
	clone->frame_list = NULL;
	clone->last_afile = el->last_afile;
	clone->last_apos  = el->last_apos;
	clone->auto_deinter = el->auto_deinter;
	clone->pixel_format = el->pixel_format;
	clone->is_clone = 1;
	int i;
	for( i = 0; i < MAX_EDIT_LIST_FILES; i ++ )
	{
		clone->video_file_list[i] = NULL;
		clone->lav_fd[i] = NULL;
		clone->num_frames[i] = 0;
		clone->yuv_taste[i] = 0;
		if( el->lav_fd[i] && el->video_file_list[i])
		{
			clone->video_file_list[i] = strdup( el->video_file_list[i] );
			clone->lav_fd[i] = el->lav_fd[i];
			clone->num_frames[i] = el->num_frames[i];
			clone->yuv_taste[i] =el->yuv_taste[i];
		}
	}

	return clone;
}

int		vj_el_framelist_clone( editlist *src, editlist *dst)
{
	if(!src || !dst) return 0;
	if(dst->frame_list)
		return 0;
	dst->frame_list = (uint64_t*) vj_malloc(sizeof(uint64_t) * src->video_frames );
	if(!dst->frame_list)
		return 0;

	veejay_memcpy(
		dst->frame_list,
		src->frame_list,
		(sizeof(uint64_t) * src->video_frames )
	); 
	
	return 1;
}

editlist	*vj_el_clone(editlist *el)
{
	editlist *clone = (editlist*) vj_el_soft_clone(el);
	if(!clone)
		return NULL;

	if( vj_el_framelist_clone( el, clone ) )
		return clone;
	else
	{
		if(clone) vj_el_free(clone);
		veejay_msg(VEEJAY_MSG_ERROR, "Cannot clone: Memory error?!");
	}	
	
	return clone;
}
