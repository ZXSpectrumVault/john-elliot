/************************************************************************

    JOYCE v2.1.0 - Amstrad PCW emulator

    Copyright (C) 1996, 2001  John Elliott <seasip.webmaster@gmail.com>

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

#include "Pcw.hxx"

PcwInitScrException::PcwInitScrException(char *s)
{
	m_explanation = new char[strlen(s)+1];
	strcpy(m_explanation, s);
}

PcwInitScrException::~PcwInitScrException()
{
	delete m_explanation;
}


PcwSdlContext::PcwSdlContext(PcwArgs *a, int w, int h) : 
		SdlContext(initScreen(a, w, h))
{
	m_icon     = NULL;
	m_ncolours = 0;
	m_args = a;
}

PcwSdlContext::~PcwSdlContext()
{
	deinitScreen();
	if (m_icon) delete m_icon;
}


void PcwSdlContext::reloadPalette(void)
{	
}


void PcwSdlContext::deinitScreen(void)
{
        if (m_icon) SDL_FreeSurface(m_icon->getSurface());
        SDL_Quit();
}

#include "mask.xbm"
#include "mask16.xbm"

SDL_Surface *PcwSdlContext::initScreen(PcwArgs *args, int width, int height)
{
/* v2.1.5: Add joystick support */
        if ( SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_JOYSTICK) < 0 )
        {
		throw new PcwInitScrException(SDL_GetError());
        }
	int flags = SDL_ANYFORMAT;

        if (args->m_fullScreen) flags |= SDL_FULLSCREEN;
        if (args->m_hardwareSurface) flags |= SDL_HWSURFACE;
        if (args->m_hardwarePalette) flags |= SDL_HWPALETTE;
        else flags |= SDL_SWSURFACE;

        SDL_Surface *screen = SDL_SetVideoMode(width, height, 8, flags);
	
	if (!screen) 
	{
		throw new PcwInitScrException(SDL_GetError());
	}
	m_surface = screen;	// So that the palette functions work.
	// OK, we have a screen. 
        joyce_dprintf("Set %dx%d at %d bits-per-pixel %s mode, %s surface\n",
			width, height,
                        screen->format->BitsPerPixel,
                 (screen->flags & SDL_FULLSCREEN) ? "full screen" : "windowed",
                 (screen->flags & SDL_HWSURFACE)  ? "hardware" : "software");

        m_pxrs = screen->format->Rshift/8;
        m_pxgs = screen->format->Gshift/8;
        m_pxbs = screen->format->Bshift/8;
	m_pitch = screen->pitch;
	m_pixw  = screen->format->BytesPerPixel;

/* This is naughty. Strictly speaking I should create 2 subclasses of 
 * PcwSdlContext with different icons. */
	const char *appn = args->appname();

	switch(appn[0])
	{
		case 'A': 	// ANNE
		case 'a': initIcon(appn, "pcw16.bmp", mask16_width, 
			 	mask16_height, mask16_bits, screen);
			  break;
		case 'J': 	// JOYCE
		case 'j': initIcon(appn, "pcw.bmp", mask_width, 
			 	mask_height, mask_bits, screen);
			  break;
	}
	initColours(screen);
	return screen;
}

void PcwSdlContext::reset(void)
{
}


void PcwSdlContext::initIcon(const char *appName, const char *bmpName, 
			     int maskwidth, int maskheight,
			     unsigned char *maskbits, SDL_Surface *screen)
{
	static char title[80];
	static bool maskFlipped = false;
	string bmpFile;

	if (!maskFlipped)
	{
		flipXbm(maskwidth, maskheight, maskbits);
		maskFlipped = true;
	}
	bmpFile = findPcwFile(FT_NORMAL, bmpName, "rb");	
	SDL_Surface *s = SDL_LoadBMP(bmpFile.c_str());
	if (s) m_icon = new SdlContext(s);
	if (m_icon) SDL_WM_SetIcon(s, maskbits);
	sprintf(title, "%s v%d.%d.%d",
			appName,
			(BCDVERS >> 8) & 0xFF,
			(BCDVERS >> 4) & 0x0F,
			(BCDVERS     ) & 0x0F);
        SDL_WM_SetCaption(title, "Pcw");

}



void PcwSdlContext::initColours(SDL_Surface *screen)
{
	int nr, ng, nb;

	SDL_Palette *palette = screen->format->palette;
	
	if (palette)	// Initialise colours table
	{
		m_ncolours = palette->ncolors;
		if (m_ncolours > 256) m_ncolours = 256;
		memcpy(m_colours, palette->colors, m_ncolours*sizeof(SDL_Colour));
	}
	m_colourNextFree = 0;

	for (nr = 0; nr < 256; nr += 85)
	    for (ng = 0; ng < 256; ng += 85)
	        for (nb = 0; nb < 256; nb += 85)
		     colourToIndex(nr, ng, nb);
	
        SDL_SetColours(getSurface(), m_colours, 0, 256);
}


Uint32 PcwSdlContext::colourToIndex(SDL_Colour &c)
{
	return colourToIndex(c.r, c.g, c.b);
}

void PcwSdlContext::indexToColour(int index, Uint8 *r, Uint8 *g, Uint8 *b)
{
	if (index >= 0 && index <= 256)
	{
		*r = m_colours[index].r;
		*g = m_colours[index].g;
		*b = m_colours[index].b;
	}	
}


void PcwSdlContext::indexToColour(int index, SDL_Colour *c)
{
	indexToColour(index, &c->r, &c->g, &c->b);
}


Uint32 PcwSdlContext::colourToIndex(Uint8 r, Uint8 g, Uint8 b)
{
        int n;
	SDL_Surface *s = getSurface();

	//
	// if m_colourNextFree is < 64, we are assigning 
	// permanent colours with guaranteed positions.
	//
	if (m_colourNextFree >= 64) for (n = 0; n < 255; n++)
        {
                if (m_colours[n].r == r && m_colours[n].g == g &&
                    m_colours[n].b == b) 
		{
			return n;
		}
        }
        m_colours[m_colourNextFree].r = r;
        m_colours[m_colourNextFree].g = g;
        m_colours[m_colourNextFree].b = b;
	// On 8-bit display, reassign palette accordingly
        if (s) SDL_SetColours(s, &m_colours[m_colourNextFree], m_colourNextFree, 1);

/* The top 128 colours can be freed when we need to */

        n = m_colourNextFree;
        ++m_colourNextFree;
        if (m_colourNextFree == 256) m_colourNextFree = 128;
        return n;


}

Uint32 PcwSdlContext::colourToPixel(SDL_Colour &c)
{
        return SDL_MapRGB(getSurface()->format, c.r, c.g, c.b);
}

Uint32 PcwSdlContext::indexToPixel(int index)
{
	return colourToPixel(m_colours[index]);
}

Uint32 PcwSdlContext::pixelToIndex(Uint32 pixel)
{
	Uint8 r,g,b;

	SDL_GetRGB(pixel, getSurface()->format, &r, &g, &b);
	return colourToIndex(r,g,b);
}





