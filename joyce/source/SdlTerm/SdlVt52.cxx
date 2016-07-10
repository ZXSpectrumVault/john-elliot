/************************************************************************

    SdlTerm v1.00: Terminal emulation in an SDL framebuffer
                  SdlVt52 parses escape sequences and passes them
                  down to SdlTerm for action.

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

#include <SDL.h>
#include "SdlContext.hxx"
#include "SdlTerm.hxx"
#include "SdlVt52.hxx"

SdlVt52::SdlVt52(SDL_Surface *s, SDL_Rect *bounds) :
	SdlTerm(s, bounds)
{
	m_esc = 0;	
}


SdlVt52::~SdlVt52()
{
}


void SdlVt52::writeSys(unsigned char c)
{
	if (!isStatusEnabled()) write(c);
	else switch(c)
	{
		case 0x07: bel(); return;
		case 0x0A:
		case 0x0D: clearStatus(); return;
		default:   SdlTerm::writeSys(c); return;
	}
}



void SdlVt52::write(unsigned char c)
{
	if (!m_esc) switch(c)
	{
		case 27: m_esc = 27; return;
		case 13: cr();     return;
		case 10: lf();     return;
		case  8: cursorLeft(true); return;
		case  7: bel();	   return;
		
		default: if (c < 32) return;
			 SdlTerm::write(c); 
			 return;
	}
	// m_esc = 27: First byte of an ESC code
	if (m_esc == 27) switch(c)
	{
		default: 	SdlTerm::write(c); m_esc = 0; return;

		case '0': enableStatus(false); m_esc = 0; return;
		case '1': enableStatus();      m_esc = 0; return;
		case '2': escMulti(c, 2);                 return;
                case '3': escMulti(c, 2);                 return;
                case '4': escMulti(c, 2);                 return;
                case '5': escMulti(c, 2);                 return;

		case 'A': cursorUp(false);    m_esc = 0; return;
		case 'B': cursorDown(false);  m_esc = 0; return;
		case 'C': cursorRight(false); m_esc = 0; return;
		case 'D': cursorLeft(false);  m_esc = 0; return;
		case 'E': clearWindow();      m_esc = 0; return;
		case 'H': home();             m_esc = 0; return;
		case 'I': cursorUp(true);     m_esc = 0; return;
		case 'J': clearEOS();         m_esc = 0; return;
		case 'K': clearEOL();         m_esc = 0; return;
		case 'L': insertLine();       m_esc = 0; return;
		case 'M': deleteLine();       m_esc = 0; return;
		case 'N': deleteChar();       m_esc = 0; return;
		case 'X': escMulti(c, 5);                return;
		case 'Y': escMulti(c, 3);                return;
		case 'b': escMulti(c, 2);                return;
		case 'c': escMulti(c, 2);                return;
		case 'd': clearBOS();        m_esc = 0;  return;
		case 'e': enableCursor();    m_esc = 0;  return;
		case 'f': enableCursor(false);m_esc = 0; return;
		case 'j': saveCursorPos();   m_esc = 0;  return;
		case 'k': loadCursorPos();   m_esc = 0;  return;
		case 'l': clearCurLine();    m_esc = 0;  return;
		case 'o': clearBOL();        m_esc = 0;  return;
		// Special effects
		case 'p': setReverse();      m_esc = 0; return;
		case 'q': setReverse(false); m_esc = 0; return;
                case 'r': setUnderline();    m_esc = 0; return;
                case 's': setAltFont();      m_esc = 0; return;
                case 't': setAltFont(false); m_esc = 0; return;
                case 'u': setUnderline(0);   m_esc = 0; return;
		case 'v': setWrap();	     m_esc = 0; return;
		case 'w': setWrap(false);    m_esc = 0; return;
		case 'x': setCentred(80,24); m_esc = 0; return;
		case 'y': setCentred(90,32); m_esc = 0; return;
		case 'z': setMaxWin();       m_esc = 0; return;
	}
	else
	{
		m_escBuf[m_esc++] = c;
		--m_escCount;
		if (!m_escCount) 
		{
			int r,g,b;

			m_esc = 0;
			switch(m_escBuf[0])
			{
				case '2': setLanguage(m_escBuf[1] & 7); break;
				case '3': setMode    (m_escBuf[1] & 3); break;
				case '4': setPageMode(m_escBuf[1] & 1); break;
				case 'X': setWindow (m_escBuf[2] - 0x20,
						     m_escBuf[1] - 0x20,
                                                     m_escBuf[4] - 0x1F,
						     m_escBuf[3] - 0x1F); break;
				case 'Y': moveCursor(m_escBuf[2] - 0x20,
						     m_escBuf[1] - 0x20); break;
				case 'b':
				case 'c':
					b = ( (m_escBuf[1] - 0x20)       & 3) * 85;
					g = (((m_escBuf[1] - 0x20) >> 2) & 3) * 85;
					r = (((m_escBuf[1] - 0x20) >> 4) & 3) * 85;
					if (m_escBuf[0] == 'b') setForeground(r,g,b);
					else			setBackground(r,g,b);
					break;				
	
			}
		}
	}	
}

//
// Prepare m_escBuf to store a multi-byte escape code
// m_escCount holds no. of bytes remaining
// m_esc      holds no. of bytes in buffer so far
// m_escBuf   holds data, eg "Y<foo><bar>"
//
void SdlVt52::escMulti(unsigned char first, int count)
{
	m_escCount = count - 1;
	m_esc = 1;
	m_escBuf[0] = first;	
}



#if 0

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

#include "Joyce.hxx"
#include "Joycevga.hxx"

/* The state of the 800x600 colour screen. This would have to be 
  moved to a struct if snapshots were to be implemented */

static byte wtop   = 0,	/* Viewport dimensions */ 
	    wleft  = 0, 
	    wbot   = 36, 
	    wright = 100;

static byte autox = 1;	/* Set to 1 if the viewport should automatically
			 * expand when the status line is disabled 
		 	 */
static byte statln= 1;	/* Is the status line enabled? */
static byte rv    = 0;	/* Reverse video enabled? */
static byte italic= 0;	/* Italic, grey text, bold, underlined etc. */
static byte grey  = 0;
static byte bold  = 0;
static byte ul    = 0;
static byte wrap  = 1;	/* Wrapping at EOL? */
static byte onesc = 0;	/* Set if in an escape code */
static byte onplus = 0;	/* Set if in a CRTPLUS sequence */
#ifndef USE_SDL
static byte curbm[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 255 };
#endif
			/* The cursor bitmap - an underline */
static byte statln_x = 0;	/* Position of cursor in status line */
static byte pgmode = 0;		/* screen page mode */
static byte lang = 0;		/* Current language */


#define SCRBOT (statln ? 36 : 37)	/* Bottom line of the screen */


/* STUB */

void sel_pgmode(byte c)
{
	pgmode = c;
}


byte lang1[]={	0x40, 0xEA,	/* Language 1 : @ -> a grave */ 
		0x5B, 0xA2,	/*              [ -> degree */
		0x5C, 0xF5,	/* 		\ -> c cedilla */
		0x5D, 0xA6,	/*		] -> Section */
		0x7B, 0xE1,	/*		{ -> e acute */
		0x7C, 0xEE,	/*		| -> u grave */
		0x7D, 0xEB,	/*		} -> e grave */
		0x7E, 0xB2,	/*		~ -> umlaut */
		0x00, 0x00	};

byte lang2[]={	0x40, 0xA6,	/* Language 2 : @ -> Section */ 
		0x5B, 0xD0,	/*              [ -> A umlaut */
		0x5C, 0xD3,	/* 		\ -> O umlaut */
		0x5D, 0xD4,	/*		] -> U umlaut */
		0x7B, 0xF0,	/*		{ -> a umlaut */
		0x7C, 0xF3,	/*		| -> o umlaut*/
		0x7D, 0xF4,	/*		} -> u umlaut */
		0x7E, 0xBA,	/*		~ -> double s */
		0x00, 0x00	};

byte lang3[]={	0x23, 0xA3,	/* Language 3 : # -> œ */
		0x00, 0x00	};

byte lang4[]={	0x5B, 0xD6,	/* Language 4 : [ -> AE */
		0x5C, 0xD8,	/* 		\ -> O slash */
		0x5D, 0xD7,	/*		] -> A ring */
		0x7B, 0xF6,	/*		{ -> ae */
		0x7C, 0xF8,	/*		| -> o slash*/
		0x7D, 0xF7,	/*		} -> a ring */
		0x00, 0x00	};

byte lang5[]={	0x40, 0xC1,	/* Language 5 : @ -> E acute */
		0x5B, 0xD0,	/*              [ -> A umlaut */
		0x5C, 0xD3,	/* 		\ -> O umlaut */
		0x5D, 0xD7,	/*		] -> A ring */
		0x5E, 0xD4,	/*		^ -> U umlaut */
		0x7B, 0xF0,	/*		{ -> a umlaut */
		0x7C, 0xF3,	/*		| -> o umlaut*/
		0x7D, 0xF7,	/*		} -> a ring */
		0x7E, 0xF4,	/*		~ -> u umlaut */
		0x00, 0x00	};

byte lang6[]={	0x5B, 0xA2,	/* Language 6 : [ -> degree */
		0x5D, 0xE1,	/*		] -> e acute */
		0x60, 0xEE,	/*		` -> u grave */
		0x7B, 0xEA,	/*		{ -> a grave */
		0x7C, 0xED,	/*		| -> o grave */
		0x7D, 0xEB,	/*		} -> e grave */
		0x7E, 0xEC,	/*		~ -> i grave */
		0x00, 0x00	};

byte lang7[]={	0x5B, 0xAF,	/* Language 7 : [ -> upside-down pling */
		0x5C, 0xD9,	/*		\ -> N tilde */
		0x5D, 0xAE,	/*		] -> upside-down query */
		0x7B, 0xB2,	/*		{ -> umlaut */
		0x7C, 0xF9,	/*		| -> n tilde */
		0x00, 0x00	};



byte *swaptbl[8] = 	/* Character swaps in language <n> */
{
	NULL,			/* Language 0 : no swaps */
	lang1, lang2, lang3,
	lang4, lang5, lang6, lang7
};


void lswap(int lang)
{
	byte *swapp = swaptbl[lang];
	byte b1, b2;
	byte swbuf[16];	

	if (!lang) return;

	do {
		b1 = swapp[0];
		b2 = swapp[1];
		swapp += 2;
		memcpy(swbuf, scrfont + 16 * b1, 16);
		memcpy(scrfont + 16 * b1, scrfont + 16 * b2, 16);
		memcpy(scrfont + 16 * b2, swbuf, 16);		
	} while (b1 != b2);
}



void sel_language(byte c)
{
	if (lang) lswap(lang);
	lang = c;
	lswap(lang);
}


void crtplus(byte c)	/* Deal with codes after ESC + */
{
	onesc = 0;

	switch(c)
	{
		case 'b': bold = 1;	break;
		case 'g': grey = 1;	break;
		case 'i': italic = 1;	break;
	}

}


void crtminus(byte c)	/* Deal with codes after ESC - */
{
	onesc = 0;

	switch(c)
	{
		case 'b': bold = 0;	break;
		case 'g': grey = 0;	break;
		case 'i': italic = 0;	break;
	}
}


void vga_bel(void)
{
#ifdef __MSDOS__
	putch(7);
#else
	/* XXX fix this */
	putchar(7);
#endif
}


void vga_moveto(byte y, byte x)	/* Position the cursor */
{
	vga_x = wleft + x - 32;
	vga_y = wtop +  y - 32;
	if (vga_x >= wright) vga_x = wright + 1;
	if (vga_y >= wbot)   vga_y = wbot + 1;
}


__inline__ void vga_cls(void)
{
	/* Clear the viewport */
	GrClearClipBox(vga_bg);	/* v1.22 dblk -> vga_bg */
}


void set_window(void)	/* Set the viewport size and position */
{
	int y1 = wtop  * 16;
	int x1 = wleft * 8;
	int y2 = wbot  * 16;
	int x2 = wright * 8;

	if (vga_x < wleft)   vga_x = wleft;
	if (vga_x >= wright) vga_x = wright - 1;
	if (vga_y < wtop)    vga_y = wtop;
	if (vga_y >= wbot)   vga_y = wbot - 1;

	GrSetClipBox(x1, y1, x2, y2);
}



void vga_setwin(byte *p)	/* Validate the viewport size & position */
{
	wtop   = p[0] - 32;         if (wtop   >= SCRBOT) wtop   = SCRBOT - 1;
	wleft  = p[1] - 32;         if (wleft  >= 100)    wleft  = 99;
	wbot   = wtop  + p[2] - 31; if (wbot   >= SCRBOT) wbot   = SCRBOT - 1;
	wright = wleft + p[3] - 31; if (wright >= 100)    wright = 100;
	set_window();
}


#ifndef USE_SDL

int colour_find(int r, int g, int b)	/* Search for a colour - if not */
{					/* found, allocate it */
	int n;
	int tr, tg, tb;
	for (n = 0; n < 255; n++)
	{
		GrQueryColor(n, &tr, &tg, &tb); 
		if (tr == r && tg == g && tb == b) return n;
	}
	n = GrAllocColor(r,g,b);
	if (n != GrNOCOLOR) return n;

/* The top 128 colours can be freed when we need to */

	GrFreeColor(colour_nextfree++);
	if (colour_nextfree == 256) colour_nextfree = 128;
	return GrAllocColor(r, g, b);
}



void set_ink(int * ink, int r, int g, int b)	/* Set foreground or background */
{
	int ocol = *ink;

	(*ink) = colour_find(r,g,b);

	if (whi == ocol) whi = (*ink);
	if (blk == ocol) blk = (*ink);
}
#endif



void set_fg(int *ink, byte c)	/* Set ink to one of 64 standard colours */
{
	c -= 0x20; /* 0 - 63 */

	set_ink(ink, ((c & 0x30) << 2), ((c & 0x0C) << 4), ((c & 0x03) << 6));
}





void vga_setmode(byte mode)	/* Switch between PCW mode and native mode */
{
	if (mode == ScrType) return;

	switch(mode)
	{
		case 0: /* PCW */
		sel_language(0);
		shadow_invalid = 1;
		ScrType = 0;
		break;

		case 1:	/* SVGA */
		GrClearScreen(dblk);
		vga_fg  = dwhi;
		vga_bg  = dblk;
		rv      = 0;
		ul      = 0;
		bold 	= 0;
		italic	= 0;
		grey	= 0;
		onesc   = 0;
		onplus  = 0;
		wrap    = 1;
		wtop    = 0;
		wleft   = 0;
		pgmode	= 0;
		statln  = 1;
		wbot    = SCRBOT;
		wright  = 100;
		vga_curon   = 1;
		vga_curvis  = 0;
		autox   = 1;
		set_window();
		sel_language(0);
#ifdef USE_SDL
	        reload_palette();
#endif
		UpdateArea(0, 0, 0, 0);	/* Update whole screen */
		ScrType = 1;
		break;

		default:
		joyce_dprintf("Unknown screen mode %d\n", mode);
		break;
	}
}


void vga_drawcur(void)	/* XOR the cursor onto the screen */
{			/* (v1.22) vga_drawcur is now public */
	int vx = 8*vga_x;
	int vy = 16*vga_y;

#ifdef USE_SDL
	xor_block(vy,   vx, 16, 8);
	UpdateArea(vx, vy,  8, 16);
#else
        draw_bitmap(vy,   vx, 16, 8, curbm, GrWhite() | GrXOR, GrXOR);
#endif
}


static __inline__ void vga_blit(int dx, int dy, int sx, int sy, int sx2, int sy2)
{
#ifdef USE_SDL
	SdlScrBlit(dx, dy, sx, sy, sx2, sy2);
#else	
	GrBitBltNC(NULL, dx, dy, NULL, sx, sy, sx2, sy2, 0);
#endif
}




static void scroll_up(void)	/* Scroll the viewport up */
{
	int offs_l = 8  * wleft;
	int offs_r = 8  * wright - 1;
	int offs_t = 16 * wtop;
	int offs_b = 16 * wbot - 1;

	vga_blit(offs_l, offs_t, offs_l, offs_t + 16, offs_r, offs_b);

	GrFilledBoxNC(offs_l, offs_b - 15, offs_r, offs_b, vga_bg);  /* v1.22 dblk -> vga_bg */
	UpdateArea(offs_l, offs_t, offs_r - offs_l, offs_b - offs_t);
}




static void escm(void)	/* Scroll part of the viewport up */
{
	int offs_l = 8  * wleft;
	int offs_r = 8  * wright;
	int offs_t = 16 * vga_y;
	int offs_b = 16 * wbot;

	vga_blit(offs_l, offs_t, offs_l, offs_t + 16, offs_r, offs_b);

	GrFilledBoxNC(offs_l, offs_b - 16, offs_r, offs_b, vga_bg); /* v1.22 dblk -> vga_bg */
        UpdateArea(offs_l, offs_t, offs_r - offs_l, offs_b - offs_t);
}




static void esck(void)	/* Delete to EOL */
{
	int offs_l = 8  * vga_x;
	int offs_r = 8  * wright;	
	int offs_v = 16 * vga_y;

	GrFilledBox(offs_l, offs_v, offs_r, offs_v + 16, vga_bg); /* v1.22 dblk -> vga_bg */
        UpdateArea(offs_l, offs_v, offs_r - offs_l, 16);
}



static void esco(void)	/* Delete to BOL */
{
	int offs_l = 8  * wleft;
	int offs_r = 8  * vga_x + 7;
	int offs_v = 16 * vga_y;

	GrFilledBox(offs_l, offs_v, offs_r, offs_v + 15, vga_bg); /* v1.22 dblk -> vga_bg */
        UpdateArea(offs_l, offs_v, offs_r - offs_l, 16);
}


static void escll(void)	/* Delete line */
{
	int offs_l = 8  * wleft;
	int offs_r = 8  * wright;
	int offs_v = 16 * vga_y;

	GrFilledBox(offs_l, offs_v, offs_r, offs_v + 16, vga_bg); /* v1.22 dblk -> vga_bg */
        UpdateArea(offs_l, offs_v, offs_r - offs_l, 16);
}




static void escn(void)	/* Delete a character */
{

	int offs_l = 8  * vga_x;
	int offs_r = 8  * wright;	
	int offs_v = 16 * vga_y;

	vga_blit(offs_l,     offs_v, offs_l + 8, offs_v, offs_r, offs_v + 16);

	GrFilledBoxNC(offs_r - 8, offs_v, offs_r, offs_v + 16, vga_bg); /* v1.22 dblk -> vga_bg */
        UpdateArea(offs_l, offs_v, offs_r - offs_l, 16);

}



static void clreos(void)	/* Clear to End Of Screen */
{
	int offs_l = 8  * vga_x;
	int offs_r = 8  * wright;
	int areah  = (wbot - vga_y) * 16;
	int offs_v = vga_y * 16;

	GrFilledBox(offs_l, offs_v, offs_r, offs_v + 16, vga_bg); /* v1.22 dblk -> vga_bg */

	offs_l = 8  * wleft;

	GrFilledBox(offs_l, offs_v + 16, offs_r, offs_v + areah, vga_bg); /* v1.22 dblk -> vga_bg */
        UpdateArea(offs_l, offs_v, offs_r - offs_l, areah);
}


static void clrbos(void)	/* Clear to Beginning Of Screen */
{
	int offs_l = 8  * wleft;
	int offs_r = 8  * vga_x ;	
	int offs_v = 16 * vga_y;
	int offs_t = 16 * wtop;

	GrFilledBox(offs_l, offs_v, offs_r, offs_v + 16, vga_bg); /* v1.22 dblk -> vga_bg */
        UpdateArea(offs_l, offs_v, offs_r - offs_l, 16);

	offs_r = 8  * wright;

	GrFilledBox(offs_l, offs_t, offs_r, offs_v, vga_bg); /* v1.22 dblk -> vga_bg */
        UpdateArea(offs_l, offs_t, offs_r - offs_l, offs_v - offs_t);
}




static void scroll_down(void)	/* Scroll the whole screen down */
{
	int offs_l = 8  * wleft;
	int offs_r = 8  * wright;
	int offs_t = 16 * wtop;
	int offs_b = 16 * wbot;

	vga_blit(offs_l, offs_t + 16, offs_l, offs_t, offs_r, offs_b - 16);

	GrFilledBoxNC(offs_l, offs_t, offs_r, offs_t + 16, vga_bg); /* v1.22 dblk -> vga_bg */
        UpdateArea(offs_l, offs_t, offs_r - offs_l, offs_b - offs_t);
}


static void escl(void)	/* Scroll part of the screen down */
{
	int offs_l = 8  * wleft;
	int offs_r = 8  * wright;
	int offs_t = 16 * vga_y;
	int offs_b = 16 * wbot;

	vga_blit(offs_l, offs_t + 16, offs_l, offs_t, offs_r, offs_b - 16);

	GrFilledBoxNC(offs_l, offs_t, offs_r, offs_t + 16, vga_bg); /* v1.22 dblk -> vga_bg */
        UpdateArea(offs_l, offs_t, offs_r - offs_l, offs_b - offs_t);
}


	
void statenable(void)	/* Enable the status line */
{
	if (wbot == 37) --wbot;
	if (vga_y == 36) { --vga_y; scroll_up(); }
	statln = 1;
}


void statdisable(void)	/* Disable the status line */
{
	if (autox && wbot == 36) ++wbot;
	statln = 0;
}


/* Draw a bitmapped character */

void draw_bitmap(int y, int x, int h, int w, byte *bm, int fg, int bg)
{
	int vx, vy;
	byte b = 0;
	int nb = 0;

	for (vy = y; vy < y+h; vy++)
	{
		for (vx = x; vx < x+w; vx++)
		{
			if (!nb) b = *(bm++);
			if (b&0x80)  GrPlotNC(vx, vy, fg);
			else         GrPlotNC(vx, vy, bg);
			b = b << 1;
			++nb; 
			if (nb == 8) nb = 0;
		}
	 }
        UpdateArea(x, y, w, h);
}


static void draw_char(byte c)	/* Apply bold, italic etc. to character and 
				 * draw it */
{
	static byte intbm[32];
	int n;

	byte *bm = scrfont+16*c;

	int vx = 8*vga_x;
	int vy = 16*vga_y;

	memcpy(intbm, bm, 16);

	if (bold)   for (n = 0; n < 16; n++) intbm[n] |= (intbm[n] >> 1);
	if (grey)   for (n = 0; n < 16; n++) intbm[n] &= (n & 1 ? 0x55 : 0xAA);
	if (italic) 
	{
		for (n = 0;  n < 2;  n++) intbm[n] = intbm[n] >> 2; 
		for (n = 2;  n < 6;  n++) intbm[n] = intbm[n] >> 1;
		for (n = 10; n < 14; n++) intbm[n] = intbm[n] << 1; 
		for (n = 14; n < 16; n++) intbm[n] = intbm[n] << 2; 
	}

	intbm[15] |= ul;
	if (rv) draw_bitmap(vy, vx, 16, 8, intbm, vga_bg, vga_fg); /* v1.22 dblk -> vga_bg */
	else    draw_bitmap(vy, vx, 16, 8, intbm, vga_fg, vga_bg); /* v1.22 dwhi -> vga_fg */
}

void __inline__ linefeed(void)	/* Feed a line */
{
	vga_x = wleft;
	vga_y++; 
	if (vga_y == wbot) { --vga_y; scroll_up(); }
}

void __inline__ charfeed(void)	/* Move right 1 character */
{
	vga_x++;
	if (vga_x >= wright)
	{
		if (!wrap) vga_x--; else linefeed();
	}
}

static void set_win(int l, int t, int r, int b)
{
	wleft  = l;
	wright = r;
	wtop   = t;
	wbot   = b;
	set_window();
	vga_cls();
}



static void escode(byte c)	/* Handle part of an escape code */
{
	static byte p[4];
	static byte ox, oy;

	if (onesc == 27) switch(c)	/* First byte after CHR$(27) */
	{
		case '-':			/* CRTPLUS */
		case '+': onesc = c;	break;

		case '0': statdisable(); break;
		case '1': statenable();	 break;
		case '2':
		case '3': 			/* 2-digit codes */
		case '4': onesc = c;			   break;
		case 'A': if (vga_y > wtop)       --vga_y; break;
		case 'B': if (vga_y < wbot - 1)   ++vga_y; break;
		case 'C': if (vga_x < wright - 1) ++vga_x; break;
		case 'D': if (vga_x > wleft)      --vga_x; break;
		case 'E': vga_cls(); break;

		case 'H': vga_x = wleft;
			  vga_y = wtop;
			  break;
		case 'I': --vga_y;
			  if (vga_y >= wtop) break;
			  vga_y = wtop;
			  scroll_down();
			  break;
		case 'J': clreos();  break;
		case 'K': esck();    break;
		case 'L': escl();    break;
		case 'M': escm();    break;
		case 'N': escn();    break;
		case 'X': onesc = 3; break;
		case 'Y': onesc = 1; break;

		case 'b': onesc = 'b'; break;
		case 'c': onesc = 'c'; break;
		case 'd': clrbos();  break;
		case 'e': vga_curon = 1; break;
		case 'f': vga_curon = 0; break;

		case 'j': ox = vga_x; oy = vga_y; break;
		case 'k': vga_x = ox; vga_y = oy; break;
		case 'l': escll(); break;
		case 'o': esco();  break;

		case 'p': rv = 1;  break;
		case 'q': rv = 0;  break;

		case 'r': ul = 0xFF; break;
		case 'u': ul = 0;    break;
	
		case 'v': wrap = 1; break;
		case 'w': wrap = 0; break;
		
		case 'x': autox = 1; set_win(10,6, 90,30);	break;	   /* 24x80  */
		case 'y': autox = 1; set_win( 5,3, 95,35);	break;	   /* 32x90  */
		case 'z': autox = 1; set_win( 0,0,100,SCRBOT);	break; /* 37x100 */

		default: draw_char(c);
			 charfeed();
			 break;
	}
	else switch(onesc)
	{
		case 1:  p[0] = c; onesc = 2;            break; /* ESC Y byte 2 */
		case 2:  vga_moveto(p[0], c); onesc = 0; break; /* ESC Y byte 3 */
		case 3:  p[0] = c; onesc = 4;            break; /* ESC X */
		case 4:  p[1] = c; onesc = 5;            break; /* ESC X */
		case 5:  p[2] = c; onesc = 6;            break; /* ESC X */
		case 6:  p[3] = c; autox = 0; 
			 vga_setwin(p);
			 onesc = 0;            		 break; /* ESC X */

		case '2': sel_language(c&7); onesc = 0;	break;	/* ESC 2 */

		case '3': autox = 1;	/* Window selected automatically */
			  onesc = 0;
			  switch (c & 3)
		          {
				case 0: 
				case 1: set_win( 5,3, 95, 35); 	break;    /* 32x90 */
				case 2: set_win(10,6, 90, 30); 	break; 	  /* 24x80 */
				case 3: set_win( 0,0,100, SCRBOT); break; /* 37x100 */
			  }
			  break;
		case '4': sel_pgmode(c); onesc = 0; break; /* ESC 4 */

		case '+': crtplus(c);	 break;		/* CRTPLUS */
		case '-': crtminus(c);	 break;

		case 'b': set_fg(&dwhi, c);   vga_fg = dwhi; onesc = 0; break; 
		case 'c': set_fg(&dblk, c);   vga_bg = dblk; onesc = 0; break; /* v1.22 dblk -> vga_bg */
	
		default: onesc = 0;
	}
	if (onesc == 27) onesc = 0;	
}





void vga_putchar(byte c)	/* Deals with characters sent by CP/M */
{
	if (ScrType != 1) vga_setmode(1);
	if (vga_curon && vga_curvis) { vga_drawcur(); vga_curvis = 0; }
	if (onesc) escode(c);
	else switch(c)
	{
		case 7:  vga_bel();	/* BEL */
			 break;

		case 8: if (wrap || vga_x) --vga_x;	/* BS */
			if (vga_x >= wleft) return;
			vga_x = wright - 1;
			--vga_y;
			if (vga_y >= wtop) return;
			vga_y = wtop;
			scroll_down();
			break;

		case 10: linefeed();	/* LF */
			 break;

		case 13: vga_x = wleft; /* CR */
			 break;

		case 27: onesc = 27;	/* ESC */
			 break;

		default: if (c > 31)
			 {
				draw_char(c);
			 	charfeed();
			 }
			 break;
	}
}


void vga_timer(void)	/* If the cursor's enabled, draw it. We do this */
			/* using the timer so that it doesn't flicker so */
			/* much during output */
{
	static int curtimer = 0, flstimer = 0;

	if (!ScrType) return; 
	if (vga_curon != vga_curvis && !curtimer) 
	{
		vga_drawcur(); 
		vga_curvis = vga_curon; 
	}
	++curtimer; 
	++flstimer;
	if (curtimer == 10) curtimer = 0;
}


void clear_statln(void)	/* Clear the status line */
{
	GrFilledBoxNC(0, 576, 799, 599, vga_bg); /* v1.22 dblk -> vga_bg */
	UpdateArea(0, 576, 800, 24);
	statln_x = 0;
}



static void draw_slchar(byte c)	/* Put a character in the status line */
{
	byte *bm = scrfont+16*c;
	int vx = 8*statln_x;

	draw_bitmap(576, vx, 16, 8, bm, vga_fg, vga_bg); /* v1.22 dblk -> vga_bg */
}


void vga_putmch(byte c)	/* Deal with system messages */
{
	if (!statln)
	{
		vga_putchar(c);
		return;
	}

	switch(c)
	{
		case 0x07:
		vga_bel();
		break;

		case 0x0D:
		case 0x0A:
		clear_statln();
		break;

	
		default:	
		draw_slchar(c);
		++statln_x;
		if (statln_x == 100) statln_x = 0;
		break;
	}
}


void vga_cursor(byte on)	/* (v1.22) Ensure the cursor is visible */
{				/* or invisible. Call immediately after */
	if (on)			/* clearing the screen */
	{
		vga_curon = vga_curvis = 1;
		vga_drawcur();
	}
	else
	{
		vga_curon = vga_curvis = 0;
	}
}

#endif
