/*
    psftools: Manipulate console fonts in the .PSF format
    Copyright (C) 2005  John Elliott

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

/* System-specific functions: */
char *video_probe(void);
char *valid_font(PSF_FILE *psf);
char *install_font(PSF_FILE *psf, int usealt, int slot);

static char helpbuf[2048];
static int use256 = 0;
static int usealt = 0;

static PSF_FILE psf, psfsub;

/* Program name */
char *cnv_progname = "LOADPSF";

/* ddash = 1 if option started with a double-dash; else 0 */
/* Return NULL if OK, else error string */
char *cnv_set_option(int ddash, char *variable, char *value)
{
	if (!stricmp(variable, "first"))  { use256 = 1; return NULL; }
	if (!stricmp(variable, "second")) { use256 = 2; return NULL; }
	if (!stricmp(variable, "alt"))    { usealt = 1; return NULL; }
	if (strlen(variable) > 2000) variable[2000] = 0;
	sprintf(helpbuf, "Unknown option: %s\n", variable);
	return helpbuf;
}


/* Return help string */
char *cnv_help(void)
{
	sprintf(helpbuf, "Syntax: %s psffile { options }\n\n", cnv_progname);
	strcat (helpbuf, "Options: \n"
		"    --first   Only load the first 256 characters of a\n"
		"              512 character font.\n"
		"    --second  Only load the second 256 characters of a\n"
		"              512 character font.\n"
		"    --alt     Load first 256 characters as alternative\n"
		"              font, second 256 (if present) as main font.");

	return helpbuf;
}

char *cnv_execute(FILE *infile, FILE *outfile)
{	
	int rv;
	int len;
	char *s;
	PSF_FILE *pf;

	s = video_probe();
	if (s) return s;

	psf_file_new(&psf);
	rv = psf_file_read(&psf, infile);

	if (rv != PSF_E_OK) return psf_error_string(rv);

	pf = &psf;
	if (use256)
	{
		len = 256;
		if (len > psf.psf_length) len = psf.psf_length;
		psf_file_new(&psfsub);
		rv = psf_file_create(&psfsub, psf.psf_width, psf.psf_height,
				len, 0);
		if (rv != PSF_E_OK) return psf_error_string(rv);
	
		switch(use256)
		{
			case 1: memcpy(psfsub.psf_data, psf.psf_data, 
					len * psf.psf_charlen);
				break;
			case 2: if (len + 256 > psf.psf_length) 
				{
					len = psf.psf_length - 256;
					if (len <= 0) 
					{
						return "--last needs more than 256 characters in the font.";
					}
				}
				memcpy(psfsub.psf_data, 
				       psf.psf_data + 256 * psf.psf_charlen, 
				       len * psf.psf_charlen);
				break;

		}
		pf = &psfsub;
	}
	s = valid_font(pf);
	if (s) return s;

	s = install_font(pf, usealt, 0);
	
	psf_file_delete(&psf);

	return s;
}

