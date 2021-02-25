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
#include <stdio.h>
#include "cnvshell.h"
#include "psflib.h"

/* Convert a PSF file to XBM, so it can be previewed */

char *cnv_progname = "PSF2XBM";

static int across = 32;
static char helpbuf[2048];

char *cnv_set_option(int ddash, char *variable, char *value)
{
	if (!strcmp(variable, "across"))
	{
		if (!value || !atoi(value))
		{
			return "The 'across' value may not be zero or omitted.";
		}
		across = atoi(value);
		return NULL;
	}
        if (strlen(variable) > 2000) variable[2000] = 0;
        sprintf(helpbuf, "Unknown option: %s\n", variable);
        return helpbuf;
}

char *cnv_help(void)
{
	sprintf(helpbuf, "Syntax: %s { --across=value } psf_file xbm_file\n"
		"\n"
		"    --across : Set the number of characters shown across the bitmap.", 
		cnv_progname);
	
	return helpbuf;
}



char *cnv_execute(FILE *fpin, FILE *fpout)
{	
	int rv;
	PSF_FILE psf;
	int x,y,chy,chx;
	int rows;
	psf_dword ch;
	psf_byte pix, xbpix;
	int height, width, count, max;

	psf_file_new(&psf);
	rv = psf_file_read(&psf, fpin);	
	if (rv != PSF_E_OK) return psf_error_string(rv);

	/* Width of bitmap in pixels */
	width  = psf.psf_width * across;

	for (rows = 0; (across * rows) < psf.psf_length; rows++) {}

	height = psf.psf_height * rows;

	fprintf(fpout, "#define psf_width  %d\n", width);
	fprintf(fpout, "#define psf_height %d\n", height);
	fprintf(fpout, "static unsigned char psf_bits[] = {\n");

	count = 0;
	xbpix = 0;	
	max   = ((height * width) + 7) / 8;
	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			ch = (y / psf.psf_height) * across + (x / psf.psf_width);
			chy = y % psf.psf_height;
			chx = x % psf.psf_width;
			psf_get_pixel(&psf, ch, chx, chy, &pix);
		
			if (pix) xbpix |= (0x01 << (x & 7)); 

			if ((x & 7) == 7)
			{
				fprintf(fpout, " 0x%02x", xbpix);
				xbpix = 0;
				++count;
				if (count < max) fputc(',', fpout);
				else 	         fprintf(fpout, "};\n");
				if ((count % 12) == 0) fputc('\n', fpout);
			}
		}
	}
	psf_file_delete(&psf);
	return 0;
}

