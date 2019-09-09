/*
    psftools: Manipulate console fonts in the .PSF format
    Copyright (C) 2003  John Elliott

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
#include "zxflib.h"

/* Convert a Spectrum font to a PSF. Optionally also re-map the characters
 * to ISO-Latin1 */

#define MD_BARE   0	/* Spectrum chars only */
#define MD_LATIN1 1	/* Spectrum chars over Latin-1 */
#define MD_MERGE1 2	/* Spectrum chars over Latin-1, re-mapped */
#define MD_SYNTH1 3	/* Synthesize an entire Latin-1 font from Spectrum */
#define MD_MOVE1  4	/* Spectrum chars only, move sterling and copyright *
			 * to Latin-1 */

/* Treatment of chars 128-160 */

#define GFX_NONE  0	/* Don't include any graphics */
#define GFX_BLOCK 1	/* Include Spectrum block graphics */
#define GFX_UDG   2	/* Include Spectrum block graphics & UDGs */
#define GFX_1252  3	/* Include Windows 1252 graphics */
			/* (Not implemented) */
static int mode     = MD_LATIN1;
static int graphics = GFX_BLOCK;
static ZXF_FORMAT format = ZXF_P3DOS;

static int v1 = 0, v2 = 0;

static char helpbuf[2048];

/* Template latin-1 font */
static psf_byte latin1[] = 
{
#include "latin1.inc"
};

/* Program name */
char *cnv_progname = "ZX2PSF";

#ifdef CPM
static void synth1s(ZXF_FILE *src, PSF_FILE *dst);
#endif

/* ddash = 1 if option started with a double-dash; else 0 */
/* Return NULL if OK, else error string */
char *cnv_set_option(int ddash, char *variable, char *value)
{
    if (!stricmp(variable, "mode")) 
    {
        if      (!stricmp(value, "bare"  )) mode = MD_BARE;
        else if (!stricmp(value, "latin1")) mode = MD_LATIN1;
        else if (!stricmp(value, "merge1")) mode = MD_MERGE1;
        else if (!stricmp(value, "synth1")) mode = MD_SYNTH1;
	else if (!stricmp(value, "move1" )) mode = MD_MOVE1;
        else return "Invalid value for --mode option. Use the --help option for the help screen.";
        return NULL;
    }
    if (!stricmp(variable, "graphics")) 
    {
        if      (!stricmp(value, "none"  )) graphics = GFX_NONE;
        else if (!stricmp(value, "block" )) graphics = GFX_BLOCK;
        else if (!stricmp(value, "udg"   )) graphics = GFX_UDG;
/*      else if (!stricmp(value, "1252"  )) graphics = GFX_1252; */
        else return "Invalid value for --graphics option. Use the --help option for the help screen.";
        return NULL;
    }
    if (!stricmp(variable, "psf1"))   { v1 = 1; return NULL; }
    if (!stricmp(variable, "psf2"))   { v2 = 1; return NULL; }
    if (!stricmp(variable, "rom"))    { format=ZXF_ROM; return NULL; }
    if (!stricmp(variable, "sna"))    { format=ZXF_SNA; return NULL; }

    if (strlen(variable) > 2000) variable[2000] = 0;
    sprintf(helpbuf, "Unknown option: %s\n", variable);
    return helpbuf;
}


/* Return help string */
char *cnv_help(void)
    {
    sprintf(helpbuf, "Syntax: %s zxfont psffile { options }\n\n", cnv_progname);
    strcat (helpbuf, "Options: \n"
                    "        --mode=bare      - Leave non-Spectrum characters as blank\n"
                    "        --mode=latin1    - Leave non-Spectrum characters as ISO Latin-1\n"
                    "        --mode=merge1    - As latin1, but move pound sign, arrow and copyright\n"
                    "        --mode=synth1    - Synthesise Latin-1 characters from Spectrum ones\n"
		    "        --mode=move1     - As bare, but move pound sign, arrow and copyright\n"
                    "        --graphics=none  - Spectrum graphics characters not transferred\n"
                    "        --graphics=block - Spectrum block graphics transferred\n"
                    "        --graphics=udg   - Spectrum block graphics transferred & UDGs simulated\n"
/*                  "        --graphics=1252  - Characters 128-159 become Windows code page 1252\n"
                    "                           characters\n"*/
		    "        --sna            - Treat input file as SNA snapshot\n"
		    "        --rom            - Treat input file as ROM dump\n"
            );
    return helpbuf;
    }


/* Plain copy of character bitmap ZX->PSF */
static void copychar(ZXF_FILE *src, PSF_FILE *dst, int from, int to)
{
    zxf_byte *sptr = src->zxf_data + 8 * (from - 32);
    psf_byte *dptr = dst->psf_data + 8 * to;
    
    memcpy(dptr, sptr, 8);
}

static void copyudg(ZXF_FILE *src, PSF_FILE *dst, int from, int to)
{
    zxf_byte *sptr = src->zxf_udgs + 8 * (from - 'A');
    psf_byte *dptr = dst->psf_data + 8 * to;
    
    memcpy(dptr, sptr, 8);
}


/* Put an accent on an upper-case character. "shift" moves the character 
  "from2" up or down. The character "from1" is squished vertically. */
static void ucaccent(PSF_FILE *dst, int from1, int from2, int to, int shift)
{
    int x;
    psf_byte buf1[8], buf2[8];
    psf_byte *sptr1 = dst->psf_data + 8 * from1;
    psf_byte *sptr2 = dst->psf_data + 8 * from2 - shift;
    psf_byte *dptr  = dst->psf_data + 8 * to;

    memcpy(buf2, sptr2, 8);
    memcpy(buf1, sptr1, 8);
    while (shift < 0) { buf2[8 + shift] = 0; shift++; }
    while (shift > 0) { buf2[shift]     = 0; shift--; }
    sptr2 = buf2;

    for (x = 5; x > 0; x--) buf1[x] = buf1[x-1];
    buf1[0] = 0;
    sptr1 = buf1;

    for (x = 0; x < 8; x++) *dptr++ = *sptr1++ | *sptr2++;
}


/* Put an accent on a lower-case character. Unlike "ucaccent" this 
 * does not squish the character. */
static void lcaccent(PSF_FILE *dst, int from1, int from2, int to, int shift)
{	
    int x;
    psf_byte buf[8];
    psf_byte *sptr1 = dst->psf_data + 8 * from1;
    psf_byte *sptr2 = dst->psf_data + 8 * from2 - shift;
    psf_byte *dptr  = dst->psf_data + 8 * to;

    memcpy(buf, sptr2, 8);
    while (shift < 0) { buf[8 + shift] = 0; shift++; }
    while (shift > 0) { buf[shift]     = 0; shift--; }
    sptr2 = buf;

    for (x = 0; x < 8; x++) *dptr++ = *sptr1++ | *sptr2++;
}

/* Squeeze two characters into horizontal space for one (eg: A,E -> Æ) */
static void hcrush(PSF_FILE *dst, int from1, int from2, int to)
{
	int x;

	psf_byte *sptr1 = dst->psf_data + 8 * from1;
	psf_byte *sptr2 = dst->psf_data + 8 * from2;
	psf_byte *dptr  = dst->psf_data + 8 * to;

	for (x = 0; x < 8; x++) 
	{
		*dptr++ = crush(*sptr1++) | (crush(*sptr2++) >> 4);
	}
}


/* Synthesize Latin-1 font from ZX */
static void synth1(ZXF_FILE *src, PSF_FILE *dst)
{	
	int x, y, m, n;

	/* Char 0 = inverse ? mark */
	copychar(src, dst, '?', 0);
	dst->psf_data[0] ^= 0x7E;
	dst->psf_data[7] ^= 0x7E;
	for (x = 1; x < 7; x++) dst->psf_data[x] ^= 0xFF;

        hcrush  (dst, 'O', 'E',  2);	/* Chars 2 & 3: Diphthongs */
        hcrush  (dst, 'o', 'e',  3);


	/* Char 21: not-equal */
	lcaccent(dst, '=', '/', 0x15, 0);

	/* Upside-down arrow */
	for (x = 0; x < 8; x++) dst->psf_data[0xC8 + x] = 
				src->zxf_data[0x1F7 - x];

	/* Rotated arrow */
	dst->psf_data[0xCF] = 0;	
	n = 0x80;
	for (x = 0; x < 8; x++)
	{
		m = 0x80;
		for (y = 0; y < 8; y++)
		{
			if (dst->psf_data[0xC0 + x] & m)
			{
				dst->psf_data[0xD8 + y] |= n;
			}
			else	dst->psf_data[0xD8 + y] &= ~n;
			m = m >> 1;
		}
		n = n >> 1;
	}
	/* Other rotated arrows, & double-headed arrows */
	for (x = 0; x < 8; x++)
	{
		dst->psf_data[0xD0 + x] = flip(dst->psf_data[0xD8 + x]);
		dst->psf_data[0xE8 + x] = dst->psf_data[0xD0+x] | 
					  dst->psf_data[0xD8+x];
                dst->psf_data[0xE0 + x] = dst->psf_data[0xC0+x] |
                                          dst->psf_data[0xC8+x];

	}

	
	/* Char 96: Backquote */
	for (x = 0; x < 8; x++) dst->psf_data[0x300 + x] = 
				flip(src->zxf_data[0x38 + x]);

	/* Char 127: Y umlaut */
	ucaccent(dst, 'Y', 0xA8, 0x7F, 0);

	/* 160 on */
	for (x = 0; x < 8; x++) dst->psf_data[0x508 + x] = 
				src->zxf_data[0x0F - x];
	ucaccent(dst, '|','c', 0xA2, -1);
	lcaccent(dst, 'Y','=', 0xA5,  0);

	hcrush(dst, '<', '<', 0xAB);
	hcrush(dst, '>', '>', 0xBB);

	for (x = 0; x < 8; x++) dst->psf_data[0x5F8 + x] = 
				flip(src->zxf_data[0xFF - x]);

#ifdef CPM
	synth1s(src, dst); 
}

static void synth1s(ZXF_FILE *src, PSF_FILE *dst)
{
#endif
	/* 192 on */
        ucaccent(dst, 'A', 0x60, 0xC0, -1);
        ucaccent(dst, 'A', 0x27, 0xC1, -1);
        ucaccent(dst, 'A', 0x5E, 0xC2,  0);
        ucaccent(dst, 'A', 0x7E, 0xC3, -1);
        ucaccent(dst, 'A', 0xA8, 0xC4,  0);
        ucaccent(dst, 'A', 0xB0, 0xC5,  0);
        hcrush  (dst, 'A', 'E',  0xC6);
	lcaccent(dst, 0xB8,'C',  0xC7, -1);
        ucaccent(dst, 'E', 0x60, 0xC8, -1);
        ucaccent(dst, 'E', 0x27, 0xC9, -1);
        ucaccent(dst, 'E', 0x5E, 0xCA,  0);
        ucaccent(dst, 'E', 0xA8, 0xCB,  0);
        ucaccent(dst, 'I', 0x60, 0xCC, -1);
        ucaccent(dst, 'I', 0x27, 0xCD, -1);
        ucaccent(dst, 'I', 0x5E, 0xCE,  0);
        ucaccent(dst, 'I', 0xA8, 0xCF,  0);
	lcaccent(dst, 'D', '-',  0xD0,  0);
        ucaccent(dst, 'N', 0x7E, 0xD1, -1);
        ucaccent(dst, 'O', 0x60, 0xD2, -1);
        ucaccent(dst, 'O', 0x27, 0xD3, -1);
        ucaccent(dst, 'O', 0x5E, 0xD4,  0);
        ucaccent(dst, 'O', 0x7E, 0xD5, -1);
        ucaccent(dst, 'O', 0xA8, 0xD6,  0);
        lcaccent(dst, ' ', 'x',  0xD7, -1);
        lcaccent(dst, 'O', '/',  0xD8,  0);
        ucaccent(dst, 'U', 0x60, 0xD9, -1);
        ucaccent(dst, 'U', 0x27, 0xDA, -1);
        ucaccent(dst, 'U', 0x5E, 0xDB,  0);
        ucaccent(dst, 'U', 0xA8, 0xDC,  0);
        ucaccent(dst, 'Y', 0x27, 0xDD, -1);
        /* 0xDE - thorn */
	/* 0xDF - Scharfes S */

	/* 224 on */
	lcaccent(dst, 'a', 0x60, 0xE0, -1);
	lcaccent(dst, 'a', 0x27, 0xE1, -1);
	lcaccent(dst, 'a', 0x5E, 0xE2,  0);
	lcaccent(dst, 'a', 0x7E, 0xE3, -1);
	lcaccent(dst, 'a', 0xA8, 0xE4,  0);
	lcaccent(dst, 'a', 0xB0, 0xE5,  0);
	hcrush  (dst, 'a', 'e',  0xE6);
	lcaccent(dst, 0xB8,'c',  0xE7, -1);
	lcaccent(dst, 'e', 0x60, 0xE8, -1);
	lcaccent(dst, 'e', 0x27, 0xE9, -1);
	lcaccent(dst, 'e', 0x5E, 0xEA,  0);
	lcaccent(dst, 'e', 0xA8, 0xEB,  0);
	lcaccent(dst, 'i', 0x60, 0xEC, -1);
	lcaccent(dst, 'i', 0x27, 0xED, -1);
	lcaccent(dst, 'i', 0x5E, 0xEE,  0);
	lcaccent(dst, 'i', 0xA8, 0xEF,  0);
	/* 0xf0 - d bar */
	lcaccent(dst, 'n', 0x7E, 0xF1, -1);
	lcaccent(dst, 'o', 0x60, 0xF2, -1);
	lcaccent(dst, 'o', 0x27, 0xF3, -1);
	lcaccent(dst, 'o', 0x5E, 0xF4,  0);
	lcaccent(dst, 'o', 0x7E, 0xF5, -1);
	lcaccent(dst, 'o', 0xA8, 0xF6,  0);
	lcaccent(dst, '-', ':',  0xF7,  0);
	lcaccent(dst, 'o', '/',  0xF8,  0);
	lcaccent(dst, 'u', 0x60, 0xF9, -1);
	lcaccent(dst, 'u', 0x27, 0xFA, -1);
	lcaccent(dst, 'u', 0x5E, 0xFB,  0);
	lcaccent(dst, 'u', 0xA8, 0xFC,  0);
	lcaccent(dst, 'y', 0x27, 0xFD, -1);
	/* 0xFE - thorn */
	lcaccent(dst, 'y', 0xa8, 0xFF,  0);
}


psf_word lpsf[32] = { 
	0xFFFD, 0x2248, 0x0152, 0x0153, 0x25C6, 0x2409, 0x240C, 0x240D,
	0x240A, 0x2591, 0x2592, 0x2593, 0x2588, 0x2584, 0x2580, 0x258C,
	0x2590, 0x2424, 0x240B, 0x2264, 0x2265, 0x2260, 0x25C0, 0x25B6,
	0x2191, 0x2193, 0x2192, 0x2190, 0x2195, 0x2194, 0x21B5, 0x03C0 };

psf_word gpsf[32] = {
	0xF801, 0x2575, 0x2575, 0x2514, 0x2577, 0x2502, 0x250C, 0x251C,
	0x2574, 0x2518, 0x2500, 0x2534, 0x2510, 0x2524, 0x252C, 0x253C,
	0xF803, 0x2579, 0x257A, 0x2517, 0x257B, 0x2503, 0x250F, 0x2523,
	0x2578, 0x251B, 0x2501, 0x253B, 0x2513, 0x2563, 0x2533, 0x254B };

psf_word upsf[16] = {
	0x2007, 0x259D, 0x2598, 0x2580, 0x2597, 0x2590, 0x259A, 0x259C,
	0x2596, 0x259E, 0x258C, 0x259B, 0x2584, 0x259F, 0x2599, 0x2598};


void add_unicode_dir(PSF_FILE *psf)
{
	int ch;

	for (ch = 32; ch < 128; ch++)
	{
		switch(ch)
		{
			case 0x5E:	// Uparrow 
				if (mode == MD_BARE || mode == MD_LATIN1 ||
				    mode == MD_MOVE1)
					psf_unicode_add(psf, ch, 0x2191);
				else	psf_unicode_add(psf, ch, ch);
				break;
			case 0x60:	// Pound 
				if (mode == MD_BARE || mode == MD_LATIN1 ||
				    mode == MD_MOVE1)
					psf_unicode_add(psf, ch, 0xA3);
				else	psf_unicode_add(psf, ch, ch);
				break;
			case 0x7F:	// Copyright 
				if (mode == MD_BARE || mode == MD_LATIN1 ||
				    mode == MD_MOVE1)
					psf_unicode_add(psf, ch, 0xA9);
				else	psf_unicode_add(psf, ch, ch);
				break;

			default:	psf_unicode_add(psf, ch, ch);
					break;
		}
	}
	psf_unicode_add(psf, 0x20, 0xA0);	/* 2nd space */
	psf_unicode_add(psf, 0x4B, 0x212A);	/* K -> Kelvin */
	psf_unicode_add(psf, 0x5F, 0xF804);	/* _ -> Private use */

	if (mode == MD_BARE || mode == MD_MOVE1)
	{
		switch(graphics)
		{
			case GFX_UDG:
				for (ch = 0; ch < 21; ch++) psf_unicode_add(psf, 0x90 + ch, 0x24B6 + ch);
			case GFX_BLOCK:
				for (ch = 0; ch < 16; ch++) psf_unicode_add(psf, 0x80 + ch, upsf[ch]);
				break;
		}
	}
	else
	{
		psf_unicode_add(psf, 0xA0, 0x2423);
		for (ch = 0xA1; ch <= 0xFF; ch++)
		{
			if (mode == MD_LATIN1 && ch == 0xA3) continue;
			if (mode == MD_LATIN1 && ch == 0xA9) continue;
	 		psf_unicode_add(psf, ch, ch);
		}
		for (ch = 0;    ch < 32;   ch++) 
		{
			if (mode == MD_LATIN1 && ch == 0x18) continue;
			psf_unicode_add(psf, ch, lpsf[ch]);
		}
		psf_unicode_add(psf, 0xAF, 0xFFF8);
		switch(graphics)
		{
			case GFX_NONE:
				for (ch = 0; ch < 32; ch++) psf_unicode_add(psf, 0x80 + ch, gpsf[ch]);
				psf_unicode_add(psf, 0x93, 0x255A);
				psf_unicode_add(psf, 0x95, 0x2551);
				psf_unicode_add(psf, 0x96, 0x2554);
				psf_unicode_add(psf, 0x97, 0x2560);
				psf_unicode_add(psf, 0x99, 0x255D);
				psf_unicode_add(psf, 0x9A, 0x2550);
				psf_unicode_add(psf, 0x9B, 0x2569);
				psf_unicode_add(psf, 0x9C, 0x2557);
				psf_unicode_add(psf, 0x9D, 0x252B);
				psf_unicode_add(psf, 0x9E, 0x2566);
				psf_unicode_add(psf, 0x9F, 0x256C);
				break;
			case GFX_UDG:
				for (ch = 0; ch < 16; ch++) psf_unicode_add(psf, 0x90 + ch, 0x24B6 + ch);
			case GFX_BLOCK:
				for (ch = 0; ch < 16; ch++) psf_unicode_add(psf, 0x80 + ch, upsf[ch]);
				break;
		}
	}

}


/* Do the conversion */
char *cnv_execute(FILE *infile, FILE *outfile)
{	
    int rv;
    int x;
    psf_byte *b;
    PSF_FILE psf, psflatin1;
    ZXF_FILE zxf;
    int nchars = 256;

    zxf_file_new(&zxf);
    rv = zxf_file_read(&zxf, infile, format);	
    if (rv != ZXF_E_OK) return zxf_error_string(rv);
    
    if (mode == MD_BARE) switch(graphics)
    {
    	case GFX_NONE:  nchars = 128; break;
    	case GFX_BLOCK: nchars = 144; break;
    	case GFX_1252:  nchars = 160; break;
    	case GFX_UDG:   nchars = 165; break;
    }
    if (mode == MD_MOVE1) nchars += 42; // Reach the copyright

	/* Load the latin-1 template from memory */
    psf_file_new(&psflatin1);
    psf_file_new(&psf);
    if (rv == PSF_E_OK) rv = psf_memory_read(&psflatin1, latin1, sizeof latin1);
    if (rv == PSF_E_OK) rv = psf_file_create(&psf, 8, 8, nchars, 0);	
    if (rv == PSF_E_OK)
    {
        /* Initialise the PSF */
	switch (mode)
	{
		case MD_BARE:
		case MD_MOVE1:
        		memset(psf.psf_data, 0, nchars * 8);
			break;
		default:
			memcpy(psf.psf_data, psflatin1.psf_data, 2048);
			break;
	}
	switch(mode)
	{
		case MD_MERGE1:
			for (x = 32; x <= 127; x++) switch(x)
			{
                	    case 0x5E: copychar(&zxf, &psf, x, 0x18); break; 
                	    case 0x60: copychar(&zxf, &psf, x, 0xA3); break;
			    case 0x7F: copychar(&zxf, &psf, x, 0xA9); break;
			    default:   copychar(&zxf, &psf, x, x);    break;
			}
			break;
		case MD_MOVE1:
			for (x = 32; x <= 127; x++) switch(x)
			{
                	    case 0x60: copychar(&zxf, &psf, x, 0xA3); break;
			    case 0x7F: copychar(&zxf, &psf, x, 0xA9); break;
			    default:   copychar(&zxf, &psf, x, x);    break;
			}
			break;
		default:
            		for (x = 32; x <= 127; x++) copychar(&zxf, &psf, x, x);
			break;
	}
	if (mode == MD_SYNTH1) synth1(&zxf, &psf);
       
	switch(graphics)
        {
            case GFX_1252:
                /* XXX Windows 1252 graphics here */
                break;
            case GFX_UDG: /* UDGs */
		if (zxf.zxf_format == ZXF_SNA) /* From a SNA; we have real UDGs*/
		{
			if (mode == MD_BARE)
				for (x = 0; x < 21; x++) 
					copyudg(&zxf, &psf, x+'A',x+144);
                	else    for (x = 0; x < 16; x++) 
					copyudg(&zxf, &psf, x+'A',x+144);
		}
		else if (mode == MD_BARE) 
			for (x = 0; x < 21; x++) 
				copychar(&zxf, &psf, x+'A',x+144);
                else    for (x = 0; x < 16; x++) 
			copychar(&zxf, &psf, x+'A',x+144);
                /* FALL THROUGH TO */
            case GFX_BLOCK:
			memset(psf.psf_data + 1024, 0, 128);
		        for (x = 128; x < 144; x++)
		        {
			        b = psf.psf_data + (x * 8);
			        if (x & 1) { b[0] |= 0x0F; b[1] |= 0x0F; b[2] |= 0x0F; b[3] |= 0x0F; }
			        if (x & 2) { b[0] |= 0xF0; b[1] |= 0xF0; b[2] |= 0xF0; b[3] |= 0xF0; }
			        if (x & 4) { b[4] |= 0x0F; b[5] |= 0x0F; b[6] |= 0x0F; b[7] |= 0x0F; }
			        if (x & 8) { b[4] |= 0xF0; b[5] |= 0xF0; b[6] |= 0xF0; b[7] |= 0xF0; }	
		        }
                break;                
        }
	psf_file_create_unicode(&psf);
	add_unicode_dir(&psf);
        if (v1) psf_force_v1(&psf);
        if (v2) psf_force_v2(&psf);

	rv = psf_file_write(&psf, outfile);
    }
    
    if (rv != PSF_E_OK)
    {
    	zxf_file_delete(&zxf);
    	psf_file_delete(&psf);
    	psf_file_delete(&psflatin1);
    	return psf_error_string(rv);
    }
    zxf_file_delete(&zxf);
    psf_file_delete(&psf);
    psf_file_delete(&psflatin1);
    return NULL;
}

