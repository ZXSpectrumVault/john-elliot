/*
    psftools: Manipulate console fonts in the .PSF format
    Copyright (C) 2004  John Elliott

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

/* Convert a Hercules .WOF font to a .PSF font. 
 */

static char helpbuf[2048];
static int width  = 8;
static int height = 0;
static int first  = 0;
static int last   = 0;
static int v1 = 0, v2 = 0;
static PSF_MAPPING *codepage = NULL;
static PSF_FILE psf;

/* Program name */
char *cnv_progname = "WOF2PSF";

/* ddash = 1 if option started with a double-dash; else 0 */
/* Return NULL if OK, else error string */
char *cnv_set_option(int ddash, char *variable, char *value)
    {
    if (!stricmp(variable, "psf1"))   { v1 = 1; return NULL; }
    if (!stricmp(variable, "psf2"))   { v2 = 1; return NULL; }
    if (!stricmp(variable, "codepage") || !stricmp(variable, "setcodepage"))  
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
    sprintf(helpbuf, "Converts a Hercules WriteOn font to PSF format.\n"
		    "Syntax: %s woffile psffile { options }\n\n", cnv_progname);
    strcat (helpbuf, "Options: \n"
	             "    --codepage=x   Create a Unicode directory from the specified code page\n"
                     "    --psf1         Output in PSF1 format\n"
                     "    --psf2         Output in PSF2 format (default) \n");
            
    return helpbuf;
    }



char *cnv_execute(FILE *infile, FILE *outfile)
{	
	int rv;
	unsigned char wof_head[22];
	unsigned hdrlen;

	if (fread(wof_head, 1, 6, infile) < 6)
	{
		return "Can't read WOF header from file.";		
	}
	if (wof_head[0] != 'E' || wof_head[1] != 'S' || wof_head[2] != 1 ||
	    wof_head[3] != 0)
	{
		return "WOF magic number not found.";
	}
	hdrlen = wof_head[4]  + 256 * wof_head[5];

	if (hdrlen >= 22) 
	{
		if (fread(wof_head + 6, 1, 16, infile) < 16)
		{
			return "Can't read WOF header from file.";		
		}
	}
	else	
	{
		if (fread(wof_head + 6, 1, hdrlen - 6, infile) < hdrlen - 6)
		{
			return "Can't read WOF header from file.";		
		}
	}


	width  = wof_head[6]  + 256 * wof_head[7];
	height = wof_head[8]  + 256 * wof_head[9]; 	
	first  = wof_head[12] + 256 * wof_head[13];
	last   = wof_head[14] + 256 * wof_head[15];

/* Skip over any additional header data */	
	while (hdrlen > 22)
	{
		if (fgetc(infile) == EOF)
			return "Unexpected EOF on input file.";
		--hdrlen;	
	}

	psf_file_new(&psf);
	rv = psf_file_create(&psf, width, height, last + 1, 0);
	if (!rv)
	{
		if (fread(psf.psf_data + first * psf.psf_charlen, 
			psf.psf_charlen, last + 1 - first, infile) < 
			last + 1 - first)
		{
			psf_file_delete(&psf);
			return "Could not read the input file.";
		}
		if (codepage)
		{
			psf_unicode_addall(&psf, codepage, first, last);
		}

		if (v1) psf_force_v1(&psf);
		if (v2) psf_force_v2(&psf);
		rv = psf_file_write(&psf, outfile);
	}
	psf_file_delete(&psf);	
	if (rv) return psf_error_string(rv);
	return NULL;
}

