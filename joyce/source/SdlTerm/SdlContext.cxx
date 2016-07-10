/************************************************************************

    SdlContext v1.00: C++ wrapper for an SDL surface

    Copyright (C) 2001  John Elliott <seasip.webmaster@gmail.com>

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

#include <SDL.h>
#include "SdlContext.hxx"
#include <string>
#include <stdlib.h>

SdlContext::SdlContext(SDL_Surface *s, SDL_Rect *bounds)
{
	SXLOG("SdlContext::SdlContext()");

	if (bounds) memcpy(&m_bounds, bounds, sizeof(m_bounds));	
	else
	{
		m_bounds.x = m_bounds.y = 0;
		m_bounds.w = s->w;
		m_bounds.h = s->h;
	}
        m_surface = s;
	m_manualUpdate = false;
}


SdlContext::~SdlContext()
{
	SXLOG("SdlContext::~SdlContext()");
}

/////////////////////////////////////////////////////////////////
// public plotting functions

Uint8 *SdlContext::getDotPos(int x, int y, int *bpp)
{
        *bpp = m_surface->format->BytesPerPixel;
        return ((Uint8 *)m_surface->pixels) + 
		y * m_surface->pitch        +
                x * (*bpp);
}


void SdlContext::dot(int x, int y, Uint32 pixel)
{
	int bpp;
	Uint8 *bits;

	bits = getDotPos(x, y, &bpp);
	dot(bits, pixel, bpp);
}


void SdlContext::dotA(int x, int y, Uint32 pixel)
{
	int bpp;
	Uint8 *bits;

	bits = getDotPos(x, y, &bpp);
	dotA(bits, pixel, bpp);
}


void SdlContext::dotO(int x, int y, Uint32 pixel)
{
	int bpp;
	Uint8 *bits;

	bits = getDotPos(x, y, &bpp);
	dotO(bits, pixel, bpp);
}


void SdlContext::dotX(int x, int y, Uint32 pixel)
{
	int bpp;
	Uint8 *bits;

	bits = getDotPos(x, y, &bpp);
	dotX(bits, pixel, bpp);
}

///////////////////////////////////////////////////////////////////////
//
// Private plotting functions
// 
void SdlContext::dot(Uint8 *bits, Uint32 pixel, int bpp)
{
        switch(bpp)
        {
                case 1:
                *((Uint8 *)(bits)) = (Uint8)pixel;
                break;

                case 2:
                *((Uint16 *)(bits)) = (Uint16)pixel;
                break;

                case 3:
                { /* Format/endian independent */
                    Uint8 r, g, b;

                    r = (pixel>>m_surface->format->Rshift)&0xFF;
                    g = (pixel>>m_surface->format->Gshift)&0xFF;
                    b = (pixel>>m_surface->format->Bshift)&0xFF;
                    *((bits)+m_surface->format->Rshift/8) = r;
                    *((bits)+m_surface->format->Gshift/8) = g;
                    *((bits)+m_surface->format->Bshift/8) = b;
                }
                break;

                case 4:
                *((Uint32 *)(bits)) = (Uint32)pixel;
                break;
        }
}



void SdlContext::dotA(Uint8 *bits, Uint32 pixel, int bpp)
{
        switch(bpp)
        {
                case 1:
                *((Uint8 *)(bits)) &= (Uint8)pixel;
                break;

                case 2:
                *((Uint16 *)(bits)) &= (Uint16)pixel;
                break;

                case 3:
                { /* Format/endian independent */
                    Uint8 r, g, b;

                    r = (pixel>>m_surface->format->Rshift)&0xFF;
                    g = (pixel>>m_surface->format->Gshift)&0xFF;
                    b = (pixel>>m_surface->format->Bshift)&0xFF;
                    *((bits)+m_surface->format->Rshift/8) &= r;
                    *((bits)+m_surface->format->Gshift/8) &= g;
                    *((bits)+m_surface->format->Bshift/8) &= b;
                }
                break;

                case 4:
                *((Uint32 *)(bits)) &= (Uint32)pixel;
                break;
        }
}

void SdlContext::dotO(Uint8 *bits, Uint32 pixel, int bpp)
{
        switch(bpp)
        {
                case 1:
                *((Uint8 *)(bits)) |= (Uint8)pixel;
                break;

                case 2:
                *((Uint16 *)(bits)) |= (Uint16)pixel;
                break;

                case 3:
                { /* Format/endian independent */
                    Uint8 r, g, b;

                    r = (pixel>>m_surface->format->Rshift)&0xFF;
                    g = (pixel>>m_surface->format->Gshift)&0xFF;
                    b = (pixel>>m_surface->format->Bshift)&0xFF;
                    *((bits)+m_surface->format->Rshift/8) |= r;
                    *((bits)+m_surface->format->Gshift/8) |= g;
                    *((bits)+m_surface->format->Bshift/8) |= b;
                }
                break;

                case 4:
                *((Uint32 *)(bits)) |= (Uint32)pixel;
                break;
        }
}

void SdlContext::dotX(Uint8 *bits, Uint32 pixel, int bpp)
{
        switch(bpp)
        {
                case 1:
                *((Uint8 *)(bits)) ^= (Uint8)pixel;
                break;

                case 2:
                *((Uint16 *)(bits)) ^= (Uint16)pixel;
                break;

                case 3:
                { /* Format/endian independent */
                    Uint8 r, g, b;

                    r = (pixel>>m_surface->format->Rshift)&0xFF;
                    g = (pixel>>m_surface->format->Gshift)&0xFF;
                    b = (pixel>>m_surface->format->Bshift)&0xFF;
                    *((bits)+m_surface->format->Rshift/8) ^= r;
                    *((bits)+m_surface->format->Gshift/8) ^= g;
                    *((bits)+m_surface->format->Bshift/8) ^= b;
                }
                break;

                case 4:
                *((Uint32 *)(bits)) ^= (Uint32)pixel;
                break;
        }
}

///////////////////////////////////////////////////////////////////////
//
// Locking functions
// 

bool SdlContext::lock(void)
{
	if (!SDL_MUSTLOCK(m_surface)) return true;

	if (SDL_LockSurface(m_surface) < 0) 
	{
		// XXX Log a message or something
		return false;
	}
	return true;
}


void SdlContext::unlock(void)
{
        if ( SDL_MUSTLOCK(m_surface) )
        {
                SDL_UnlockSurface(m_surface);
        }
}


///////////////////////////////////////////////////////////////////////
//
// Rectangle non-fills
// 

void SdlContext::rect(const SDL_Rect &rc, Uint32 pixel)
{
	rect(rc.x, rc.y, rc.w, rc.h, pixel);
}

void SdlContext::rect(int x, int y, int w, int h, Uint32 colour)
{
        SDL_Rect rc1;

        if (!lock()) return;
	rc1.x = x;
	rc1.y = y;
	rc1.w = 1;
	rc1.h = h;
        SDL_FillRect(m_surface, &rc1, colour);
        rc1.x = x + w - 1;
        SDL_FillRect(m_surface, &rc1, colour);
        rc1.x = x;
        rc1.w = w;
        rc1.h = 1;
        SDL_FillRect(m_surface, &rc1, colour);
        rc1.y = y + h - 1;
        SDL_FillRect(m_surface, &rc1, colour);
	unlock();
	if (!m_manualUpdate) SDL_UpdateRect(m_surface, x, y, w, h);
}

///////////////////////////////////////////////////////////////////////
//
// Rectangle fills
// 
void SdlContext::fillRect(int x, int y, int w, int h, Uint32 pixel)
{
	SDL_Rect rect;
        rect.x = x;
        rect.y = y;
        rect.w = w;
        rect.h = h;
	fillRect(rect, pixel);
}

void SdlContext::fillRect(const SDL_Rect &rect, Uint32 pixel)
{
	SDL_Rect rcReal;

	memcpy(&rcReal, &rect, sizeof(rcReal));
	rcReal.x += m_bounds.x;
	rcReal.y += m_bounds.y;

        if (!lock()) return;
        SDL_FillRect(m_surface, &rcReal, pixel);
        unlock();
        if (!m_manualUpdate) SDL_UpdateRects(m_surface, 1, &rcReal);
}


///////////////////////////////////////////////////////////////////////
//
// Pixel reading
// 
Uint32 SdlContext::pixelAt(int x, int y)
{
	int bpp;
	Uint8 *bits;

	bits = getDotPos(x, y, &bpp);
	return pixelAt(bits, bpp);
}



Uint32 SdlContext::pixelAt(Uint8 *bits, int bpp)
{
	Uint32 r,g,b;

        switch(bpp)
        {
                case 1:
                return *((Uint8 *)(bits));

                case 2:
                return *((Uint16 *)(bits));

                case 3:
                r = *((bits)+m_surface->format->Rshift/8);
                g = *((bits)+m_surface->format->Gshift/8);
                b = *((bits)+m_surface->format->Bshift/8);

		r <<= m_surface->format->Rshift;
		g <<= m_surface->format->Gshift;
		b <<= m_surface->format->Bshift;
		return (r | g | b); 

                case 4:
                return *((Uint32 *)(bits));
        }
	return 0;
}



