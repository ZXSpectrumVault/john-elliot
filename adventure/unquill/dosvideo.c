/* UnQuill: Disassemble games written with the "Quill" adventure game system
    Copyright (C) 1996-2000, 2013  John Elliott

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
*/

#include "unquill.h"



static int screen_w, screen_h;
static uchr dos_x, dos_y, dos_attr = 0x1F, dos_attrmask = 0xFF;
static uchr dos_ink = 7, dos_paper = 1, dos_flash = 0, dos_bright = 1, 
	dos_inverse = 0, dos_over = 0;
static ushrt dos_curshape;
static int window_x, window_y, window_w, window_h, window_r, window_b;
static int dos_softfont;
static ushrt dos_framebuffer;
static uchr old_mode;
static uchr cg_mode;
static ushrt cg40_fg = 0xFFFF, cg40_bg = 0;
static int cg40_curshown = 0;

static uchr dos_attrmap[8] = { 0, 1, 4, 5, 2, 3, 6, 7 };

static uchr spectrum_font[2048] =
{
#include "tsans08.inc"
};

static uchr dos_font[2048];

/* Expand a byte bitmap to a (byteswapped) word */
static ushrt dos_stretch[256] = 
{
	0x0000, 0x0300, 0x0C00, 0x0F00, 0x3000, 0x3300, 0x3C00, 0x3F00,
	0xC000, 0xC300, 0xCC00, 0xCF00, 0xF000, 0xF300, 0xFC00, 0xFF00,
	0x0003, 0x0303, 0x0C03, 0x0F03, 0x3003, 0x3303, 0x3C03, 0x3F03,
	0xC003, 0xC303, 0xCC03, 0xCF03, 0xF003, 0xF303, 0xFC03, 0xFF03,
	0x000C, 0x030C, 0x0C0C, 0x0F0C, 0x300C, 0x330C, 0x3C0C, 0x3F0C,
	0xC00C, 0xC30C, 0xCC0C, 0xCF0C, 0xF00C, 0xF30C, 0xFC0C, 0xFF0C,
	0x000F, 0x030F, 0x0C0F, 0x0F0F, 0x300F, 0x330F, 0x3C0F, 0x3F0F,
	0xC00F, 0xC30F, 0xCC0F, 0xCF0F, 0xF00F, 0xF30F, 0xFC0F, 0xFF0F,

	0x0030, 0x0330, 0x0C30, 0x0F30, 0x3030, 0x3330, 0x3C30, 0x3F30,
	0xC030, 0xC330, 0xCC30, 0xCF30, 0xF030, 0xF330, 0xFC30, 0xFF30,
	0x0033, 0x0333, 0x0C33, 0x0F33, 0x3033, 0x3333, 0x3C33, 0x3F33,
	0xC033, 0xC333, 0xCC33, 0xCF33, 0xF033, 0xF333, 0xFC33, 0xFF33,
	0x003C, 0x033C, 0x0C3C, 0x0F3C, 0x303C, 0x333C, 0x3C3C, 0x3F3C,
	0xC03C, 0xC33C, 0xCC3C, 0xCF3C, 0xF03C, 0xF33C, 0xFC3C, 0xFF3C,
	0x003F, 0x033F, 0x0C3F, 0x0F3F, 0x303F, 0x333F, 0x3C3F, 0x3F3F,
	0xC03F, 0xC33F, 0xCC3F, 0xCF3F, 0xF03F, 0xF33F, 0xFC3F, 0xFF3F,

	0x00C0, 0x03C0, 0x0CC0, 0x0FC0, 0x30C0, 0x33C0, 0x3CC0, 0x3FC0,
	0xC0C0, 0xC3C0, 0xCCC0, 0xCFC0, 0xF0C0, 0xF3C0, 0xFCC0, 0xFFC0,
	0x00C3, 0x03C3, 0x0CC3, 0x0FC3, 0x30C3, 0x33C3, 0x3CC3, 0x3FC3,
	0xC0C3, 0xC3C3, 0xCCC3, 0xCFC3, 0xF0C3, 0xF3C3, 0xFCC3, 0xFFC3,
	0x00CC, 0x03CC, 0x0CCC, 0x0FCC, 0x30CC, 0x33CC, 0x3CCC, 0x3FCC,
	0xC0CC, 0xC3CC, 0xCCCC, 0xCFCC, 0xF0CC, 0xF3CC, 0xFCCC, 0xFFCC,
	0x00CF, 0x03CF, 0x0CCF, 0x0FCF, 0x30CF, 0x33CF, 0x3CCF, 0x3FCF,
	0xC0CF, 0xC3CF, 0xCCCF, 0xCFCF, 0xF0CF, 0xF3CF, 0xFCCF, 0xFFCF,

	0x00F0, 0x03F0, 0x0CF0, 0x0FF0, 0x30F0, 0x33F0, 0x3CF0, 0x3FF0,
	0xC0F0, 0xC3F0, 0xCCF0, 0xCFF0, 0xF0F0, 0xF3F0, 0xFCF0, 0xFFF0,
	0x00F3, 0x03F3, 0x0CF3, 0x0FF3, 0x30F3, 0x33F3, 0x3CF3, 0x3FF3,
	0xC0F3, 0xC3F3, 0xCCF3, 0xCFF3, 0xF0F3, 0xF3F3, 0xFCF3, 0xFFF3,
	0x00FC, 0x03FC, 0x0CFC, 0x0FFC, 0x30FC, 0x33FC, 0x3CFC, 0x3FFC,
	0xC0FC, 0xC3FC, 0xCCFC, 0xCFFC, 0xF0FC, 0xF3FC, 0xFCFC, 0xFFFC,
	0x00FF, 0x03FF, 0x0CFF, 0x0FFF, 0x30FF, 0x33FF, 0x3CFF, 0x3FFF,
	0xC0FF, 0xC3FF, 0xCCFF, 0xCFFF, 0xF0FF, 0xF3FF, 0xFCFF, 0xFFFF,
};

static int is_convertible = 0;
static int is_vga = 0;
static int is_ega = 0;
static int is_cga = 0;
static int is_mda = 0;


void detect_video(void)
{
	union REGS rg;
	struct SREGS sg;
	unsigned char far *ptr;

	is_convertible = is_vga = is_ega = is_cga = is_mda = 0;

	/* Get system type code. If it's 0F9h, this is a Convertible. */
	rg.h.ah = 0xC0;
	int86x(0x15, &rg, &rg, &sg);	
	if (rg.h.ah == 0)
	{
		ptr = MK_FP(sg.es,rg.x.bx);
		if (ptr[2] == 0xF9)
		{
			is_convertible = 1;
			return;
		}
	}
	/* Detect VGA */
	rg.x.ax = 0x1200;
	rg.h.bl = 0x32;
	int86(0x10, &rg, &rg);
	if (rg.h.al == 0x12)
	{
		is_vga = 1;
		return;
	}

	/* Detect EGA */
	rg.h.ah = 0x12;
	rg.x.bx = 0xFF10;
	int86(0x10, &rg, &rg);
	if (rg.h.bh != 0xFF)
	{
		is_ega = 1;
		return;
	}
	/* Detect MDA */
	rg.h.ah = 0x0F;
	int86(0x10, &rg, &rg);
	if (rg.h.al == 7) 
	{
		is_mda = 1;
	}
	else
	{
		is_cga = 1;
	}
}


static void dos_load_palette(void)
{
	int n;
	union REGS rg;

	static uchr st_palette[] = 
	{
		0x00, 0x00, 0x00,
		0x00, 0x00, 0x2F,
		0x00, 0x2F, 0x00,
		0x00, 0x2F, 0x2F,
		0x2F, 0x00, 0x00,
		0x2F, 0x00, 0x2F,
		0x2F, 0x2F, 0x00,
		0x2F, 0x2F, 0x2F,
		0x0F, 0x0F, 0x0F,
		0x00, 0x00, 0x3F,
		0x00, 0x3F, 0x00,
		0x00, 0x3F, 0x3F,
		0x3F, 0x00, 0x00,
		0x3F, 0x00, 0x3F,
		0x3F, 0x3F, 0x00,
		0x3F, 0x3F, 0x3F,
	};
	if (is_ega || is_vga)
	{
		/* Map dark yellow to dark yellow, not brown */
		rg.x.ax = 0x1000;
		rg.h.bl = 6;
		rg.h.bh = 6;
		int86(0x10, &rg, &rg);
	}
	if (is_vga)
	{
		/* Load the palette for the VGA colours */	
		for (n = 0; n < 16; n++)
		{	
			rg.x.ax = 0x1010;
			rg.x.bx = (n < 8) ? n : (n | 0x30);
			rg.h.ch = st_palette[n * 3 + 1];
			rg.h.cl = st_palette[n * 3 + 2];
			rg.h.dh = st_palette[n * 3];
			int86(0x10, &rg, &rg);
		}
	}
}

/* Resort to asm to load the font; PPD can't pass BP through int86() */

char *egafont(unsigned char far *buffer, int count, int height, int map);

#asm
	.globl  large_data
	.globl  large_code
	.psect  _TEXT,class=CODE
	.globl	_egafont

_egafont:

	push	bp
	mov	bp,sp
	push	si
	push	di
	push	es

	mov	dx,ax	  ;DX = count
	mov	di,6[bp]
	mov	ax,8[bp]
	mov	es,ax	  ;ES:DI = address
	mov	cx,10[bp]  ;CX = height
	mov	bx,12[bp] ;BX = map
	mov	bh,cl	  ;BH=height BL=map 
	mov	cx,dx     ;CX = count
	mov	dx,#0     ;DX = base
	mov	ax,#0x1110
	push	bp	  ;ES:BP->font
	mov	bp,di
	int	#0x10
	pop	bp		
	
	mov	dx,#0	 ; Return NULL
	mov	ax,dx

	pop	es
	pop	di
	pop	si
	mov	sp,bp
	pop	bp
	retf	#8
#endasm



/* Load an 8x8 font into the video card's character generator 
 * (if it has one) */
static void dos_load_font(void)
{
	union REGS rg;
	struct SREGS sg;
	uchr fontbuf[4096];
	int n;

	dos_softfont = 0;
	if (is_vga)
	{
 	/* For VGA, it's simple: Just double each row in the font. */
		for (n = 0; n < 4096; n++) fontbuf[n] = dos_font[n/2];
		egafont(fontbuf, 256, 16, 0);
		dos_softfont = 1;
	}
	else if (is_ega)
	{
	/* For EGA, we have to stretch an 8x8 grid to 8x14. */
		for (n = 0; n < 256; n++) 
		{
			unsigned char *d = fontbuf + 14 * n;
			unsigned char *s = dos_font + 8 * n;

			d[ 0] = s[0];
			d[ 1] = d[2]  = s[1];
			d[ 3] = d[4]  = s[2];
			d[ 5] = d[6]  = s[3];
			d[ 7] = d[8]  = s[4];
			d[ 9] = d[10] = s[5];
			d[11] = d[12] = s[6];
			d[13] = s[7];
		}
		egafont(fontbuf, 256, 14, 0);
		dos_softfont = 1;
	}
	else if (is_convertible)
	{
	/* The Convertible is easy, because it has an 8x8 font */
		rg.x.ax = 0x1400;
		rg.x.bx = 0x800;
		rg.x.cx = 256;
		rg.x.dx = 0;
		rg.x.di = FP_OFF(dos_font);
		sg.es   = FP_SEG(dos_font);
		int86x(0x10, &rg, &rg, &sg);	
		dos_softfont = 1;
	}
}


static void dos_load_udgs(uchr *data, ushrt first, ushrt count)
{
	memcpy(dos_font + 8 * first, data, 8 * count);
	dos_load_font();
}

static void gfx_load_udgs(uchr *data, ushrt first, ushrt count)
{
	memcpy(dos_font + 8 * first, data, 8 * count);
}

static void dos_setwindow(int w, int h)
{
	window_x = (screen_w - w) / 2;	/* Top left hand corner */
	window_y = (screen_h - h) / 2;
	window_r = (window_x + w - 1);	/* Bottom right hand corner */
	window_b = (window_y + h - 1);
	window_w = w;			/* Window dimensions */
	window_h = h;
}


static void co40_mkattr(void)
{
	if (dos_ink == 9)
	{
		dos_attrmask |= 7;
		dos_attr &= ~7;
		dos_attr |= (dos_paper < 4) ? 7 : 0;
	}
	else if (dos_ink == 8) 
	{
		dos_attrmask &= ~7; 
	}
	else 
	{
		dos_attrmask |= 7;
		dos_attr &= ~7;
		dos_attr |= dos_attrmap[dos_ink & 7];	
	}
	if (dos_paper == 9)
	{
		dos_attrmask |= 0x70;
		dos_attr &= ~0x70;
		dos_attr |= (dos_ink >= 4 && dos_ink > 7) ? 0 : 7;
	}
	else if (dos_paper == 8)
	{
		dos_attrmask &= 0x70;
	}
	else
	{
		dos_attrmask |= 0x70;
		dos_attr &= ~0x70;
		dos_attr |= (dos_attrmap[dos_paper & 7]) << 4;	
	}
	if (dos_bright == 8)
	{
		dos_attrmask &= ~8;
	}
	else 
	{
		dos_attrmask |= 8;
		if (dos_bright) dos_attr |= 8; else dos_attr &= ~8;
	}
	if (dos_flash == 8)
	{
		dos_attrmask &= ~0x80;
	}
	else 
	{
		dos_attrmask |= 0x80;
		if (dos_flash) dos_attr |= 0x80; else dos_attr &= ~0x80;
	}
	if (dos_inverse)
	{
		dos_attr = (dos_attr & 0x88) |
			   (dos_attr & 0x70) >> 4 |
			   (dos_attr & 7)    << 4;
	}
}

static void mono_mkattr(void)
{
	uchr attr;

	if (dos_paper == dos_ink)
	{
		if (dos_paper < 4) attr = 0;
		else attr = 0x77;
	}
	else if (dos_paper < dos_ink)
	{
		attr = 0x07;
	}
	else
	{
		attr = 0x70;
	}
	/* If we're in mono mode but not on a real MDA, map bright 
 	 * to underline */
	if (!is_mda && dos_bright != 8 && dos_bright != 0)
	{
		attr |= 1;
	}

	if (dos_ink == 9)
	{
		dos_attrmask |= 7;
		dos_attr &= ~7;
		dos_attr |= (dos_paper < 4) ? 7 : 0;
	}
	else if (dos_ink == 8) 
	{
		dos_attrmask &= ~7; 
	}
	else 
	{
		dos_attrmask |= 7;
		dos_attr &= ~7;
		dos_attr |= (attr & 7);
	}
	if (dos_paper == 9)
	{
		dos_attrmask |= 0x70;
		dos_attr &= ~0x70;
		dos_attr |= (dos_ink >= 4 && dos_ink > 7) ? 0 : 7;
	}
	else if (dos_paper == 8)
	{
		dos_attrmask &= 0x70;
	}
	else
	{
		dos_attrmask |= 0x70;
		dos_attr &= ~0x70;
		dos_attr |= (attr & 0x70);
	}
	/* On something that isn't an MDA, we can't rely on white-on-grey
	 * text being visible, so force bright off if background is not black */
	if (!is_mda && (attr & 0x70))
	{
		dos_attrmask |= 8;
		dos_attr &= ~8;
	}
	else if (dos_bright == 8)
	{
		dos_attrmask &= ~8;
	}
	else 
	{
		dos_attrmask |= 8;
		if (dos_bright && dos_ink != 0) 
			dos_attr |= 8; 
		else dos_attr &= ~8;
	}
	if (dos_flash == 8)
	{
		dos_attrmask &= ~0x80;
	}
	else 
	{
		dos_attrmask |= 0x80;
		if (dos_flash) dos_attr |= 0x80; else dos_attr &= ~0x80;
	}
	if (dos_inverse)
	{
		dos_attr = (dos_attr & 0x88) |
			   (dos_attr & 0x70) >> 4 |
			   (dos_attr & 7)    << 4;
	}
/* Diagnostic dump of ink/paper to attr mapping 
	{
		char buf[80];
		int n;
		uchr far *pf = MK_FP(dos_framebuffer, 0);

		sprintf(buf, "ink=%d paper=%d attr=%02x mask=%02x\n",
			dos_ink, dos_paper, dos_attr, dos_attrmask);
		for (n = 0; buf[n]; n++)
		{
			*pf++ = buf[n];
			*pf++ = 0x70;
		}
		getch();
	}
*/
}



void co40_ink    (uchr c) { dos_ink     = c; co40_mkattr(); }
void co40_paper  (uchr c) { dos_paper   = c; co40_mkattr(); }
void co40_flash  (uchr c) { dos_flash   = c; co40_mkattr(); }
void co40_bright (uchr c) { dos_bright  = c; co40_mkattr(); }
void co40_inverse(uchr c) { dos_inverse = c; co40_mkattr(); }
void co40_over   (uchr c) { dos_over    = c; co40_mkattr(); }

void mono_ink    (uchr c) { dos_ink     = c; mono_mkattr(); }
void mono_paper  (uchr c) { dos_paper   = c; mono_mkattr(); }
void mono_flash  (uchr c) { dos_flash   = c; mono_mkattr(); }
void mono_bright (uchr c) { dos_bright  = c; mono_mkattr(); }
void mono_inverse(uchr c) { dos_inverse = c; mono_mkattr(); }
void mono_over   (uchr c) { dos_over    = c; mono_mkattr(); }

void cg40_ink    (uchr c)
{
	dos_ink = c;
	switch (c)
	{
		case 0: case 1: cg40_fg = 0x0000; break;
		case 2: case 3: cg40_fg = 0xaaaa; break;
		case 4: case 5: cg40_fg = 0x5555; break;
		case 6: case 7: cg40_fg = 0xffff; break;
		case 8: /* XXX */ break;
		case 9: if (cg40_bg & 0x5555) cg40_fg = 0;
			else cg40_fg = 0xffff;
	}
}

void cg40_paper(uchr c)
{
	dos_paper = c;
	switch (c)
	{
		case 0: case 1: cg40_bg = 0x0000; break;
		case 2: case 3: cg40_bg = 0xaaaa; break;
		case 4: case 5: cg40_bg = 0x5555; break;
		case 6: case 7: cg40_bg = 0xffff; break;
		case 8: /* XXX */ break;
		case 9: if (cg40_fg & 0x5555) cg40_bg = 0;
			else cg40_bg = 0xffff;
	}
}

void cg40_inverse(uchr c) { dos_inverse = c; }
void cg40_over(uchr c)    { dos_over = c; }

void dos_border (uchr c)
{
	union REGS rgi, rgo;

	rgi.x.ax = 0x0B00;		/* Set border colour */
	rgi.x.bx = dos_attrmap[c & 7];	/* Attribute to use */
	int86(0x10, &rgi, &rgo);

	if (window_w < screen_w || window_h < screen_h)
	{
		rgi.x.ax = 0x600;	/* Clear window */
		rgi.h.bh = (dos_attrmap[c & 7] << 4);	/* Attribute to use */

		/* Top panel */
		if (window_h < screen_h && window_y > 0)
		{
			rgi.x.cx = 0x0000;	/* Top left */
			rgi.x.dx = ((window_y - 1) << 8) | (screen_w - 1);
			int86(0x10, &rgi, &rgo);	
		}
		/* Bottom panel */
		if (window_h < screen_h && window_b < screen_h - 1)
		{
			rgi.x.cx = ((window_b + 1) << 8);
			rgi.x.dx = ((screen_h - 1) << 8) | (screen_w - 1);
			int86(0x10, &rgi, &rgo);	
		}
		/* Left panel */
		if (window_w < screen_w && window_x > 0)
		{
			rgi.x.cx = 0x0000;	/* Top left */
			rgi.x.dx = ((screen_h - 1) << 8) | (window_x - 1);
			int86(0x10, &rgi, &rgo);	
		}
		/* Right panel */
		if (window_w < screen_w && window_r < screen_w - 1)
		{
			rgi.x.cx = (window_r + 1);
			rgi.x.dx = ((screen_h - 1) << 8) | (screen_w - 1);
			int86(0x10, &rgi, &rgo);	
		}
	}
}

void cg40_border (uchr c)
{
	union REGS rgi, rgo;
	static uchr attrmap[8] = { 0, 0, 2, 2, 1, 1, 3, 3 };
	uchr attr = attrmap[c & 7];

	if (window_w < screen_w || window_h < screen_h)
	{
		rgi.x.ax = 0x600;	/* Clear window */
		rgi.h.bh = attr;	/* Attribute to use */

		/* Top panel */
		if (window_h < screen_h && window_y > 0)
		{
			rgi.x.cx = 0x0000;	/* Top left */
			rgi.x.dx = ((window_y - 1) << 8) | (screen_w - 1);
/* In mode 6 the BIOS assumes 80 columns, so convert X-coordinates here  */
			if (cg_mode == 6) rgi.h.dl = rgi.h.dl * 2 + 1;
			int86(0x10, &rgi, &rgo);	
		}
		/* Bottom panel */
		if (window_h < screen_h && window_b < screen_h - 1)
		{
			rgi.x.cx = ((window_b + 1) << 8);
			rgi.x.dx = ((screen_h - 1) << 8) | (screen_w - 1);
			if (cg_mode == 6) rgi.h.dl = rgi.h.dl * 2 + 1;
			int86(0x10, &rgi, &rgo);	
		}
		/* Left panel */
		if (window_w < screen_w && window_x > 0)
		{
			rgi.x.cx = 0x0000;	/* Top left */
			rgi.x.dx = ((screen_h - 1) << 8) | (window_x - 1);
			if (cg_mode == 6) rgi.h.dl = rgi.h.dl * 2 + 1;
			int86(0x10, &rgi, &rgo);	
		}
		/* Right panel */
		if (window_w < screen_w && window_r < screen_w - 1)
		{
			rgi.x.cx = (window_r + 1);
			rgi.x.dx = ((screen_h - 1) << 8) | (screen_w - 1);
			if (cg_mode == 6) 
			{
				rgi.h.dl  = rgi.h.dl * 2 + 1;
				rgi.h.cl *= 2;
			}
			int86(0x10, &rgi, &rgo);	
		}
	}
}


void mcga_border (uchr c)
{
	union REGS rgi, rgo;
	uchr attr = dos_attrmap[c & 7];

	if (window_w < screen_w || window_h < screen_h)
	{
		rgi.x.ax = 0x600;	/* Clear window */
		rgi.h.bh = attr;	/* Attribute to use */

		/* Top panel */
		if (window_h < screen_h && window_y > 0)
		{
			rgi.x.cx = 0x0000;	/* Top left */
			rgi.x.dx = ((window_y - 1) << 8) | (screen_w - 1);
			int86(0x10, &rgi, &rgo);	
		}
		/* Bottom panel */
		if (window_h < screen_h && window_b < screen_h - 1)
		{
			rgi.x.cx = ((window_b + 1) << 8);
			rgi.x.dx = ((screen_h - 1) << 8) | (screen_w - 1);
			int86(0x10, &rgi, &rgo);	
		}
		/* Left panel */
		if (window_w < screen_w && window_x > 0)
		{
			rgi.x.cx = 0x0000;	/* Top left */
			rgi.x.dx = ((screen_h - 1) << 8) | (window_x - 1);
			int86(0x10, &rgi, &rgo);	
		}
		/* Right panel */
		if (window_w < screen_w && window_r < screen_w - 1)
		{
			rgi.x.cx = (window_r + 1);
			rgi.x.dx = ((screen_h - 1) << 8) | (screen_w - 1);
			int86(0x10, &rgi, &rgo);	
		}
	}
}



void dos_cr()
{
	dos_x = window_x;
}

void dos_lf()
{
	union REGS rg;

	++dos_y;
	if (dos_y >= window_b)
	{
		/* Scroll up */
		--dos_y;
		rg.x.ax = 0x0601;
		rg.h.bh = dos_attr;
		rg.x.cx = (window_y << 8) | (window_x);
		rg.x.dx = (window_b << 8) | (window_r);
		int86(0x10, &rg, &rg);
	}
}

void cg40_lf()
{
	union REGS rg;

	++dos_y;
	if (dos_y >= window_b)
	{
		--dos_y;
		rg.x.ax = 0x0601;
		rg.h.bh = cg40_bg & 3;
		rg.x.cx = (window_y << 8) | (window_x);
		rg.x.dx = (window_b << 8) | (window_r);
		if (cg_mode == 6) 
		{
			rg.h.dl  = rg.h.dl * 2 + 1;
			rg.h.cl *= 2;
		}
		int86(0x10, &rg, &rg);
	}
}


void mcga_lf()
{
	union REGS rg;

	++dos_y;
	if (dos_y >= window_b)
	{
		--dos_y;
		rg.x.ax = 0x0601;
		rg.h.bh = (dos_attr & 0x70) >> 4;
		if (dos_attr & 8 && rg.h.bh != 0) rg.h.bh += 0x38;
		rg.x.cx = (window_y << 8) | (window_x);
		rg.x.dx = (window_b << 8) | (window_r);
		int86(0x10, &rg, &rg);
	}
}



void dos_clrscr(void)
{
	union REGS rg;

	dos_x = window_x;
	dos_y = window_y;

	rg.x.ax = 0x0600;
	rg.h.bh = dos_attr;
	rg.x.cx = (window_y << 8) | window_x;
	rg.x.dx = (window_b << 8) | window_r;
	int86(0x10, &rg, &rg);	
	dos_load_palette();
}

uchr dos_remap(uchr ch)
{
	switch(ch)
	{
		case  96: return 156;		/* Pound sign */
		case 128: return ' ';	
		case 131: return 223;		/* Block graphics common */
		case 133: return 222;		/* to Spectrum character set */
		case 138: return 221;		/* and codepage 437 */
		case 140: return 219;
	}
	if (ch >= 128 || ch < 32) return 0xFE;	/* Not mapped */
	return ch;
}

void dos_putchar(char ch)
{
	uchr far *p;

	if (ch == '\n' || dos_x > window_r)
	{
		dos_cr();
		dos_lf();
		if (ch == '\n') return;
	}
	if (ch == 8)
	{
		if (dos_x > window_x) --dos_x;
		return;
	}
	if (!dos_softfont)
	{
		ch = dos_remap(ch);
	}

 	p = MK_FP(dos_framebuffer, 2 * (dos_x + (dos_y * screen_w)));
	p[0] = ch;
	if (dos_attrmask == 0xFF) 
	{
		p[1] = dos_attr;
	}
	else if (dos_attrmask != 0)
	{
		p[1] &= ~dos_attrmask;
		p[1] |= (dos_attr & dos_attrmask);
	}
	++dos_x;
	/* Don't auto-CRLF until it tries to write the next character 
	if (dos_x > window_r)
	{
		dos_cr();
		dos_lf();
	} */
}


void cg40_showcur(uchr show)
{
	ushrt far *p;
	int n;

	if (show == cg40_curshown) return;

 	p = MK_FP(dos_framebuffer, 2 * dos_x + (dos_y * 320));
	for (n = 0; n < 8; n++)
	{
		p[0] ^= 0xFFFF;
		if (n & 1) 
		{
			p -= 0x1000;
			p += 40;
		}
		else	p += 0x1000;
	}
	cg40_curshown = show;
}



void cg40_putchar(char ch)
{
	uchr *src;
	ushrt far *p;
	int n;

	if (ch == '\n' || dos_x > window_r)
	{
		dos_cr();
		cg40_lf();
		if (ch == '\n') return;
	}
	if (ch == 8)
	{
		if (dos_x > window_x) --dos_x;
		return;
	}
	src = dos_font + 8 * (uchr)ch;


 	p = MK_FP(dos_framebuffer, 2 * dos_x + (dos_y * 320));
	for (n = 0; n < 8; n++)
	{
		ushrt v = dos_stretch[src[n]];

		if (dos_over)
		{
			if (dos_inverse)
			{
				p[0] = (p[0] & ~v) ^ (cg40_bg & v);
				p[0] = (p[0] & v)  ^ (cg40_fg & ~v);
			}
			else
			{
				p[0] = (p[0] & ~v) ^ (cg40_fg & v);
				p[0] = (p[0] & v)  ^ (cg40_bg & ~v);
			}
		}
		else
		{
			if (dos_inverse)
			{
				p[0] = (p[0] & ~v) | (cg40_bg & v);
				p[0] = (p[0] & v)  | (cg40_fg & ~v);
			}
			else
			{
				p[0] = (p[0] & ~v) | (cg40_fg & v);
				p[0] = (p[0] & v)  | (cg40_bg & ~v);
			}
		}
		if (n & 1) 
		{
			p -= 0x1000;
			p += 40;
		}
		else	p += 0x1000;
	}
	++dos_x;
	/* Don't auto-CRLF until it tries to write the next character 
	if (dos_x > window_r)
	{
		dos_cr();
		dos_lf();
	} */
}


void cg40_clrscr(void)
{
	union REGS rg;

	dos_x = window_x;
	dos_y = window_y;

	rg.x.ax = 0x0600;
	rg.h.bh = cg40_bg & 3;
	rg.x.cx = (window_y << 8) | window_x;
	rg.x.dx = (window_b << 8) | window_r;
	if (cg_mode == 6) 
	{
		rg.h.dl  = rg.h.dl * 2 + 1;
		rg.h.cl *= 2;
	}
	int86(0x10, &rg, &rg);	

	/* Set the background colour. We only support 0 (black) and 1 (blue)
	 * this way, because otherwise we'd risk duplicating a foreground
	 * colour. For example, if we allowed the background to be white,
	 * there'd be no reasonable mapping for black ink. */
	if (cg_mode != 6)
	{
		rg.x.ax = 0x0B00;
		rg.h.bl = (dos_paper == 1) ? 1 : 0;
		int86(0x10, &rg, &rg);
	}
	cg40_curshown = 0;
	dos_load_palette();
}

void mcga_clrscr(void)
{
	union REGS rg;

	dos_x = window_x;
	dos_y = window_y;

	rg.x.ax = 0x0600;
	rg.h.bh = ((dos_attr & 0x70) >> 4);
	if (dos_attr & 8 && rg.h.bh != 0) rg.h.bh += 0x38;
	rg.x.cx = (window_y << 8) | window_x;
	rg.x.dx = (window_b << 8) | window_r;
	int86(0x10, &rg, &rg);	

	cg40_curshown = 0;
	dos_load_palette();
}


void mcga_showcur(uchr show)
{
	uchr far *p;
	int n, m;

	if (show == cg40_curshown) return;

 	p = MK_FP(dos_framebuffer, 8 * (unsigned)dos_x + (2560 * (unsigned)dos_y));
	for (n = 0; n < 8; n++)
	{
		for (m = 0; m < 8; m++) p[m] ^= 0x3F;
		p += 320;
	}
	cg40_curshown = show;
}


void dos_showcur(uchr c)
{
	union REGS rg;

	rg.h.ah = 1;
	rg.x.cx = c ? dos_curshape : 0x3F1F;

	int86(0x10, &rg, &rg);
	rg.h.ah = 2;
	rg.h.bh = 0;
	rg.h.dh = dos_y;
	rg.h.dl = dos_x;
	int86(0x10, &rg, &rg);
}


void mcga_putchar(char ch)
{
	uchr *src;
	uchr far *p;
	int n, m;
	uchr mask, fg, bg;

	if (ch == '\n' || dos_x > window_r)
	{
		dos_cr();
		mcga_lf();
		if (ch == '\n') return;
	}
	if (ch == 8)
	{
		if (dos_x > window_x) --dos_x;
		return;
	}
	src = dos_font + 8 * (uchr)ch;

 	p = MK_FP(dos_framebuffer, 8 * (unsigned)dos_x + 2560 * (unsigned)dos_y);
	fg = dos_attr & 7;
	bg = (dos_attr >> 4) & 7;
	
	if (dos_attr & 8)
	{
		if (fg != 0) fg += 0x38;
		if (bg != 0) bg += 0x38;
	}
	for (n = 0; n < 8; n++)
	{
		for (m = 0, mask = 0x80; m < 8; m++, mask = mask >> 1)
		{
			if (src[n] & mask)	/* Foreground */
			{
				if (dos_over) p[m] ^= fg;
				else	      p[m] = fg; 
			}
			else
			{
				if (dos_over) p[m] ^= bg;
				else	      p[m] = bg; 
			}
		}
		p += 320;
	}
	++dos_x;
	/* Don't auto-CRLF until it tries to write the next character 
	if (dos_x > window_r)
	{
		dos_cr();
		dos_lf();
	} */
}


void ega_showcur(uchr show)
{
	uchr far *p;
	int n;

	if (show == cg40_curshown) return;

 	p = MK_FP(dos_framebuffer, (unsigned)dos_x + (320 * (unsigned)dos_y));
	outpw(0x3CE, 0x0007);	/* Colour don't care */
	outpw(0x3CE, 0x0A05);	/* Write mode 2, read mode 1 */
	outpw(0x3CE, 0xFF08);	/* Reset bit mask */
	outpw(0x3CE, 0x1803);	/* Read/modify/write bits */
	for (n = 0; n < 8; n++)
	{
		p[0] &= 0x0F;	/* Invert all 4 planes */
		p += 40;
	}
	outpw(0x3CE, 0x0005);	/* Default read/write mode */
	outpw(0x3CE, 0x0003);	/* Default Read/modify/write */
	outpw(0x3CE, 0x0F07);	/* Default Colour Don't Care */
	cg40_curshown = show;
}


void ega_putchar(char ch)
{
	uchr *src;
	uchr far *p;
	int n;
	uchr fg, bg;

	if (ch == '\n' || dos_x > window_r)
	{
		dos_cr();
		mcga_lf();
		if (ch == '\n') return;
	}
	if (ch == 8)
	{
		if (dos_x > window_x) --dos_x;
		return;
	}
	src = dos_font + 8 * (uchr)ch;

 	p = MK_FP(dos_framebuffer, (unsigned)dos_x + 320 * (unsigned)dos_y);
	fg = dos_attr & 7;
	bg = (dos_attr >> 4) & 7;
	
	if (dos_attr & 8)
	{
		if (fg != 0) fg += 0x38;
		if (bg != 0) bg += 0x38;
	}
	outpw(0x3CE, 0x0007);	/* Colour don't care */
	outpw(0x3CE, 0x0A05);	/* Write mode 2, read mode 1 */
	if (dos_over)
	{
		outpw(0x3CE, 0x1803);	/* Read/modify/write bits */
	}
	else
	{
		outpw(0x3CE, 0x0003);	/* Read/modify/write bits */
	}
	for (n = 0; n < 8; n++)
	{
		ushrt mask = (src[n] << 8) | 8;
		outpw(0x3CE, mask); /* Bit mask register */
		p[40 * n] &= fg;
		outpw(0x3CE, mask ^ 0xFF00); /* Invert bitmask */
		p[40 * n] &= bg;
	}
	outpw(0x3CE, 0xFF08);	/* Reset bit mask */
	outpw(0x3CE, 0x0005);	/* Default read/write mode */
	outpw(0x3CE, 0x0003);	/* Default Read/modify/write */
	outpw(0x3CE, 0x0F07);	/* Default Colour Don't Care */
	++dos_x;
	/* Don't auto-CRLF until it tries to write the next character 
	if (dos_x > window_r)
	{
		dos_cr();
		dos_lf();
	} */
}



char *dos_gets(char *buf, int len)
{
	char tmp[2];
	int n, xold, yold, maxshow, offset;

	(console->showcursor)(1);
	xold = dos_x;
	yold = dos_y;
	maxshow = window_r + 1 - dos_x;

	buf[0] = 0;
	while (1)
	{
		int c = getch();
		if (c == 0)	/* Function key */
		{
			c = 0x100 | getch();
		}
		if (c == 156) c = 0x60;	/* Spectrum pound sign */
		/* XXX More control keys here. Maybe even a command history? */
		switch (c)
		{
			case 8:	/* Backspace */
				if (strlen(buf))
				{
					buf[strlen(buf) - 1] = 0;
					/* Either draw the backspaced-over 
					 * character */
					(console->showcursor)(0);
					if (dos_x > xold)
					{
						--dos_x;
						(console->putch)(' ');
						--dos_x;
					} 
					/* Or redraw the whole buffer */
					else
					{
						offset = strlen(buf) - maxshow;
						if (offset < 0) offset = 0;

						dos_x = xold;
						for (n = 0; n < maxshow && buf[n]; n++)
							(console->putch)(buf[n + offset]);
					}
					(console->showcursor)(1);
				}
				break;	

			case '\n':
			case '\r':
				(console->showcursor)(0);	
				(console->putch)('\n');
				return buf;
		}
		if (strlen(buf) < len && c < 0x100 && c >= ' ')
		{
			tmp[0] = c;
			tmp[1] = 0;
			strcat(buf, tmp);
			/* If we're appending a character and there's room
			 * to show it, just draw that character */
			(console->showcursor)(0);
			if (dos_x <= window_r)
			{
				(console->putch)(c);
			}
			else	/* We have to scroll */
			{
				int offset = strlen(buf) - maxshow;
				if (offset < 0) offset = 0;

				dos_x = xold;
				for (n = 0; n < maxshow && buf[n]; n++)
					(console->putch)(buf[n + offset]);
			}
			(console->showcursor)(1);
		}	
	}
	(console->showcursor)(0);
	return buf;
}

void cg40_nop(uchr c) {}

CONSOLE cg40 = 
{ 
	dos_setwindow,
	dos_gets,
	cg40_putchar,
	cg40_clrscr,
	cg40_showcur,
	cg40_ink,	
	cg40_paper,
	cg40_nop,	/* flash */
	cg40_nop,	/* bright */
	cg40_inverse,
	cg40_nop,	/* over */
	cg40_border,
	gfx_load_udgs
};


CONSOLE co40 = 
{ 
	dos_setwindow,
	dos_gets,
	dos_putchar,
	dos_clrscr,
	dos_showcur,
	co40_ink,
	co40_paper,
	co40_flash,
	co40_bright,
	co40_inverse,
	co40_over,
	dos_border,
	dos_load_udgs
};

CONSOLE mono = 
{ 
	dos_setwindow,
	dos_gets,
	dos_putchar,
	dos_clrscr,
	dos_showcur,
	mono_ink,
	mono_paper,
	mono_flash,
	mono_bright,
	mono_inverse,
	mono_over,
	dos_border,
	dos_load_udgs
};

CONSOLE mcga = 
{ 
	dos_setwindow,
	dos_gets,
	mcga_putchar,
	mcga_clrscr,
	mcga_showcur,
	co40_ink,	
	co40_paper,
	co40_flash,
	co40_bright,
	co40_inverse,
	co40_over,
	mcga_border,
	gfx_load_udgs
};

CONSOLE ega = 
{ 
	dos_setwindow,
	dos_gets,
	ega_putchar,
	mcga_clrscr,
	ega_showcur,
	co40_ink,	
	co40_paper,
	co40_flash,
	co40_bright,
	co40_inverse,
	co40_over,
	mcga_border,
	gfx_load_udgs
};



void video_term(void)
{
	union REGS rg;

	rg.x.ax = old_mode;
	int86(0x10, &rg, &rg);
}


void video_init(int display)
{
	union REGS rg;
	FILE *fp = fopen("spectrum.rom", "rb");
#ifdef LIBDIR
	char fname[PATH_MAX];

	if (!fp)
	{
		strcpy(fname, LIBDIR);
		strcat(fname, "spectrum.rom");
		fp = fopen(fname, "rb");
	}
#endif

	/* We can't bundle the Spectrum ROM font, because it's not Free
	 * Software. So instead bundle the TSANS font, which is derived from
	 * the one in GEM (which *is* Free Software).
	 *
 	 * Always assuming, of course, that bitmap fonts can be copyrighted
	 * and (if they can) that the copyright in your jurisdiction hasn't
	 * already expired.
	 */
	if (fp)
	{
		fseek(fp, 0x3D00, SEEK_SET);
		fread(spectrum_font + 0x100, 1, 0x300, fp);
		fclose(fp);	
	}

	rg.x.ax = 0x0F00;

	int86(0x10, &rg, &rg);
	old_mode = rg.h.al;
	atexit(video_term);

	memcpy(dos_font, spectrum_font, sizeof(dos_font));	

	detect_video();

	/* Display is -1 for autodetect */
	if (display < 0)
	{
		/* Plump for the graphical modes over the text ones */
		if      (is_vga) display = 19;
		else if (is_ega) display = 13;
		else if (is_cga) display = 5;
		else if (is_mda) display = 7;
		else display = 2;	/* ? */
	}

	switch (display)
	{
		case 7:
			console = &mono; 
			screen_w = 80;
			screen_h = 25;
			dos_framebuffer = 0xB000;
	
			rg.x.ax = 0x07;	/* MONO */
			int86(0x10, &rg, &rg);
			dos_load_font();

			rg.h.ah = 3;	/* Get cursor shape */
			rg.h.bh = 0;
			int86(0x10, &rg, &rg);
			dos_curshape = rg.x.cx;
			break;
		case 4:
		case 5:
		case 6:
			console = &cg40;
			screen_w = 40;
			screen_h = 25;
			dos_framebuffer = 0xB800;

			cg_mode = display;	
			rg.x.ax = display;	/* CGA graphics modes */
			int86(0x10, &rg, &rg);
			break;
		case 13:
			console = &ega;
			screen_w = 40;
			screen_h = 25;
			dos_framebuffer = 0xA000;
	
			rg.x.ax = 0x0D;	/* EGA 320x200x16 graphics */
			int86(0x10, &rg, &rg);
			break;
		case 19:
			console = &mcga;
			screen_w = 40;
			screen_h = 25;
			dos_framebuffer = 0xA000;
	
			rg.x.ax = 0x13;	/* MCGA 320x200x256 graphics */
			int86(0x10, &rg, &rg);
			break;

		default:
			console = &co40;
			screen_w = 40;
			screen_h = 25;
			dos_framebuffer = 0xB800;
	
			rg.x.ax = 0x01;	/* CO40 */
			int86(0x10, &rg, &rg);
			dos_load_font();
	
			rg.h.ah = 3;	/* Get cursor shape */
			rg.h.bh = 0;
			int86(0x10, &rg, &rg);
			dos_curshape = rg.x.cx;
			break;
	}
	dos_load_palette();
	(console->showcursor)(0);
}
