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

#include "Pcw.hxx"
#include "inline.hxx"

#define P1_WORD R->fetchW(R->sp + 2)
#define P2_WORD R->fetchW(R->sp + 4)
#define P3_WORD R->fetchW(R->sp + 6)
#define P4_WORD R->fetchW(R->sp + 8)

#define P1_BYTE R->fetchB(R->sp + 2)
#define P3_BYTE R->fetchB(R->sp + 6)	/* After 2 words */
#define P4_BYTE R->fetchB(R->sp + 7)
#define P5_BYTE R->fetchB(R->sp + 10)	/* After 4 words */
#define P6_BYTE R->fetchB(R->sp + 11)


/* LIOS interface:
 
  E=function number
 SP->stack frame:
	DW	return		or	DW	return
	DW	param1			DB	param1 
	DW	param2			DB	param2
	...				...

functions are:

E=	Parameters 	Description
--------------------------------------------------------------------
0:	none		Initialise. By this time, the info block has
			already been used to set up the screen size;
			so the Amstrad LIOS (and the JOYCE LIOS) 
			change it when the RSX is awakened (function 
			0FFh below) and then restart the COM file 
			with a JP 100h. This routine, which will be
			called the second time around, is therefore
			a no-op.
1:	word 0		Terminate
2:	none		Turn on text mode, no graphics
3:	byte line	Set split line (no. of text lines visible)
4:	none		CONIN - return in HL
5:	none		CONST - return in HL
6:	byte ch		CONOUT - print char if text screen available
7:	word T,		Clear text window from T to B
	word B
8:	word X,		Move text cursor to X,Y
	word Y
9:	word T,		Scroll text window from T to B
	word B
10:	word X,		Plot a point. pen is 2 for PX else PD
	word Y,		
	byte pen,
	byte colour
11:	word X1,	Draw a line from X1,Y1 to X2,Y2
	word Y1,
	word X2,
	word Y2,
	byte pen,
	byte colour
12:	word colour     Set background colour?
13:	word colour,	Clear screen - height=pixels
	word pixels
14:	none		Beep
15:	none		Get top of memory
16:	word X,		Read pixel colour into L
	word Y
17:	none		LISTST
18:	byte ch		LIST
19-23:	none		no-ops? Not called by DR Logo
24:	word BUF,	Generate record <RECNO> of a picture savefile.
	word RECNO	Return 1 in HL if more records follow
25:	word BUF,	Load record <RECNO> of a picture savefile.
	word RECNO

26:	none?		These are not hooked in the Amstrad LIOS or JOYCE, but
27:	none?		are used for 'pal' and 'setpal' respectively.

0FFh: 	word PARAMS	Initialise. Enter with HL->message buffer, write 
			message here if there is a failure. Return HL=0
			if failure, else 1

PARAMS:	DW	0	;+0	Text screen height, chars
	DW	0	;+2	Text screen width, chars
	DW	0	;+4	Text screen height, chars
	DW	0	;+6	Text screen width, chars
	DW	0	;+8	Standard split line, chars
	DW	0	;+A	Default foreground colour
	DW	0	;+C	Default background colour
	DW	0	;+E
	DW	0	;+10	Number of foreground colours - 1
	DW	0	;+12
	DW	0	;+14
	DW	0	;+16
	DW	0	;+18	Graphics screen width, pixels
	DW	0	;+1A	Graphics screen height, pixels
	DW	0	;+1C
	DW	0	;+1E	Maximum scrunch ratio
	DW	0	;+20	1 if split screen disabled?
	DW	0	;+22	Number of background colours - 1
	DW	0	;+24	Text screen height, chars
	DW	0	;+26	Scrunch ratio, numerator
	DW	0	;+28	Scrunch ratio, denominator
	DW	0	;+2A	->Version-specific title string, ASCIIZ
	DW	0	;+2C	->"Edit", ASCIIZ
	DW	0	;+2E	->"_",    ASCIIZ

*/

PcwLogoTerm::PcwLogoTerm(PcwSystem *s) : PcwSdlTerm("logo", s)
{
	XLOG("PcwLogoTerm::PcwLogoTerm()");
}


PcwLogoTerm::~PcwLogoTerm()
{
	XLOG("PcwLogoTerm::~PcwLogoTerm()");
}



void PcwLogoTerm::doSplit(void)
{
	SDL_Rect &rc = m_term->getBounds();

	if (!m_splitSz) m_gfxY = rc.h;
	else            m_gfxY = 16 * m_splitSz; 
}



int PcwLogoTerm::makerop(byte pen, byte *colour)
{
	switch(pen)
	{
		default: return 0;		/* pd */
		case 2:  *colour = 0; return 2;	/* pu */
		case 3:	 return 3;		/* px */
		case 4:  *colour = 0; return 0;	/* pe */
	}
}



void PcwLogoTerm::zstrcpy(word dest, char *src)
{
	do
	{
		m_sys->m_cpu->storeB(dest++, *src);
	} while (*src++);
}

word PcwLogoTerm::lios_init(word liospb, word msgbuf)
{
	char title[90];
	SDL_Rect rcPixels, rcChars;

	sprintf(title,"Amstrad LOGO v2.0 under JOYCE %d.%d.%d", 
			BCDVERS >> 8, (BCDVERS & 0xF0) >> 4, BCDVERS & 0x0F);

//
// Switch from PCW screen mode into VGA
//
	m_oldTerm = m_sys->m_activeTerminal;
	m_sys->selectTerminal(this);

	m_term = new SdlTerm(m_sysVideo->getSurface());

	GrSetClipBox(0, 0, 800, 600);

	rcPixels = m_term->getBounds();
	m_term->getWindow(&rcChars);
	m_term->enableCursor(0);
	m_term->setBackground(0x00, 0x00, 0x00);
	m_term->setForeground(0xFF, 0xFF, 0xFF);
	m_term->clearWindow();
	m_sys->m_cpu->storeW(liospb,      rcChars.h);
	m_sys->m_cpu->storeW(liospb + 2,  rcChars.w - 1);
	m_sys->m_cpu->storeW(liospb + 4,  rcChars.h);
	m_sys->m_cpu->storeW(liospb + 6,  rcChars.w - 1);
	m_sys->m_cpu->storeW(liospb + 8,  (3 * rcChars.h) / 4);
	m_sys->m_cpu->storeW(liospb +10, (word)m_sysVideo->colourToIndex(0xFF, 0xFF, 0xFF));
	m_sys->m_cpu->storeW(liospb +12, (word)m_sysVideo->colourToIndex(0x00, 0x00, 0x00));
	m_sys->m_cpu->storeW(liospb +16, 255);		/* Allow 256 colours */
	m_sys->m_cpu->storeW(liospb +24, rcPixels.w);
	m_sys->m_cpu->storeW(liospb +26, rcPixels.h);
	m_sys->m_cpu->storeW(liospb +34, 255);		/* 256 background colours */
	m_sys->m_cpu->storeW(liospb +36, rcChars.h);
	m_sys->m_cpu->storeW(liospb +38,  32);		/* 1:1 scrunch ratio */
	m_sys->m_cpu->storeW(liospb +42,  msgbuf);
	zstrcpy(msgbuf,title); 
	m_splitSz = (3 * rcChars.h) / 4;
	doSplit();
	return 1;
}

void PcwLogoTerm::lios1(void)
{
	m_sys->selectTerminal(m_oldTerm);
	delete m_term;
	
	m_term = NULL;
}

void PcwLogoTerm::lios2(void)
{
	m_splitSz = -1;
	m_term->enableCursor();
}

void PcwLogoTerm::lios3(byte split)
{
	m_splitSz = split;
	m_term->enableCursor(split != 0);
	doSplit();
}


void PcwLogoTerm::lios7(word T, word B)	/* Clear text lines T to B */
{
	m_term->clearLineRange(T-1, B-1);
}

void PcwLogoTerm::lios8(word X, word Y)  /* Move text cursor to X,Y */
{
	m_term->moveCursor(X - 1, Y - 1);
}


void PcwLogoTerm::lios9(word T, word B) /* Scroll text window */
{
	m_term->scrollLineRangeUp(T - 1, B - 1);
	m_term->clearCurLine();
}

word PcwLogoTerm::lios10(word X, word Y, byte colour, byte pen)
{
	int rop = makerop((pen == 2) ? 3 : 1, &colour);	

	GrPlotRop(X, Y, colour, rop);
	return 1;
}


word PcwLogoTerm::lios11(word X1, word Y1, word X2, word Y2, byte colour, byte pen)
{
	int rop = makerop((pen == 2) ? 3 : 1, &colour);	

        GrLineRop(X1, Y1, X2, Y2, colour, rop);

	if (X1 > X2) { word t = X2; X2 = X1; X1 = t; }
	if (Y1 > Y2) { word t = Y2; Y2 = Y1; Y1 = t; }

        UpdateArea(X1, Y1, X2-X1+1, Y2-Y1+1);
	return 1;
}


word PcwLogoTerm::lios13(word colour, word pixels)
{
	Uint32 pixel = m_sysVideo->indexToPixel(colour);
	SDL_Rect rc = m_term->getBounds();

	if (rc.h > pixels) rc.h = pixels;

	m_term->fillRect(rc, pixel);
	return 1;
}

word PcwLogoTerm::lios15(void)
{ 
	return m_sys->m_cpu->fetchW(6); 
}



word PcwLogoTerm::lios16(word X, word Y)
{
	return (word)(GrPixel(X, Y));
}


word PcwLogoTerm::lios24(word buf, word recno)
{
	return get_bmp_record(buf, recno);
}

word PcwLogoTerm::lios25(word buf, word recno)
{
	return put_bmp_record(buf, recno);
}

void PcwLogoTerm::drLogo(Z80 *R)
{
/*	joyce_dprintf("LIOS call %d ", R->DE.B.l);
	switch(R->DE.B.l)
	{
		case 3: joyce_dprintf("(%d)", P1_BYTE); break;
		case 6: joyce_dprintf("(%c)", P1_BYTE); break;
		case 7:
		case 8:
		case 9:
		case 13:
		case 16:joyce_dprintf("(%d,%d)", P1_WORD, P2_WORD); break;
		case 10:joyce_dprintf("(%d,%d,%d,%d)", P1_WORD, P2_WORD, P3_BYTE, P4_BYTE); break;
		case 11:joyce_dprintf("(%d,%d,%d,%d,%d,%d)", P1_WORD, P2_WORD, P3_WORD, P4_WORD, P5_BYTE, P6_BYTE); break;
	}
	joyce_dprintf(" SP=%x\n", R->sp);
*/	switch(R->e)
	{
		case 0:	lios0(); break;
		case 1: lios1(); break;
		case 2: lios2(); break;
		case 3:	lios3(P1_BYTE); break;
		case 4: lios4(); break; /* done in Z80 code */
		case 5: lios5(); break;	/* done in Z80 code */
		case 6: lios6(P1_BYTE); break;
		case 7: lios7(P1_WORD, P2_WORD); break;
		case 8: lios8(P1_WORD, P2_WORD); break;
		case 9: lios9(P1_WORD, P2_WORD); break;
		case 10:R->setHL(lios10(P1_WORD, P2_WORD, P3_BYTE, P4_BYTE)); break;
		case 11:R->setHL(lios11(P1_WORD, P2_WORD, P3_WORD, P4_WORD,
			       P5_BYTE, P6_BYTE)); break;
		case 12:lios12(); break;
		case 13:R->setHL(lios13(P1_WORD, P2_WORD)); break;
		case 14:lios14(); break;
		case 15:R->setHL(lios15()); break;
		case 16:R->setHL(lios16(P1_WORD, P2_WORD)); break;
		case 17:lios17(); break; /* done in Z80 code */
		case 18:lios18(); break; /* done in Z80 code */
		case 19:lios19(); break;
		case 20:lios20(); break;
		case 21:lios21(); break;
		case 22:lios22(); break;
		case 23:lios23(); break;
		case 24:R->setHL(lios24(P1_WORD, P2_WORD)); break;
		case 25:R->setHL(lios25(P1_WORD, P2_WORD)); break;

		case 0xFF: R->setHL(lios_init(P1_WORD, R->getHL())); break;

		default: joyce_dprintf("Unrecognised LIOS call %d\n", R->e);
	}
}

/* BMP headers. HDR1 used for I/O; stdhdr holds an 800x600x256 bmp header */
	
static byte stdhdr[54] = 
{		   'B', 'M', 	  /* (2) Magic number */
		   54, 87, 7, 0,  /* (4) file size */
		   0,   0, 0, 0,  /* (4) zero */
		   54,  4, 0, 0,  /* (4) offset to image data */
		   40,  0, 0, 0,  /* (4) length of info area*/
		   32, 3, 0, 0,	  /* (4) width  = 800 */
		   88, 2, 0, 0,   /* (4) height = 600 */
		   1,  0,         /* (2) planes */
		   8,  0,         /* (2) colour depth */
		   0,  0, 0, 0,   /* (4) no compression */
		   0, 83, 7, 0,   /* (4) picture size, 800*600 */
		   0,  0, 0, 0,   /* (4) horizontal resolution */
		   0,  0, 0, 0,   /* (4) vertical resolution */		   		   
		   0,  0, 0, 0,   /* (4) colours used */
		   0,  0, 0, 0,   /* (4) important colours */
};				  /* total: 54 bytes */


int PcwLogoTerm::isbmp(void)
{
	if ((m_hdr1[0] != 'B') || (m_hdr1[1] != 'M')) return 0;
	m_hdrlen = m_hdr1[14] + 256 * m_hdr1[15] + 65536 * m_hdr1[16] + 16777216 * m_hdr1[17];
	m_hdrused = (m_hdrlen < 40 ? m_hdrlen : 40);
	return 1;
}


int PcwLogoTerm::is8bit(void)
{
	if ((m_hdr1[26] != 1) || (m_hdr1[28] != 8) || (m_hdr1[30] != 0)) return 0;
	m_pallen = m_hdr1[10] + 256 * m_hdr1[11] + 65536 * m_hdr1[12] + 16777216 * m_hdr1[13];
	m_pallen -= (m_hdrlen + 14);
	m_palused = (m_pallen < 1024 ? m_pallen : 1024);
	m_piclen = m_hdr1[2] + 256 * m_hdr1[3] + 65536 * m_hdr1[4] + 16777216 * m_hdr1[5];
	m_picw   = m_hdr1[18] + 256 * m_hdr1[19] + 65536 * m_hdr1[20] + 16777216 * m_hdr1[21];
	m_pich   = m_hdr1[22] + 256 * m_hdr1[23] + 65536 * m_hdr1[24] + 16777216 * m_hdr1[25];
	while (m_picw & 3) ++m_picw;	

	m_palbegin = m_hdrlen   + 14;
	m_palend   = m_palbegin + m_pallen;
	return 1;

}



void PcwLogoTerm::get_hdr1(void)
{
	memcpy(&m_hdr1, &stdhdr, 54);
}



void PcwLogoTerm::get_palette(void)
{
	int n;
	Uint8 r, g, b;

	memset(m_palette, 0, sizeof(m_palette));
	for (n = 0; n < 256; n++)
	{
		m_sysVideo->indexToColour(n, &r, &g, &b);
		m_palette[n*4    ] = b;
		m_palette[n*4 + 1] = g;
		m_palette[n*4 + 2] = r;
	}
}





void PcwLogoTerm::put_palette(void)
{
	int n, m;
	int r, g, b;

	for (n = 0; n < 256; n++)
	{
		b = m_palette[n*4    ];
		g = m_palette[n*4 + 1];
		r = m_palette[n*4 + 2];
		m = m_sysVideo->colourToIndex(r,g,b);
		colmap[n] = m;
	}
}


/* Generate any 128-byte record from a .BMP file */

word PcwLogoTerm::get_bmp_record(word Z80addr, word recno)
{
	unsigned int n;
	unsigned long where = recno * 128;

	if (where > (800*600 + sizeof(m_palette) + sizeof(m_hdr1))) return 0;

	get_hdr1();
	get_palette();

	for (n = 0; n < 128; n++)
	{
		register unsigned int m = n + where;

		if (m < sizeof(m_hdr1)) 
			m_sys->m_cpu->storeB(Z80addr++, m_hdr1[m]);
		else if (m < (1024 + sizeof(m_hdr1)))
			m_sys->m_cpu->storeB(Z80addr++, m_palette[m - sizeof(m_hdr1)]);
		else	
		{
			int px, py, pz;

			pz = (m - sizeof(m_hdr1) - sizeof(m_palette));
			py = (pz / 800);
			px = (pz % 800);

			m_sys->m_cpu->storeB(Z80addr++, GrPixel(px,599 - py));
		}
	}
	return 1;
}




word PcwLogoTerm::put_bmp_record(word Z80addr, word recno)
{
	int bmpptr, z80ptr = 0;
	int where = 128 * recno;
	static int loadpal = 0;

	if (!recno)
	{
/*
 * Record 0: Load the header, which gives the sizes of the other objects in
 *          the BMP. This is rather complicated.
 * 
 * 1. Read in the header (14 bytes) and the first dword of the info area
 *   (4 bytes)
 *
 */
		for (z80ptr = 0; z80ptr < 18; z80ptr++)
		{
			m_hdr1[z80ptr] = m_sys->m_cpu->fetchB(Z80addr + z80ptr);
		}
		if (!isbmp()) return 0;
/*
 * hdrlen  = number of bytes in info area 
 * hdrused = number of info bytes we can use ( <= hdrlen)
 *
 * 2. Read in the rest of the info area ((hdrused - 4) bytes)
 * 
 */
		for (z80ptr = 18; z80ptr < 14 + m_hdrused; z80ptr++)
		{
			m_hdr1[z80ptr] = m_sys->m_cpu->fetchB(Z80addr + z80ptr);
		}
		if (!is8bit()) return 0;
		z80ptr = m_hdrused + 14;	/* header + used info area */
	}
/*
 * 3. Read palette or picture data
 */
	while (z80ptr < 128)
	{
		bmpptr = where + z80ptr;
		if      (bmpptr < m_hdrlen + 14) continue; /* Skipping bits of header */
		else if (bmpptr < m_palend) /* Loading palette */
		{
			if (bmpptr - m_palbegin < 1024)
			{
				m_palette[bmpptr - m_palbegin] = m_sys->m_cpu->fetchB(Z80addr + z80ptr);
			}
			loadpal = 1;
		}
		else if (bmpptr < m_palend) /* Skipping bits of palette */
		{	
			++z80ptr;
			continue;
		}
		else	/* Part of the picture - plot it */
		{
			int px, py, pz;

			if (loadpal)
			{
				put_palette();
				loadpal = 0;			
			}
			pz = (bmpptr - m_palend);
			py = (pz / m_picw);
			px = (pz % m_picw);

			GrPlot(px, m_pich - py - 1, colmap[m_sys->m_cpu->fetchB(Z80addr + z80ptr)]);
		}
		++z80ptr;
	}
	if (where + 128 > m_piclen) 
	{
	       // XXX SDL_UpdateRect(screen, 0, 0, 0, 0);
		return 0;
	}
	return 1;
}





