/* veejay - Linux VeeJay
 * 	     (C) 2002-2004 Niels Elburg <nelburg@looze.net> 
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

/** \defgroup avcodec FFmpeg AVCodec
 */

#include <config.h>
#include <libel/vj-avcodec.h>
#include <libel/vj-el.h>
#include <libvjmsg/vj-common.h>
#include <libvjmem/vjmem.h>
#include <string.h>
#include <libyuv/yuvconv.h>
#include <veejay/defs.h>
#ifdef SUPPORT_READ_DV2
#define __FALLBACK_LIBDV
#include <libel/vj-dv.h>
static vj_dv_encoder *dv_encoder = NULL;
#endif
//@@ FIXME
static int out_pixel_format = FMT_420; 

#define YUV420_ONLY_CODEC(id) ( ( id == CODEC_ID_MJPEG || id == CODEC_ID_MJPEGB || id == CODEC_ID_MSMPEG4V3 || id == CODEC_ID_MPEG4 ) ? 1: 0)


static	char*	el_get_codec_name(int codec_id )
{
	char name[20];
	switch(codec_id)
	{
		case CODEC_ID_MJPEG: sprintf(name, "MJPEG"); break;
		case CODEC_ID_MPEG4: sprintf(name, "MPEG4"); break;
		case CODEC_ID_MSMPEG4V3: sprintf(name, "DIVX"); break;
		case CODEC_ID_DVVIDEO: sprintf(name, "DVVideo"); break;
		case -1 : sprintf(name, "RAW YUV"); break;
		default:
			sprintf(name, "Unknown"); break;
	}
	char *res = strdup(name);
	return res;
}

static vj_encoder *_encoders[NUM_ENCODERS];

static vj_encoder	*vj_avcodec_new_encoder( int id, void *edl, int pixel_format)
{
	vj_encoder *e = (vj_encoder*) vj_malloc(sizeof(vj_encoder));
	if(!e) return NULL;
	memset(e, 0, sizeof(vj_encoder));
   /*     if( YUV420_ONLY_CODEC(id ))
        {
                e->data[0] = (uint8_t*) vj_malloc(sizeof(uint8_t) *
                                el->video_width * el->video_height );
                e->data[1] = (uint8_t*) vj_malloc(sizeof(uint8_t) *
                                el->video_width * el->video_height /2 );
                e->data[2] = (uint8_t*) vj_malloc(sizeof(uint8_t) *
                                el->video_width * el->video_height /2);
                memset( e->data[0], 0,  el->video_width * el->video_height );
                memset( e->data[1], 0,  el->video_width * el->video_height/2 );
                memset( e->data[2], 0,          el->video_width * el->video_height/2 );
        }*/
		
	if(id != 998 && id != 999 )
	{
#ifdef __FALLBACK_LIBDV
		if(id != CODEC_ID_DVVIDEO)
		{
#endif
			e->codec = avcodec_find_encoder( id );
			if(!e->codec)
			{
			 char *descr = el_get_codec_name(id);
			 veejay_msg(VEEJAY_MSG_ERROR, "Cannot find Encoder codec %s", 	descr );
			 free(descr);
			}
#ifdef __FALLBACK_LIBDV
		}
#endif

	}

	if( id != 998 && id != 999 )
	{
#ifdef __FALLBACK_LIBDV
	  if(id != CODEC_ID_DVVIDEO )
		{
#endif
		e->context = avcodec_alloc_context();
		e->context->bit_rate = 2750 * 1024;
		e->context->max_b_frames =0;
		//e->context->strict_std_compliance = FF_COMPLIANCE_EXPERIMENTAL;
		e->context->width = vj_el_get_width(edl);
 		e->context->height = vj_el_get_height(edl);
#if LIBAVCODEC_BUILD > 5010
		e->context->time_base = (AVRational) { 1, vj_el_get_fps(edl) };
#else
		e->context->frame_rate = vj_el_get_fps( edl );
		e->context->frame_rate_base = 1;
#endif
		e->context->qcompress = 0.0;
		e->context->qblur = 0.0;
		e->context->flags = CODEC_FLAG_QSCALE;
		e->context->gop_size = 0;
		e->context->sub_id = 0;
		e->context->me_method = 0; // motion estimation algorithm
		e->context->workaround_bugs = FF_BUG_AUTODETECT;
		e->context->prediction_method = 0;
		e->context->dct_algo = FF_DCT_AUTO; //global_quality?
	//	e->context->pix_fmt = (pixel_format == FMT_420 ? PIX_FMT_YUV420P : PIX_FMT_YUV422P );
/*
#if LIBAVCODEC_BUILD > 5010
                if( YUV420_ONLY_CODEC(id) )
                e->context->pix_fmt = PIX_FMT_YUV420P;
                else
                        e->context->pix_fmt = (pixel_format == FMT_420 ? PIX_FMT_YUVJ420P : PIX_FMT_YU
VJ422P );
                //@@ mjpeg encoder requires 4:2:0 planar ? wow
                //(pixel_format == FMT_420 ? 
                //              PIX_FMT_YUVJ420P : PIX_FMT_YUVJ422P );
#else
                e->context->pix_fmt = (pixel_format == FMT_420 ? PIX_FMT_YUV420P : PIX_FMT_YUV422P );
#endif
                if ( avcodec_open( e->context, e->codec ) < 0 )
                {
                        char *descr = el_get_codec_name( id );
                        veejay_msg(VEEJAY_MSG_DEBUG, "Cannot open codec '%s'" , descr );
                        if(e) free(e);
                        if(descr) free(descr);
                        return NULL;
                }*/

		if ( avcodec_open( e->context, e->codec ) < 0 )
		{
			if(e) free(e);
			return NULL;
		}

#ifdef __FALLBACK_LIBDV
	}
#endif
	}

	e->len = vj_el_get_width(edl) * vj_el_get_height(edl);
	if(pixel_format == PIX_FMT_YUV422P)
		e->uv_len = e->len / 2;
	else
		e->uv_len = e->len / 4;
	e->width = vj_el_get_width(edl);
	e->height = vj_el_get_height(edl);

	e->out_fmt = pixel_format;
	e->encoder_id = id;

/*
	if( el->has_audio )
	{
		e->audiocodec = avcodec_find_encoder( CODEC_ID_PCM_U8 );
		if(!e->audiocodec)
		{
			veejay_msg(VEEJAY_MSG_ERROR, "Error initializing audio codec");
			if(e) free(e);
		}
		e->context->sample_rate = el->audio_rate;
		e->context->channels	= el->audio_chans;
		if( avcodec_open( e->context, e->audiocodec ) < 0)
		{
			veejay_msg(VEEJAY_MSG_ERROR, "Cannot open audio context");
			if(e) free(e);
			return NULL;
		}

	}
*/
	return e;
}

static	void		vj_avcodec_close_encoder( vj_encoder *av )
{
	if(av)
	{
		if(av->context)
		{
			avcodec_close( av->context );
			free(av->context);	
		}
		free(av);
	}
	av = NULL;
}


int		vj_avcodec_init(void *el)
{
	int fmt;
//	fmt = out_pixel_format = vj_el_get_fmt(el);
	fmt = 1;
	veejay_msg(0, "Warning: Encoder in fmt 1");
	_encoders[ENCODER_MJPEG] = vj_avcodec_new_encoder( CODEC_ID_MJPEG, el, fmt );
	if(!_encoders[ENCODER_MJPEG]) return 0;

#ifdef __FALLBACK_LIBDV
	dv_encoder = vj_dv_init_encoder( (void*)el , out_pixel_format);
	if(!dv_encoder)
	{
		veejay_msg(VEEJAY_MSG_ERROR, "Unable to initialize quasar DV codec");
		return 0;
	}
#else
	_encoders[ENCODER_DVVIDEO] = vj_avcodec_new_encoder( CODEC_ID_DVVIDEO, el, fmt );
	if(!_encoders[ENCODER_DVVIDEO]) return 0;
#endif

	_encoders[ENCODER_DIVX] = vj_avcodec_new_encoder( CODEC_ID_MSMPEG4V3 , el, fmt);
	if(!_encoders[ENCODER_DIVX]) return 0;

	_encoders[ENCODER_MPEG4] = vj_avcodec_new_encoder( CODEC_ID_MPEG4, el, fmt);
	if(!_encoders[ENCODER_MPEG4]) return 0;

	_encoders[ENCODER_YUV420] = vj_avcodec_new_encoder( 999, el, fmt);
	if(!_encoders[ENCODER_YUV420]) return 0;

	_encoders[ENCODER_YUV422] = vj_avcodec_new_encoder( 998, el, fmt);
	if(!_encoders[ENCODER_YUV422]) return 0;


	return 1;
}

int		vj_avcodec_free()
{
	int i;
	for( i = 0; i < NUM_ENCODERS; i++)
	{
		if(_encoders[i]) vj_avcodec_close_encoder(_encoders[i]);
	}
#ifdef __FALLBACK_LIBDV
	vj_dv_free_encoder(dv_encoder);
#endif
	return 1;
}
void	yuv422p_to_yuv420p3( uint8_t *src, uint8_t *dst[3], int w, int h)
{
	AVPicture pict1,pict2;
	memset(&pict1,0,sizeof(pict1));
	memset(&pict2,0,sizeof(pict2));

	pict1.data[0] = src;
	pict1.data[1] = src+(w*h);
	pict1.data[2] = src+(w*h)+((w*h)/2);
	pict1.linesize[0] = w;
	pict1.linesize[1] = w >> 1;
	pict1.linesize[2] = w >> 1;
	pict2.data[0] = dst[0];
	pict2.data[1] = dst[1];
	pict2.data[2] = dst[2];
	pict2.linesize[0] = w;
	pict2.linesize[1] = w >> 1;
	pict2.linesize[2] = w >> 1;	

	img_convert( &pict2, PIX_FMT_YUV420P, &pict1, PIX_FMT_YUV422P, w, h );
	return;
}
void	yuv422p_to_yuv420p2( uint8_t *src[3], uint8_t *dst[3], int w, int h)
{
	AVPicture pict1,pict2;
	memset(&pict1,0,sizeof(pict1));
	memset(&pict2,0,sizeof(pict2));

	pict1.data[0] = src[0];
	pict1.data[1] = src[1];
	pict1.data[2] = src[2];
	pict1.linesize[0] = w;
	pict1.linesize[1] = w >> 1;
	pict1.linesize[2] = w >> 1;
	pict2.data[0] = dst[0];
	pict2.data[1] = dst[1];
	pict2.data[2] = dst[2];
	pict2.linesize[0] = w;
	pict2.linesize[1] = w >> 1;
	pict2.linesize[2] = w >> 1;	

	img_convert( &pict2, PIX_FMT_YUV420P, &pict1, PIX_FMT_YUV422P, w, h );
	return;
}

int	yuv422p_to_yuv420p( uint8_t *src[3], uint8_t *dst, int w, int h)
{

	int len = w* h ;
	int uv_len = len / 4;
	AVPicture pict1,pict2;
	memset(&pict1,0,sizeof(pict1));
	memset(&pict2,0,sizeof(pict2));

	pict1.data[0] = src[0];
	pict1.data[1] = src[1];
	pict1.data[2] = src[2];
	pict1.linesize[0] = w;
	pict1.linesize[1] = w >> 1;
	pict1.linesize[2] = w >> 1;
	pict2.data[0] = dst;
	pict2.data[1] = dst + len;
	pict2.data[2] = dst + len + uv_len;
	pict2.linesize[0] = w;
	pict2.linesize[1] = w >> 1;
	pict2.linesize[2] = w >> 1;	

	img_convert( &pict2, PIX_FMT_YUV420P, &pict1, PIX_FMT_YUV422P, w, h );
	return (len + uv_len + uv_len);

}
int	yuv420p_to_yuv422p2( uint8_t *sY,uint8_t *sCb, uint8_t *sCr, uint8_t *dst[3], int w, int h )
{
	const int len = w * h;
	const int uv_len = len / 2; 
	AVPicture pict1,pict2;
	memset(&pict1,0,sizeof(pict1));
	memset(&pict2,0,sizeof(pict2));
	pict1.data[0] = sY;
	pict1.data[1] = sCb;
	pict1.data[2] = sCr;
	pict1.linesize[0] = w;
	pict1.linesize[1] = w >> 1;
	pict1.linesize[2] = w >> 1;
	pict2.data[0] = dst[0];
	pict2.data[1] = dst[1];
	pict2.data[2] = dst[2];
	pict2.linesize[0] = w;
	pict2.linesize[1] = w >> 1;
	pict2.linesize[2] = w >> 1;	

	img_convert( &pict2, PIX_FMT_YUV422P, &pict1, PIX_FMT_YUV420P, w, h );
	return (len + uv_len + uv_len);

}
int	yuv420p_to_yuv422p( uint8_t *sY,uint8_t *sCb, uint8_t *sCr, uint8_t *dst[3], int w, int h )
{
	const int len = w * h;
	const int uv_len = len / 2; 
	AVPicture pict1,pict2;
	memset(&pict1,0,sizeof(pict1));
	memset(&pict2,0,sizeof(pict2));
	pict1.data[0] = sY;
	pict1.data[1] = sCb;
	pict1.data[2] = sCr;
	pict1.linesize[0] = w;
	pict1.linesize[1] = w >> 1;
	pict1.linesize[2] = w >> 1;
	pict2.data[0] = dst[0];
	pict2.data[1] = dst[1];
	pict2.data[2] = dst[2];
	pict2.linesize[0] = w;
	pict2.linesize[1] = w >> 1;
	pict2.linesize[2] = w >> 1;	
	img_convert( &pict2, PIX_FMT_YUV422P, &pict1, PIX_FMT_YUV420P, w, h );
	return (len + uv_len + uv_len);
}

static	int	vj_avcodec_copy_frame( vj_encoder  *av, uint8_t *src[3], uint8_t *dst )
{
	if(!av)
	{
		veejay_msg(VEEJAY_MSG_ERROR, "No encoder !!");
		return 0;
	}
	if( (av->encoder_id == 999 && av->out_fmt == PIX_FMT_YUV420P) || (av->encoder_id == 998 && av->out_fmt == PIX_FMT_YUV422P))
	{
		/* copy */
		veejay_memcpy( dst, src[0], av->len );
		veejay_memcpy( dst+(av->len), src[1], av->uv_len );
		veejay_memcpy( dst+(av->len+av->uv_len) , src[2], av->uv_len);
		return ( av->len + av->uv_len + av->uv_len );
	}
	/* copy by converting */
	if( av->encoder_id == 999 && av->out_fmt == PIX_FMT_YUV422P) 
	{
		yuv422p_to_yuv420p( src, dst, av->width, av->height);
		return ( av->len + (av->len/4) + (av->len/4));
	}

	if( av->encoder_id == 998 && av->out_fmt == PIX_FMT_YUV420P)
	{
		uint8_t *d[3];
		d[0] = dst;
		d[1] = dst + av->len;
		d[2] = dst + av->len + (av->len / 2);
		yuv420p_to_yuv422p2( src[0],src[1],src[2], d, av->width,av->height );
		return ( av->len + av->len );
	}


/*	if(av->sub_sample)
	{
		return(yuv422p_to_yuv420p(src,dst, av->width, av->height ));
	}
	else
	{
		veejay_memcpy( dst, src[0], av->len );
		veejay_memcpy( dst+(av->len), src[1], av->uv_len );
		veejay_memcpy( dst+(av->len+av->uv_len) , src[2], av->uv_len);
	}
	return (av->len + av->uv_len + av->uv_len);
*/

	
	return 0;
}



int		vj_avcodec_encode_frame( int format, uint8_t *src[3], uint8_t *buf, int buf_len)
{
	AVFrame pict;
	vj_encoder *av = _encoders[format];
	int res=0;
	memset( &pict, 0, sizeof(pict));

	if(format == ENCODER_YUV420) // no compression, just copy
		return vj_avcodec_copy_frame( _encoders[ENCODER_YUV420],src, buf );
	if(format == ENCODER_YUV422) // no compression, just copy
		return vj_avcodec_copy_frame( _encoders[ENCODER_YUV422],src, buf );


#ifdef __FALLBACK_LIBDV
	if(format == ENCODER_DVVIDEO )
		return vj_dv_encode_frame( dv_encoder,src, buf );
#endif

	pict.quality = 1;
	pict.data[0] = src[0];
	pict.data[1] = src[1];
	pict.data[2] = src[2];

	if( out_pixel_format == FMT_422 )
	{
		pict.linesize[0] = av->context->width;
		pict.linesize[1] = av->context->width;
		pict.linesize[2] = av->context->width;
	}
	else
	{
		pict.linesize[0] = av->context->width;
		pict.linesize[1] = av->context->width >> 1;
		pict.linesize[2] = av->context->width >> 1;
	}
	res = avcodec_encode_video( av->context, buf, buf_len, &pict );

	return res;
}
/*
static	int	vj_avcodec_copy_audio_frame( uint8_t *src, uint8_t *buf, int len)
{
	veejay_memcpy( buf, src, len );
	return len;
}

int		vj_avcodec_encode_audio( int format, uint8_t *src, uint8_t *dst, int len, int nsamples )
{
	if(format == ENCODER_YUV420)
		return vj_avcodec_copy_audio_frame;
	if(format == ENCODER_YUV422)
		return vj_avcodec_copy_audio_frame;
	vj_encoder *av = _encoders[format];

	int len = avcodec_encode_audio( av->context, src, len, nsamples );
	return len;
}
*/