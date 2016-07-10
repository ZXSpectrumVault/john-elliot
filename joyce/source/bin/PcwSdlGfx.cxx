/************************************************************************

    JOYCE v2.1.0 - Amstrad PCW emulator

    Copyright (C) 1996, 2001-2  John Elliott <seasip.webmaster@gmail.com>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

*************************************************************************/

/* JOYCE for DOS was written atop the GRX graphics library. JOYCE
 * for X uses SDL (there is a GRX for X, but for various reasons
 * I'm using SDL instead). This file emulates those GRX functions
 * that JOYCE uses and that aren't in SDL.
 *
 */

#include "Pcw.hxx"
#include "PcwMenuFont.hxx"

static int clipx1, clipy1, clipx2, clipy2;

/* Graphics primitives */


void GrPlotNC(int x, int y, int colour)
{
	gl_sys->m_video->dot(x, y, colour);
}

int GrPixel(int x, int y)
{
	Uint32 pixel = gl_sys->m_video->pixelAt(x,y);

	return gl_sys->m_video->pixelToIndex(pixel);
}



void GrPlotRopNC(int x, int y, int colour, int rop)
{
	SdlContext *c = gl_sys->m_video;
	Uint32 pixel  = gl_sys->m_video->indexToPixel(colour);

	switch(rop)
	{
		case SDLT_DRAW: c->dot (x, y, pixel); break;
		case SDLT_AND:  c->dotA(x, y, pixel); break;
		case SDLT_OR:   c->dotO(x, y, pixel); break;
		case SDLT_XOR:  c->dotX(x, y, pixel); break;
	}
}




void GrFilledBoxNC(int x, int y, int x2, int y2, int colour)
{
        gl_sys->m_video->fillRect(x, y, x2-x+1, y2-y+1, 
		gl_sys->m_video->indexToPixel(colour));
}



void GrClearScreen(int index)
{
	GrFilledBoxNC(0, 0, 799, 599, index);
}

void GrBoxNC(int x1, int y1, int x2, int y2, int colour)
{
	GrHLineNC(x1, x2, y1, colour);
	GrHLineNC(x1, x2, y2, colour);
	GrVLineNC(x1, y1, y2, colour);
	GrVLineNC(x2, y1, y2, colour);
}




void GrHLineNC(int x1,int x2,int y, int c)
{
        gl_sys->m_video->fillRect(x1, y, x2-x1+1, 1, 
		gl_sys->m_video->indexToPixel(c));
}




	
void GrVLineNC(int x,int y1,int y2, int c)
{
        gl_sys->m_video->fillRect(x, y1, 1, y2-y1+1, 
                gl_sys->m_video->indexToPixel(c));
}

void GrHLine(int x, int y1, int y2, int c)
{
	if (x  < clipx1)  x = clipx1;
	if (y1 < clipy1) y1 = clipy1;
	if (y2 < clipy1) y2 = clipy1;
	if (x >  clipx2)  x = clipx2;
	if (y1 > clipy2) y1 = clipy2;
	if (y2 > clipy2) y2 = clipy2;
	if (y1 > y2) { int t; t = y1; y1 = y2; y2 = t; }
	GrHLineNC(x, y1, y2, c);
}

void GrVLine(int x1, int x2, int y, int c)
{
        if (y  < clipy1)  y = clipy1;
        if (x1 < clipx1) x1 = clipx1;
        if (x2 < clipx1) x2 = clipx1;
        if (y  > clipy2)  y = clipy2;
        if (x1 > clipy2) x1 = clipx2;
        if (x2 > clipy2) x2 = clipx2; 
        if (x1 > x2) { int t; t = x1; x1 = x2; x2 = t; }
        GrVLineNC(x1, x2, y, c);
}



void GrClearClipBox(int col)
{
        GrFilledBoxNC(clipx1, clipy1, clipx2, clipx2, col);
	UpdateArea(clipx1, clipy1, clipx2, clipy2);
}


void GrSetClipBox(int x1, int y1, int x2, int y2)
{
	clipx1 = x1; clipx2 = x2;
	clipy1 = y1; clipy2 = y2;

	if (clipx1 < 0)   clipx1 = 0;
	if (clipx2 < 0)   clipx2 = 0;
	if (clipx1 > 799) clipx1 = 799;
	if (clipx2 > 799) clipx2 = 799;

        if (clipy1 < 0)   clipy1 = 0;
        if (clipy2 < 0)   clipy2 = 0;
        if (clipy1 > 599) clipy1 = 599;
        if (clipy2 > 599) clipy2 = 599;

	if (clipx2 < clipx1) { int t; t = clipx1; clipx1 = clipx2; clipx2 = t; }
	if (clipy2 < clipy1) { int t; t = clipy1; clipy1 = clipy2; clipy2 = t; }
}


void GrFilledBox(int x1, int y1, int x2, int y2, int colour)
{
        if (x1 < clipx1) x1 = clipx1;
        if (x2 < clipx1) x2 = clipx1;
        if (x1 > clipx2) x1 = clipx2;
        if (x2 > clipx2) x2 = clipx2;

        if (y1 < clipy1) y1 = clipy1;
        if (y2 < clipy1) y2 = clipy1;
        if (y1 > clipy2) y1 = clipy2;
        if (y2 > clipy2) y2 = clipy2;

        if (x2 < x1) { int t; t = x1; x1 = x2; x2 = t; }
        if (y2 < y1) { int t; t = y1; y1 = y2; y2 = t; }

	GrFilledBoxNC(x1, y1, x2, y2, colour);
}





void GrPlotRop(int x1, int y1, int colour, int rop)
{
        if (x1 < clipx1) x1 = clipx1;
        if (x1 > clipx2) x1 = clipx2;

        if (y1 < clipy1) y1 = clipy1;
        if (y1 > clipy2) y1 = clipy2;

	GrPlotRopNC(x1, y1, colour, rop);
}



void GrPlot(int x1, int y1, int colour)
{
        if (x1 < clipx1) x1 = clipx1;
        if (x1 > clipx2) x1 = clipx2;

        if (y1 < clipy1) y1 = clipy1;
        if (y1 > clipy2) y1 = clipy2;

        GrPlotNC(x1, y1, colour);
}

void UpdateArea(int x, int y, int w, int h)
{
	SDL_UpdateRect(gl_sys->m_video->getSurface(), x,y,w,h);
}

/*
void SdlScrBlit(int dx, int dy, int sx, int sy, int sx2, int sy2)
{
	Uint8 *sbits, *dbits;
	int bpp = screen->format->BytesPerPixel;
	int lpi = screen->pitch;
	int y,w = (sx2 - sx + 1) * bpp;

	if (SDL_MUSTLOCK(screen) && SDL_LockSurface(screen) < 0 ) return;

	if (sy < dy && dy < sy2) for (y = (sy2-sy); y > 0; y--)
        {
                sbits = ((Uint8 *)screen->pixels) + ((y+sy) * lpi) + (sx * bpp);
                dbits = ((Uint8 *)screen->pixels) + ((y+dy) * lpi) + (dx * bpp);

                memmove(dbits, sbits, w);
        }

        else for (y = 0; y < (sy2-sy); y++)
        {
                sbits = ((Uint8 *)screen->pixels) + ((y+sy) * lpi) + (sx * bpp);
		dbits = ((Uint8 *)screen->pixels) + ((y+dy) * lpi) + (dx * bpp);

		memmove(dbits, sbits, w);
	}

	if (SDL_MUSTLOCK(screen)) SDL_UnlockSurface(screen);
}*/

__inline__ int sgn(int x)
{
	/* Beautiful, isn't it? */
	return (!x ? 0 : (( x < 0 ) ? -1 : 1));
}


/* This line-drawing algorithm comes straight from the ZX81 manual, I kid
   you not. The comments are transcribed exactly, and the ZX81 has no
   lower-case letters; hence the shouting. */

void GrLineRop(int a, int b, int c, int d, int colour, int rop)
{
	int u   = c - a;	/* U SHOWS HOW MANY STEPS ALONG WE NEED TO GO */
	int v   = d - b;	/* V SHOWS HOW MANY STEPS UP */
	int d1x = sgn(u);	/* (D1X,D1Y) IS A SINGLE STEP IN A */
	int d1y = sgn(v);	/*           DIAGONAL DIRECTION    */ 
	int d2x = d1x;
	int d2y = 0;		/* (D2X,D2Y) IS A SINGLE STEP LEFT OR RIGHT */
	int m   = abs(u);
	int n   = abs(v);
	int s, i;

	if (m <= n)
	{
		d2x = 0;
		d2y = d1y;	/* NOW (D2X,D2Y) IS A SINGLE STEP UP OR DOWN */
	
		m = abs(v);
		n = abs(u);	
	}			/* M IS THE LARGER OF ABS U & ABS V, N IS */
				/* THE SMALLER */
	s = m / 2;		/* WE WANT TO MOVE FROM (A,B) TO (C,D) IN */
				/* M STEPS USING N UP-DOWN OR RIGHT-LEFT  */
				/* STEPS D2, & M-N DIAGONAL STEPS D1, */
				/* DISTRIBUTED AS EVENLY AS POSSIBLE */
	for (i = 0; i < m; i++)
	{
		GrPlotRop(a,b,colour,rop);
		s += n;
		if (s >= m)
		{
			s -= m;
			a += d1x;
			b += d1y;	/* A DIAGONAL STEP */
		}
		else
		{
			a += d2x;
			b += d2y;	/* AN UP-DOWN OR LEFT-RIGHT STEP */
		}
	}	
}



