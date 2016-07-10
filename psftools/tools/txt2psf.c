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
static int v1 = 0;
static int v2 = 0;
static PSF_FILE psf;

/* Program name */
char *cnv_progname = "TXT2PSF";

/* ddash = 1 if option started with a double-dash; else 0 */
/* Return NULL if OK, else error string */
char *cnv_set_option(int ddash, char *variable, char *value)
{
	if (!stricmp(variable, "psf1")) { v1 = 1; return NULL; }
	if (!stricmp(variable, "psf2")) { v2 = 1; return NULL; }

	if (strlen(variable) > 2000) variable[2000] = 0;
	sprintf(helpbuf, "Unknown option: %s\n", variable);
	return helpbuf;
}


/* Return help string */
char *cnv_help(void)
{
    sprintf(helpbuf, "Syntax: %s textfile psffile { options }\n\n", cnv_progname);
    strcat (helpbuf, "Options:\n\n"
		    "--psf1: Save in PSF1 format.\n"
		    "--psf2: Save in PSF2 format.\n");
            
    return helpbuf;
}

#define ATSTART  0
#define INHEADER 1
#define INCHAR   2
#define INBITMAP 3

char *cnv_execute(FILE *infile, FILE *outfile)
{	
	int rv, n, x = 0, y = 0;
	int state = 0;	
	char linebuf[2000];
	char unibuf[2000];
	char *c;
	int version = -1, flags = -1, length = -1, width = -1, height = -1;
	int nchar = 0;
	int line = 0, wb = 0, havebitmap = 0;
	psf_byte *charbits = NULL;

	psf_file_new(&psf);
	while (fgets(linebuf, sizeof(linebuf), infile))
	{
		++line;
/* If a long line, devour the rest of it */
		if (!strchr(linebuf, '\n'))
		{
			do
			{
				n = fgetc(infile);
			}
			while (n != EOF && n != '\n');	
		}
		c = strchr(linebuf, '\n'); if (c) *c = 0;
		c = strstr(linebuf, "//"); if (c) *c = 0;

		if (linebuf[0] == 0) continue;
		if (linebuf[0] == '%')
		{
			if (!strncmp(linebuf + 1, "PSF2", 4) && state == ATSTART)
			{
				state = INHEADER;
				continue;
			}
/* %STOP: Immediately stop parsing */
			else if (!strncmp(linebuf + 1, "STOP", 4))
			{
				break;	
			}	
			else if (state == INHEADER)
			{
				if (version == -1) return "Version not set";
				if (flags == -1)   return "Flags not set";
				if (width == -1)   return "Width not set";
				if (height == -1)  return "Height not set";
				if (length == -1)  return "Length not set";
				if (version > 0)   return "Versions greater than 0 are not supported"; 
				rv = psf_file_create(&psf, width, height,
					       length, (flags & 1));	
				if (!rv) 
				{
					charbits = malloc(psf.psf_charlen);
					if (!charbits) rv = PSF_E_NOMEM;
				}
				if (rv) return psf_error_string(rv);
				state = INCHAR;
				unibuf[0] = 0;
				wb = ((width + 7) / 8);
			}
			else if (state == INCHAR || state == INBITMAP)
			{
				if (version == -1) return "Version not set";
				if (nchar < psf.psf_length) memcpy
					(psf.psf_data + nchar * psf.psf_charlen,
					charbits, psf.psf_charlen);
				if (unibuf[0])
				{
					rv = psf_unicode_from_string(&psf, nchar, unibuf);
					if (rv) 
					{
						fprintf(stderr, "Line %d: Failed to decode %s\n", line, unibuf);
						return psf_error_string(rv);
					}
				}
				havebitmap = 0;
				state = INCHAR;
				nchar++;
				unibuf[0] = 0;
			}
			else 
			{
				fprintf(stderr, "Line %d: Unexpected %% line\n", line);
				return "Invalid input format";
			}	
		}
		else
		{
			c = strstr(linebuf, ": ");
			if (c)
			{
				*c = 0;
				c++;
				while (*c == ' ') c++;
				if (state == INHEADER)
				{
					if (!strcmp(linebuf, "Version")) 
						version = atol(c);
					else if (!strcmp(linebuf, "Flags"))
						flags = atol(c);
					else if (!strcmp(linebuf, "Width"))
						width = atol(c);
					else if (!strcmp(linebuf, "Height"))
						height = atol(c);
					else if (!strcmp(linebuf, "Length"))
						length = atol(c);
					else
					{
						fprintf(stderr, "Line %d: Unknown variable name '%s'\n", line, linebuf);
						return "Invalid input format";
					}
				}
				else if (state == INCHAR)
				{
					if (!strcmp(linebuf, "Unicode"))
						strcpy(unibuf, c);
					else if (!strcmp(linebuf, "Bitmap"))
					{
						strcpy(linebuf, c);
						state = INBITMAP;	
						havebitmap = 0;
						x = y = 0;
					} 
					else
					{
						fprintf(stderr, "Line %d: Unknown variable name '%s'\n", line, linebuf);
						return "Invalid input format";
					}
				}
				else	/* Not INCHAR and not INHEADER */
				{
					fprintf(stderr, "Line %d: Unknown variable name '%s'\n", line, linebuf);
					return "Invalid input format";
				}
				if (state != INBITMAP) continue;
			} /* end if (c) */
			if (state == INBITMAP)
			{
				psf_byte *dest, mask;
				c = linebuf;
				while (*c)
				{
					if (*c != '-' && *c != '#') 
					{
						c++;
						continue;
					}
					dest = charbits + (y * wb) + (x/8);
					mask = 0x80 >> (x & 7);
					if (*c == '#') *dest |= mask;
					else	       *dest &= ~mask;
					x++;
					if (x >= psf.psf_width) { x = 0; y++; }
					if (y >= psf.psf_height) 
					{
						state = INCHAR;
						havebitmap = 1;	
						break;
					}
					++c;
				}
				continue;
			}	
			fprintf(stderr, "Line %d: Not a Variable: Value pair\n", line);
			return "Invalid input format";
		}
	}
	if (state == INCHAR && havebitmap)
	{
		if (nchar < psf.psf_length) memcpy(psf.psf_data + nchar * 
				psf.psf_charlen, charbits, psf.psf_charlen);
		if (unibuf[0])
		{
			rv = psf_unicode_from_string(&psf, nchar, unibuf);
			if (rv) return psf_error_string(rv);
		}
	}
	if (v1) psf_force_v1(&psf);
	if (v2) psf_force_v2(&psf);
	rv = psf_file_write(&psf, outfile);
	psf_file_delete(&psf);	
	if (rv) return psf_error_string(rv);
	return NULL;
}

