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

#include "Joyce.hxx"
#include "UiContainer.hxx"
#include "UiLabel.hxx"
#include "UiSeparator.hxx"
#include "UiCommand.hxx"
#include "UiSetting.hxx"
#include "UiMenu.hxx"
#include "UiTextInput.hxx"
#include "UiColourEntry.hxx"


JoycePcwTerm::JoycePcwTerm(PcwBeeper *b) : PcwTerminal("pcw")
{
	m_beepMode = BM_SOUND;
	m_beeper   = b;
	initSound();

	m_beepFg.g = m_beepFg.b = 0x00;
	m_beepFg.r = 0xFF;
	m_beepBg.r   = m_beepBg.g   = m_beepBg.b = 0x00;
	m_screenBg.r = m_screenBg.g = m_screenBg.b = 0x00;
	m_screenFg.r = m_screenFg.g = m_screenFg.b = 0xFF;
	reset();

	m_scrRefresh = 75;	// Default refresh: 12Hz
}


JoycePcwTerm::~JoycePcwTerm()
{
}

//
// Static variables for speed. This rather fouls up the OO paradigm, but
// there is only one screen in the system.
//
// They all start gl_ so they can be easily seen.
//
static Uint32       gl_pxw, gl_pxb;
static Uint8  	    gl_whi, gl_blk;
static Uint8        gl_pxwr, gl_pxwg, gl_pxwb, gl_pxbr, gl_pxbg, gl_pxbb;
static int          gl_pxrs, gl_pxgs, gl_pxbs;
static SDL_Surface *gl_screen;
static bool         gl_usedCol[90];
static byte         gl_screenShadow[90][256];
static int          gl_lineLen;
static byte         gl_shadowInvalid;
static byte         gl_debugMode = false;

/* These are macros rather than functions for speed. Perhaps one day I'll
 * get round to making them inline functions. */

#define PLINE_BEGIN	                \
	for (x=0;x<90;x++)              \
        {                               \
        	v=(*bytes);             \
                if (v != gl_screenShadow[x][yoff] || gl_shadowInvalid)    \
                                                        /* (v1.30) */ \
                {                                                     \
			gl_usedCol[x] = true;				      \
                        touched = 1;                                  \
                        gl_screenShadow[x][yoff] = v; /* (v1.30) */     \
                        for (b=0;b<8;b++)                             \
                        {                                             

#define PLINE_END(step)				\
				v = (v << 1);	\
			}			\
		}				\
		else yp += step;		\
		bytes += 8;			\
		}

/*
#define PLINE_8 \
		PLINE_BEGIN					\
                       data[yp++]=(v & 0x80 ? gl_whi:gl_blk);	\
		PLINE_END(8)
*/
#define PLINE_8 \
		PLINE_BEGIN					\
                       data[yp++]=(v & 0x80 ? gl_pxw:gl_pxb);	\
		PLINE_END(8)

#define PLINE_16 \
		PLINE_BEGIN					\
                       *(Uint16*)(data+yp) = (v & 0x80) ?       \
                                  (Uint16)gl_pxw : (Uint16)gl_pxb;    \
                       yp += 2;                                 \
		PLINE_END(16)

#define PLINE_32 \
                PLINE_BEGIN                                     \
                       *(Uint32*)(data+yp) = (v & 0x80) ?       \
                                  (Uint32)gl_pxw : (Uint32)gl_pxb;    \
                       yp += 4;                                 \
                PLINE_END(32)

#define PLINE_24 					      \
		PLINE_BEGIN				      \
			if (v & 0x80)			      \
			{				      \
				data[yp+gl_pxrs] = gl_pxwr;   \
				data[yp+gl_pxgs] = gl_pxwg;   \
				data[yp+gl_pxbs] = gl_pxwb;   \
			}				      \
			else				      \
			{				      \
                                data[yp+gl_pxrs] = gl_pxbr;   \
                                data[yp+gl_pxgs] = gl_pxbg;   \
                                data[yp+gl_pxbs] = gl_pxbb;   \
			}				      \
                        yp += 3;                              \
                PLINE_END(24)                                                       

static int drawLine8(unsigned char *bytes, short yoff) /* Draw a PCW line */
{
	int x;
	int touched = 0;
	register int b;
	register byte v;
	long yp;
	byte *data = (byte *)gl_screen->pixels;

	if (gl_debugMode)
	{
		yp = ((yoff+22)*gl_lineLen + 40);
		PLINE_8

	}
	else
	{
		yp = ((yoff*2+44)*gl_lineLen+40);
		PLINE_8
		memcpy(data+yp+gl_lineLen-720, data+yp-720, gl_lineLen);
	}
	return touched;
}




static int drawLine16(unsigned char *bytes, short yoff) /* Draw a PCW line */
{
	int x;
	register int b;
	register byte v;
	long yp;
	int touched = 0;
	byte *data = (byte *)gl_screen->pixels;

	if (gl_debugMode)
	{
		yp = ((yoff+22)*gl_lineLen + 80);
		PLINE_16
	}
	else
	{
		yp = ((yoff*2+44)*gl_lineLen+80);
		PLINE_16
		memcpy(data+yp+gl_lineLen-1440, data+yp-1440, gl_lineLen);
	}
	return touched;
}



static int drawLine32(unsigned char *bytes, short yoff) /* Draw a PCW line */
{
	int x;
	int touched = 0;
	register int b;
	register byte v;
	long yp;
	byte *data = (byte *)gl_screen->pixels;

	if (gl_debugMode)
	{
		yp = ((yoff+22)*gl_lineLen + 160);
		PLINE_32

	}
	else
	{
		yp = ((yoff*2+44)*gl_lineLen + 160);
		PLINE_32
		memcpy(data+yp+gl_lineLen-2880, data+yp-2880, gl_lineLen);
	}
	return touched;
}



static int drawLine24(unsigned char *bytes, short yoff) /* Draw a PCW line */
{
	int x;
	int touched = 0;
	register int b;
	register byte v;
	long yp;
	byte *data = (byte *)gl_screen->pixels;

	if (gl_debugMode)
	{
		yp = ((yoff+22)*gl_lineLen + 120);
		PLINE_24

	}
	else
	{
		yp = ((yoff*2+44)*gl_lineLen+120);
		PLINE_24
		memcpy(data+yp+gl_lineLen-2160, data+yp-2160, gl_lineLen);
	}
	return touched;
}

void JoycePcwTerm::enable(bool b)
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



void JoycePcwTerm::reset(void)
{	
	m_scrEnabled = 1;
	m_scrCount = 0;
	m_colourReg = 0;
	m_prevColourReg = ~m_colourReg;
	m_scrHeight = 256;	// Full screen
	m_scrOrigin = 0;	// No vertical funnies
	if (m_sysVideo)
	{
		m_pcwBlack = m_pcwDBlack = m_sysVideo->colourToIndex(m_screenBg);
       		m_pcwWhite = m_pcwDWhite = m_sysVideo->colourToIndex(m_screenFg);
		m_beepClr  = m_sysVideo->colourToIndex(m_beepFg);
	}
	m_rollerRam = PCWRAM;		// XXX fix this

	gl_shadowInvalid = 1;
	m_borderInvalid = 1;
}


void JoycePcwTerm::onScreenTimer(void)
{
        if (++m_scrCount >= m_scrRefresh) // Draw the screen at 50Hz
        {
                drawPcwScr();
                m_scrCount=0;
        }
}


void JoycePcwTerm::onGainFocus(void)
{
	m_visible = 1;
	gl_shadowInvalid = 1;
	PcwTerminal::onGainFocus();
	border(1);
}


void JoycePcwTerm::onLoseFocus(void)
{
        m_visible = 0;
        PcwTerminal::onLoseFocus();
}



void JoycePcwTerm::drawPcwScr(void)	/* Draw the PCW screen */
{
	short y;
	register int rrvalue;
	int bpp;
	int touched = 0, ltouched = 0;
	Uint32 pxdb;
	/* Work out which columns & lines need updating */
	int tf = -1, tl = -1;
	int cf = -1, cl = -1;
	int r;

	if (!m_visible || !m_sysVideo) return;

	if (!m_sysVideo->lock()) return;	

	gl_screen = m_sysVideo->getSurface();
        bpp = m_sysVideo->m_pixw;
	gl_whi = m_pcwWhite;
	gl_blk = m_pcwBlack;
	gl_pxrs = m_sysVideo->m_pxrs;
	gl_pxgs = m_sysVideo->m_pxgs;
	gl_pxbs = m_sysVideo->m_pxbs;
	gl_lineLen = m_sysVideo->m_pitch;

	memset(gl_usedCol, 0, sizeof(gl_usedCol));
	gl_pxwr = m_sysVideo->m_colours[gl_whi].r;
	gl_pxwg = m_sysVideo->m_colours[gl_whi].g;
       	gl_pxwb = m_sysVideo->m_colours[gl_whi].b;
	gl_pxbr = m_sysVideo->m_colours[gl_blk].r;
       	gl_pxbg = m_sysVideo->m_colours[gl_blk].g;
     	gl_pxbb = m_sysVideo->m_colours[gl_blk].b;

        gl_pxw  = SDL_MapRGB(gl_screen->format, gl_pxwr, gl_pxwg, gl_pxwb);
	gl_pxb  = SDL_MapRGB(gl_screen->format, gl_pxbr, gl_pxbg, gl_pxbb);
	pxdb = (m_pcwBlack == m_pcwDBlack) ? gl_pxb : gl_pxw;


/* If you get problems with screen updates uncomment this line
 *
 *	gl_shadowInvalid = 1;
 */
	if (m_scrEnabled == 2) 	/* Screen switched off? */
	{
		SDL_Rect rc;
		rc.x = rc.y = 0;
		rc.w = 800;
		rc.h = gl_debugMode ? 300 : 600;
		SDL_FillRect(gl_screen, &rc, pxdb);
		gl_shadowInvalid = 1;	/* (v1.30) */
		touched = 1;
		m_scrEnabled = 0;
	}
	else if (m_scrEnabled && (m_colourReg & 0x40))
	{
		for (y = 0; y < m_scrHeight; y++)
		{
			register short yy=((y+m_scrOrigin)&0xFF)<<1;

			rrvalue=m_rollerRam[yy+1];
			rrvalue=rrvalue <<8;
			rrvalue+=m_rollerRam[yy];
			switch(bpp)
			{
				case 1:	ltouched = drawLine8 (PCWRAM+(rrvalue    << 1) - (rrvalue & 7),y); break;
				case 2:	ltouched = drawLine16(PCWRAM+(rrvalue    << 1) - (rrvalue & 7),y); break;
				case 3:	ltouched = drawLine24(PCWRAM+(rrvalue    << 1) - (rrvalue & 7),y); break;
				case 4:	ltouched = drawLine32(PCWRAM+(rrvalue    << 1) - (rrvalue & 7),y); break;
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
			if (gl_debugMode) { tl += 22;      tf += 22;     }
			else              { tl =  2*tl+44; tf = 2*tf+44; }

			for (y = 0; y < 90; y++) if (gl_usedCol[y])
			{
				if (cf < 0) cf = (y * 8) + 40;
				else        cl = (y * 8) + 48;	
			}

			if (cf < 0) cf = 40;
			if (cl < 0) cl = 760;
			SDL_UpdateRect(gl_screen, cf, tf, cl-cf, tl-tf); 
		}
	}	
}

void JoycePcwTerm::out(word port, byte value)
{
	switch(port & 0xFF)
	{
		case 0xF5:
		m_rollerRam = &PCWRAM[(long)value*512L];
		break;

		case 0xF6:	/* Move Y origin */
		m_scrOrigin = value;
		break;

		case 0xF7:	/* Set colour register */
		m_colourReg=value;
		if (m_prevColourReg != value)
		{
			gl_shadowInvalid = 1;	/* (v1.30) */
			m_prevColourReg = value;
			border(1);
		}			
		break;
	}
}

bool JoycePcwTerm::parseNode(xmlDocPtr doc, xmlNsPtr ns, xmlNodePtr cur)
{
	char *s;
	int r = 0;
	parseInteger(doc, ns, cur, "refresh", &r);
	if (r) m_scrRefresh = r;
	s = (char *)xmlGetProp(cur, (xmlChar *)"beep");
	if (s)
	{
		if (!strcmp(s, "border")) m_beepMode = BM_BORDER;
		if (!strcmp(s, "sound"))  m_beepMode = BM_SOUND;
		if (!strcmp(s, "none"))   m_beepMode = BM_NONE;
		xmlFree(s);
	}
	if (!parseColour(doc, ns, cur, "beep", m_beepFg, m_beepBg)) return false;
        return parseColour(doc, ns, cur, "colour", m_screenFg, m_screenBg);
}

bool JoycePcwTerm::storeNode(xmlDocPtr doc, xmlNsPtr ns, xmlNodePtr cur)
{
	storeInteger(doc, ns, cur, "refresh", m_scrRefresh);
	switch(m_beepMode)
	{
		case BM_BORDER: xmlSetProp(cur, (xmlChar *)"beep", (xmlChar *)"border"); break;
		case BM_SOUND:  xmlSetProp(cur, (xmlChar *)"beep", (xmlChar *)"sound");  break;
		case BM_NONE:   xmlSetProp(cur, (xmlChar *)"beep", (xmlChar *)"none");   break;
	}
	if (!storeColour(doc, ns, cur, "beep", m_beepFg, m_beepBg)) return false;
        return storeColour(doc, ns, cur, "colour", m_screenFg, m_screenBg);
}


void JoycePcwTerm::border(byte forced)
{
        int lblk, count, n;
        SDL_Rect rects[4];
        Uint32 pixel;

        if (!m_visible || !m_sysVideo || !(m_scrEnabled & 1)) return;

        if (m_colourReg & 0x80)  /* Colours inverted */
        {
                lblk = gl_blk = m_pcwBlack = m_pcwDWhite;
                gl_whi  = m_pcwWhite = m_pcwDBlack;
        }
	else
	{
                lblk = gl_blk = m_pcwBlack = m_pcwDBlack;
                gl_whi  = m_pcwWhite = m_pcwDWhite;
	}
        if (!forced && !m_borderInvalid) return;
        m_borderInvalid = 0;

	if (m_beepMode == BM_BORDER && m_beepStat)
        {
                lblk = m_beepClr; 
        }
	pixel  = m_sysVideo->indexToPixel(lblk);
	        memset(rects, 0, sizeof(rects));

        if (m_colourReg & 0x40)  /* Border covers screen? */
        {
                rects[0].w = rects[3].w = 800;
                rects[1].w = rects[2].w = 40;
                rects[0].h = rects[3].h = gl_debugMode ? 22 : 44;
                rects[1].y = rects[2].y = rects[0].h;
                rects[1].h = rects[2].h = gl_debugMode ? 256 : 512;
                rects[2].x = 760;
                rects[3].y = rects[1].y + rects[1].h;
                count = 4;
        }
        else
        {
                count = 1;
                rects[0].w = 800;
                rects[0].h = gl_debugMode ? 300 : 600;
        }
        if (!m_sysVideo->lock()) return;
        for (n = 0; n < count; n++)
        {
                SDL_FillRect(m_sysVideo->getSurface(), &rects[n], pixel);
        }

	m_sysVideo->unlock();
	SDL_UpdateRects(m_sysVideo->getSurface(), count, rects);
}


void JoycePcwTerm::setForeground(byte r, byte g, byte b) 
{
        m_screenFg.r = r;
	m_screenFg.g = g;
	m_screenFg.b = b;
        m_pcwDWhite = m_sysVideo->colourToIndex(m_screenFg);
	gl_shadowInvalid = 1;
	border(1);
}


void JoycePcwTerm::setBackground(byte r, byte g, byte b)
{
        m_screenBg.r = r;
	m_screenBg.g = g;
	m_screenBg.b = b; 
        m_pcwDBlack = m_sysVideo->colourToIndex(m_screenBg);
        gl_shadowInvalid = 1;
        border(1);
}

void JoycePcwTerm::onDebugSwitch(bool b)
{
	if (m_debugMode != b)
	{
		gl_debugMode = b;
		gl_shadowInvalid = 1;
		m_borderInvalid = 1;
	}
	PcwTerminal::onDebugSwitch(b);
}


PcwTerminal &JoycePcwTerm::operator << (unsigned char c)
{
	// XXX to be done
	return (*this);
}

bool JoycePcwTerm::hasSettings(SDLKey *key, string &caption)
{
	*key = SDLK_v;
	caption = "  Video  ";	
	return true;
}

UiEvent JoycePcwTerm::customClrs(UiDrawer *d)
{	
	UiEvent uie;
	int sel = -1;
 
        while (1)
        {
                UiMenu uim(d);
                uim.add(new UiLabel("  PCW screen colours  ", d));
                uim.add(new UiSeparator(d));
		uim.add(new UiColourEntry(SDLK_f, "  Foreground:  __  ", m_screenFg, d));
		uim.add(new UiColourEntry(SDLK_b, "  Background:  __  ", m_screenBg, d));
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
		m_pcwBlack = m_pcwDBlack = m_sysVideo->colourToIndex(m_screenBg);
       		m_pcwWhite = m_pcwDWhite = m_sysVideo->colourToIndex(m_screenFg);
		m_beepClr  = m_sysVideo->colourToIndex(m_beepFg);
			return UIE_OK;
			default:		break;
		}
	} 
	return UIE_CONTINUE;
}


UiEvent JoycePcwTerm::settings(UiDrawer *d)
{
        int x,y,sel;
        UiEvent uie;
	bool green, white;
	char upd[50];
	double upf;

        x = y = 0;
        sel = -1;
        while (1)
        {
                UiMenu uim(d);
		green = (m_screenFg.r == 0    && 
			 m_screenFg.g == 0xC0 && 
			 m_screenFg.b == 0x54 &&
			 m_screenBg.r == 0    &&
			 m_screenBg.g == 0    &&
			 m_screenBg.b == 0);
		white = (m_screenFg.r == 0xFF && 
			 m_screenFg.g == 0xFF && 
			 m_screenFg.b == 0xFF &&
			 m_screenBg.r == 0    &&
			 m_screenBg.g == 0    &&
			 m_screenBg.b == 0);

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
		uim.add(new UiSetting(SDLK_g, green, "  Green screen  ", d));
		uim.add(new UiSetting(SDLK_w, white, "  White screen  ", d));
                uim.add(new UiSetting(SDLK_c, !(green||white), "  Custom colours...",  d));
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
			case SDLK_g: setForeground(0x00, 0xC0, 0x54);
				     setBackground(0x00, 0x00, 0x00);
				     break;
			case SDLK_w: setForeground(0xFF, 0xFF, 0xFF);
				     setBackground(0x00, 0x00, 0x00);
				     break;
			case SDLK_c: uie = customClrs(d);
				     if (uie == UIE_QUIT) return uie;
				     break;
			case SDLK_ESCAPE: return UIE_OK;
			default: break;
		}
	
	}
	return UIE_CONTINUE;
}



void JoycePcwTerm::tick(void)
{
	if (m_beeper) m_beeper->tick();
}



void JoycePcwTerm::beepOn(void)
{
	PcwTerminal::beepOn();
	switch(m_beepMode)
	{
		case BM_BORDER:  m_borderInvalid = 1; border(1); break;
		case BM_SOUND:   if (m_beeper) m_beeper->beepOn(); break;
		default:	 break;
	}
}

void JoycePcwTerm::beepOff(void)
{
	PcwTerminal::beepOff();
	switch(m_beepMode)
	{
		case BM_BORDER:  m_borderInvalid = 1; border(1); break;
		case BM_SOUND:   if (m_beeper) m_beeper->beepOff(); break;
		default:	 break;
	}
}


void JoycePcwTerm::deinitSound(void)
{
	if (m_beeper) m_beeper->deinitSound();
}

void JoycePcwTerm::initSound(void)
{
	if (m_beeper) m_beeper->initSound();
}

