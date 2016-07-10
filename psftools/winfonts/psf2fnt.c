/*
    psftools: Manipulate console fonts in the .PSF format
    Copyright (C) 2000, 2005  John Elliott

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

/* Convert a PSF file to a Windows .FNT font. Note that to get this
 * font displayable by Windows, it has to be added to a program's 
 * resources or converted to a .FON, which is a black art. */

#include "cnvshell.h"
#include "psflib.h"
#include "mswfnt.h"


static char *copyright = "Copyright is unknown";
static char *font_name = "ConvPSF";
static char fnbuf[80];
static char cpybuf[200];

static PSF_FILE psfi;	/* Input file */
static PSF_FILE psf;	/* Source file */

static PSF_MAPPING *codepage = NULL;
static MSW_FONTINFO fnt_head;	/* Target file */
static unsigned nchars;		/* No. of characters (256 or 512) */
static psf_byte charset = 0; 	/* ANSI charset */
static psf_byte bold    = 0;
static psf_byte italic  = 0;	/* Bold/italic */
static unsigned chartable_len;
static unsigned head_len;	/* Lengths of various bits */
static unsigned bitmap_len;
static unsigned bitmap_width;
static unsigned font_ver = 0x200;	/* Use 2.0 format by default. */
				/* Current Windows versions can't use the
				 * 1.0 format, and 3.0 (real mode) can't 
				 * use the 3.0 format */
static unsigned char defchar = 0x2E;
static unsigned char brkchar = 0x20;
static int pointsize = -1;
static int vertres = 48;
static int horzres = 48;
static int ascent = -1;
static int firstchar = 0;
static int lastchar = -1;


/* Write character table */
static int msw_chartable_write(FILE *fp)
{
	unsigned n;

	/* No character table in 1.x fixed font */
	if (font_ver == 0x100) return 0;

/* lastchar + 1 for the 'absolute-space' character */
	for (n = firstchar; n <= lastchar + 1; n++)
	{
		if (write_word(fp, psf.psf_width)) return -1;
		if (write_word(fp, (msw_word)(head_len + chartable_len + 
					(psf.psf_charlen * n))) ) return -1;
		if (font_ver == 0x300 && write_word(fp, 0)) return -1;
	}
	return 0;
}

/* Write glyphs table */
static int msw_glyphs_write(FILE *fp)
{
	unsigned n, m, x, vx, ch, wb;
	psf_byte v, mask, b;
	
	wb = (psf.psf_width + 7) / 8;
	if (font_ver == 0x100) 
	{
		if (write_byte(fp, 0)) return -1;	/* dfReserved */
		for (m = 0; m < psf.psf_height; m++)
		{ 
			for (vx = n = 0; n < bitmap_width; n++)
			{
				mask = 0x80;
				v = 0;
				for (x = 0; x < 8; x++)
				{
					ch = vx / psf.psf_width;
					psf_get_pixel(&psf, ch, 
						vx % psf.psf_width, m, &b);
		 			if (b) v |= mask; 
					++vx;
					mask = mask >> 1;
				} 
				if (write_byte(fp, v)) return -1;
			}
	 	}
	}
	else 
	{ 
		for (n = firstchar; n <= lastchar; n++) 
		    for (x = 0; x < wb; x++)
			for (m = 0; m < psf.psf_height; m++)
			{
				if (write_byte(fp, psf.psf_data[n * 
					    psf.psf_charlen + m * wb + x])) 
		 			return -1;
			}
		for (m = 0; m < psf.psf_charlen; m++)
		{
			if (write_byte(fp, 0)) return -1;
		}		
	}
	return 0;
}

/* Write font name */
static int msw_name_write(FILE *fp)
{
	if (fputs(font_name, fp)==EOF) return -1;

	return write_byte(fp, 0);   /* Terminating zero */
}

/* Set up the font header */
static void setup_fnthead(MSW_FONTINFO *head)
{
	head->dfVersion         = font_ver;

	if (font_ver == 0x300) head_len +=  30;

	head->dfSize             = head_len + chartable_len 
					+ bitmap_len + 1 +strlen(font_name);
	strncpy(head->dfCopyright, copyright, 60);
	head->dfCopyright[59]   = 0;
	head->dfType            = 0;            /* Raster font */
	head->dfPoints          = pointsize;    /* Point size := pixel height */
	head->dfVertRes         = vertres;      /* this is based on CGA 
						   	    aspect ratio! */
	head->dfHorizRes        = horzres;      /* this isn't based on 
						   		anything */
	head->dfAscent          = ascent;
	head->dfInternalLeading = 0;
	head->dfExternalLeading = 0;
	head->dfItalic          = italic;
	head->dfUnderline       = 0;
	head->dfStrikeOut       = 0;
	head->dfWeight          = bold ? 700 : 400;
	head->dfCharSet         = charset;
	head->dfPixWidth        = psf.psf_width;
	head->dfPixHeight       = psf.psf_height;
	head->dfPitchAndFamily  = (3 << 4);  /* FF_MODERN */
	head->dfAvgWidth        = head->dfPixWidth;
	head->dfMaxWidth        = head->dfPixWidth;
	head->dfFirstChar       = firstchar;
	head->dfLastChar        = lastchar;
	head->dfDefaultChar     = defchar;
	head->dfBreakChar       = brkchar;
/* Width in bytes, rounded up to nearest word */
	head->dfWidthBytes      = bitmap_width;
	head->dfDevice          = 0;
	head->dfFace            = head_len + chartable_len + bitmap_len;
	head->dfBitsPointer     = 0;
	head->dfBitsOffset      = head_len + chartable_len;
	head->dfReserved        = 0;
	head->dfFlags           = 1;         /* Fixed width */
	head->dfAspace          = 0;
	head->dfBspace          = 8;
	head->dfCspace          = 0;
	head->dfColorPointer    = 0;
}


/* Public interface to this program */

char *cnv_progname = "PSF2FNT";


char *cnv_help(void)
{
	static char helpbuf[2048];

	sprintf(helpbuf, "Syntax: %s psffile fntfile { options }\n\n", cnv_progname);
	strcat (helpbuf, "Options: \n"
		"        --ascent                  - Set ascent\n"
		"        --bold                    - Mark font as bold\n"
		"        --brkchar                 - Break character\n"
		"        --copy=copyright-string   - Set copyright string\n"
		"        --codepage=cp             - Extract the named "
			"codepage from the source\n" 
		"                                   file.\n"
		"        --charset=ch              - Set character set\n"
		"                 ch can be ANSI, OEM, SYMBOL or a number\n"
		"        --defchar                 - Default character\n"
		"        --first                   - First character to include\n"
		"        --fontname=name           - Set font name\n"
		"        --fontver=1, 2 or 3       - Font format version\n"
		"        --horzres                 - Set horizontal resolution\n"
		"        --italic                  - Mark font as italic\n"
		"        --last                    - Last character to include\n"
		"        --pointsize               - Set font point size\n"
		"        --vertres                 - Set vertical resolution\n");
	return helpbuf;
}

static char errbuf[150];

char *cnv_set_option(int ddash, char *variable, char *value)
{
	if (!stricmp(variable, "copy"   )) 
	{
		copyright = cpybuf;
		strncpy(cpybuf, value, sizeof(cpybuf));
		cpybuf[sizeof (cpybuf)-1] = 0;
	}
	else if (!stricmp(variable, "charset"))
		{
		if      (!stricmp(value, "ansi"  )) charset = 0;
		else if (!stricmp(value, "symbol")) charset = 2;
		else if (!stricmp(value, "oem"   )) charset = 0xFF;
		else                                charset = atoi(value);
		}
	else if (!stricmp(variable, "codepage"))  
	{
		codepage = psf_find_mapping(value);
		if (codepage == NULL) return "Code page name not recognised.";
	}

	else if (!stricmp(variable, "fontname")) 
	{
		strncpy(fnbuf, value, sizeof(fnbuf));
		fnbuf[sizeof(fnbuf) - 1] = 0;
		font_name = fnbuf;	
	}
	else if (!stricmp(variable, "ascent"))   ascent=atoi(value);
	else if (!stricmp(variable, "horzres"))  horzres=atoi(value);
	else if (!stricmp(variable, "vertres"))  vertres=atoi(value);
	else if (!stricmp(variable, "pointsize")) pointsize=atoi(value);
	else if (!stricmp(variable, "firstchar")) firstchar=atoi(value);
	else if (!stricmp(variable, "first"))     firstchar=atoi(value);
	else if (!stricmp(variable, "lastchar"))  lastchar=atoi(value);
	else if (!stricmp(variable, "last"))      lastchar=atoi(value);
	else if (!stricmp(variable, "defchar"))   defchar=atoi(value);
	else if (!stricmp(variable, "brkchar"))   brkchar=atoi(value);
	else if (!stricmp(variable, "bold"))	 bold = 1;
	else if (!stricmp(variable, "italic"))   italic = 1;
	else if (!stricmp(variable, "fontver"))
	{
	if      (!strcmp(value, "1")) font_ver = 0x100;
	else if (!strcmp(value, "2")) font_ver = 0x200;
	else if (!strcmp(value, "3")) font_ver = 0x300;
	else return "--fontver option must be 1, 2 or 3";
	}
	else 
	{
		/* Brute force avoidance of buffer overflows */
		if (strlen(variable) > 120) variable[120] = 0;
		sprintf(errbuf, "Unrecognised option: %s", variable);
	}
	return NULL;
}


char *cnv_execute(FILE *fpin, FILE *fpout)
{
	int rv;
	int wb;
	psf_dword nc, glyph;

	psf_file_new(&psf);
	psf_file_new(&psfi);

/* Because the --codepage option was added fairly late, I found it easiest
 * to implement as a separate pass which creates a new PSF with just the 
 * characters required */
	if (codepage)
	{
		rv = psf_file_read(&psfi, fpin);
		if (!rv) rv = psf_file_create(&psf, psfi.psf_width, 
			psfi.psf_height,
			(psfi.psf_length < 256) ? psfi.psf_length : 256, 0);
		if (!rv)
		{
			for (nc = 0; nc < psfi.psf_length; nc++)
			{
				if (!psf_unicode_lookupmap(&psfi, codepage,
					nc, &glyph, NULL))
				{
					memcpy(psf.psf_data+nc*psf.psf_charlen,
					  psfi.psf_data+glyph*psfi.psf_charlen,
					  psf.psf_charlen);
				}
			}
			psf_file_delete(&psfi);
		}
	}
	else
	{
		rv = psf_file_read(&psf, fpin);
	}
	if (rv) return psf_error_string(rv);
	nchars = psf.psf_length;

	/* OK, we have the PSF. */
	head_len      = 0x76;           /* Length of font header for v1 / v2 */

	wb = (psf.psf_width + 7) / 8;
	if (lastchar  < 0) lastchar  = nchars - 1;
	switch(font_ver)
	{
		case 0x100: chartable_len = 0;
			    bitmap_width = wb * ((lastchar + 1) - firstchar);
			    break;
			    /* 4 bytes/char in table. */
		case 0x200: chartable_len = (lastchar + 2 - firstchar) * 4; 
			    bitmap_width = wb * ((lastchar + 2) - firstchar);
			    if (bitmap_width & 1) ++bitmap_width;
			    break;
			    /* 6 bytes/char in table. */
		case 0x300: chartable_len = (lastchar + 2 - firstchar) * 6; 
			    bitmap_width = wb * ((lastchar + 2) - firstchar);
			    if (bitmap_width & 1) ++bitmap_width;
			    break;
	}
	if (pointsize < 0) pointsize = psf.psf_height;
	if (ascent    < 0) ascent    = psf.psf_height - 1;

	if (font_ver < 0x200)  bitmap_len = psf.psf_height * bitmap_width;
	else                   bitmap_len = psf.psf_height * (nchars + 1);

	msw_fontinfo_new(&fnt_head);

	setup_fnthead(&fnt_head);

	if (msw_fontinfo_write(&fnt_head, fpout) || 
	    msw_chartable_write(fpout)           ||
	    msw_glyphs_write(fpout)              ||
	    msw_name_write(fpout))
	{
	    psf_file_delete(&psf);
	    return "Error writing output file";
	}
	psf_file_delete(&psf);
   	return NULL;
}
