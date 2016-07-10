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

/* Convert an MDA character ROM to a .PSF font. 
 * 
 * We assume the MDA ROM format, with 256 characters, each one 8x14. The 
 * ninth column is synthesized.
 */

static char helpbuf[2048];
static char mdabuf[8192];
static PSF_FILE psf;
static PSF_MAPPING *codepage = NULL;

/* Program name */
char *cnv_progname = "MDA2PSF";

/* ddash = 1 if option started with a double-dash; else 0 */
/* Return NULL if OK, else error string */
char *cnv_set_option(int ddash, char *variable, char *value)
{
	if (!stricmp(variable, "codepage"))
	{
		codepage = psf_find_mapping(value);
		if (codepage == NULL) return "Code page name not recognised.";
		return NULL;
	}

	if (strlen(variable) > 2000) variable[2000] = 0;
	sprintf(helpbuf, "Unknown option: %s\n", variable);
	return helpbuf;
}


/* Return help string */
char *cnv_help(void)
    {
    sprintf(helpbuf, "Syntax: %s mda_rom psffile { options }\n\n", cnv_progname);
    strcat (helpbuf, "Options:\n\n"
		    "--codepage=x: Create a Unicode directory from the "
		    "specified code page\n\n"
		    "Input file must be in MDA ROM format.\n");
            
    return helpbuf;
    }


char *cnv_execute(FILE *infile, FILE *outfile)
{	
	int rv, n, m;
	long pos;
	
	if (infile == stdin) pos = -1;
	else pos = ftell(infile);

/* Character size is fixed: 8x14. The ROM must be 4k in size; this contains
 * 512 8x8 cells (the first 256 give the top halves of characters, and the
 * second 256 give the bottom halves plus 2 blank lines). */

	psf_file_new(&psf);
	rv = psf_file_create(&psf, 9, 14, 256, 0);
	if (!rv)
	{
		if (fread(mdabuf, 1, 4096, infile) < 4096)
		{
			psf_file_delete(&psf);
			return "Could not read the input file.";
		}
		for (n= 0; n < 256; n++)
		{
			for (m = 0; m < 8; m++)
			{
				psf.psf_data[28*n + 2*m     ] = mdabuf[8*n+m];
				psf.psf_data[28*n + 2*m + 16] = mdabuf[8*n+m+2048];
/* Characters 192-224 duplicate the 8th column to give the 9th column. */
				if (n >= 192 && n < 224)
				{
					psf.psf_data[28*n + 2*m +  1] = (mdabuf[8*n+m] & 1)      ? 0x80 : 0;
					psf.psf_data[28*n + 2*m + 17] = (mdabuf[8*n+m+2048] & 1) ? 0x80 : 0;
				}	
				else
				{
					psf.psf_data[28*n + 2*m +  1] = 0;
					psf.psf_data[28*n + 2*m + 17] = 0;
				}	
			}	
		}	
                if (codepage)
                {
			psf_file_create_unicode(&psf); 
			for (n = 0; n < 256; n++)
				psf_unicode_addmap(&psf, n, codepage, n);
		}
		rv = psf_file_write(&psf, outfile);
	}
	psf_file_delete(&psf);	
	if (rv) return psf_error_string(rv);
	return NULL;
}

