/***************************************************************************
 *                                                                         *
 *    HHEDIT: Hungry Horace editor                                         *
 *    Copyright (C) 2009 John Elliott <jce@seasip.demon.co.uk>             *
 *                                                                         *
 *    This program is free software; you can redistribute it and/or modify *
 *    it under the terms of the GNU General Public License as published by *
 *    the Free Software Foundation; either version 2 of the License, or    *
 *    (at your option) any later version.                                  *
 *                                                                         *
 *    This program is distributed in the hope that it will be useful,      *
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of       *
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the        *
 *    GNU General Public License for more details.                         *
 *                                                                         *
 *    You should have received a copy of the GNU General Public License    *
 *    along with this program; if not, write to the Free Software          *
 *    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.            *
 *                                                                         *
 ***************************************************************************/

#define VERSION "0.3"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dos.h>
#include <conio.h>
#include "pc_mouse.h"
#include "pc_video.h"


char *gl_filename;
byte sna[49179];

/* Graphics chars */
#define HLINE 0xCD
#define VLINE 0xBA
#define TLJ 0xC9
#define TRJ 0xBB
#define BLJ 0xC8
#define BRJ 0xBC
#define LJ 0xB9
#define RJ 0xCC
#define BJ 0xCB
#define TJ 0xCA
#define XROADS 0xCE
#define STOPT 0xD0
#define STOPB 0xD2
#define STOPL 0xB5
#define STOPR 0xC6

#define HORACE 0x7CE7
#define KEEPER 0x7DE7
#define BELL   0x7F07

byte peek(unsigned addr) 
{	
	return sna[addr - 0x3FE5]; 
}

void poke(unsigned addr, byte value)
{
	sna[addr - 0x3FE5] = value;
}

void patch(unsigned char *src, unsigned dest, unsigned len)
{
	memcpy(sna + dest - 0x3FE5, src, len);
}

void movedata(unsigned src, unsigned dest, unsigned len)
{
	memcpy(sna + dest - 0x3FE5, sna + src - 0x3FE5, len);
}


int is16k()
{
	return (peek(0x6100) == 0xe6);
}

int maxlevel()
{
	if (is16k())
	{
		return 4;
	}
	return 42;
}

int levelcount()
{
	if (is16k())
	{
		return 4;
	}
	return peek(0x6101);
}


int setlast(int nl)
{
	if (!is16k())
	{
		poke (0x6101, nl);
		poke (0x60EE, nl);
	}
}



int coord_to_offset(int y, int x)
{
	signed int lv = (y / 8) * 2048;

	y -= ((y/8)*8);
	lv += 32 * y;
	lv += x;

	return lv;
}


void offset_to_coord(int offset, int *y, int *x)
{
	*y = (offset / 2048) * 8;
	offset = offset - ((offset / 2048) * 2048);
	*y += (offset / 32);
	offset -= ((offset / 32) * 32);
	*x = offset;	
}


void setoffset(int level, int val, int offset)
{
	poke(0x6FD7 + 8*level + 2*val, offset & 0xFF);
	poke(0x6FD8 + 8*level + 2*val, offset >> 8);
}



void setcoord(int level, int val, int y, int x)
{
	int offset = coord_to_offset(y, x);

	setoffset(level, val, offset);
}


signed int getoffset(int level, int val)
{
	signed int lv = (0xFF & peek(0x6FD7 + 2*val + 8 * level));

	lv = lv | ((int)peek(0x6fd8 + 2*val + 8 * level) << 8);
	return lv;
}

void getcoord(int level, int val, int *y, int *x)
{
	signed int lv = getoffset(level, val);
	offset_to_coord(lv, y, x);
}






unsigned getmap(unsigned sheet)
{
	if (is16k())
	{
		switch(sheet)
		{
			case 0: return 0x72F7;
			case 1: return 0x6FF7;
			case 2: return 0x75F7;
			case 3: return 0x78F7;
			default: return 0x8000;
		}
	}
	return 0x8000 + 768 * sheet;
}

unsigned tiles(void)
{
	return peek(0x6BB5) + 256 * peek(0x6BB6);
}

/* Upgrade code... */
#define NEWLEVELS 42		/* Maximum that fit in 32k */
#define LEVELBASE 0x6FD7
#define TILEBASE  (LEVELBASE + 8 * NEWLEVELS)

unsigned char x_tiles[] = 
{
	0x66, 0xE7, 0xE7, 0x00, 0x00, 0xFF, 0xFF, 0,	/* Junction T */
	0x00, 0xFF, 0xFF, 0x00, 0x00, 0xE7, 0xE7, 0x66, /* Junction B */
	0x66, 0xE6, 0xE6, 0x06, 0x06, 0xE6, 0xE6, 0x66,	/* Junction L */
	0x66, 0x67, 0x67, 0x60, 0x60, 0x67, 0x67, 0x66,	/* Junction R */
	0x66, 0xE7, 0xE7, 0x00, 0x00, 0xE7, 0xE7, 0x66,	/* Crossroads */
	0x66, 0x66, 0x66, 0x66, 0x66, 0x7E, 0x7E, 0x00, /* Stopper T */
	0x00, 0x7E, 0x7E, 0x66, 0x66, 0x66, 0x66, 0x66, /* Stopper B */
	0x00, 0xFE, 0xFE, 0x06, 0x06, 0xFE, 0xFE, 0x00, /* Stopper L */
	0x00, 0x7F, 0x7F, 0x60, 0x60, 0x7F, 0x7F, 0x00, /* Stopper R */
};

unsigned char lookup_patch[] = 
{
	/* ORG 6100h */
	0xFE, NEWLEVELS,	/* CP maxlevel */
	0x38, 0x01,		/* JR C, noreset */
	0xAF,			/* XOR A */
	0x32, 0x63, 0x7C,	/* LD (level), A */
	0x6F,			/* LD L, A */
	0x26, 0x00,		/* LD H, 0 */
	0x29,			/* ADD HL, HL */	
	0x29,			/* ADD HL, HL */	
	0x29,			/* ADD HL, HL */	
	0x11, (LEVELBASE & 0xFF), (LEVELBASE >> 8),	/* LD DE, 6FD7 */
	0x19, 			/* ADD HL, DE */
	0xCD, 0x9E, 0x6A,	/* CALL init_data */
	0x57,			/* LD D, A */
	0x1E, 0x00,		/* LD E, 0 */
	0x21, 0x00, 0x80,	/* LD HL, 8000h */
	0x19, 			/* ADD HL, DE */
	0x19, 			/* ADD HL, DE */
	0x19, 			/* ADD HL, DE */
	0xc3, 0x40, 0x61,	/* JP init_map */
};


void upgrade(void)
{
	movedata(0x72f7, 0x8000, 768);	/* Move the four maps to 8000h */
	movedata(0x6ff7, 0x8300, 768);
	movedata(0x75f7, 0x8600, 768);
	movedata(0x78f7, 0x8900, 768);

	movedata(0x7bf7, TILEBASE, 80);	/* Move the tiles to TILEBASE */
	/* Add extra tile bitmaps  to end of tiles */
	patch(x_tiles, TILEBASE + 80, sizeof(x_tiles));
	patch(lookup_patch, 0x6100, sizeof(lookup_patch));

	poke (0x6BB5, TILEBASE & 0xFF);	/* New tiles pointer */
	poke (0x6BB6, TILEBASE >> 8);
	/* New level count */
	poke (0x60ED, 0xFE);		/* CP maxlevel  ; was AND ~3 */
	poke (0x60EE, NEWLEVELS);
	poke (0x60EF, 0x38);		/* JR C         ; was JR NZ */
}


void help()
{
	savescr();
	popup("Hungry Horace Editor " VERSION "\nKeys\n\n"
/*		 1...5...10...15...20...25...30...35... */
		"Cursor keys move, SPACE draws cell\n"
		"T/Y: Previous/next cell type      \n"
		"0-9: Select that cell type        \n"
		"-/+: Previous/next level          \n"
		"H: Position Horace                \n"
		"B: Position Bell                  \n"
		"K: Position Keeper                \n"
		"D: Set displacement for teleports \n"
		"U: Upgrade 16k game engine to 48k \n"
		"L: Make this the last level       \n"
		"Ctrl+T: Edit current tile         \n"
		"*: Edit title screen              \n"
		"F1 = Help                         \n"
		"F2 = Save changes                 \n"
		"F4 = Revert to last saved file    \n"
		"ESC= Quit                         \n\n"
		"Copyright 2009 John Elliott\n"
		"Distribution terms: GNU GPL v2");
	getch();
	restorescr();
}

int confirm (const char *s)
{
	char buf[160];
	int c;

	sprintf(buf, "%s\n\nAre you sure (Y/N)?", s);
	savescr();
	popup(buf);
	do
	{
		c = getch();
	} while (c != 'y' && c != 'Y' && c != 'n' && c != 'N');
	restorescr();
	return (c == 'Y' || c == 'y');
}


int confirm2(const char *s)
{
	char buf[160];
	int c;

	sprintf(buf, "%s (Y/N)?", s);
	savescr();
	popup(buf);
	do
	{
		c = getch();
	} while (c != 'y' && c != 'Y' && c != 'n' && c != 'N');
	restorescr();
	return (c == 'Y' || c == 'y');
}



void alert(const char *s)
{
	savescr();
	popup(s);
	getch();
	restorescr();
}


int load_snap(void)
{
	FILE *fp = fopen(gl_filename, "rb");

	if (!fp)
	{
		return 0;
	}
	if ((unsigned)fread(sna, 1, sizeof(sna), fp) < sizeof(sna))
	{
		fclose(fp);
		return 0;
	}
	fclose(fp);
	return 1;
}


int save_snap(void)
{
	FILE *fp = fopen(gl_filename, "wb");

	if (!fp)
	{
		return 0;
	}
	if ((unsigned)fwrite(sna, 1, sizeof(sna), fp) < sizeof(sna))
	{
		fclose(fp);
		return 0;
	}
	fclose(fp);
	return 1;
}




void dump_sheet(unsigned base)
{
	int r, c;
	byte cur;

	for (r = 0; r < 24; r++)
	{
		for (c = 0; c < 32; c++)
		{
			cur = sna[base + 32*r + c];
			switch(cur)
			{
				case 0: putchar(' ');  break;
				case 1: putchar(HLINE); break;
				case 2: putchar('*');  break;
				case 3: putchar('>');  break;
				case 4: putchar(VLINE); break;
				case 5: putchar(TLJ); break;
				case 6: putchar(TRJ); break;
				case 7: putchar(BLJ); break;
				case 8: putchar(BRJ); break;
				case 9: putchar('!'); break;
				case 10: putchar(TJ); break;
				case 11: putchar(BJ); break;
				case 12: putchar(LJ); break;
				case 13: putchar(RJ); break;
				case 14: case 15: case 16: case 17: case 18:
				case 19: case 20: case 21: case 22: case 23:
					 putchar(cur - 14 + '0');
					 break;
				default: printf("%d", cur); break;
			}
		}
		putchar('\n');
	}
}




static int gl_dirty = 0;
static int gl_maxtile = 0;
static int gl_level = 0;
static int gl_tile = 0;
static int gl_cellr = 0;
static int gl_cellc = 0;
static int gl_tiler = 0;
static int gl_tilec = 0;
static int gl_ter = 0;
static int gl_tec = 0;
static char st_buf[41];


void save(void)
{
	if (!save_snap())
	{
		alert("Failed to save snapshot");
	}
	else
	{
		alert("Saved.");
		gl_dirty = 0;
	}
}

void status(void)
{
	int n;
	memset(st_buf, 0, sizeof(st_buf));
	sprintf(st_buf, "Level %2d row=%2d col=%2d tile=%02x %s %s", 
			gl_level, gl_cellr, gl_cellc, gl_tile,
			gl_level == (levelcount() - 1) ? "LAST" : "    ",
			is16k() ? "HH16" : "HH48");
	for (n = strlen(st_buf); n < 40; n++) st_buf[n] = ' ';
	putstatus(st_buf);
}


void disp_status(int offset, int base)
{
	int n;
	int dy, dx;
	int r0, c0, r1, c1;

	offset_to_coord(base, &r0, &c0);
	offset_to_coord(base + offset, &r1, &c1);
	dy = r1 - r0;
	dx = c1 - c0;
	memset(st_buf, 0, sizeof(st_buf));
	sprintf(st_buf, "Setting displacement: dy=%3d dx=%3d", dy, dx);
	for (n = strlen(st_buf); n < 40; n++) st_buf[n] = ' ';
	putstatus(st_buf);
}


void disp_redraw(int offset, int *sr, int *sc, int *sd)
{
	int r, c, r2, c2;
	unsigned addr = getmap(gl_level);
	int source, dest;
	int active;
	byte v1, vleft, vabove, vbelow;

	*sr = -1;
	*sc = -1;
	for (r = 0; r < 24; r++)
	{
		for (c = 0; c < 32; c++)
		{
			v1 = peek(addr + 32*r + c);
			vleft  = (c ==  0) ? 1 : peek(addr + 32*r + (c- 1));
			vabove = (r ==  0) ? 1 : peek(addr + 32*r + (c-32));
			vbelow = (r == 23) ? 1 : peek(addr + 32*r + (c+32));

			source = coord_to_offset(r, c);
			if (v1 == 9 && vleft != 9)
			{
				dest   = source + offset;

				if (*sr == -1)
				{
					*sr = r;
					*sc = c;
					active = 1;
				}
				else	active = 0;
				if (dest >= 0 && dest < 0x1800 && vbelow == 0)
				{
					offset_to_coord(dest, &r2, &c2);
					draw_displacement(r+1, c, r2+1, c2, active);
					if (active) *sd = 1;
					active = 0;
				}
				dest   = source - offset;
				if (dest >= 0 && dest < 0x1800 && vabove == 0)
				{
					offset_to_coord(dest, &r2, &c2);
					draw_displacement(r-1, c, r2-1, c2, active);
					if (active) *sd = -1;
					active = 0;
				}
			}
		}
	}
}

void compose(int base, int *offset, int sd, int dy, int dx)
{
	int y, x;
	int value = *offset;

	offset_to_coord(base + (sd * value), &y, &x);
	y += dy;
	x += dx;
	value = ((coord_to_offset(y, x) - base)) / sd;
	*offset = value;
}




int set_displacement(void)
{
	int offset = getoffset(gl_level, 0);
	int offset0 = offset;
	int base;
	int sr, sc, sd;
	int c;

	savescr();
	disp_redraw(offset, &sr, &sc, &sd);
	base = coord_to_offset(sr, sc);
	disp_status(offset, base);
	while (1)
	{
		c = getevent();

		switch(c)
		{
			case K_MOUSE:
				if (gl_xmouse < 256 && gl_ymouse > 8)
				{
					int y = (gl_ymouse - 8) / 8;
					int x = (gl_xmouse / 8);

					offset = ((coord_to_offset(y, x) - 
								base)) / sd;
					restorescr();
					disp_redraw(offset, &sr, &sc, &sd);
					disp_status(offset, base);
				}
				break;
			case 'Q': case 'q': case 'E' - '@':
				compose(base, &offset, sd, -1, 0);
				restorescr();
				disp_redraw(offset, &sr, &sc, &sd);
				disp_status(offset, base);
				break;
			case 'A': case 'a': case 'Z': case 'z': case 'X' - '@':
				compose(base, &offset, sd, 1, 0);
				restorescr();
				disp_redraw(offset, &sr, &sc, &sd);
				disp_status(offset, base);
				break;
			case 'I': case 'i': case 'o': case 'O': case 'S' - '@':
				compose(base, &offset, sd, 0, -1);
				restorescr();
				disp_redraw(offset, &sr, &sc, &sd);
				disp_status(offset, base);
				break;
			case 'P': case 'p': case 'D' - '@':
				compose(base, &offset, sd, 0, 1);
				restorescr();
				disp_redraw(offset, &sr, &sc, &sd);
				disp_status(offset, base);
				break;
			case 'W' - '@':
				setoffset(gl_level, 0, offset);
				offset0 = offset;
				restorescr();
				save();
				savescr();
				disp_redraw(offset, &sr, &sc, &sd);
				disp_status(offset, base);
				break;
			case '\r':
			case '\n':
			case 'D': case 'd':
			case 0x1B:
				restorescr();
				setoffset(gl_level, 0, offset);
				return (!(offset == offset0));
		}
	}

	restorescr();
	return (!(offset == offset0));
}




void putcell(int r, int c, unsigned type)
{
	unsigned addr = getmap(gl_level);

	poke(addr + 32 * r + c, type);
}


void drawlevel(void)
{
	int row, col;

	unsigned addr = getmap(gl_level);

	for (row = 0; row < 24; row++)
		for (col = 0; col < 32; col++)
		{
			drawcell(row, col, peek(addr + 32 * row + col), 0);
		}
	getcoord(gl_level, 1, &row, &col); drawsprite(row, col, BELL, 0);
	getcoord(gl_level, 2, &row, &col); drawsprite(row, col, KEEPER, 0);
	getcoord(gl_level, 3, &row, &col); drawsprite(row, col, HORACE, 0);

}

void drawcur(int row, int col, int d)
{
	int r, c;
	unsigned addr = getmap(gl_level);

	getcoord(gl_level, 1, &r, &c); drawsprite(r, c, BELL, 0);
	getcoord(gl_level, 2, &r, &c); drawsprite(r, c, KEEPER, 0);
	getcoord(gl_level, 3, &r, &c); drawsprite(r, c, HORACE, 0);
	drawcell(row, col, peek(addr + 32 * row + col), d);
	status();
}



void ti_toggle(int r, int c)
{
	unsigned title_addr = 0x6E8F + (r/2) * 32;
	byte val;

	val = peek(title_addr + (c/2));
	if (r & 1) 
	{
	       if (c & 1) val ^= 2; else val ^= 1;
	}
	else
	{
	       if (c & 1) val ^= 0x20; else val ^= 0x10;
	}
	poke(title_addr + (c/2), val);
}


unsigned char ti_cell(int r, int c)
{
	byte val;
	unsigned title_addr = 0x6E8F + (r/2) * 32;

	val = peek(title_addr + (c/2));
	if (r & 1) val &= 0x0F;
	else 	   val = (val >> 4) & 0x0F;

	if (c & 1) return (val & 2);
	else	   return (val & 1);
}

void ti_drawcell(int r, int c)
{
	unsigned char val;

	if (r == 10 || r == 11) val = 0;
	else 
	{
		if (r > 9) r -= 2;
		val = ti_cell(r,c) ? 4 : 1;
		if (r > 9) r += 2;
	}
	
	put_4x4(r + 2, c + 8, val);
}


void ti_repaint(void)
{
	int r, c;

	for (r = 0; r < 22; r++)
	{
		for (c = 0; c < 64; c++)
		{
			ti_drawcell(r, c);
		}
	}
}

void ti_drawcur(int r, int c, int draw)
{
	unsigned char val;
	if (!draw)
	{
		ti_drawcell(r, c);
		return;
	}

	if (r == 10 || r == 11) val = 0;
	else 
	{
		if (r > 9) r -= 2;
		val = ti_cell(r,c) ? 3 : 2;
		if (r > 9) r += 2;
	}
	
	put_4x4(r + 2, c + 8, val);
}







int title_edit(void)
{
	unsigned char copy[320];	/* Title is 10 rows of 32 bytes */
	unsigned title_addr = 0x6E8F;
	int c, dirty;

	for (c = 0; c < 320; c++) 
	{
		copy[c] = peek(title_addr + c);
	}
	if (gl_graphics == MDA)
	{
		rectangle(30, 14, 260, 180, 0);
	}
	else
	{
		rectangle(30, 62, 260, 91, 0);
		rectangle(31, 63, 258, 89, 15);
	}
	ti_repaint();
	ti_drawcur(gl_ter, gl_tec, 1);

	while (1)
	{
		c = getevent();

		switch(c)
		{
			case K_MOUSE:
				if (gl_xmouse >=  32 && gl_xmouse < 288 &&
				    gl_ymouse >=  64 && gl_ymouse < 240 &&
				    (gl_ymouse < 104 || gl_ymouse >= 112) &&
				    gl_graphics != MDA)
				{
					ti_drawcur(gl_ter, gl_tec, 0);
					gl_ter = (gl_ymouse - 64) / 4;
					gl_tec = (gl_xmouse - 32) / 4;
					if (gl_ter > 9) gl_ter -= 2;
					ti_toggle(gl_ter, gl_tec);
					if (gl_ter > 9) gl_ter += 2;
					ti_drawcur(gl_ter, gl_tec, 1);
					debounce_mouse();
				} 
				if (gl_xmouse >=  32 && gl_xmouse < 288 &&
				    gl_ymouse >=  16 && gl_ymouse < 192 &&
				    (gl_ymouse <  96 || gl_ymouse >= 112) &&
				    gl_graphics == MDA)
				{
					ti_drawcur(gl_ter, gl_tec, 0);
					gl_ter = (gl_ymouse - 16) / 8;
					gl_tec = (gl_xmouse - 32) / 4;
					if (gl_ter > 9) gl_ter -= 2;
					ti_toggle(gl_ter, gl_tec);
					if (gl_ter > 9) gl_ter += 2;
					ti_drawcur(gl_ter, gl_tec, 1);
					debounce_mouse();
				}
				break;
			case 'A': case 'a': case 'X' - '@': case 'z': case 'Z':
				if (gl_ter < 21)
				{
					ti_drawcur(gl_ter, gl_tec, 0);
					++gl_ter;
					if (gl_ter == 10) gl_ter = 12;
					ti_drawcur(gl_ter, gl_tec, 1);
				}
				break;
			case 'Q': case 'q': case 'E' - '@':
				if (gl_ter > 0)
				{
					ti_drawcur(gl_ter, gl_tec, 0);
					--gl_ter;
					if (gl_ter == 11) gl_ter = 9;
					ti_drawcur(gl_ter, gl_tec, 1);
				}
				break;
			case 'I': case 'i': case 'S' - '@': case 'o': case 'O':
				if (gl_tec > 0)
				{
					ti_drawcur(gl_ter, gl_tec, 0);
					--gl_tec;
					ti_drawcur(gl_ter, gl_tec, 1);
				}
				break;
			case 'P': case 'p': case 'D' - '@':
				if (gl_tec < 63)
				{
					ti_drawcur(gl_ter, gl_tec, 0);
					++gl_tec;
					ti_drawcur(gl_ter, gl_tec, 1);
				}
				break;
			case ' ':
				if (gl_ter > 9) gl_ter -= 2;
				ti_toggle(gl_ter, gl_tec);
				if (gl_ter > 9) gl_ter += 2;
				ti_drawcur(gl_ter, gl_tec, 1);
				break;

			case 0x0D: case 0x0A:
			case 0x1B: case 'Q' - '@':
				dirty = 0;
				for (c = 0; c < 320; c++) 
				{
					if (copy[c] != peek(title_addr + c)) 
						dirty = 1;
				}
				if (dirty == 0 || 
					!confirm2("Save changes to title"))
				{
					for (c = 0; c < 320; c++) 
						poke(title_addr+c, copy[c]);
					return 0;	
				}
				return 1;
		}
	}

	return 0;
}

void te_repaint(void)
{
	int r, c;
	unsigned tile_addr = gl_tile * 8 + tiles();
	byte mask, val;
	unsigned char row[19], *ptr;

	for (r = 0; r < 8; r++)
	{
		mask = 0x80;
		val = peek(tile_addr++);
		ptr = row;
		for (c = 0; c < 8; c++)
		{
			*ptr++ = (val & mask) ? 254 : 250;
			if (gl_graphics == MDA) *ptr++ = ' ';
			mask = mask >> 1;
		}
		*ptr = 0;
		putstr(r + 8, 16, (char *)row, 1);
	}
}

void te_drawcur(int r, int c, int show)
{
	unsigned tile_addr = gl_tile * 8 + tiles();
	byte mask, val;
	unsigned char data[2];

	data[1] = 0;
	mask = 0x80 >> c;
	val = peek(tile_addr + r);
	if (show)
	{
		data[0] = (val & mask) ? 0xb2 : 0xb0;
	}
	else
	{
		data[0] = (val & mask) ? 254 : 250;
	}
	putstr(r + 8, 16 + c, (char *)data, 1);

	drawcell(11, 25, gl_tile, 0);
}


void te_toggle(int r, int c)
{
	unsigned tile_addr = gl_tile * 8 + tiles();
	byte mask, val;
	unsigned char data[2];

	data[1] = 0;
	mask = 0x80 >> c;
	val = peek(tile_addr + r);
	val ^= mask;
	poke(tile_addr + r, val);
}





int tile_edit(void)
{
	unsigned char copy[8];
	unsigned tile_addr = gl_tile * 8 + tiles();
	int c, dirty;

	for (c = 0; c < 8; c++) 
	{
		copy[c] = peek(tile_addr + c);
	}
	rectangle(126, 63, 84, 66, 0);
	if (gl_graphics != MDA)
		rectangle(127, 64, 82, 64, 15);

	te_repaint();
	te_drawcur(gl_tiler, gl_tilec, 1);
	
	while (1)
	{
		c = getevent();

		switch(c)
		{
			case K_MOUSE:
				if (gl_xmouse >= 128 && gl_xmouse < 192 &&
				    gl_ymouse >=  64 && gl_ymouse < 128)
				{
					te_drawcur(gl_tiler, gl_tilec, 0);
					gl_tiler = (gl_ymouse - 64) / 8;
					gl_tilec = (gl_xmouse - 128) / 8;
					te_toggle(gl_tiler, gl_tilec);
					te_drawcur(gl_tiler, gl_tilec, 1);
					debounce_mouse();
				}
				break;
			case 'A': case 'a': case 'X' - '@': case 'z': case 'Z':
				if (gl_tiler < 7)
				{
					te_drawcur(gl_tiler, gl_tilec, 0);
					++gl_tiler;
					te_drawcur(gl_tiler, gl_tilec, 1);
				}
				break;
			case 'Q': case 'q': case 'E' - '@':
				if (gl_tiler > 0)
				{
					te_drawcur(gl_tiler, gl_tilec, 0);
					--gl_tiler;
					te_drawcur(gl_tiler, gl_tilec, 1);
				}
				break;
			case 'I': case 'i': case 'S' - '@': case 'o': case 'O':
				if (gl_tilec > 0)
				{
					te_drawcur(gl_tiler, gl_tilec, 0);
					--gl_tilec;
					te_drawcur(gl_tiler, gl_tilec, 1);
				}
				break;
			case 'P': case 'p': case 'D' - '@':
				if (gl_tilec < 7)
				{
					te_drawcur(gl_tiler, gl_tilec, 0);
					++gl_tilec;
					te_drawcur(gl_tiler, gl_tilec, 1);
				}
				break;
			case ' ':
				te_toggle(gl_tiler, gl_tilec);
				te_drawcur(gl_tiler, gl_tilec, 1);
				break;

			case 0x0D: case 0x0A:
			case 0x1B: case 'Q' - '@':
				dirty = 0;
				for (c = 0; c < 8; c++) 
				{
					if (copy[c] != peek(tile_addr + c)) 
						dirty = 1;
				}
				if (dirty == 0 || 
					!confirm2("Save changes to tile"))
				{
					for (c = 0; c < 8; c++)
					       poke(tile_addr + c, copy[c]);
					return 0;	
				}
				return 1;
		}
	}	
	return 0;
}


void showtile(void)
{
	int n, t, y;

	y = 3;
	for (n = gl_tile - 4; n < (gl_tile + 4); n++)
	{
		t = (n < 0 || n >= gl_maxtile) ? 0 : n;

		drawcell(y, 36, t, 0);
		++y;
	}
	status();	
}


void editor(void)
{
	int c;

	gl_dirty = 0;
	gl_maxtile = is16k() ? 10 : 256;
	gl_cellr = gl_cellc = 0;
	cls();
	drawlevel();
	showtile();
	drawcur(gl_cellr, gl_cellc, 1);
	while (1)
	{
		c = getevent();

		switch(c)
		{
			case K_MOUSE:
				if (gl_xmouse < 256 && gl_ymouse > 8)
				{
					drawcur(gl_cellr, gl_cellc, 0);
					gl_cellr = (gl_ymouse - 8) / 8;
					gl_cellc = (gl_xmouse / 8);
					putcell(gl_cellr, gl_cellc, gl_tile);
					drawcur(gl_cellr, gl_cellc, 1);
					gl_dirty = 1;
				}
				if (gl_xmouse >= 280 && gl_xmouse <= 304)
				{
					if (gl_ymouse >= 24 && gl_ymouse <= 32)
					{
						debounce_mouse();
						if (gl_tile > 0)
						{
							--gl_tile;
							showtile();
						}
					}
					if (gl_ymouse >= 96 && gl_ymouse <= 104)
					{
						debounce_mouse();
						if (gl_tile < (gl_maxtile - 1))
						{
							++gl_tile;
							showtile();
						}
					}
				}
				break;	
			case '?':
				help();
				break;
			case '0': case '1': case '2': case '3': case '4':
			case '5': case '6': case '7': case '8': case '9':
				gl_tile = c - '0';
				showtile();
		       		break;	       
			case 'Z' - '@':
				if (!confirm("Do you want to revert?\n"
					"All changes since the last save\n"
					"will be lost.\n"))
				{
					break;
				}
				if (!load_snap())
				{
					alert("Failed to reload");
				}
				else
				{
					gl_dirty = 0;
					drawlevel();
					drawcur(gl_cellr, gl_cellc, 1);
				}
				break;
			case 'W' - '@':
				save();
				break;
			case '*':
				if (title_edit()) gl_dirty = 1;
				cls();
				drawlevel();
				showtile();
				drawcur(gl_cellr, gl_cellc, 1);
				break;
			case 'T' - '@':
				if (tile_edit()) gl_dirty = 1;
				showtile();
				drawlevel();
				drawcur(gl_cellr, gl_cellc, 1);
				break;
			case 'T': case 't':
				if (gl_tile >= gl_maxtile - 1) gl_tile = 0;
				else ++gl_tile;
				showtile();
				break;
			case 'Y': case 'y':
				if (gl_tile <= 0) gl_tile = gl_maxtile - 1;
				else --gl_tile;
				showtile();
				break;
			case 'Q': case 'q': case 'E' - '@':
				if (gl_cellr > 0)
				{
					drawcur(gl_cellr, gl_cellc, 0);
					--gl_cellr;
					drawcur(gl_cellr, gl_cellc, 1);
				}
				break;
			case 'A': case 'a': case 'X' - '@':
			case 'Z': case 'z':
				if (gl_cellr < 23)
				{
					drawcur(gl_cellr, gl_cellc, 0);
					++gl_cellr;
					drawcur(gl_cellr, gl_cellc, 1);
				}
				break;
			case 'I': case 'i': case 'S' - '@':
			case 'O': case 'o':
				if (gl_cellc > 0)
				{
					drawcur(gl_cellr, gl_cellc, 0);
					--gl_cellc;
					drawcur(gl_cellr, gl_cellc, 1);
				}
				break;
			case 'P': case 'p': case 'D' - '@':
				if (gl_cellc < 31)
				{
					drawcur(gl_cellr, gl_cellc, 0);
					++gl_cellc;
					drawcur(gl_cellr, gl_cellc, 1);
				}
				break;
			case '+': case 'N' - '@':
				if ((gl_level + 1) < maxlevel())
				{
					gl_level++;
					drawlevel();
					drawcur(gl_cellr, gl_cellc, 1);
				}
				break;
			case '-': case 'P' - '@':
				if (gl_level > 0)
				{
					gl_level--;
					drawlevel();
					drawcur(gl_cellr, gl_cellc, 1);
				}
				break;
			case ' ':
				putcell(gl_cellr, gl_cellc, gl_tile);
				drawcur(gl_cellr, gl_cellc, 1);
				gl_dirty = 1;
				break;	
			case 'H': case 'h':
				setcoord(gl_level, 3, gl_cellr, gl_cellc);
				gl_dirty = 1;
				drawlevel();
				drawcur(gl_cellr, gl_cellc, 1);
				break;

			case 'K': case 'k':
				setcoord(gl_level, 2, gl_cellr, gl_cellc);
				gl_dirty = 1;
				drawlevel();
				drawcur(gl_cellr, gl_cellc, 1);
				break;

			case 'B': case 'b':
				setcoord(gl_level, 1, gl_cellr, gl_cellc);
				gl_dirty = 1;
				drawlevel();
				drawcur(gl_cellr, gl_cellc, 1);
				break;

			case 'U': case 'u':
				if (!is16k())
				{
					alert("Already 48k");
					break;
				}
				if (!confirm("Upgrade to 48k game"))
				{
					break;
				}
				gl_dirty = 1;
				upgrade();
				drawlevel();
				gl_maxtile = is16k() ? 10 : 256;
				showtile();
				drawcur(gl_cellr, gl_cellc, 1);
				break;
			case 0x1B:
			case 'Q' - '@':
				if (gl_dirty == 0 || confirm("Quit the editor\n"
				"Changes since last save will be lost."))
				{
					return;
				}
				break;
			case 'L': case 'l':
				setlast(gl_level + 1);
				drawcur(gl_cellr, gl_cellc, 1);
				gl_dirty = 1;
				break;
			case 'D': case 'd':
				if (set_displacement()) gl_dirty = 1;
				drawlevel();
				drawcur(gl_cellr, gl_cellc, 1);
				break;

//			case 'Q' - '@': return;
		}
	}	
}




static int oldmode = 3;

static void resetmode(void)
{
	setmode(oldmode);
}

int main(int argc,char **argv)
{
	int n;

	gl_graphics = UNKNOWN;

	gl_filename = "hhorace.sna";
	for (n = 1; n < argc; n++)
	{
		if (argv[n][0] == '-')
		{
			if (argv[n][1] == 'V' || argv[n][1] == 'v') 
				gl_graphics = VGA; 
			if (argv[n][1] == 'C' || argv[n][1] == 'c') 
				gl_graphics = CGA; 
			if (argv[n][1] == 'M' || argv[n][1] == 'm') 
				gl_graphics = MDA; 
		}
		else gl_filename = argv[n];
	}
	if (!load_snap())
	{
		fprintf(stderr, "Could not load %s\n", gl_filename);
		return 1;

	}

	oldmode = getmode();
	atexit(resetmode);

	pc_mouse_init();
	pc_idle_init();
	switch (gl_graphics)
	{
		case UNKNOWN:
		if (vga_init())
		{
			gl_graphics = VGA;	
		}
		else if (cga_init())
		{
			gl_graphics = CGA;
		}
		else if (mda_init())
		{
			gl_graphics = MDA;
		}
		break;
	
		case VGA: if (!vga_init()) gl_graphics = UNKNOWN; break;
		case CGA: if (!cga_init()) gl_graphics = UNKNOWN; break;
		case MDA: if (!mda_init()) gl_graphics = UNKNOWN; break;
	}
	if (gl_graphics == UNKNOWN)
	{
		fprintf(stderr, "Could not select a graphics mode\n");
		return 1;
	}
	editor();

	return 0;
}

