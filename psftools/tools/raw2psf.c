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

#ifndef SEEK_SET	/* Why doesn't Pacific define this? */
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2
#endif

/* Convert a raw font to a .PSF font. 
 * 
 * If input is coming from a file, we can guess the height of glyphs by 
 * assuming 256 glyphs in the file. If input is coming from a pipe, the 
 * user has to specify glyph height.
 */

static char helpbuf[2048];
static int width  = 8;
static int height = 0;
static int skip   = 0;
static int skipbytes = 0;
static int doflip = 0;
static int first  = 0;
static int last   = 255;
static int v1 = 0, v2 = 0;
static PSF_MAPPING *codepage = NULL;
static PSF_FILE psf;

/* Program name */
char *cnv_progname = "RAW2PSF";

/* ddash = 1 if option started with a double-dash; else 0 */
/* Return NULL if OK, else error string */
char *cnv_set_option(int ddash, char *variable, char *value)
    {
    if (!stricmp(variable, "width"))  { width  = atoi(value); return NULL; }
    if (!stricmp(variable, "height")) { height = atoi(value); return NULL; }
    if (!stricmp(variable, "skip"))   { skip   = atoi(value); return NULL; }
    if (!stricmp(variable, "skipbytes")) { skipbytes = atoi(value); return NULL; }
    if (!stricmp(variable, "first"))  { first  = atoi(value); return NULL; }
    if (!stricmp(variable, "last"))   { last   = atoi(value); return NULL; }
    if (!stricmp(variable, "flip"))   { doflip = 1; return NULL; }
    if (!stricmp(variable, "psf1"))   { v1 = 1; return NULL; }
    if (!stricmp(variable, "psf2"))   { v2 = 1; return NULL; }
    if (!stricmp(variable, "512"))    { first = 0; last = 511; return NULL; }
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
    sprintf(helpbuf, "Syntax: %s rawfnt psffile { options }\n\n", cnv_progname);
    strcat (helpbuf, "Options: \n"
	             "    --height=x     Height of glyphs\n"
	             "    --width=x      Width of glyphs (defaults to 8)\n"
	             "    --skip=x       Skip x characters at start of raw file (defaults to 0)\n"
	             "    --skipbytes=x  Skip x bytes at start of raw file (defaults to 0)\n"
                     "    --first=x      First character to use in the PSF\n"
	             "    --last=x       Last character to go into PSF (defaults to 255)\n"
	             "    --codepage=x   Create a Unicode directory from the specified code page\n"
		     "    --flip         Mirror all bytes in the file\n"
                     "    --512          Equivalent to --first=0 --last=511\n"
                     "    --psf1         Output in PSF1 format\n"
                     "    --psf2         Output in PSF2 format (default) \n");
            
    return helpbuf;
    }


char *cnv_execute(FILE *infile, FILE *outfile)
{	
	int rv, n, max;
	psf_dword count, pos;
	long len = 0;

	count = last + 1;	
	if (infile == stdin) pos = -1;
	else pos = ftell(infile);

/* Guess height from file size, if possible */
	if (pos != -1)
	{
		if (fseek(infile, 0,   SEEK_END) >= 0 &&
                    (len = ftell(infile))        >= 0 &&
		    fseek(infile, pos, SEEK_SET) >= 0);
		else  pos = -1;
	}
	if (pos != -1 && height == 0) 
	{
		len -= pos;	
		height = (len / count) / ((width + 7) /8);
		fprintf(stderr, "From file size: height=%d\n",height);
	}

	if (height == 0)
	{
		return "Can't convert with no \"--height\" option";
	}

	psf_file_new(&psf);
	rv = psf_file_create(&psf, width, height, last + 1, 0);
	if (!rv)
	{
		skip *= psf.psf_charlen;
		while (skip)
		{
			if (fgetc(infile) == EOF) return "Unexpected end of file";
			--skip;
		}	
		while (skipbytes)
		{
			if (fgetc(infile) == EOF) return "Unexpected end of file";
			--skipbytes;
		}	
		if (fread(psf.psf_data + first * psf.psf_charlen, 
				psf.psf_charlen, last + 1 - first, infile) < last + 1 - first)
		{
			psf_file_delete(&psf);
			return "Could not read the input file.";
		}
		if (doflip)
		{
			max = last * psf.psf_charlen;
			for (n = first * psf.psf_charlen; 
			     n < max; n++) 
			{
				psf.psf_data[n] = flip(psf.psf_data[n]);
			}

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

