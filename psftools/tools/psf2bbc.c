/*
    psftools: Manipulate console fonts in the .PSF format
    Copyright (C) 2005, 2008  John Elliott

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

/* Convert a PSF file to a BBC Micro-format soft font
 * (on an Arc, this would be filetype FF7) */

#define MAX_H 8	/* Max font height */

char *cnv_progname = "PSF2BBC";

static char helpbuf[2048];
static int fx = 0;
static int first = 32;
static int last = 255;
static psf_byte charbuf[MAX_H];

char *cnv_set_option(int ddash, char *variable, char *value)
{
/*
	if (!stricmp(variable, "dest"))
	{
		dest = atoi(value);
		if (dest < 32 || dest > 255)
			return "--dest: Character cell must be 32-255\n";
		return NULL;	
	} */
	if (!stricmp(variable, "first")) { first = atoi(value); return NULL; }
	if (!stricmp(variable, "last"))  { last  = atoi(value); return NULL; }
	if (!stricmp(variable, "fx"))    { fx = 1; return NULL; }
	if (!stricmp(variable, "fx20"))  { fx = 1; return NULL; }
	    			     
	sprintf(helpbuf, "Unknown option: %s", variable);
	return helpbuf;
}

char *cnv_help(void)
    {
    sprintf(helpbuf, "Syntax: %s { options } psf_file softfont_file\n\n"
		     "Options:\n"
		     "   --dest=x   The font will start at character cell x (32-255)\n"
		     "   --first=x  First character to go into the soft font\n"
		     "   --last=x   Last character to go into the soft font\n"
		     "   --fx       Generate *FX20 commands for the original\n"
		     "              BBC Micro to expand its character RAM\n"
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
	if (psf.psf_height > MAX_H)
	{
		fprintf(stderr, "Warning: Input file is higher than %d pixels. Truncating.\n", MAX_H);
	}
	if ((psf.psf_length - 1) < first) first = psf.psf_length - 1;
	if ((psf.psf_length - 1) < last)  last  = psf.psf_length - 1;

	if (fx) for (y = 1; y <= 6; y++)
	{
		fprintf(fpout, "*FX20 %ld\r", y);
	}
	/* wb = number of bytes per line */	
	wb = psf.psf_charlen / psf.psf_height;	
	for (ch = first; ch <= last; ch++)
	{
		if (ch < 32)
		{
			ch = 31;
			fprintf(stderr, "Warning: Characters numbered below 32 will be ignored.\n");
			continue;
		}
		memset(charbuf, 0, sizeof(charbuf));

		/* Populate character buffer */
		for (y = 0; y < psf.psf_height; y++)
		{
			if (y >= MAX_H) break;
			charbuf[y] = psf.psf_data[ch * psf.psf_charlen + 
			       			  y * wb];	
		}
		fputc(0x17, fpout);		// VDU23 
		fputc(ch,   fpout);		// character
		for (y = 0; y < 8; y++)
		{
			fprintf(fpout, "%c", charbuf[y]);	// Char shape
		}
	}

	psf_file_delete(&psf);
	return 0;
}

