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
#include "config.h"
#include <stdio.h>
#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif
#include "cnvshell.h"
#include "cpi.h"

static char helpbuf[2048];
static int inter  = 1;
static int nt     = 0;

/* Program name */
char *cnv_progname = "CPIDCOMP";

/* ddash = 1 if option started with a double-dash; else 0 */
/* Return NULL if OK, else error string */
char *cnv_set_option(int ddash, char *variable, char *value)
{
    if (!stricmp(variable, "nointer")) { inter = 0; return NULL; }
    if (!stricmp(variable, "nt"))      { nt = 1; return NULL; }
    if (strlen(variable) > 2000) variable[2000] = 0;
    sprintf(helpbuf, "Unknown option: %s\n", variable);
    return helpbuf;
}


/* Return help string */
char *cnv_help(void)
{
    sprintf(helpbuf, "Decompresses a CPI file into FONT format\n"
		     "Syntax: %s cpifile cpifile { options }\n\n", cnv_progname);
    strcat (helpbuf, "Options: \n"
	             "    --nointer      Don't interleave headers and data in output file\n"
                     "                   (not recommended!)\n"
	             "    --nt           Output in FONT.NT format\n");
            
    return helpbuf;
}


/* Convert a normal .CPI font into a DRFONT.
 */

char *cnv_execute(FILE *infile, FILE *outfile)
{	
	CPI_FILE f;
	int err;

	cpi_new(&f, CPI_FONT);
	err = cpi_loadfile(&f, infile);
	if (!err && strcmp(f.format, "DRFONT "))
	{
		fprintf(stderr, "Warning: File is not in DRFONT format - will not decompress.\n");
	}
	if (!err) err = cpi_bloat(&f);
	if (!err)
	{
		f.magic0 = 0xFF;
		if (nt) strcpy(f.format, "FONT.NT");
		else	strcpy(f.format, "FONT   ");
		err = cpi_savefile(&f, outfile, inter);
	}
	cpi_delete(&f);

	switch(err)
	{
		case CPI_ERR_OK:     return NULL;
		case CPI_ERR_NOMEM:  return "Out of memory";
		case CPI_ERR_BADFMT: return "Input file is not a suitable CPI file";
		case CPI_ERR_ERRNO:  return strerror(errno);
		case CPI_ERR_EMPTY:  return "Input file is empty";
		default:	     return "Unknown error";
	}
}
