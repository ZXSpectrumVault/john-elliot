/*************************************************************************
**	 Copyright 2005, John Elliott                                   **
**       Copyright 1999, Caldera Thin Clients, Inc.                     ** 
**       This software is licenced under the GNU Public License.        **
**       Please see LICENSE.TXT for further information.                ** 
**                                                                      ** 
**                  Historical Copyright                                ** 
**                                                                      **
**                                                                      **
**                                                                      **
**  Copyright (c) 1987, Digital Research, Inc. All Rights Reserved.     **
**  The Software Code contained in this listing is proprietary to       **
**  Digital Research, Inc., Monterey, California and is covered by U.S. **
**  and other copyright protection.  Unauthorized copying, adaptation,  **
**  distribution, use or display is prohibited and may be subject to    **
**  civil and criminal penalties.  Disclosure to others is prohibited.  **
**  For the terms and conditions of software code use refer to the      **
**  appropriate Digital Research License Agreement.                     **
**                                                                      **
*************************************************************************/

#include "sdsdl.hxx"
/*
static Uint32 word_mask_table[] =
{
	0xFFFFFFFFL, 0x7FFFFFFFL, 0x3FFFFFFFL, 0x1FFFFFFFL, 
        0x0FFFFFFFL, 0x07FFFFFFL, 0x03FFFFFFL, 0x01FFFFFFL, 
        0x00FFFFFFL, 0x007FFFFFL, 0x003FFFFFL, 0x001FFFFFL, 
        0x000FFFFFL, 0x0007FFFFL, 0x0003FFFFL, 0x0001FFFFL, 
        0x0000FFFFL, 0x00007FFFL, 0x00003FFFL, 0x00001FFFL, 
        0x00000FFFL, 0x000007FFL, 0x000003FFL, 0x000001FFL, 
        0x000000FFL, 0x0000007FL, 0x0000003FL, 0x0000001FL, 
        0x0000000FL, 0x00000007L, 0x00000003L, 0x00000001L, 
};*/

int SdSdl::lock_surface()
{
        if ( SDL_MUSTLOCK(m_globals->m_surface) )
        {
		if ( SDL_LockSurface(m_globals->m_surface) < 0 )
                {
			fprintf(stderr, "SDL: failed to lock surface(%s)\n", 
			                    SDL_GetError());
			return -1;
		}
	}
	m_globals->m_sdl_locked++;
	return 0;
}

void SdSdl::unlock_surface()
{
	if (m_globals->m_sdl_locked) --m_globals->m_sdl_locked;
	if (SDL_MUSTLOCK(m_globals->m_surface))
		SDL_UnlockSurface(m_globals->m_surface);
}

void SdSdl::update_rect(Sint32 xmin, Sint32 ymin, Sint32 xmax,
			Sint32 ymax)
{
	if (ymin > ymax || xmin > xmax || ymin < 0 || xmin < 0) return;
	if (m_CLIP)
	{
                if (xmin < m_XMN_CLIP)
                {
                        if (xmax < m_XMN_CLIP) return;
                        else xmin = m_XMN_CLIP;
                }
                if (xmax > m_XMX_CLIP)
                {
                        if (xmin > m_XMX_CLIP) return;
                        else xmax = m_XMX_CLIP;
                }
                if (ymin < m_YMN_CLIP)
                {
                        if (ymax < m_YMN_CLIP) return;
                        else ymin = m_YMN_CLIP;
                }
                if (ymax > m_YMX_CLIP)
                {
                        if (ymin > m_YMX_CLIP) return;
                        else ymax = m_YMX_CLIP;
                }
	}
	if (xmax >= m_globals->m_requestW) xmax = m_globals->m_requestW - 1;
	if (ymax >= m_globals->m_requestH) ymax = m_globals->m_requestH - 1;
	if (xmin < 0) xmin = 0;
	if (ymin < 0) ymin = 0;

/*	static int recurs = 0;
	if (!recurs)
	{
		recurs++;
		GPoint lpt[5];
		m_LN_MASK = 0x55aa55aa;
		lpt[0].x = lpt[3].x = lpt[4].x = xmin;
		lpt[1].x = lpt[2].x = xmax - 1;
		lpt[0].y = lpt[1].y = lpt[4].y = ymin;
		lpt[2].y = lpt[3].y = ymax - 1;
		pline(5, lpt);
		recurs--;
	} */
//	printf("UpdateRect %d %d %d %d\n",
//			xmin, ymin, xmax-xmin+1, ymax-ymin+1);
	SDL_UpdateRect(m_globals->m_surface, xmin, ymin, xmax-xmin+1,
							   ymax-ymin+1);
//	getchar();
}
	


void SdSdl::sdl_plot(Uint8 *bits, Uint32 pixel)
{
	Uint8 r,g,b;
	SDL_Surface *s = m_globals->m_surface;

/* Simple clip that stops pixels off the top of the screen from being drawn */
	if (bits < (Uint8 *)s->pixels)
	{
		fprintf(stderr, "OUT-OF-RANGE: plot %p\n", bits);
		fflush(stderr); 
		return;
	}
/* XXX XXX XXX Clip pixels that are offscreen. Speed killer! */
	{
		long offset = bits - ((Uint8 *)s->pixels);
		long y      = (offset / m_sdl_pitch);

		offset  -= (y * m_sdl_pitch);	
		offset /= m_sdl_bpp;
		if (y < 0 || y >= s->h || offset < 0 || offset > s->w) 
		{
			fprintf(stderr, "OUT-OF-RANGE: plot %ld, %ld  \n", offset, y);
			fflush(stderr); return;
		}
//		fprintf(stderr, "plot %ld, %ld, %x\n", offset, y, pixel);
	}
	switch(m_sdl_bpp)
	{
		case 1: *((Uint8  *)(bits)) = (Uint8)pixel; break;
		case 2: *((Uint16 *)(bits)) = (Uint16)pixel; break;
		case 3:  r = (pixel >> s ->format -> Rshift)&0xFF;
                         g = (pixel >> s ->format -> Gshift)&0xFF;
                         b = (pixel >> s ->format -> Bshift)&0xFF;
                         *((bits) + s -> format -> Rshift/8) = r;
                         *((bits) + s -> format -> Gshift/8) = g;
                         *((bits) + s -> format -> Bshift/8) = b;
			break;
		case 4: *((Uint32 *)(bits)) = (Uint32)pixel; break;

	}
}


void SdSdl::sdl_xor(Uint8 *bits, Uint32 pixel)
{
        Uint8 r,g,b;
        SDL_Surface *s = m_globals->m_surface;

        switch(m_sdl_bpp)
        {
                case 1: *((Uint8  *)(bits)) ^= (Uint8)pixel; break;
                case 2: *((Uint16 *)(bits)) ^= (Uint16)pixel; break;
                case 3:  r = (pixel >> s ->format -> Rshift)&0xFF;
                         g = (pixel >> s ->format -> Gshift)&0xFF;
                         b = (pixel >> s ->format -> Bshift)&0xFF;
                         *((bits) + s -> format -> Rshift/8) ^= r;
                         *((bits) + s -> format -> Gshift/8) ^= g;
                         *((bits) + s -> format -> Bshift/8) ^= b;
                        break;
                case 4: *((Uint32 *)(bits)) ^= (Uint32)pixel; break;

        }
}




typedef void (*BOXFUNC)(SdSdl *self, Uint8 *bits);

void box_replace(SdSdl *self, Uint8 *bits)
{
	if (self->m_BOX_MODE) self->sdl_plot(bits, self->m_FG_BP_1);
	else		      self->sdl_plot(bits, self->m_bg_pixel);
}

void box_tran(SdSdl *self, Uint8 *bits)
{
        if (self->m_BOX_MODE) self->sdl_plot(bits, self->m_FG_BP_1);
}


void box_xor(SdSdl *self, Uint8 *bits)
{
	if (self->m_BOX_MODE) self->sdl_xor(bits, self->m_FG_BP_1);
}

void box_invtran(SdSdl *self, Uint8 *bits)
{
        if (self->m_BOX_MODE) self->sdl_plot(bits, self->m_bg_pixel);
}


static BOXFUNC boxdraw[4]={ box_replace, box_tran, box_xor, box_invtran };


void SdSdl::BOX_FILL_LINE(long Y, Uint8 **b)
{
	int X;
	Uint32 mask;
	Uint32 pattern;
	Uint32 pixel;
	SDL_Rect rc;
	
	Uint8 *bits = (*b);

	mask = 0x80000000L >> (m_X1 & 0x1F);

/* Optimise: If plotting solid colour, use SDL_FillRect */
	pattern = m_patptr[ (Y & m_patmsk) ];
	rc.x = m_X1;
	rc.y = Y;
	rc.w = m_X2 - m_X1 + 1;
	rc.h = 1;
// All zeroes
	if (pattern == 0) switch(m_WRT_MODE)
	{
		case 0: pixel = m_bg_pixel; 
			SDL_FillRect(m_globals->m_surface, &rc, pixel);
			(*b) += m_sdl_pitch;
			return;	
		default: 
			(*b) += m_sdl_pitch;	/* Next pixel line */
			return;
	}
// All ones
	if (pattern == 0xFFFFFFFF) switch(m_WRT_MODE)
	{
		case 1:
		case 0: pixel = m_FG_BP_1; 
			SDL_FillRect(m_globals->m_surface, &rc, pixel);
			(*b) += m_sdl_pitch;
			return;	
		case 3: pixel = m_bg_pixel; 
			SDL_FillRect(m_globals->m_surface, &rc, pixel);
			(*b) += m_sdl_pitch;
			return;	
	}
	
	for (X = m_X1; X <= m_X2; X++)
	{
		m_BOX_MODE = (mask & pattern);
	
		(*boxdraw[m_WRT_MODE]) (this, bits);
		mask = mask >> 1;
		if (!mask) mask = 0x80000000L;
		bits += m_sdl_bpp;
	}
	(*b) += m_sdl_pitch;	/* Next pixel line */
}







#include <assert.h>

Uint8 *SdSdl::concat(Sint32 x, Sint32 y)
{
        SDL_Surface *surface = m_globals->m_surface;
	Uint8 *bits;
/*
	assert(x >= 0);
	assert(y >= 0);	
	assert(y <= m_globals->m_yres);
	assert(x <= m_globals->m_xres);
*/
	bits = ((Uint8 *)surface->pixels)+
                        ((y) * m_sdl_pitch) + (x * m_sdl_bpp);

        {
                long offset = bits - ((Uint8 *)surface->pixels);
                long y2     = (offset / m_sdl_pitch);

                offset  -= (y2 * surface->pitch);
                offset /= m_sdl_bpp;

//		fprintf(stderr, "concat (%u,%u) -> (%u, %u)\n",
//				x, y, offset, y2);
        }
	return bits;
}



/******************************************************************************
* xline
*       Draw a horizontal line with pattern
*
*       Entry   X1,Y1 = left edge  inclusive
*               X2,Y1 = right edge inclusive
*               WRT_MODE = writing mode ( 0 - 3 )
*
*******************************************************************************/

long SdSdl::xline_noswap(Uint8 **bits)
{
	Uint32 patmsk   = m_patmsk;
	Uint32 *patptr  = m_patptr;
	Uint32 NEXT_PAT = m_NEXT_PAT;
	Sint32 X2       = m_X2;

	m_patptr = &(m_LN_MASK);
	m_patmsk = 0;
	m_NEXT_PAT = 0;
	if (m_WRT_MODE == 2 && m_LSTLIN == 0 && m_X1 != m_X2)
	{
		--m_X2;
	}
	BOX_FILL_LINE(m_Y1, bits);
	m_X2       = X2;
	m_NEXT_PAT = NEXT_PAT;
	m_patptr   = patptr;
	m_patmsk   = patmsk;
	return m_LN_MASK;
}

long SdSdl::xline_swap(Uint8 **bits)
{
	Uint32 patmsk   = m_patmsk;
	Uint32 *patptr  = m_patptr;
	Sint32 NEXT_PAT = m_NEXT_PAT;
	Sint32 X1, X2;

	/* Swap and save X1, X2 */
	X2 = m_X1; 
	X1 = m_X1 = m_X2; 
	m_X2 = X2;
	
	m_patptr = &(m_LN_MASK);
	m_patmsk = 0;
	m_NEXT_PAT = 0;
	if (m_WRT_MODE == 2 && m_LSTLIN == 0 && m_X1 != m_X2)
	{
		++m_X1;
		*bits += m_sdl_bpp;
	}
	BOX_FILL_LINE(m_Y1, bits);
	m_X2       = X2;
	m_NEXT_PAT = NEXT_PAT;
	m_patptr   = patptr;
	m_patmsk   = patmsk;
	return m_LN_MASK;
}

/************************************************************************
*TENNIS                                                                 *
*       Entry   dx - delta count for ABLINE (count includes last point) *
*       Exit    dx is decremented by one if:                            *
*                       XOR writing mode and the line about to be       *
*                       drawn is not the last of the poly line          *
*                       else dx is left alone                           *
*       Purpose:  Xor does not Xor itself at intersection points of     *
*                 polline or plydpl of fill                             *
*************************************************************************/

long SdSdl::tennis(long dx)
{
	if (m_WRT_MODE != 2) return dx;
	if (m_LSTLIN)        return dx;
	if (dx == 1) return dx;
	return dx - 1; 
}


/* [JCE] Rewritten to use an implementation that doesn't bend my mind... */

long SdSdl::do_line(void)
{	
	Uint8 *bits;
	Sint32 dx, dy, n, s, maxdx, mindx, y, x;
	Sint32 xstep, ystep, dxstep, dystep, pxstep, pystep;

	dx = m_X2 - m_X1;
	dy = m_Y2 - m_Y1;
	bits = concat(m_X1, m_Y1);
	y = m_Y1;
	x = m_X1;
	if (dx >= 0)
	{
		xstep = 1;

		if (!dy) return xline_noswap(&bits); 
	}
	else
	{
		if (!dy) 
		{
	                bits = concat(m_X2, m_Y2);
			return xline_swap(&bits);
		}
		xstep = -1;
		dx = - dx;
	}
	ystep = 1;
	if (dy < 0) { ystep = -1; dy = -dy; }

	// dx = absolute horizontal distance. 

	if (dx >= dy)
	{
		dystep = 0;
		dxstep = xstep;
		s = dx / 2;
		maxdx = tennis(1 + dx);
		mindx = dy;
	}
	else
	{
		dystep = ystep;
		dxstep = 0;
		s = dy / 2;
		maxdx = tennis(1 + dy);
		mindx = dx;	
	}	
	for (n = 0; n < maxdx; n++)
	{
		m_BOX_MODE = (m_LN_MASK & m_LN_RMASK);

/* Clip this within y & x bounds */
		if (y >= 0 && y <= yres && x >= 0 && x <= xres)
		{
             		(*boxdraw[m_WRT_MODE]) (this, bits);	
		}
		s += mindx;
		if (s >= maxdx) /* diagonal step */
		{
			s -= maxdx;
			pxstep = xstep;
			pystep = ystep;
		}
		else		/* horizontal step */
		{
			pxstep = dxstep;
			pystep = dystep;
		}
		bits += pxstep*m_sdl_bpp;
		bits += pystep*m_sdl_pitch;
		y += pystep;
		x += pxstep;

		m_LN_RMASK = m_LN_RMASK >> 1;
		if (!m_LN_RMASK) m_LN_RMASK = 0x80000000L;
	}
	return m_LN_RMASK;
}


/****************************************************************
*Subroutine     abline                                          *
*    Entry:     X1-coordinate                                   *
*               Y1-coordinate                                   *
*               X2-coordinate                                   *
*               Y2-coordinate                                   *
*    Purpose:                                                   *
*               This routine will draw a line from (x1,y1) to   *
*               (x2,y2) using Bresenham's algorithm.            *
*                                                               *
*                                                               *
*    Variables: for Bresenham's algorithm defined for           *
*               delta y > delta x after label "ckslope".        *
*               delta y <= delta x                              *
*****************************************************************/

void SdSdl::ABLINE()
{
	m_LN_RMASK = do_line();
}


/******************************************************************************
* BOX_FILL
*       Fill a rectangular area with pattern
*
*       Entry   X1,Y1 = upper left corner  inclusive
*               X2,Y2 = lower right corner inclusive
*
******************************************************************************/

void SdSdl::BOX_FILL()
{
	int Y;

	Uint8 *bits = concat(m_X1, m_Y1);

	for (Y = m_Y1; Y <= m_Y2; Y++)
	{
		BOX_FILL_LINE(Y, &bits);
	}	
}


/******************************************************************************
*
* PROCEDURE:    RECTFIL         Fill Rectangle
*
* Inputs:       X1,Y1 = upper left
*               X2,Y2 = lower right
*
******************************************************************************/

void SdSdl::RECTFILL()
{
	if (m_CLIP)
	{
		if (m_X1 < m_XMN_CLIP)
		{
			if (m_X2 < m_XMN_CLIP) return;
			else m_X1 = m_XMN_CLIP;
		}
		if (m_X2 > m_XMX_CLIP)
		{
			if (m_X1 > m_XMX_CLIP) return;
			else m_X2 = m_XMX_CLIP;
		}
                if (m_Y1 < m_YMN_CLIP)
                {
                        if (m_Y2 < m_YMN_CLIP) return;
                        else m_Y1 = m_YMN_CLIP;
                }
                if (m_Y2 > m_YMX_CLIP)
                {
                        if (m_Y1 > m_YMX_CLIP) return;
                        else m_Y2 = m_YMX_CLIP;
                }
	}
	// Always clip to screen region.
	if (m_Y1 < 0) m_Y1 = 0;
	if (m_Y1 > m_globals->m_yres) m_Y1 = m_globals->m_yres;
	if (m_X1 < 0) m_X1 = 0;
	if (m_X1 > m_globals->m_xres) m_X1 = m_globals->m_xres;
	if (m_Y2 < 0) m_Y2 = 0;
	if (m_Y2 > m_globals->m_yres) m_Y2 = m_globals->m_yres;
	if (m_X2 < 0) m_X2 = 0;
	if (m_X2 > m_globals->m_xres) m_X2 = m_globals->m_xres;
	BOX_FILL();
}



/****************************************************************
*Subroutine     HABLINE                                         *
*       Entry:  X1-coordinate                                   *
*               Y1-coordinate                                   *
*               X2-coordinate                                   *
*               patptr - pointer to fill pattern table          *
*                                                               *
*       Purpose:                                                *
*               This routine will draw a line from (X1,Y1) to   *
*               (X2,Y1) using a horizontal line algorithm.      *
*****************************************************************/

void SdSdl::HABLINE()
{
	long Y2 = m_Y2;	/* Plugh */

	m_Y2 = m_Y1;
	BOX_FILL();
	m_Y2 = Y2;
}

void SdSdl::HLINE_CLIP()
{
	if (m_CLIP)
	{
		if (m_X1 < m_XMN_CLIP)
		{
			if (m_X2 < m_XMN_CLIP) return;
			else m_X1 = m_XMN_CLIP;
		}
		if (m_X2 > m_XMX_CLIP)
		{
			if (m_X1 > m_XMX_CLIP) return;
			else m_X2 = m_XMX_CLIP;
		}
	}
	if (m_X1 < 0) m_X1  = 0;
	if (m_X1 >= xres) m_X1 = xres - 1;
	if (m_X2 < 0) m_X2 = 0;
	if (m_X2 >= xres) m_X2 = xres - 1;
	if (m_Y1 < 0 || m_Y1 >= yres) return;
	HABLINE();
}
