/*
    psftools: Manipulate console fonts in the .PSF format
    Copyright (C) 2003, 2007  John Elliott

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

#define MAX_H 32	/* Maximum font height */

#ifndef SEEK_SET	/* Why doesn't Pacific define this? */
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2
#endif

/* Convert a Wyse soft-font to a .PSF font. 
 * 
 * We assume a maximum of 512 characters with a maximum height of 31.
 */
static char workbuf[512 * MAX_H];

static char helpbuf[2048];
static int width  = 8;
static int height = 0;
static int max_height = 0;
static int v1 = 0, v2 = 0;
static PSF_MAPPING *codepage = NULL;
static PSF_FILE psf;
static int firstc = -1;
static int lastc = -1;

/* Program name */
char *cnv_progname = "WYSE2PSF";

/* ddash = 1 if option started with a double-dash; else 0 */
/* Return NULL if OK, else error string */
char *cnv_set_option(int ddash, char *variable, char *value)
    {
    if (!stricmp(variable, "width"))  { width  = atoi(value); return NULL; }
    if (!stricmp(variable, "height")) { height = atoi(value); return NULL; }
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
    sprintf(helpbuf, "Syntax: %s softfont psffile { options }\n\n", cnv_progname);
    strcat (helpbuf, "Options: \n"
	             "    --height=x     Height of glyphs (defaults to guessed from softfont)\n"
	             "    --width=x      Width of glyphs (defaults to 8)\n"
	             "    --codepage=x   Create a Unicode directory from the specified code page\n"
                     "    --psf1         Output in PSF1 format\n"
                     "    --psf2         Output in PSF2 format (default) \n");
            
    return helpbuf;
    }

/* Read 2 bytes from the file and parse as hex */
static int readhex(FILE *fp)
{
	char buf[3];
	int val;
	int ch;

	ch = fgetc(fp); 
	if (ch == EOF) return -1;	/* End of file */
	if (ch == 0x19) return -2;	/* End of char */
	buf[0] = ch;
	ch = fgetc(fp);
	if (ch == EOF) return -1;
	if (ch == 0x19) return -2;	/* End of char */
	buf[1] = ch;
	buf[2] = 0;
	if (!sscanf(buf, "%x", &val)) return -2;
	return val;
}

char *cnv_execute(FILE *infile, FILE *outfile)
{	
	int rv; 
	char buf[3];
	int ch, bank, curchar, y, wb;

	buf[0] = buf[1] = buf[2] = 0;
/* Work through the file until we find ESC c A  (redefine character) */

	memset(workbuf, 0, sizeof(workbuf));
	while (1)
	{
		ch = fgetc(infile); if (ch == EOF) break;

		buf[0] = buf[1];
		buf[1] = buf[2];
		buf[2] = ch;
		if (buf[0] != 0x1B || buf[1] != 'c' || buf[2] != 'A') 
			continue;
/* We have got ESC c A. Next character should be the bank. */
		ch = fgetc(infile); if (ch == EOF) break;
		bank = (ch - '0') & 3;
/* Next 2 characters should be the char cell number */
		ch = readhex(infile); 
		if (ch < 0) break;
		curchar = (ch & 0x7F) + (bank * 128);
		y = 0;
/* And then read bytes until a ^Y is encountered */
		while ((ch = readhex(infile)) >= 0)	
		{
			if (y < MAX_H) workbuf[curchar * MAX_H + y] = ch;
			++y;
		}
		if (ch == -1) break;	/* EOF in the middle of a character */
/* Character successfully defined. Move on to the next one */
		if (y > max_height) max_height = y;
		if (curchar < firstc || firstc == -1) firstc = curchar;
		if (curchar > lastc  || lastc  == -1) lastc = curchar;
		buf[0] = buf[1] = buf[2] = 0;
	}
	if (height == 0) height = max_height;

	if (firstc == -1 || lastc == -1 || height == 0)
	{
		return "No WY60 character definitions found in input";
	}

	psf_file_new(&psf);
	rv = psf_file_create(&psf, width, height, lastc - firstc + 1, 0);
	if (!rv)
	{
		wb = psf.psf_charlen / psf.psf_height;
		for (ch = firstc; ch <= lastc; ch++)
		{
			if (ch >= 512) break;

			for (y = 0; y < height; y++)
			{
				if (y >= MAX_H) break;
				psf.psf_data[psf.psf_charlen * (ch-firstc) 
					+ y * wb] = workbuf[ch * MAX_H + y];
			}
		}

		if (codepage)
		{
			psf_unicode_addall(&psf, codepage, 0, lastc - firstc + 1);
		}


		if (v1) psf_force_v1(&psf);
		if (v2) psf_force_v2(&psf);
		rv = psf_file_write(&psf, outfile);
	}
	psf_file_delete(&psf);	
	if (rv) return psf_error_string(rv);
	return NULL;
}

