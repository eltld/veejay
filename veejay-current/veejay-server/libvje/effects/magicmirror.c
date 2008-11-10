/* 
 * Linux VeeJay
 *
 * Copyright(C)2004 Niels Elburg <elburg@hio.hen.nl>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License , or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307 , USA.
 */
#include <config.h>
#include "magicmirror.h"
#include <stdlib.h>
#include <math.h>
#include "common.h"
// if d or n changes, tables need to be calculated
static uint8_t *magicmirrorbuf[3];
static double *funhouse_x = NULL;
static double *funhouse_y = NULL;
static unsigned int *cache_x = NULL;
static unsigned int *cache_y = NULL;
static unsigned int last[2] = {0,0};
//static uint8_t *p0_frame_ = NULL;

vj_effect *magicmirror_init(int w, int h)
{
    vj_effect *ve = (vj_effect *) vj_calloc(sizeof(vj_effect));
    ve->num_params = 4;
    ve->defaults = (int *) vj_calloc(sizeof(int) * ve->num_params);	/* default values */
    ve->limits[0] = (int *) vj_calloc(sizeof(int) * ve->num_params);	/* min */
    ve->limits[1] = (int *) vj_calloc(sizeof(int) * ve->num_params);	/* max */

    ve->defaults[0] = w/4;
    ve->defaults[1] = h/4;
    ve->defaults[2] = 20;
    ve->defaults[3] = 20;

    ve->limits[0][0] = 0;
    ve->limits[1][0] = w/2;
    ve->limits[0][1] = 0;
    ve->limits[1][1] = h/2;
    ve->limits[0][2] = 0;
    ve->limits[1][2] = 100;
    ve->limits[0][3] = 0;
    ve->limits[1][3] = 100;

    ve->sub_format = 1;
    ve->description = "Magic Mirror Surface";
	ve->has_user =0;
    ve->extra_frame = 0;
	ve->param_description = vje_build_param_list(ve->num_params, "X", "Y", "X","Y" );
    return ve;
}
// FIXME private
#define    RUP8(num)(((num)+8)&~8)

static int n__ = 0;
static int N__ = 0;
int magicmirror_malloc(int w, int h)
{
	magicmirrorbuf[0] = (uint8_t*)vj_yuvalloc(w,h);
	if(!magicmirrorbuf[0]) return 0;
	magicmirrorbuf[1] = magicmirrorbuf[0] + (w*h);
	magicmirrorbuf[2] = magicmirrorbuf[1] + (w*h);
	
	funhouse_x = (double*)vj_calloc(sizeof(double) * w );
	if(!funhouse_x) return 0;

	cache_x = (unsigned int *)vj_calloc(sizeof(unsigned int)*w);
	if(!cache_x) return 0;

	funhouse_y = (double*)vj_calloc(sizeof(double) * h );
	if(!funhouse_y) return 0;

	cache_y = (unsigned int*)vj_calloc(sizeof(unsigned int)*h);
	if(!cache_y) return 0;
	veejay_memset(cache_x,0,w);
	veejay_memset(cache_y,0,h);

	n__ =0;
	N__ =0;
	//p0_frame_ = (uint8_t*) vj_malloc( sizeof(uint8_t) * RUP8( w  * h * 3) );

	return 1;
}

void magicmirror_free()
{
	if(magicmirrorbuf[0]) free(magicmirrorbuf[0]);
	if(funhouse_x) free(funhouse_x);
	if(funhouse_y) free(funhouse_y);
	if(cache_x) free(cache_x);
	if(cache_y) free(cache_y);
//	if(p0_frame_) free(p0_frame_);	
//	p0_frame_=0;
	magicmirrorbuf[0] = NULL;
	magicmirrorbuf[1] = NULL;
	magicmirrorbuf[2] = NULL;
	cache_x = NULL;
	cache_y  = NULL;
	funhouse_x = NULL;
	funhouse_y = NULL;
}

void magicmirror_apply( VJFrame *frame, int w, int h, int vx, int vy, int d, int n )
{
	double c1 = (double)vx;
	double c2 = (double)vy;
	int motion = 0;
	if( motionmap_active())
	{
		motionmap_scale_to( 100,100,0,0, &d, &n, &n__, &N__ );
		motion = 1;
	}
	else
	{
		n__ = 0;
		N__ = 0;
	}

	double c3 = (double)d * 0.001;
	unsigned int dx,dy,x,y,p,q,len=w*h;
	double c4 = (double)n * 0.001;
	int changed = 0;
  	uint8_t *Y = frame->data[0];
	uint8_t *Cb= frame->data[1];
	uint8_t *Cr= frame->data[2];
	int interpolate = 1;

	if( n__ == N__ || n__ == 0)
		interpolate = 0;

	if( d != last[1] )
	{
		changed = 1; last[1] =d;
	}
	if( n != last[0] )
	{
		changed = 1; last[0] = n;
	}

	if(changed==1)
	{	// degrees x or y changed, need new sin
		for(x=0; x < w ; x++)
		{
			double res;
			fast_sin(res,(double)(c3*x));
			funhouse_x[x] = res;
			//funhouse_x[x] = sin(c3 * x);  
		}
		for(y=0; y < h; y++)
		{
			double res;
			fast_sin(res,(double)(c4*y));
			funhouse_y[y] = res;
			//funhouse_y[y] = sin(c4 * y);
		}
	}

	veejay_memcpy( magicmirrorbuf[0], Y, len );
	veejay_memcpy( magicmirrorbuf[1], Cb, len );
	veejay_memcpy( magicmirrorbuf[2], Cr, len );


	for(x=0; x < w; x++)
	{
		dx = x + funhouse_x[x] * c1;
		if(dx < 0) dx += w;
		if(dx < 0) dx = 0; else if (dx >= w) dx = w-1;
		cache_x[x] = dx;
	}
	for(y=0; y < h; y++)
	{
		dy = y + funhouse_y[y] * c2;
		if(dy < 0) dy += h;
		if(dy < 0) dy = 0; else if (dy >= h) dy = h-1;
		cache_y[y] = dy;
	}

	for(y=1; y < h-1; y++)
	{
		for(x=1; x < w-1; x++)
		{
			p = cache_y[y] * w + cache_x[x];
			q = y * w + x;
			Y[q] = magicmirrorbuf[0][p];
			Cb[q] = magicmirrorbuf[1][p];
			Cr[q] = magicmirrorbuf[2][p];
		}
	}
/*
	uint8_t *p0[3] = { 
		p0_frame_ + 0,
		p0_frame_ + (w*h),
		p0_frame_ + (2*w*h) };

	if(interpolate)
	{
		VJFrame prev;
		veejay_memcpy(&prev, frame, sizeof(VJFrame));
		prev->data[0] = p0_frame_;
		prev->data[1] = p0_frame_ + (w*h);
		prev->data[2] = p0_frame_ + (2*w*h);

		motionmap_lerp_frame( frame, &prev, N__, n__ );
	}

	if( motionmap_active())
	{
		veejay_memcpy( prev->data[0], Y, w*h);
		veejay_memcpy( prev->data[1], Cb, w*h);
		veejay_memcpY( prev->data[2], Cr,w*h);
	}*/

	if( interpolate )
	{
		motionmap_interpolate_frame( frame, N__, n__ );
	}

	if( motion )
	{
		motionmap_store_frame(frame);
	}

}
