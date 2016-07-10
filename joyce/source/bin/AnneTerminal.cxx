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

#include "Anne.hxx"
#include "UiContainer.hxx"
#include "UiLabel.hxx"
#include "UiSeparator.hxx"
#include "UiCommand.hxx"
#include "UiSetting.hxx"
#include "UiMenu.hxx"
#include "UiTextInput.hxx"
#include "UiColourEntry.hxx"

static int defPalette[] =
{
        127,127,127,    /* 0 */
        127,127,127,    /* 1 */
        127,  0,127,    /* 2 */
        127,  0,127,    /* 3 */
        127,127,127,    /* 4 */
        127,127,127,    /* 5 */
        127,255,127,    /* 6 */
        127,255,127,    /* 7 */
          0,  0,127,    /* 8 */
          0,  0,  0,    /* 9 */
        127,  0,255,    /* 10 */
          0,  0,255,    /* 11 */
        127,  0,  0,    /* 12 */
        255,  0,127,    /* 13 */
        255,  0,127,    /* 14 */
        255,  0,255,    /* 15 */
          0,255,127,    /* 16 */
          0,255,  0,    /* 17 */
        127,255,255,    /* 18 */
          0,255,255,    /* 19 */
        127,255,  0,    /* 20 */
        255,255,  0,    /* 21 */
        255,255,127,    /* 22 */
        255,255,255,    /* 23 */
          0,127,127,    /* 24 */
          0,127,  0,    /* 25 */
        127,127,255,    /* 26 */
          0,127,255,    /* 27 */
        127,127,  0,    /* 28 */
        255,127,  0,    /* 29 */
        255,127,127,    /* 30 */
        255,127,255     /* 31 */
};


AnneTerminal::AnneTerminal(PcwBeeper *b) : PcwTerminal("pcw16")
{
	int n;

	m_beepMode = BM_SOUND;	
	m_beeper   = b;
	m_monochrome = 0;
	initSound();

	m_beepFg.g = m_beepFg.b = 0x00;
	m_beepFg.r = 0xFF;
	m_beepBg.r   = m_beepBg.g   = m_beepBg.b = 0x00;
	for (n = 0; n < 16; n++)
	{
		m_pcwInk[n] = m_pcwDInk[n] = 0;
	}
	m_inks[0].r = m_inks[0].g = m_inks[0].b = 0x00;
	m_inks[1].r = m_inks[1].g = m_inks[1].b = 0xFF;
	reset();

	m_bmpR = NULL;
	m_bmpG = NULL;
	
	m_scrRefresh = 75;	// Default refresh: 12Hz
}


AnneTerminal::~AnneTerminal()
{
	deinitSound();
}

//
// Static variables for speed. This rather fouls up the OO paradigm, but
// there is only one screen in the system.
//
// They all start gl_ so they can be easily seen.
//
static Uint32       gl_px[16], gl_pxrs, gl_pxgs, gl_pxbs;
static Uint8        gl_pxr[16], gl_pxg[16], gl_pxb[16];
static int	    gl_pcwink[16];
static SDL_Surface *gl_screen;
static bool         gl_usedCol[80];
static byte         gl_screenShadow[80][480];
static int          gl_lineLen;
static byte         gl_shadowInvalid;

/* These are macros rather than functions for speed. Perhaps one day I'll
 * get round to making them inline functions. */

// 2-colour versions:
#define PLINE_BEGIN(pixels_per_byte)    \
	for (x=0;x<80;x++)              \
        {                               \
        	v=(*bytes);             \
                if (v != gl_screenShadow[x][yoff] || gl_shadowInvalid)    \
                                                        /* (v1.30) */ \
                {                                                     \
			gl_usedCol[x] = true;				      \
                        touched = 1;                                  \
                        gl_screenShadow[x][yoff] = v; /* (v1.30) */     \
                        for (b=0;b< (pixels_per_byte) ;b++)                             \
                        {                                             

#define PLINE_END(step, bits_per_pixel)				\
				v = (v << (bits_per_pixel));	\
			}					\
		}						\
		else yp += step;				\
		bytes++;					\
	}


#define PLINE_2_8 \
		PLINE_BEGIN(8)					  \
                       data[yp++]=(v & 0x80 ? gl_px[1]:gl_px[0]); \
		PLINE_END(8,1)

#define PLINE_2_16 \
		PLINE_BEGIN(8)					\
                       *(Uint16*)(data+yp) = (v & 0x80) ?       \
                                  (Uint16)gl_px[1] : (Uint16)gl_px[0];    \
                       yp += 2;                                 \
		PLINE_END(16,1)

#define PLINE_2_32 \
                PLINE_BEGIN(8)                                  \
                       *(Uint32*)(data+yp) = (v & 0x80) ?       \
                                  (Uint32)gl_px[1] : (Uint32)gl_px[0];    \
                       yp += 4;                                 \
                PLINE_END(32,1)

#define PLINE_2_24 					      \
		PLINE_BEGIN(8)				      \
			if (v & 0x80)			      \
			{				      \
				data[yp+gl_pxrs] = gl_pxr[1];   \
				data[yp+gl_pxgs] = gl_pxg[1];   \
				data[yp+gl_pxbs] = gl_pxb[1];   \
			}				      \
			else				      \
			{				      \
                                data[yp+gl_pxrs] = gl_pxr[0];   \
                                data[yp+gl_pxgs] = gl_pxg[0];   \
                                data[yp+gl_pxbs] = gl_pxb[0];   \
			}				      \
                        yp += 3;                              \
                PLINE_END(24,1)                                                       

// Versions for the 4-colour mode:
#define PLINE_4_8 \
		PLINE_BEGIN(4)					\
                       data[yp++]=gl_px[(v >> 6) & 3];		\
                       data[yp++]=gl_px[(v >> 6) & 3];		\
		PLINE_END(8,2)

#define PLINE_4_16 \
		PLINE_BEGIN(4)					\
                       *(Uint16*)(data+yp)   = (Uint16)(gl_px[(v >> 6) & 3]); \
                       *(Uint16*)(data+yp+2) = (Uint16)(gl_px[(v >> 6) & 3]); \
                       yp += 4;                                 \
		PLINE_END(16,2)

#define PLINE_4_32 \
                PLINE_BEGIN(4)                                  \
                       *(Uint32*)(data+yp  ) = (Uint32)(gl_px[(v >> 6) & 3]); \
                       *(Uint32*)(data+yp+4) = (Uint32)(gl_px[(v >> 6) & 3]); \
                       yp += 8;                                 \
                PLINE_END(32,2)

#define PLINE_4_24 					      \
                PLINE_BEGIN(4)                                  \
			data[yp+gl_pxrs] = gl_pxr[(v >> 6) & 3];   \
			data[yp+gl_pxgs] = gl_pxg[(v >> 6) & 3];   \
			data[yp+gl_pxbs] = gl_pxb[(v >> 6) & 3];   \
			data[yp+3+gl_pxrs] = gl_pxr[(v >> 6) & 3];   \
			data[yp+3+gl_pxgs] = gl_pxg[(v >> 6) & 3];   \
			data[yp+3+gl_pxbs] = gl_pxb[(v >> 6) & 3];   \
			yp += 6; \
		PLINE_END(24, 2)

// Versions for the 16-colour mode:
#define PLINE_16_8 \
		PLINE_BEGIN(2)					\
                       data[yp++]=gl_px[(v >> 4) & 15];		\
                       data[yp++]=gl_px[(v >> 4) & 15];		\
                       data[yp++]=gl_px[(v >> 4) & 15];		\
                       data[yp++]=gl_px[(v >> 4) & 15];		\
		PLINE_END(8,4)

#define PLINE_16_16 \
		PLINE_BEGIN(2)					\
                       *(Uint16*)(data+yp) = (Uint16)(gl_px[(v >> 4) & 15]); \
                       *(Uint16*)(data+yp+2) = (Uint16)(gl_px[(v >> 4) & 15]); \
                       *(Uint16*)(data+yp+4) = (Uint16)(gl_px[(v >> 4) & 15]); \
                       *(Uint16*)(data+yp+6) = (Uint16)(gl_px[(v >> 4) & 15]); \
                       yp += 8;                                 \
		PLINE_END(16,4)

#define PLINE_16_32 \
                PLINE_BEGIN(2)                                  \
                       *(Uint32*)(data+yp) = (Uint32)(gl_px[(v >> 4) & 15]); \
                       *(Uint32*)(data+yp+4) = (Uint32)(gl_px[(v >> 4) & 15]); \
                       *(Uint32*)(data+yp+8) = (Uint32)(gl_px[(v >> 4) & 15]); \
                       *(Uint32*)(data+yp+12) = (Uint32)(gl_px[(v >> 4) & 15]); \
                       yp += 16;                                 \
                PLINE_END(32,4)

#define PLINE_16_24 					      \
                PLINE_BEGIN(2)                                  \
			data[yp+gl_pxrs] = gl_pxr[(v >> 4) & 15];   \
			data[yp+gl_pxgs] = gl_pxg[(v >> 4) & 15];   \
			data[yp+gl_pxbs] = gl_pxb[(v >> 4) & 15];   \
			data[yp+gl_pxrs+3] = gl_pxr[(v >> 4) & 15];   \
			data[yp+gl_pxgs+3] = gl_pxg[(v >> 4) & 15];   \
			data[yp+gl_pxbs+3] = gl_pxb[(v >> 4) & 15];   \
			data[yp+gl_pxrs+6] = gl_pxr[(v >> 4) & 15];   \
			data[yp+gl_pxgs+6] = gl_pxg[(v >> 4) & 15];   \
			data[yp+gl_pxbs+6] = gl_pxb[(v >> 4) & 15];   \
			data[yp+gl_pxrs+9] = gl_pxr[(v >> 4) & 15];   \
			data[yp+gl_pxgs+9] = gl_pxg[(v >> 4) & 15];   \
			data[yp+gl_pxbs+9] = gl_pxb[(v >> 4) & 15];   \
			yp += 12; \
		PLINE_END(24, 4)



static int drawLine8(unsigned char *bytes, short yoff, int mode) /* Draw a PCW line */
{
	int x;
	int touched = 0;
	register int b;
	register byte v;
	long yp;
	byte *data = (byte *)gl_screen->pixels;

	yp = ((yoff+60)*gl_lineLen + 80);
	switch(mode)
	{
		case 0: PLINE_2_8; break;
		case 1: PLINE_4_8; break;
		default: PLINE_16_8; break;
	}

	return touched;
}




static int drawLine16(unsigned char *bytes, short yoff, int mode) /* Draw a PCW line */
{
	int x;
	register int b;
	register byte v;
	long yp;
	int touched = 0;
	byte *data = (byte *)gl_screen->pixels;

	yp = ((yoff+60)*gl_lineLen + 160);
	switch(mode)
	{
		case 0: PLINE_2_16; break;
		case 1: PLINE_4_16; break;
		default: PLINE_16_16; break;
	}
	return touched;
}



static int drawLine32(unsigned char *bytes, short yoff, int mode) /* Draw a PCW line */
{
	int x;
	int touched = 0;
	register int b;
	register byte v;
	long yp;
	byte *data = (byte *)gl_screen->pixels;

	yp = ((yoff+60)*gl_lineLen + 320);
	switch(mode)
	{
		case 0: PLINE_2_32; break;
		case 1: PLINE_4_32; break;
		default: PLINE_16_32; break;
	}
	return touched;
}



static int drawLine24(unsigned char *bytes, short yoff, int mode) /* Draw a PCW line */
{
	int x;
	int touched = 0;
	register int b;
	register byte v;
	long yp;
	byte *data = (byte *)gl_screen->pixels;

	yp = ((yoff+60)*gl_lineLen + 240);
	switch(mode)
	{
		case 0: PLINE_2_24; break;
		case 1: PLINE_4_24; break;
		default: PLINE_16_24; break;
	}
	return touched;
}

void AnneTerminal::enable(bool b)
{
	if (b)
	{
        	m_scrEnabled = 1;
        	gl_shadowInvalid = 1;
        	m_borderInvalid = 1;
        	drawPcwScr();
		border(1);
	}
	else
	{	
		m_scrEnabled = 2;	// Becomes 0 after screen is blanked
		drawPcwScr();	
	}
}



void AnneTerminal::reset(void)
{	
	int n;

	m_scrEnabled = 1;
	m_scrCount = 0;
	m_colourReg = 0;
	m_prevColourReg = ~m_colourReg;
	if (m_sysVideo)
	{
		for (n = 0; n < 32; n++)
		{
			m_indices[n] = m_sysVideo->colourToIndex(defPalette[n*3], defPalette[n*3+1], defPalette[n*3+2]);
			if (n < 16) m_pcwInk[n] = m_pcwDInk[n] = m_sysVideo->colourToIndex(m_inks[n]);
		}
		m_beepClr  = m_sysVideo->colourToIndex(m_beepFg);
	}
	m_rollerRam = PCWRAM + 0xFC00;		// XXX fix this

	gl_shadowInvalid = 1;
	m_borderInvalid = 1;
	m_power = false;
	m_powerDrawn = true;
}


void AnneTerminal::onScreenTimer(void)
{
        if (++m_scrCount >= m_scrRefresh) // Draw the screen at 50Hz
        {
                drawPcwScr();
                m_scrCount=0;
        }
}


void AnneTerminal::onGainFocus(void)
{
	m_visible = 1;
	gl_shadowInvalid = 1;
	PcwTerminal::onGainFocus();
	border(1);
}


void AnneTerminal::onLoseFocus(void)
{
        m_visible = 0;
        PcwTerminal::onLoseFocus();
}



void AnneTerminal::drawPcwScr(void)	/* Draw the PCW screen */
{
	short y;
	register int rrvalue;
	int bpp;
	int touched = 0, ltouched = 0;
	Uint32 pxdb;
	/* Work out which columns & lines need updating */
	int tf = -1, tl = -1;
	int cf = -1, cl = -1;
	int r, n;

	if (!m_visible || !m_sysVideo) return;

	if (!m_sysVideo->lock()) return;	

	gl_screen = m_sysVideo->getSurface();
        bpp = m_sysVideo->m_pixw;
	for (n = 0; n < 16; n++) gl_pcwink[n] = m_pcwInk[n];
	gl_pxrs = m_sysVideo->m_pxrs;
	gl_pxgs = m_sysVideo->m_pxgs;
	gl_pxbs = m_sysVideo->m_pxbs;
	gl_lineLen = m_sysVideo->m_pitch;

	memset(gl_usedCol, 0, sizeof(gl_usedCol));
	for (n = 0; n < 16; n++)
	{
		switch(m_monochrome)
		{
			case 0: 	// Full colour
			gl_pxr[n] = m_sysVideo->m_colours[gl_pcwink[n]].r;
		       	gl_pxg[n] = m_sysVideo->m_colours[gl_pcwink[n]].g;
     			gl_pxb[n] = m_sysVideo->m_colours[gl_pcwink[n]].b;	
			break;

			case 1: 	// Greys
		       	gl_pxr[n] = gl_pxg[n] = gl_pxb[n] = m_sysVideo->m_colours[gl_pcwink[n]].g;
			break;

			case 2:		// Green
		       	gl_pxg[n] = m_sysVideo->m_colours[gl_pcwink[n]].g;
			gl_pxr[n] = gl_pxb[n] = 0;
			break;
		}
		gl_px[n]  = SDL_MapRGB(gl_screen->format, gl_pxr[n], gl_pxg[n], gl_pxb[n]);
	}
	pxdb = (m_pcwInk[0] == m_pcwDInk[0]) ? gl_px[0] : gl_px[1];


/* If you get problems with screen updates uncomment this line
 *
 *	gl_shadowInvalid = 1;
 */
	if (m_scrEnabled == 2) 	/* Screen switched off? */
	{
		SDL_Rect rc;
		rc.x = rc.y = 0;
		rc.w = 800;
		rc.h = 600;
		SDL_FillRect(gl_screen, &rc, pxdb);
		gl_shadowInvalid = 1;	/* (v1.30) */
		touched = 1;
		m_scrEnabled = 0;
	}
	else if (m_scrEnabled)//  && (m_colourReg & 0x40))
	{
		for (y = 0; y < 480; y++)
		{
			register short yy=(y<<1);

			rrvalue=m_rollerRam[yy+1];
			rrvalue=rrvalue <<8;
			rrvalue+=m_rollerRam[yy];
	
			switch(bpp)
			{
				case 1:	ltouched = drawLine8 (PCWRAM+((rrvalue & 0x3FFF)   << 4) ,y, rrvalue >> 14); break;
				case 2:	ltouched = drawLine16(PCWRAM+((rrvalue & 0x3FFF)   << 4) ,y, rrvalue >> 14); break;
				case 3:	ltouched = drawLine24(PCWRAM+((rrvalue & 0x3FFF)   << 4) ,y, rrvalue >> 14); break;
				case 4:	ltouched = drawLine32(PCWRAM+((rrvalue & 0x3FFF)   << 4) ,y, rrvalue >> 14); break;
			}
			if      (ltouched && tf < 0) tf = y;
			else if (ltouched)           tl = (y + 1); 
			touched |= ltouched;
		}
		gl_shadowInvalid = 0;
	}
	r = gl_sys->m_termMenu->drawPopup();

	if (r >  0) touched = 1;
	if (r == 2) gl_shadowInvalid = 1;	

	// Work out correct update rectangle, and update it.
	m_sysVideo->unlock();
	if (touched)
	{
		if (tf < 0) SDL_UpdateRect(gl_screen, 0, 0, 0, 0);
		else 
		{
			if (tl < 0) tl = tf + 1;
			tl += 60;      
			tf += 60;     

			for (y = 0; y < 80; y++) if (gl_usedCol[y])
			{
				if (cf < 0) cf = (y * 8) + 80;
				else        cl = (y * 8) + 88;	
			}

			if (cf < 0) cf = 80;
			if (cl < 0) cl = 720;
			SDL_UpdateRect(gl_screen, cf, tf, cl-cf, tl-tf); 
		}
	}
	if (m_power != m_powerDrawn) drawPower();
}

void AnneTerminal::drawPower(void)
{
	SDL_Rect rc, rc2;

	if (!gl_screen) return;

	m_powerDrawn = m_power;
	SDL_Surface *s;
	if (m_power) s = m_bmpG;
	else	     s = m_bmpR;

	if (!s) return;	// Power bitmaps not loaded
	rc.w = rc2.w = 59;
	rc.h = rc2.h = 43;
	rc.x = rc.y = 0;
	rc2.x = 0;
	rc2.y = 557;
	SDL_BlitSurface(s, &rc, gl_screen, &rc2);
	SDL_UpdateRects(gl_screen, 1, &rc2);
}


void AnneTerminal::out(word port, byte value)
{
	/* Palette bits */
	port &= 0xFF;
	if (port <= 0xEF && port >= 0xE0)
	{
		port &= 0x0F;
		value &= 0x1F;
		m_palette[port] = value;
		gl_shadowInvalid = 1;
		m_pcwDInk[port] = m_indices[value];
		m_pcwInk[port]  = m_indices[value];
		m_inks[port].r  = defPalette[port*3];
		m_inks[port].g  = defPalette[port*3+1];
		m_inks[port].b  = defPalette[port*3+2];
		return;
	}
	if (port == 0xF7)	/* Screen enabled, inverse, border */
	{
		m_colourReg=value;
		if (m_prevColourReg != value)
		{
			gl_shadowInvalid = 1;	/* (v1.30) */
			m_prevColourReg = value;
			border(1);
		}			
	}
}

bool AnneTerminal::parseNode(xmlDocPtr doc, xmlNsPtr ns, xmlNodePtr cur)
{
	char *s;
	int r = 0;
	parseInteger(doc, ns, cur, "refresh", &r);
	if (r) m_scrRefresh = r;
	parseInteger(doc, ns, cur, "monochrome", &m_monochrome);
	s = (char *)xmlGetProp(cur, (xmlChar *)"beep");
	if (s)
	{
		if (!strcmp(s, "border")) m_beepMode = BM_BORDER;
		if (!strcmp(s, "sound"))  m_beepMode = BM_SOUND;
		if (!strcmp(s, "none"))   m_beepMode = BM_NONE;
		xmlFree(s);
	}
	if (!parseColour(doc, ns, cur, "beep", m_beepFg, m_beepBg)) return false;
        return parseColour(doc, ns, cur, "colour", m_inks[1], m_inks[0]);
}

bool AnneTerminal::storeNode(xmlDocPtr doc, xmlNsPtr ns, xmlNodePtr cur)
{
	storeInteger(doc, ns, cur, "refresh", m_scrRefresh);
	switch(m_beepMode)
	{
		case BM_BORDER: xmlSetProp(cur, (xmlChar *)"beep", (xmlChar *)"border"); break;
		case BM_SOUND:  xmlSetProp(cur, (xmlChar *)"beep", (xmlChar *)"sound");  break;
		case BM_NONE:   xmlSetProp(cur, (xmlChar *)"beep", (xmlChar *)"none");   break;
	}
	storeInteger(doc, ns, cur, "monochrome", m_monochrome);
	if (!storeColour(doc, ns, cur, "beep", m_beepFg, m_beepBg)) return false;
        return storeColour(doc, ns, cur, "colour", m_inks[1], m_inks[0]);
}


void AnneTerminal::border(byte forced)
{
        int count, n;
        SDL_Rect rects[4];
        Uint32 pixel;
	Uint8 g;
	int border_idx = m_indices[m_colourReg & 0x1F];

        if (!m_visible || !m_sysVideo || !(m_scrEnabled & 1)) return;

        if (m_colourReg & 0x80)  /* Colours inverted */
        {
                gl_pcwink[0] = m_pcwInk[0] = m_pcwDInk[1];
                gl_pcwink[1] = m_pcwInk[1] = m_pcwDInk[0];
        }
	else
	{
                gl_pcwink[0] = m_pcwInk[0] = m_pcwDInk[0];
                gl_pcwink[1] = m_pcwInk[1] = m_pcwDInk[1];
	}
        if (!forced && !m_borderInvalid) return;
        m_borderInvalid = 0;

	g = m_sysVideo->m_colours[border_idx].g;
// Colouring the border?
	gl_screen = m_sysVideo->getSurface();
	if (m_beepMode == BM_BORDER && m_beepStat)
        {
                border_idx = m_beepClr; 
		pixel  = m_sysVideo->indexToPixel(border_idx);
        }
	else switch(m_monochrome)
	{
		case 0: 	// Full colour
		pixel  = m_sysVideo->indexToPixel(border_idx);
		break;
	
		case 1: 	// Grey
		pixel = SDL_MapRGB(gl_screen->format, g, g, g);
		break;	
	
		case 2: 	// Green
		pixel = SDL_MapRGB(gl_screen->format, 0, g, 0);
		break;	
	}
	memset(rects, 0, sizeof(rects));

        if (m_colourReg & 0x40)  /* Border covers screen? */
        {
                rects[0].w = rects[3].w = 800;
                rects[1].w = rects[2].w = 80;
                rects[0].h = rects[3].h = 60;
                rects[1].y = rects[2].y = rects[0].h;
                rects[1].h = rects[2].h = 480;
                rects[2].x = 720;
                rects[3].y = rects[1].y + rects[1].h;
                count = 4;
        }
        else
        {
                count = 1;
                rects[0].w = 800;
                rects[0].h = 600;
        }
        if (!m_sysVideo->lock()) return;
        for (n = 0; n < count; n++)
        {
                SDL_FillRect(m_sysVideo->getSurface(), &rects[n], pixel);
        }

	m_sysVideo->unlock();
	drawPower();
	SDL_UpdateRects(m_sysVideo->getSurface(), count, rects);
}


void AnneTerminal::setForeground(byte r, byte g, byte b) 
{
        m_inks[1].r = r;
	m_inks[1].g = g;
	m_inks[1].b = b;
        m_pcwDInk[1] = m_sysVideo->colourToIndex(m_inks[1]);
	gl_shadowInvalid = 1;
	border(1);
}


void AnneTerminal::setBackground(byte r, byte g, byte b)
{
        m_inks[0].r = r;
	m_inks[0].g = g;
	m_inks[0].b = b; 
        m_pcwDInk[0] = m_sysVideo->colourToIndex(m_inks[0]);
        gl_shadowInvalid = 1;
        border(1);
}

void AnneTerminal::onDebugSwitch(bool b)
{
	if (m_debugMode != b)
	{
		//gl_shadowInvalid = 1;
		//m_borderInvalid = 1;
	}
	PcwTerminal::onDebugSwitch(b);
}


PcwTerminal &AnneTerminal::operator << (unsigned char c)
{
	// XXX to be done
	return (*this);
}

bool AnneTerminal::hasSettings(SDLKey *key, string &caption)
{
	*key = SDLK_v;
	caption = "  Video  ";	
	return true;
}

UiEvent AnneTerminal::customClrs(UiDrawer *d)
{	
	UiEvent uie;
	int sel = -1;
 
        while (1)
        {
                UiMenu uim(d);
                uim.add(new UiLabel("  PCW screen colours  ", d));
                uim.add(new UiSeparator(d));
		uim.add(new UiColourEntry(SDLK_f, "  Foreground:  __  ", m_inks[1], d));
		uim.add(new UiColourEntry(SDLK_b, "  Background:  __  ", m_inks[0], d));
		uim.add(new UiColourEntry(SDLK_v, "  Visual beep: __  ", m_beepFg, d));
                uim.add(new UiSeparator(d));
		uim.add(new UiCommand(SDLK_ESCAPE, "  EXIT  ", d));
                d->centreContainer(uim);
                uim.setSelected(sel);
		uie = uim.eventLoop();
		
		if (uie == UIE_QUIT) return uie;
                sel = uim.getSelected(); 
		switch(uim.getKey(sel))
		{
			case SDLK_ESCAPE:
		m_pcwInk[0] = m_pcwDInk[0] = m_sysVideo->colourToIndex(m_inks[0]);
       		m_pcwInk[1] = m_pcwDInk[1] = m_sysVideo->colourToIndex(m_inks[1]);
		m_beepClr  = m_sysVideo->colourToIndex(m_beepFg);
			return UIE_OK;
			default:		break;
		}
	} 
	return UIE_CONTINUE;
}


UiEvent AnneTerminal::settings(UiDrawer *d)
{
        int x,y,sel;
        UiEvent uie;
	char upd[50];
	double upf;

        x = y = 0;
        sel = -1;
        while (1)
        {
                UiMenu uim(d);

		sprintf(upd, "%03.2f", 900.0 / m_scrRefresh);

                uim.add(new UiLabel("  Video settings  ", d));
                uim.add(new UiSeparator(d));
		uim.add(new UiTextInput(SDLK_u, "  Video refresh: ______ Hz  ",d));
		uim.add(new UiLabel("  (higher values are smoother but slower)  ", d)); 
                uim.add(new UiSeparator(d));
                uim.add(new UiSetting(SDLK_d, (m_beepMode == BM_NONE),  "  Disable beeper", d));
                uim.add(new UiSetting(SDLK_a, (m_beepMode == BM_SOUND), "  Audio beeper",  d));
                uim.add(new UiSetting(SDLK_v, (m_beepMode == BM_BORDER),"  Visual beeper",  d));
                uim.add(new UiSeparator(d));
		uim.add(new UiSetting(SDLK_g, (m_monochrome == 2),  "  Green screen  ", d));
		uim.add(new UiSetting(SDLK_w, (m_monochrome == 1),  "  White screen  ", d));
                uim.add(new UiSetting(SDLK_c, (m_monochrome == 0), "  Colour screen  ",  d));
                uim.add(new UiSeparator(d));
		uim.add(new UiCommand(SDLK_ESCAPE, "  EXIT  ", d));
                d->centreContainer(uim);
                uim.setSelected(sel);

		((UiTextInput &)uim[2]).setText(upd);

                uie =  uim.eventLoop();
        
                if (uie == UIE_QUIT) return uie;
                sel = uim.getSelected(); 
		switch(uim.getKey(sel))
		{
			case SDLK_u: upf = atof(((UiTextInput &)uim[2]).getText().c_str());
				     if (upf)
				     {
					m_scrRefresh = (int)(900.0 / upf);
					if (m_scrRefresh < 18) m_scrRefresh = 18;
                                     }
				     break;	
			case SDLK_d: m_beepMode = BM_NONE; break;
			case SDLK_a: m_beepMode = BM_SOUND; break;
			case SDLK_v: m_beepMode = BM_BORDER; break;
			case SDLK_g: m_monochrome = 2; break;
			case SDLK_w: m_monochrome = 1; break;
			case SDLK_c: m_monochrome = 0; break;
			case SDLK_ESCAPE: return UIE_OK;
			default: break;
		}
	
	}
	return UIE_CONTINUE;
}

bool AnneTerminal::loadBitmaps()
{
        string bmpFile;

        bmpFile = findPcwFile(FT_NORMAL, "16power2.bmp", "rb");
        SDL_Surface *s = SDL_LoadBMP(bmpFile.c_str());
	if (!s)
	{
		alert("Failed to load " + bmpFile);
		return false;
	}
        m_bmpR = s; 
        bmpFile = findPcwFile(FT_NORMAL, "16power3.bmp", "rb");
        s = SDL_LoadBMP(bmpFile.c_str());
	if (!s)
	{
		alert("Failed to load " + bmpFile);
		return false;
	}
        m_bmpG = s;

	return true;
}

void AnneTerminal::powerLED(bool green)
{
	m_power = green;
}



void AnneTerminal::tick(void)
{
	if (m_beeper) m_beeper->tick();
}



void AnneTerminal::beepOn(void)
{
	PcwTerminal::beepOn();
	switch(m_beepMode)
	{
		case BM_BORDER:  m_borderInvalid = 1; border(1); break;
		case BM_SOUND:   if (m_beeper) m_beeper->beepOn(); break;
		default:	 break;
	}
}

void AnneTerminal::beepOff(void)
{
	PcwTerminal::beepOff();
	switch(m_beepMode)
	{
		case BM_BORDER:  m_borderInvalid = 1; border(1); break;
		case BM_SOUND:   if (m_beeper) m_beeper->beepOff(); break;
		default:	 break;
	}
}


void AnneTerminal::deinitSound(void)
{
	if (m_beeper) m_beeper->deinitSound();
}

void AnneTerminal::initSound(void)
{
	if (m_beeper) m_beeper->initSound();
}


bool AnneTerminal::doBlit(unsigned xfrom, unsigned yfrom, unsigned w, unsigned h,
                               unsigned xto, unsigned yto)
{
	unsigned rowfrom, rowto, last, x;
	int step;
	byte *dataFrom, *dataTo;
	word rrvalue;
	byte maskFrom, maskTo;
	byte buf[80];

	if (yfrom >= yto)
	{
		rowfrom = yfrom;
		rowto   = yto;
		step = 1;
		last = yfrom + h;
	}
	else
	{
		rowfrom = yfrom + h - 1;
		rowto   = yto   + h - 1;
		step = -1;
		last = yfrom - 1;	
	}
	for (; rowfrom != last; rowfrom += step, rowto += step)
	{
/* Don't blit rows that aren't onscreen */
		if (rowfrom >= 480 || rowto >= 480) continue;

		register short yy = (rowfrom << 1);
		rrvalue = m_rollerRam[yy+1];
		rrvalue = rrvalue <<8;
		rrvalue += m_rollerRam[yy];
		dataFrom = PCWRAM + ((rrvalue & 0x3FFF)   << 4);
/* To get round the problem of overlapping areas, we keep a copy of the
 * current row locally and work from that */
		memcpy(buf, dataFrom, sizeof(buf));
		yy = (rowto << 1);
		rrvalue = m_rollerRam[yy+1];
		rrvalue = rrvalue <<8;
		rrvalue += m_rollerRam[yy];
		dataTo = PCWRAM + ((rrvalue & 0x3FFF)   << 4);
		maskFrom = 0x80 >> (xfrom % 8);
		maskTo   = 0x80 >> (xto   % 8);

		dataTo   = dataTo + (xto / 8);
		dataFrom = buf    + (xfrom / 8);
	
		for (x = 0; x < w; x++)
		{
/* Check if we've run off the screen */
			if ((x + xfrom) > 640 || (x + xto) >= 640) break;

/* Short cut. If the masks are the same and we're at the start of 
 * a byte and there's a whole byte to copy, do it in one go */
			if (maskFrom == maskTo && maskFrom == 0x80 && 
			    x < (w-8))
			{
				*dataTo++ = *dataFrom++;
				x += 8;
				continue;
			}

			if (dataFrom[0] & maskFrom) dataTo[0] |=  maskTo;
			else			    dataTo[0] &= ~maskTo;
			maskFrom = maskFrom >> 1;
			maskTo   = maskTo   >> 1;
			if (maskFrom == 0)
			{
				++dataFrom;
				maskFrom = 0x80;
			}
			if (maskTo == 0)
			{
				++dataTo;
				maskTo = 0x80;
			}
		}
	}


/*	SDL_Rect rfrom, rto;

	rfrom.x = xfrom + 80;
	rfrom.y = yfrom + 60;
	rfrom.w = w;
	rfrom.h = h;

	rto.x = xto + 80;
	rto.y = yto + 60;
	rto.w = w;
	rto.h = h;
        SDL_BlitSurface(gl_screen, &rfrom, gl_screen, &rto);
*/
	return true;
}

// Greyscale patterns
static byte g0[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };	// 00
static byte g1[] = { 0x00, 0x00, 0x44, 0x00, 0x00, 0x00, 0x44, 0x00 };	// 01
static byte g2[] = { 0x44, 0x00, 0x44, 0x00, 0x44, 0x00, 0x44, 0x00 };	// 02
static byte g3[] = { 0x44, 0x11, 0x44, 0x11, 0x44, 0x11, 0x44, 0x11 };	// 03
static byte g4[] = { 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55 };	// 04
static byte g5[] = { 0xBB, 0xEE, 0xBB, 0xEE, 0xBB, 0xEE, 0xBB, 0xEE };	// 05
static byte g6[] = { 0xBB, 0xFF, 0xBB, 0xFF, 0xBB, 0xFF, 0xBB, 0xFF };	// 06
static byte g7[] = { 0xFF, 0xFF, 0xBB, 0xFF, 0xFF, 0xFF, 0xBB, 0xFF };	// 07
static byte g8[] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };  // 08

static byte *shades[9] =
{
	g0, g1, g2, g3, g4, g5, g6, g7, g8
};

/* Accelerated rectangle drawing. This draws in the PCW's screen RAM; it 
 * could do direct SDL painting as well, but that's for the future. */
bool AnneTerminal::doRect(unsigned x, unsigned y, unsigned w, unsigned h, 
	byte fill, byte rasterOp, byte *f)
{
	byte lmask = 0, rmask = 0;
	byte *row;
	unsigned x1, x2, lcol, rcol, col;
	word rrvalue;
	short yy;

	/* Pass f non-null for custom fill */
	if (f == NULL)
	{
		f = shades[(fill & 0x7F) % 9];
	}
/*
	SDL_Rect rect;

	rect.x = x + 80;
	rect.y = y + 60;
	rect.w = w;
	rect.h = h;
	SDL_FillRect(gl_screen, &rect, gl_px[4]);
*/

/* Area to fill consists of: 
 * >> Left stripe
 * >> Centre area
 * >> Right stripe 
 *
 * Caveats:
 * If x is a multiple of 8, omit left stripe.
 * If (x+w) is a multiple of 8, omit right stripe
 * If (x+w) & 0xF8 == (x & 0xF8) omit centre area
 */ 
	x1 = x;
	while (x1 & 7)	
	{ 
		lmask = (lmask << 1) | 1; 
		++x1; 
	}
/* lmask = bitmask for left stripe, draw in column (x/8) */
/* x1 = Left edge of centre area */
	lcol = (x + 7) / 8;
	rcol = (x + w) / 8;
/* lcol = first column to include in central area */
/* rcol = first column not to be in central area */
	x2 = x + w;	/* First column not to paint */
	while (x2 & 7)
	{
		rmask = (rmask >> 1) | 0x80;
		--x2;
	}
/* If lcol > rcol, the left and right stripes are in the same column */
//	fprintf(stderr, "|x = %x lcol=%x lmask=%x\n", x, lcol, lmask);
//	fprintf(stderr, "|       rcol=%x rmask=%x\n",    rcol, rmask);
	if (lcol > rcol)
	{
		lmask &= rmask;
		rmask = 0;
	}

//	fprintf(stderr, " x = %x lcol=%x lmask=%x\n", x, lcol, lmask);
//	fprintf(stderr, "        rcol=%x rmask=%x\n",    rcol, rmask);
/* rmask = bitmask for right stripe, draw in column rcol */
	while (h)
	{
		register short yy=(y<<1);

		rrvalue=m_rollerRam[yy+1];
		rrvalue=rrvalue <<8;
		rrvalue+=m_rollerRam[yy];

		row = PCWRAM+((rrvalue & 0x3FFF)   << 4);

		for (col = lcol; col < rcol; col++)
		{
			switch(rasterOp)
			{
				case 0: row[col]  = f[y & 7]; break;
				case 2: row[col] |= ~f[y & 7]; break;
				case 1: row[col] &= ~f[y & 7]; break;
				case 3: row[col] ^= ~f[y & 7]; break;
			}
		}
		if (lmask && lcol > 0)
		{
			switch(rasterOp)
			{
				case 0: row[lcol - 1] &= ~lmask;
					row[lcol - 1] |= ((f[y & 7]) & lmask); break;
				case 2: row[lcol - 1] |= ((~f[y & 7]) & lmask); break;
				case 1: row[lcol - 1] &= ((~f[y & 7]) | (~lmask)); break;
				case 3: row[lcol - 1] ^= ((~f[y & 7]) & lmask); break;
			}
		}
		if (rmask)
		{
			switch(rasterOp)
			{
				case 0: row[rcol] &= ~rmask;
					row[rcol] |= (f[y & 7] & rmask); break;
				case 2: row[rcol] |= ((~f[y & 7]) & rmask); break;
				case 1: row[rcol] &= ((~f[y & 7]) | (~rmask)); break;
				case 3: row[rcol] ^= ((~f[y & 7]) & rmask); break;
			}
		}
		++y;
		--h;
	}	

	return true;
}




bool AnneTerminal::doBitmap(unsigned x, unsigned y, unsigned w, unsigned h,
                                byte *data, byte *mask, int slice)
{
	unsigned x1, y1;
	unsigned ssize = slice * 8;
	unsigned wb = 2 * slice + 1;

	if (!slice)
	{
		return drawBitmap(x, y, w, h, data, mask, (w+7)/8);
	}

	/* A sliced bitmap is stored as 9 tiles. The four corner ones are
	 * ssize*ssize; the centre one is 8*8; and the four side ones 
	 * are therefore either ssize*8 or 8*ssize. The side and inner 
	 * tiles are repeated as many times as necessary to fill the space
	 * required. */

	drawBitmap(x, y, ssize, ssize, data, mask, wb);
	for (x1 = ssize; x1 < (w - ssize); x1 += 8)
	{
		drawBitmap(x + x1, y, 8, ssize, data + slice, mask + slice, wb);
	}
	x1 = w - ssize;
	drawBitmap(x + x1, y, ssize, ssize, data + slice + 1, mask + slice + 1, wb);

	data += ssize * wb;
	mask += ssize * wb;
	for (y1 = ssize; y1 < (h - ssize); y1 += 8)
	{
		drawBitmap(x, y + y1, ssize, 8, data, mask, wb);
		for (x1 = ssize; x1 < (w - ssize); x1 += 8)
		{
			drawBitmap(x + x1, y + y1, 8, 8, data + slice, mask + slice, wb);
		}
		x1 = w - ssize;
		drawBitmap(x + x1, y + y1, ssize, 8, data + slice + 1, mask + slice + 1, wb);
	}	
	data += 8 * wb;
	mask += 8 * wb;
	y1 = h - ssize;
	drawBitmap(x, y + y1, ssize, ssize, data, mask, wb);
	for (x1 = ssize; x1 < (w - ssize); x1 += 8)
	{
		drawBitmap(x + x1, y + y1, 8, ssize, data + slice, mask + slice, wb);
	}
	x1 = w - ssize;
	drawBitmap(x + x1, y + y1, ssize, ssize, data + slice + 1, mask + slice + 1, wb);


	return true;
}




bool AnneTerminal::drawBitmap(unsigned x0, unsigned y, unsigned w, unsigned h,
                                byte *data, byte *mask, unsigned swb)
{
	byte smask, dmask;
	byte *src, *msk, *row;
	unsigned dwb, w1, dy, xbyte;
	word rrvalue;

	dwb = (w + 7) / 8;
//	fprintf(stderr,"AnneTerminal::drawBitmap(%d,%d,%d,%d,%p,%p,%d) %d\n",
//			x0,y,w,h,data,mask,swb,dwb);
	dy = 0;
        while (h)
        {
                register short yy=((y+dy)<<1);

                rrvalue =  m_rollerRam[yy+1];
                rrvalue =  rrvalue << 8;
                rrvalue += m_rollerRam[yy];

                row = PCWRAM+((rrvalue & 0x3FFF)   << 4);
		smask = 0x80;
		dmask = 0x80 >> (x0 & 7);
		src = data + dy * swb;
		if (mask) msk = mask + (dy * swb);
		else 	  msk = NULL;
		xbyte = (x0 >> 3);

		for (w1 = 0; w1 < w; w1++)
		{
			if (mask == NULL || ((*msk) & smask))
			{
				if ((*src) & smask) row[xbyte] |= dmask;
				else 	            row[xbyte] &= ~dmask;
			}
			smask = smask >> 1; 
			if (!smask) 
			{
				smask = 0x80;
				++src;
				++msk;
			}
			dmask = dmask >> 1; 
			if (!dmask) 
			{
				dmask = 0x80;
				++xbyte;
			}
		}	
		++dy;
		--h;
	}
        return true;
}



