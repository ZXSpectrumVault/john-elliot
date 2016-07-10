/************************************************************************

    SdlTerm v1.01 - Terminal emulation in an SDL framebuffer

    Copyright (C) 1996, 2001, 2006  John Elliott <seasip.webmaster@gmail.com>

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
#include "SdlTerm.hxx"

static Uint8 fontDef[] = 
{
#include "font8x16.inc"
};

SdlTerm::SdlTerm(SDL_Surface *s, SDL_Rect *bounds) : SdlContext(s, bounds)
{
	int n, m;
	SXLOG("SdlTerm::SdlTerm()");
	for (n = 0; n < SDLK_LAST; n++)
	{
		for (m = 0; m < 8; m++)
		{
		        m_keyMap[m][n] = new std::string;
		}	
	}
	m_font    = NULL;
	m_curVisible = false;
	reset();
	resetInput();
	SXLOG("SdlTerm::SdlTerm() done");

}

SdlTerm::~SdlTerm()
{
	int n, m;

	SXLOG("SdlTerm::~SdlTerm()");
	
	if (m_font) 
	{
		delete [] m_font;
		m_font = NULL;
	}
	for (n = 0; n < SDLK_LAST; n++)
	{
		for (m = 0; m < 8; m++)
		{
		        delete m_keyMap[m][n];
		}	
	}
	SXLOG("SdlTerm::~SdlTerm() done");
}


void SdlTerm::reset()
{
	m_cx = m_cy = 0;	
	m_slcx = 0;
	m_autoX = 1;
	m_language = 0;

	setFont(fontDef);
	m_windowC.x   = m_windowC.y = 0;
	m_windowC.w   = m_bounds.w   / m_charw;
	m_windowC.h   = m_bounds.h   / m_charh;
	calcWindow();	

	setBold(false);	
	setItalic(false);
	setGrey(false);
	setReverse(false);
	setUnderline(0);
	setAltFont(false);
	setWrap(true);

	setForeground(0xFF, 0xFF, 0xFF);
	setBackground(0x00, 0x00, 0xAA);
	clearWindow();
	saveCursorPos();
	m_curVisible = false;
	enableCursor();
	setCursorShape();
	setPageMode(false);
	enableStatus(true);
}


void SdlTerm::calcWindow(void)
{
	m_windowP.x = m_windowC.x * m_charw;
	m_windowP.y = m_windowC.y * m_charh;
	m_windowP.w = m_windowC.w * m_charw;
	m_windowP.h = m_windowC.h * m_charh;
}

void SdlTerm::setForeground(Uint8 r, Uint8 g, Uint8 b)
{
	setForeground(SDL_MapRGB(m_surface->format, r, g, b));
}


void SdlTerm::setBackground(Uint8 r, Uint8 g, Uint8 b)
{
        setBackground(SDL_MapRGB(m_surface->format, r, g, b));
}


void SdlTerm::getForeground(Uint8 *r, Uint8 *g, Uint8 *b)
{
	SDL_GetRGB(m_pixelFG, m_surface->format, r, g, b);
}

void SdlTerm::getBackground(Uint8 *r, Uint8 *g, Uint8 *b)
{
        SDL_GetRGB(m_pixelBG, m_surface->format, r, g, b);
}




void SdlTerm::setFont(Uint8 *psf)
{
	Uint8 *buf;

	if (psf[0] != 0x36 || psf[1] != 0x04) return;
	buf = new Uint8[4 + 256 * psf[3]];
	if (!buf) return;

	memcpy(buf, psf, 4 + 256 * psf[3]);
	if (m_font) delete [] m_font;
	m_font = buf;

	m_charmax = 256;
	m_charw   = 8;
	m_charh   = psf[3];	
// Transform this font to the correct national language variant.
	if (m_language) lswap(m_language);
}


void SdlTerm::beginMergeUpdate(void)
{
	m_x0 = m_bounds.x + m_bounds.w;
	m_x1 = m_bounds.x;
	m_y0 = m_bounds.y + m_bounds.h;
	m_y1 = m_bounds.y;
	m_oldManualUpdate = m_manualUpdate;
	m_manualUpdate = true;
}

void SdlTerm::condUpdate(SDL_Rect *rc)
{
	condUpdate(rc->x, rc->y, rc->w, rc->h);
}

void SdlTerm::condUpdate(int x, int y, int w, int h)
{
	if (m_manualUpdate)
	{
		if (m_x0 > x)		m_x0 = x;
		if (m_x1 < (x + w)) 	m_x1 = x + w;
		if (m_y0 > y)		m_y0 = y;
		if (m_y1 < (y + h)) 	m_y1 = y + h;	
	}
	else	SDL_UpdateRect(m_surface, x, y, w, h);
}


void SdlTerm::endMergeUpdate(void)
{
	if (m_x0 < m_x1 && m_y0 < m_y1 && !m_oldManualUpdate)
	{
		SDL_UpdateRect(m_surface, m_x0, m_y0, m_x1 - m_x0, m_y1 - m_y0);
	}
	m_manualUpdate = m_oldManualUpdate;
}

SdlTerm& SdlTerm::operator << (unsigned char c)
{
	write(c); 
	return *this;
}

SdlTerm& SdlTerm::operator << (const char *s)
{
	beginMergeUpdate();
	while (*s)
	{
		operator <<((unsigned char)(*s));
		++s;
	}
	endMergeUpdate();
	return *this;
}


SdlTerm& SdlTerm::operator << (Uint16 u)
{
	char buf[20];
	sprintf(buf, "%u", u);
	return operator <<(buf);
}

SdlTerm& SdlTerm::operator << (Sint16 s)
{
        char buf[20];
        sprintf(buf, "%d", s);
        return operator <<(buf);
}

SdlTerm& SdlTerm::operator << (Uint32 u)
{
        char buf[20];
        sprintf(buf, "%u", u);
        return operator <<(buf);
}

SdlTerm& SdlTerm::operator << (Sint32 s)
{
        char buf[20];
        sprintf(buf, "%d", s);
        return operator <<(buf);
}



void SdlTerm::clearWindow(void)
{
	hideCursor();
	fillRect(m_windowP, m_pixelBG);
}


void SdlTerm::clearCurLine(void)
{
	SDL_Rect rc;

	hideCursor();
	rc = m_windowP;
	rc.y += m_charh * m_cy;
	rc.h = m_charh;
	fillRect(rc, m_pixelBG);
}

void SdlTerm::clearLineRange(int first, int last)
{
        SDL_Rect rc;

	if (last < first) last = first;

        hideCursor();
        rc = m_windowP;
        rc.y += (m_charh * first);
        rc.h  = (m_charh * last ) - rc.y + m_charh;
        fillRect(rc, m_pixelBG);

}


void SdlTerm::clearEOS(void)
{
	SDL_Rect rc;

	clearEOL();	// does the hideCursor() automatically
	rc = m_windowP;
	rc.y += m_charh * (1+m_cy);
	rc.h = m_windowP.y + m_windowP.h - rc.y;
	if (rc.h) fillRect(rc, m_pixelBG);
}

void SdlTerm::clearBOS(void)
{
        SDL_Rect rc;

        clearBOL();     // does the hideCursor() automatically
        rc = m_windowP;
	rc.h = m_charh * m_cy;
        if (rc.h) fillRect(rc, m_pixelBG);

}


void SdlTerm::clearEOL(void)
{
	SDL_Rect rc;

	hideCursor();
	rc = m_windowP;
	rc.y += m_charh * m_cy;
	rc.h  = m_charh;
	rc.x += m_charw * m_cx;
	rc.w  = m_windowP.x + m_windowP.w - rc.x;
	if (rc.w) fillRect(rc, m_pixelBG);
}


void SdlTerm::clearBOL(void)
{
        SDL_Rect rc;

        hideCursor();
        rc = m_windowP;
        rc.y += m_charh * m_cy;
        rc.h  = m_charh;
        rc.w  = m_charw * (1+m_cx);
        fillRect(rc, m_pixelBG);
}

void SdlTerm::deleteLine(void)
{
	hideCursor();
	if (m_cy < (m_windowC.h + 1)) 
	{
		scrollLineRangeUp(m_cy, m_windowC.h);
	}
	clearCurLine();
}


void SdlTerm::insertLine(void)
{
	hideCursor();
	if (m_cy < (m_windowC.h + 1)) 
	{
		scrollLineRangeDown(m_cy, m_windowC.h);
	}
	clearCurLine();
}


void SdlTerm::deleteChar(void)
{
        SDL_Rect sr, dr;

	if (m_cx >= (m_windowC.w + 1))
	{
		clearEOL();
		return;
	}


        sr = m_windowP;
       	sr.x += m_bounds.x;
	sr.y += m_bounds.y;

	sr.x += m_cx * m_charw;
	sr.w = (m_windowC.w - m_cx) * m_charh;

	if (sr.x < 0) sr.x = 0;
	if (sr.x + sr.w + m_charw > m_bounds.x + m_bounds.w)
	{
		sr.w = m_bounds.x + m_bounds.w - sr.x - m_charw;
		if (sr.w <= 0) { clearEOL(); return; }
	}
        dr = sr;
        sr.x += m_charw;
        blit(sr, dr);
	dr.w += m_charw;
	condUpdate(&dr);
}





void SdlTerm::cursorUp(bool bScroll)
{
	hideCursor();

	if (m_cy > 0) --m_cy;
	else if (bScroll)
	{
		if (m_pageMode) 
		{ 
			m_cy = m_windowC.h - 1; 
			clearCurLine();
		}
		else  scrollDown(); 
	}
}

void SdlTerm::cursorDown(bool bScroll)
{
	hideCursor();
	if (m_cy < (m_windowC.h + 1)) ++m_cy;
	else if (bScroll) lf();
}


void SdlTerm::cursorLeft(bool bWrap)
{
	hideCursor();
	--m_cx;
	if (m_cx < 0)
	{
		if (bWrap && m_wrap)
		{
			cursorUp(true);
			m_cx = m_windowC.w - 1;
		}
		else ++m_cx;
	}
}


void SdlTerm::cursorRight(bool bWrap)
{
	hideCursor();
        m_cx++;
        if (m_cx >= m_windowC.w)
        {
                if (bWrap && m_wrap) { cr(); lf(); }
                else --m_cx;
        }
}



#if 0
static Uint8 statln= 1;	/* Is the status line enabled? */
static Uint8 statln_x = 0;	/* Position of cursor in status line */


#define SCRBOT (statln ? 36 : 37)	/* Bottom line of the screen */

#endif

static const Uint8 lang1[]=
	{	0x40, 0xEA,	/* Language 1 : @ -> a grave */ 
		0x5B, 0xA2,	/*              [ -> degree */
		0x5C, 0xF5,	/* 		\ -> c cedilla */
		0x5D, 0xA6,	/*		] -> Section */
		0x7B, 0xE1,	/*		{ -> e acute */
		0x7C, 0xEE,	/*		| -> u grave */
		0x7D, 0xEB,	/*		} -> e grave */
		0x7E, 0xB2,	/*		~ -> umlaut */
		0x00, 0x00	};

static const Uint8 lang2[]=
	{	0x40, 0xA6,	/* Language 2 : @ -> Section */ 
		0x5B, 0xD0,	/*              [ -> A umlaut */
		0x5C, 0xD3,	/* 		\ -> O umlaut */
		0x5D, 0xD4,	/*		] -> U umlaut */
		0x7B, 0xF0,	/*		{ -> a umlaut */
		0x7C, 0xF3,	/*		| -> o umlaut*/
		0x7D, 0xF4,	/*		} -> u umlaut */
		0x7E, 0xBA,	/*		~ -> double s */
		0x00, 0x00	};

static const Uint8 lang3[]=
	{	0x23, 0xA3,	/* Language 3 : # -> œ */
		0x00, 0x00	};

static const Uint8 lang4[]=
	{	0x5B, 0xD6,	/* Language 4 : [ -> AE */
		0x5C, 0xD8,	/* 		\ -> O slash */
		0x5D, 0xD7,	/*		] -> A ring */
		0x7B, 0xF6,	/*		{ -> ae */
		0x7C, 0xF8,	/*		| -> o slash*/
		0x7D, 0xF7,	/*		} -> a ring */
		0x00, 0x00	};

static const Uint8 lang5[]=
	{	0x40, 0xC1,	/* Language 5 : @ -> E acute */
		0x5B, 0xD0,	/*              [ -> A umlaut */
		0x5C, 0xD3,	/* 		\ -> O umlaut */
		0x5D, 0xD7,	/*		] -> A ring */
		0x5E, 0xD4,	/*		^ -> U umlaut */
		0x7B, 0xF0,	/*		{ -> a umlaut */
		0x7C, 0xF3,	/*		| -> o umlaut*/
		0x7D, 0xF7,	/*		} -> a ring */
		0x7E, 0xF4,	/*		~ -> u umlaut */
		0x00, 0x00	};

static const Uint8 lang6[]=
	{	0x5B, 0xA2,	/* Language 6 : [ -> degree */
		0x5D, 0xE1,	/*		] -> e acute */
		0x60, 0xEE,	/*		` -> u grave */
		0x7B, 0xEA,	/*		{ -> a grave */
		0x7C, 0xED,	/*		| -> o grave */
		0x7D, 0xEB,	/*		} -> e grave */
		0x7E, 0xEC,	/*		~ -> i grave */
		0x00, 0x00	};

static const Uint8 lang7[]=
	{	0x5B, 0xAF,	/* Language 7 : [ -> upside-down pling */
		0x5C, 0xD9,	/*		\ -> N tilde */
		0x5D, 0xAE,	/*		] -> upside-down query */
		0x7B, 0xB2,	/*		{ -> umlaut */
		0x7C, 0xF9,	/*		| -> n tilde */
		0x00, 0x00	};



static const Uint8 *swaptbl[8] = 	
				/* Character swaps in language <n> */
{
	NULL,			/* Language 0 : no swaps */
	lang1, lang2, lang3,
	lang4, lang5, lang6, lang7
};


void SdlTerm::lswap(int lang)
{
	const Uint8 *swapp = swaptbl[lang];
	Uint8 b1, b2;
	Uint8 *swbuf;

	if (!lang || !swapp) return;

	swbuf = new Uint8[m_charh];
	if (!swbuf) return;

	do {
		b1 = swapp[0];
		b2 = swapp[1];
		swapp += 2;
		memcpy(swbuf,                     m_font + 4 + m_charh * b1, m_charh);
		memcpy(m_font + 4 + m_charh * b1, m_font + 4 + m_charh * b2, m_charh);
		memcpy(m_font + 4 + m_charh * b2, swbuf,                     m_charh);
	} while (b1 != b2);
	delete swbuf;
}



void SdlTerm::setLanguage(int c)
{
	if (m_language) lswap(m_language);
	m_language = c;
	lswap(m_language);
}


// This terminal doesn't have different modes. On the Spectrum +3,
// the modes are 24x32, 24x51, 24x80. 
void SdlTerm::setMode(int m)
{
	switch(m)
	{
		case 2: setCentred(80,24); break;
		default: setMaxWin(); break;
	}
}

#if 0
void crtplus(Uint8 c)	/* Deal with codes after ESC + */
{
	onesc = 0;

	switch(c)
	{
		case 'b': bold = 1;	break;
		case 'g': grey = 1;	break;
		case 'i': italic = 1;	break;
	}

}


void crtminus(Uint8 c)	/* Deal with codes after ESC - */
{
	onesc = 0;

	switch(c)
	{
		case 'b': bold = 0;	break;
		case 'g': grey = 0;	break;
		case 'i': italic = 0;	break;
	}
}

#endif
void SdlTerm::bel(void)
{
	// XXX To be done	
}


void    SdlTerm::moveCursor(int x, int y)
{
	if (x < 0) x = 0;
	if (y < 0) y = 0;

	hideCursor();
	m_cx = x;
	m_cy = y;
	if (m_cx >= m_windowC.w) m_cx = m_windowC.w - 1;
	if (m_cy >= m_windowC.h) m_cy = m_windowC.h - 1;
}

void SdlTerm::saveCursorPos(void)
{
	m_scx = m_cx; m_scy = m_cy;
}


void SdlTerm::setWindow(int x, int y, int w, int h)
{
	hideCursor();
	if (x * m_charw >= m_bounds.w) x = (m_bounds.w / m_charw) - 1;
	if (y * m_charh >= m_bounds.h) y = (m_bounds.h / m_charh) - 1;
	if (x < 0) x = 0;
	if (y < 0) y = 0;

	if ((x + w) * m_charw > m_bounds.w) w = (m_bounds.w / m_charw) - x;
	if ((y + h) * m_charh > m_bounds.h) h = (m_bounds.h / m_charh) - y;
	if (w < 1) w = 1;
	if (h < 1) h = 1;	
	
	m_windowC.x = x;
	m_windowC.y = y;
	m_windowC.w = w;
	m_windowC.h = h;
	calcWindow();
}

void SdlTerm::getWindow(SDL_Rect *rc)
{
	memcpy(rc, &m_windowC, sizeof(SDL_Rect));	
}


void SdlTerm::setMaxWin(void)
{
	int w = m_bounds.w / m_charw;
	int h = m_bounds.h / m_charh;

	setWindow(0,0,w,h);
}

void SdlTerm::setCentred(int ww, int hh)
{
	int x,y;
        int w = m_bounds.w / m_charw;
        int h = m_bounds.h / m_charh;

	if (w < ww) x = 0; else x = (w-ww)/2;
	if (h < hh) y = 0; else y = (h-hh)/2;
	setWindow(x,y,ww,hh);
}


/* Non-destructive copy between overlapping areas of the screen (which
 * you can't trust SDL_BlitSurface to do without an intermediate 
 * area)
 */

void SdlTerm::blit(SDL_Rect &sr, SDL_Rect &dr)
{
	int d = m_surface->format->BitsPerPixel;

//
// If the display is in 8-bit mode, use a 24-bit intermediate 
// surface to avoid palette problems.
//
	if (d == 8) d = 24;
	SDL_Surface *s2 = SDL_AllocSurface(SDL_SWSURFACE, sr.w, sr.h, 
					d, 0, 0, 0, 0);
	if (s2)
	{
		SDL_Rect r3 = sr;

		r3.x = r3.y = 0;
		SDL_BlitSurface(m_surface, &sr, s2, &r3);
		SDL_BlitSurface(s2, &r3, m_surface, &dr);
		SDL_FreeSurface(s2);	
	}
	else SDL_BlitSurface(m_surface, &sr, m_surface, &dr);
}


void SdlTerm::enableStatus(bool b)
{
	int wb = m_windowC.y + m_windowC.h;
	int sb = (m_bounds.y + m_bounds.h) / m_charh;


	if (b)
	{
//
// If the bottom line of our window area is in use, move it up by one.
//
//		printf("wb=%d sb=%d\n",wb, sb);

		if (wb == sb) { --m_windowC.h; }
		if (m_cy == m_windowC.h) { --m_cy; scrollUp(); }
		m_statLine = true;
	}
	else
	{
		if (wb == sb) ++m_windowC.h;
		m_statLine = false;
	}
}



void SdlTerm::write(unsigned char c)
{
	hideCursor();

	glyph(c);
	cursorRight(true);
}


void SdlTerm::cr(void)
{
	hideCursor();
	m_cx = 0;
}

void SdlTerm::lf(void)
{
	hideCursor();
	++m_cy;
	if (m_pageMode) 
	{
		if (m_cy >= m_windowC.h) m_cy = 0;
		clearCurLine();
	}
	else if (m_cy >= m_windowC.h) 
	{
		--m_cy;
		scrollUp();
	}
}

void SdlTerm::scrollUp(void)
{
	SDL_Rect sr, dr;

	sr = m_windowP;
	sr.x += m_bounds.x;
	sr.y += m_bounds.y;
	sr.h -= m_charh;
	dr = sr;	
	sr.y += m_charh;
	blit(sr, dr);

	dr.h += m_charh;	
	condUpdate(&dr);
	clearCurLine();
}


void SdlTerm::scrollLineRangeDown(int first, int last)
{
        SDL_Rect sr, dr;

        sr = m_windowP;
        sr.x += m_bounds.x;
        sr.y += m_bounds.y;

	sr.y += first * m_charh;
	sr.h = (1+last-first) * m_charh;

	if (sr.y < 0) sr.y = 0;
	if (sr.y + sr.h + m_charh > m_bounds.y + m_bounds.h)
	{
		sr.h = m_bounds.y + m_bounds.h - sr.y - m_charh;
		if (sr.h <= 0) return;
	}
        dr = sr;
        dr.y += m_charh;

	SDL_Surface *s2 = SDL_AllocSurface(SDL_SWSURFACE, sr.w, sr.h, 
					24, 0, 0, 0, 0);

	sr.h += m_charh;
	condUpdate(&sr);
}




void SdlTerm::scrollLineRangeUp(int first, int last)
{
        SDL_Rect sr, dr;

        sr = m_windowP;
        sr.x += m_bounds.x;
        sr.y += m_bounds.y;

	sr.y += first * m_charh;
	sr.h = (1+last-first) * m_charh;

	if (sr.y < 0) sr.y = 0;
	if (sr.y + sr.h + m_charh > m_bounds.y + m_bounds.h)
	{
		sr.h = m_bounds.y + m_bounds.h - sr.y - m_charh;
		if (sr.h <= 0) return;
	}
        dr = sr;
        sr.y += m_charh;
	blit(sr, dr);
	dr.h += m_charh;
	condUpdate(&dr);
}



void SdlTerm::scrollDown(void)
{
        SDL_Rect sr, dr;

        sr.x += m_bounds.x;
        sr.y += m_bounds.y;

        sr = m_windowP;
        sr.h -= m_charh;
        dr = sr;
        dr.y += m_charh;
	blit(sr, dr);
	sr.h += m_charh;
	condUpdate(&sr);
        clearCurLine();
}





//
// Output a character bitmap
//
void SdlTerm::glyph(unsigned char c)
{
	Uint16 ch = c;	// Index into character table
	Uint8 *intbm = new Uint8[2 * m_charh];
	int n;
	
	if (!intbm || !m_font) return;

	if (m_charmax > 256 && m_altFont) ch += 256;
	// Get character bitmap
	memcpy(intbm, (m_font + 4 + (ch * m_charh)), m_charh);

	if (m_bold)   for (n = 0; n < m_charh; n++) intbm[n] |= (intbm[n] >> 1);
	if (m_grey)   for (n = 0; n < m_charh; n++) intbm[n] &= (n & 1 ? 0x55 : 0xAA);
	if (m_italic) 
	{
		int chh = m_charh;
		int step1, step2, step3;

		step1 = chh / 5; chh -= (2 * step1);
		step2 = chh / 3; chh -= (2 * step2);
		step3 = chh;	

		for (n = 0;   n < step1;       n++) intbm[n] = intbm[n] >> 2; 
		chh = step1;
		for (n = chh; n < chh + step2; n++) intbm[n] = intbm[n] >> 1;
		chh += step2 + step3;
		for (n = chh; n < chh + step2; n++) intbm[n] = intbm[n] << 1; 
		chh += step2;	
		for (n = chh; n < chh + step1; n++) intbm[n] = intbm[n] << 2; 
	}
	intbm[m_charh - 1] |= m_ul;

	if (m_reverse) for (n = 0; n < m_charh; n++) intbm[n] = ~intbm[n];

	rawGlyph(m_windowP.x + m_cx * 8, m_windowP.y + m_cy * m_charh, intbm, 0);
	delete [] intbm;
}



void SdlTerm::rawGlyph(int x, int y, Uint8 *bitmap, int rast_op)
{
	int vx, vy;
	Uint8 b = 0;
        int bpp;
        Uint8 *bits;
	Uint32 pixel;

	x += m_bounds.x;
	y += m_bounds.y;

	bits = getDotPos(x, y, &bpp);

	if (!lock()) return;

	for (vy = 0; vy < m_charh; vy++)
	{
		b = *bitmap++;
		for (vx = 0; vx < 8; vx++)
		{
			if (b & 0x80) pixel = m_pixelFG; 
			else          pixel = m_pixelBG;
		
			switch(rast_op)
			{
				default:
				case SDLT_DRAW: dot (bits + vx * bpp, pixel, bpp); break;
                                case SDLT_AND:  dotA(bits + vx * bpp, pixel, bpp); break;
                                case SDLT_OR:   dotO(bits + vx * bpp, pixel, bpp); break;
                                case SDLT_XOR:  dotX(bits + vx * bpp, pixel, bpp); break;
			}
			b = b << 1;
		}	
		bits += m_surface->pitch;
	}
	unlock();
	condUpdate(x, y, m_charw, m_charh);
}

#if 0

static void set_win(int l, int t, int r, int b)
{
	wleft  = l;
	wright = r;
	wtop   = t;
	wbot   = b;
	set_window();
	vga_cls();
}

#endif


void SdlTerm::setCursorShape(int shape)
{
	hideCursor();
	if (shape >= CSR_MIN && shape <= CSR_MAX) m_curShape = shape;
}

void SdlTerm::drawCursor()
{
	int x = (m_cx * m_charw) + m_windowP.x;
	int y = (m_cy * m_charh) + m_windowP.y;
	int y0, y1, vy, vx;
	int bpp;

	if (!lock()) return;	
	switch(m_curShape)
	{
		default:  
		case CSR_BLOCK:		
			y0 = 0; 
			y1 = m_charh; 
			break;
		case CSR_HALFBLOCK:
			y0 = m_charh / 2; 
			y1 = m_charh; 
			break;
		case CSR_UNDERLINE:
			y1 = m_charh;
			y0 = y1 - 1;
			break;
	}
        Uint8 *bits;

	x += m_bounds.x;
	y += m_bounds.y;

        bits = getDotPos(x, y + y0, &bpp);

        for (vy = y0; vy < y1; vy++)
        {
                for (vx = 0; vx < m_charw; vx++)
		{
			dotX(bits + vx * bpp, 0xFFFFFFFF, bpp);
		}
		bits += m_surface->pitch;
	}
	unlock();
	condUpdate(x, y + y0, x + m_charw, y1 - y0);
}


void SdlTerm::hideCursor(void)
{
	if (m_curVisible)
	{
		drawCursor();
		m_curVisible = false;
	}
}

void SdlTerm::showCursor(void)
{
	if (m_curEnabled && !m_curVisible)
	{
		drawCursor();
		m_curVisible = true;
	}
}


void SdlTerm::enableCursor(bool enabled)
{
	m_curEnabled = enabled;
	if (!enabled) hideCursor();
}



void SdlTerm::onTimer50(void)
{
	showCursor();
}


void SdlTerm::glyphSys(unsigned char ch)
{
	int ocx = m_cx;
	int ocy = m_cy;
	m_cx = m_slcx;
	m_cy = m_windowC.h - 1;
	glyph(ch);
	m_cx = ocx;
	m_cy = ocy;
}


void SdlTerm::clearStatus(void)
{
        SDL_Rect rc;

        hideCursor();
        rc = m_windowP;
        rc.y = (m_windowC.h - 1) * m_charh;
        rc.h = m_charh;
        fillRect(rc, m_pixelBG);
	m_slcx = 0;
}

void SdlTerm::sysCursorRight(void)
{
	++m_slcx;
	if (m_slcx >= m_windowC.w) m_slcx = 0;
}


void SdlTerm::writeSys(unsigned char ch)
{
	if (!isStatusEnabled())
        {
		write(ch);
                return;
        }
        hideCursor();

        glyphSys(ch);
        sysCursorRight();
	
}

