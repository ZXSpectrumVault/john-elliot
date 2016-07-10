/*
    psftools: Manipulate console fonts in the .PSF format
    Copyright (C) 2005, 2007  John Elliott

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
#include <stdio.h>
#include "cnvshell.h"
#include "psflib.h"

/* Convert a PSF file to a Wyse-format soft font */

#define MAX_H 32	/* Max font height */

char *cnv_progname = "PSF2WYSE";

static char helpbuf[2048];
static int bank = 0;
static int dest = 0;
static int first = 0;
static int height = 16;
static int nulls = 8;
static int last = 511;
static psf_byte charbuf[MAX_H];

char *cnv_set_option(int ddash, char *variable, char *value)
{
	if (!stricmp(variable, "bank")) 
	{
		bank = atoi(value);
		if (bank < 0 || bank > 3) return "--bank: Bank must be 0 to 3";
		return NULL;	
	}
	if (!stricmp(variable, "dest"))
	{
		dest = atoi(value);
		if (dest < 0 || dest > 127)
			return "--dest: Character cell must be 0-127\n";
		return NULL;	
	}
	if (!stricmp(variable, "first")) { first = atoi(value); return NULL; }
	if (!stricmp(variable, "last"))  { last  = atoi(value); return NULL; }
	if (!stricmp(variable, "height")){ height = atoi(value); return NULL; }
	if (!stricmp(variable, "nulls")) { nulls = atoi(value); return NULL; }
	    			     
	sprintf(helpbuf, "Unknown option: %s", variable);
	return helpbuf;
}

char *cnv_help(void)
    {
    sprintf(helpbuf, "Syntax: %s { options } psf_file softfont_file\n\n"
		     "Options:\n"
		     "   --bank=x   The font will use font bank x (0-3)\n"
		     "              (eg: bank 0 for 25-line modes, bank 2 for 43-line modes)\n"
		     "   --dest=x   The font will start at character cell x (0-127)\n"
		     "   --height=x Height of defined characters (default 16)\n"
		     "   --first=x  First character to go into the soft font\n"
		     "   --last=x   Last character to go into the soft font\n"
		     "   --nulls=x  Send x NUL characters after each definition\n"
		    ,cnv_progname);
    return helpbuf;
    }



char *cnv_execute(FILE *fpin, FILE *fpout)
{	
	int rv;
	PSF_FILE psf;
	psf_dword ch;
	psf_dword y, wb;

	psf_file_new(&psf);
	rv = psf_file_read(&psf, fpin);	
	if (rv != PSF_E_OK) return psf_error_string(rv);

	if (psf.psf_width > 8)
	{
		fprintf(stderr, "Warning: Input file is wider than 8 pixels. Truncating at 8.\n");
	}
	if (height > MAX_H)
	{
		fprintf(stderr, "Warning: Height cannot exceed %d. Truncating.\n", MAX_H);
		height = MAX_H;
	}
	if ((psf.psf_length - 1) < first) first = psf.psf_length - 1;
	if ((psf.psf_length - 1) < last)  last  = psf.psf_length - 1;

	/* wb = number of bytes per line */	
	wb = psf.psf_charlen / psf.psf_height;	
	for (ch = first; ch <= last; ch++)
	{
		memset(charbuf, 0, sizeof(charbuf));

		/* Populate character buffer */
		for (y = 0; y < psf.psf_height; y++)
		{
			if (y >= MAX_H) break;
			charbuf[y] = psf.psf_data[ch * psf.psf_charlen + 
			       			  y * wb];	
		}
		fputc(0x1B, fpout);		// ESC
		fputc('c',  fpout);		// c
		fputc('A',  fpout);		// A  : Redefine
		fputc(bank + '0', fpout);	// Bank
		fprintf(fpout, "%02x", dest);	// Dest cell
		for (y = 0; y < height; y++)
		{
			fprintf(fpout, "%02x", charbuf[y]);	// Char shape
		}
		fputc(0x19, fpout);		// End of char definition
		for (y = 0; y < nulls; y++)
		{
			fputc(0, fpout);
		}
		++dest;
		if (dest == 0x80)
		{
			dest = 0;
			++bank;
			if (bank > 3) break;
		}
	}

	psf_file_delete(&psf);
	return 0;
}

