/*
    psftools: Manipulate console fonts in the .PSF format
    Copyright (C) 2000  John Elliott

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
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* ddash = 1 if option started with a double-dash; else 0 */
/* Return NULL if OK, else error string */
extern char *cnv_set_option(int ddash, char *variable, char *value);
/* Return help string */
extern char *cnv_help(void);
/* Do the conversion */
extern char *cnv_execute(FILE *infile, FILE *outfile);
/* Program name */
extern char *cnv_progname;

#ifdef _WIN32
#define HAVE_STRICMP 1
#endif

#ifndef HAVE_STRICMP
extern int stricmp(char *s, char *t);
#endif
