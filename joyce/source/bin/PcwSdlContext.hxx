/************************************************************************

    JOYCE v1.90 - Amstrad PCW emulator

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

#include "SdlContext.hxx"

class PcwInitScrException
{
	char *m_explanation;

public:
	PcwInitScrException(char *s);
	~PcwInitScrException();
	inline char *getExplanation() { return m_explanation; }
};


class PcwSdlContext : public SdlContext
{
public:
	PcwSdlContext(PcwArgs *a, int w, int h);	// throws PcwInitScrException
	virtual ~PcwSdlContext();

	void deinitScreen(void);
	void reloadPalette(void);
	void initPalette(void);	
	void initColours(SDL_Surface *screen);
	Uint32 pixelToIndex(Uint32 pixel);
	Uint32 colourToIndex(Uint8 r, Uint8 g, Uint8 b);
	Uint32 colourToIndex(SDL_Colour &c);
	Uint32 colourToPixel(SDL_Colour &c);
	Uint32 indexToPixel (int index);
	void   indexToColour(int index, Uint8 *r, Uint8 *g, Uint8 *b);
	void   indexToColour(int index, SDL_Colour *c);
	void reset(void);

	SdlContext *m_icon;
	SDL_Colour  m_colours[256];
	int	m_ncolours, m_colourNextFree;
	int	m_pxrs, m_pxgs, m_pxbs, m_pitch, m_pixw;
protected:

	void initIcon(const char *appName, const char *bmpName,
                             int mask_width, int mask_height,
                             unsigned char *mask_bits, SDL_Surface *screen);

	SDL_Surface *initScreen(PcwArgs *args, int w, int h);
	PcwArgs   *m_args;

	char   m_popup[80];
	time_t m_popupTimeout;
	SdlContext *m_popupBacking;
	SDL_Rect m_popupRect;
};





