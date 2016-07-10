/*
    psftools: Manipulate console fonts in the .PSF format
    Copyright (C) 2005-6  John Elliott

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
#include "cnvshell.h"
#include "psflib.h"
#include "dos.h"
#include "intrpt.h"

/* Set this to 1 to enable custom Herc mode settings (see Richard Wilton's 
 * "Programmer's Guide to PC & PS/2 Video Systems, p327) allowing character
 * sizes other than 9x14 
 *
 * This is turned off because when I tried it, it didn't work very well.
 * Fonts 8 pixels wide (ie, nearly all of them) put the screen in a 90-column 
 * mode that went off the right-hand side; an 8x14 font selected a 90x25 
 * mode that was too big to fit in available video RAM; and the smaller 
 * sizes caused my MDA monitor to lose sync. 
 */
#define HERC_CUSTOM	0


/* XXX Add support for the 3270PC Programmed Symbols board if none of the 
 * other graphics cards is present */
char *herc_loadfont (psf_byte far *buffer, int count, int width, int height, 
		int map);
char *ega_loadfont (psf_byte far *buffer, int count, int height, int map);
char *conv_loadfont(psf_byte far *buffer, int count, int height, int map);
char *apricot_loadfont(psf_byte far *buffer, int count, int width, int height,
		int map);
static char *apricot_pc_loadfont(psf_byte far *buffer, int count, 
		int width, int height, int map);
static char *apricot_f_loadfont(psf_byte far *buffer, int count, 
		int height, int map);

static int convertible = 0;
static int ega = 0;
static int herc = 0;
static int apricot = 0;

static int herc_probe(void);

char *video_probe(void)
{
	struct SREGS sg;
	union REGS rg;
	unsigned char far *ptr;

	/* Check for Apricot type PCs. This must be done first because
	 * the Apricot doesn't have to have an INT 10h */
	ptr = MK_FP(0x40, 0);
	if (ptr[4] == 0x34 && ptr[5] == 0x12 && 
	    ptr[6] == 0x78 && ptr[7] == 0x56 &&
	    ptr[1] < 3)
	{
		apricot = 1 + ptr[1];
		return NULL;
	}

	/* Check for IBM Convertible */
	rg.h.ah = 0xC0;
	int86x(0x15, &rg, &rg, &sg);
	if (rg.h.ah == 0)
	{
		ptr = MK_FP(sg.es, rg.x.bx);
		if (ptr[2] == 0xF9)
		{
			convertible = 1;
			return 0;
		}
	}
	/* Check for EGA/VGA */
	rg.h.ah = 0x12;
	rg.x.bx = 0xFF10;
	int86(0x10, &rg, &rg);
	if (rg.h.bh != 0xFF)
	{
		ega = 1;
		return 0;
	}
	/* Check for Hercules Plus here! */
	if (herc_probe())
	{
		herc = 1;
		return 0;
	}	
	      //1...5...10...15...20...25...30...35...40...45...50...55
	return "No supported video hardware detected. Must be one of:\n"
	       "* EGA/VGA\n"
	       "* IBM Convertible\n"
	       "* Hercules Plus\n"
	       "* Apricot PC, Xi or F-Series\n";
}

char *valid_font(PSF_FILE *psf)
{
	if (herc)
	{
#ifdef HERC_CUSTOM
		if (psf->psf_height < 4 || psf->psf_height > 16)
			return "Hercules Plus only supports fonts "
				"4-16 pixels high.";
		if (psf->psf_width > 9)
			return "Hardware does not support fonts wider than "
				"9 pixels.";
#else
		if (psf->psf_height != 14 || psf->psf_width != 8)
		{
			return "Fonts for Hercules Plus must be 8 pixels wide "
				"and 14 high.";
		}
#endif
	}
	else if (apricot) 
	{
		unsigned char far *ptr = MK_FP(0x70, 0);
		if (apricot == 1)	/* PC/Xi */
		{
			if (psf->psf_width > 10 || psf->psf_height > 16)
				return "Apricot PC/Xi fonts can be at most "
					"10x16 in size";

		}
		else
		{
			unsigned allowed = 0;

			if (psf->psf_width > 8) 
				return "Hardware does not support "
				"fonts wider than 8 pixels.";
			if (psf->psf_height == 8) allowed = 1;
			if (psf->psf_height == 10 && ptr[0x11] >= 0x12)
				allowed = 1;
			if (!allowed)
			{
				if (ptr[0x11] < 0x12) 
					return "Only 8x8 fonts can be loaded";
				return "Only 8x8 and 8x10 fonts can be loaded";
			}
		}
	}
	else
	{
		if (psf->psf_width > 8) return "Hardware does not support "
			"fonts wider than 8 pixels.";
		if (convertible && psf->psf_height != 8)
			return "Convertible only supports fonts 8 pixels high.";
	}
	return NULL;
}


static int herc_probe(void)
{
	unsigned n;
	psf_byte y;
	union REGS rg;

	rg.h.ah = 0x0F;
/* Check that we're in mono mode */
	int86(0x10, &rg, &rg);
	if (rg.h.al != 7) 
	{
		return 0;
	}

	di();
	for (n = 0; n < 50000; n++)
	{
		y = inportb(0x3BA);
		if ((y & 0x80) == 0) 
		{
/* OK, this is a Herc and not an MDA */	
			ei();
			switch(y & 0x70)
			{
				case 0x50: return 2;	/* InColor */
				case 0x10: return 1;	/* Herc Plus */
				default:   return 0;	/* Herc */
			}	
		}
	}
	ei();
	return 0;
}
 

static char *load_set(unsigned char far *data, int count, 
		int width, int height, int slot)
{
	char *s;

	if (apricot) s = apricot_loadfont( data, count, width, height, slot);
	else if (convertible) s = conv_loadfont( data, count, height, slot);
	else if (ega && slot < 8) s = ega_loadfont( data, count, height, slot);
	else if (herc) s = herc_loadfont( data, count, width, height, slot);
	else return "No supported video hardware present.";

	if (s) return s;
	if (slot == 1)	/* Loading into slot 1. Use it for high-intensity  */
	{		/* characters. */
		union REGS rg;
		if (convertible)
		{
			rg.x.ax = 0x1402;
			rg.h.bl = 3;	/* Use slot 1 for bold */
			int86(0x10, &rg, &rg);
		}
		if (ega)
		{
			rg.x.ax = 0x1103;
			rg.h.bl = 4;	/* Use slot 1 for bold, slot 0 normal */
			int86(0x10, &rg, &rg);
		}
	}
	return NULL;
}


char *install_font(PSF_FILE *psf, int usealt, int unused)
{
	char *s;
	psf_byte far *base;
	long remaining;
	int count;
	int slot;
	int uslot;

	base      = psf->psf_data;
	remaining = psf->psf_length;	
	slot      = 0;

	while (remaining > 0)
	{
		if (remaining < 256) count = remaining;
		else		     count = 256;

		uslot = slot;
	/* usealt swaps the first two slots over */
		if (slot < 2 && usealt) uslot ^= 1;
	/* Load up to 256 chars into the current slot */
		s = load_set(base, count, psf->psf_width, psf->psf_height,
				uslot);
		if (s) return s;
	/* Move on to the next chunk */
		slot++;
		base += count * psf->psf_charlen;
		remaining -= count;
	}
	return s;
}


char *conv_loadfont(psf_byte far *buffer, int count, int height, int slot)
{
	union REGS rg;
	struct SREGS sg;

	/* Convertible supports only 2 fonts */
	if (slot >= 2) return NULL;

	rg.x.ax = 0x1400;
	rg.h.bh = height;
	rg.h.bl = slot;
	rg.x.cx = count;
	rg.x.dx = 0;
	rg.x.di = FP_OFF(buffer);
	sg.es   = FP_SEG(buffer);
	int86x(0x10, &rg, &rg, &sg);
	return NULL;
}
	
static char *apricot_pc_loadfont(psf_byte far *buffer, int count, int width, 
		int height, int slot)
{
	int n, wb, x, y;
	unsigned char far *src;
	unsigned char far *dest;
/*	unsigned short far *ptr = MK_FP(0x70, 0x0C);
	unsigned char far *font = MK_FP(ptr[1], ptr[0]); */
	unsigned char far *font = MK_FP(0x80, 0);
	unsigned short w, mask, mk;

	if (slot >= 1) return NULL;
	if (count > 256) count = 256;
	wb = (width + 7) / 8;
	for (n = 0; n < count; n++)
	{
		dest = font + n * 32; 
		for (y = 0; y < 16; y++)
		{
			src  = buffer + (n * height + y) * wb;
			w = 0;
			mask = 1;
			mk = 0x80;
			if (y < height) for (x = 0; x < 10; x++)
			{
				if (x < width && ((*src & mk))) w |= mask;
				mask = mask << 1;
				mk = mk >> 1;
				if (mk == 0) { mk = 0x80; src++; }
			}
			dest[0] = (w & 0xFF);
			dest[1] &= 0xFC;
			dest[1] |= ((w >> 8) & 3);
			dest += 2;
		}
	}
	return NULL;
}

static char *apricot_f_loadfont(psf_byte far *buffer, int count, 
		int height, int slot)
{
	int max, n;
	unsigned short far *ptr = MK_FP(0x70, 0x0C);
	unsigned char far *font = MK_FP(ptr[1], ptr[0]);

	if (slot >= 1) return NULL;
	if (height == 10) font += 0x800;
	if (count > 256) count = 256;
	max = count * height;
	for (n = 0; n < max; n++)
	{
		font[n] = buffer[n];
	}
	return NULL;	
}


char *apricot_loadfont(psf_byte far *buffer, int count, int width, int height,
		int map)
{
	if (apricot == 1) return apricot_pc_loadfont(buffer, count, width,
		height, map);
	return apricot_f_loadfont(buffer, count, height, map);	
}

#if HERC_CUSTOM
static psf_byte h_params[] = 
{
	0x6D, 0x5A, 0x5C, 0x0F,	/* 8 pixels wide */
	0x61, 0x50, 0x52, 0x0F	/* 9 pixels wide */
};

static psf_byte v_params[] = 
{
	0x5C, 0x02, 0x58, 0x59,	/*  4 lines high */
	0x4A, 0x00, 0x46, 0x46,	/*  5 lines high */
	0x3D, 0x04, 0x3A, 0x3B,	/*  6 lines high */
	0x34, 0x06, 0x32, 0x33,	/*  7 lines high */
	0x2D, 0x02, 0x2B, 0x2C,	/*  8 lines high */
	0x28, 0x01, 0x26, 0x27,	/*  9 lines high */
	0x24, 0x00, 0x23, 0x23,	/* 10 lines high */
	0x20, 0x00, 0x1F, 0x20,	/* 11 lines high */
	0x1D, 0x0A, 0x1D, 0x1D,	/* 12 lines high */
	0x1B, 0x06, 0x1A, 0x1B,	/* 13 lines high */
	0x19, 0x06, 0x19, 0x19,	/* 13 lines high */
	0x17, 0x0A, 0x17, 0x17,	/* 13 lines high */
	0x16, 0x02, 0x15, 0x16,	/* 13 lines high */
};
#endif

char *herc_loadfont(psf_byte far *buffer, int count, 
		int width, int height, int slot)
{
	psf_byte far *dest;
	psf_byte far *src;
	psf_byte far *biosdata;
	unsigned n, m;
	int wb;
	static psf_byte x_ramfont = 1;

	if (slot >= 12) return NULL;	/* Herc supports 12 fonts */
	if (slot > 0) x_ramfont = 5;	/* 48k RAMfont mode */

	if (width < 8) width = 8;
	wb = (width + 7) / 8;
	biosdata = MK_FP(0x40, 0);
	/* Blank the screen */
	di();
	outportb(0x3B8, 0);
#if HERC_CUSTOM
	/* Reprogram the CRTC */
	m = (wb - 1) * 4;
	for (n = 0; n < 4; n++)
	{
		outportb(0x3B4, n);
		outportb(0x3B5, h_params[n + m]);
	}
	m = (height - 4) * 4;
	for (n = 0; n < 4; n++)
	{
		outportb(0x3B4, n + 4);
		outportb(0x3B5, v_params[n + m]);
	}
#endif
	/* Enable the RAM character generator */
	outportb(0x3B4, 0x14);	/* Select CRTC register 0x14 */

	outportb(0x3B5, x_ramfont); /* RAMfont mode */
	if (slot > 4) /* Slots 5-11 require the Herc to go into Full mode */
	{
		outportb(0x3BF, 3);
	}
	else
	{
		outportb(0x3BF, 1);	
		/* Make character RAM visible at 0xB4000 */
	}
	for (n = 0; n < count; n++)
	{
		dest = MK_FP(0xB400, slot * 4096 + n * 16);
		src = buffer + n * height * wb;
		for (m = 0; m < height; m++)
		{
			dest[m] = src[m * wb];
		}	
	}
	outportb(0x3BF, 0x00);	/* Hide character RAM and Full-mode RAM*/

#if HERC_CUSTOM
	biosdata[0x4A] = 720 / width;		/* CRT columns */
	biosdata[0x84] = (350 / height) - 1;	/* CRT rows minus 1 */
	n = (biosdata[0x84] + 1) * biosdata[0x4A]; /* CRT characters */
	n *= 2;
	biosdata[0x4C] = (n & 0xFF);
	biosdata[0x4D] = (n >> 8) & 0xFF;
#endif
	/* Re-enable video */
	outportb(0x3B8, biosdata[0x65]);
	ei();
	return NULL;
}
#undef di	/* We want to use it in asm */

/* Resort to asm to load the font, because PPD can't pass BP through int86() */
#asm
	.globl	large_data
	.globl	large_code
	.psect	_TEXT,class=CODE
	.globl	_ega_loadfont
	.signat	_ega_loadfont,16444

_ega_loadfont:
	push	bp
	mov	bp,sp

	push	si
	push	di
	push	es

	mov	dx,ax		; Count
	mov	di,6[bp]	; Address: offset
	mov	ax,8[bp]	; Address: segment
	mov	es,ax		; ES:DI->font
	mov	cx,10[bp]	; Height
	mov	bx,12[bp]	; Font to load
	mov	bh,cl		; BH=height BL=map
	
	mov	ax,#0x1110	; Function
	mov	cx,dx		; Count
	mov	dx,#0		; First char
	push	bp
	mov	bp,di		; ES:BP->font
	int	#0x10
	pop	bp
	pop	es
	pop	di
	pop	si
	pop	bp
	mov	dx,#0
	mov	ax,#0		; Return NULL
	retf	#8
#endasm


