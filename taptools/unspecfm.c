/************************************************************************

    TAPTOOLS v1.1.0 - Tapefile manipulation utilities

    Copyright (C) 1996, 2014  John Elliott <jce@seasip.demon.co.uk>

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

*************************************************************************/

#include "config.h"
#include "cnvshell.h"

char *cnv_set_option(int ddash, char *variable, char *value)
{
	return "No options.";
}

char *cnv_help(void)
{
	return "Syntax: unspecform infile outfile";
}

char *cnv_execute(FILE *infile, FILE *outfile)
{
	unsigned char header[128];
	unsigned char sum;
	int n, len;

	if (fread(header, 1, 128, infile) < 128) return "No +3DOS header on input file.";

	for (sum = n = 0; n < 127; n++) sum += header[n];
	if (sum != header[n] || memcmp(header, "PLUS3DOS\032", 9)) return
		"No +3DOS header on input file.";

	len = header[14];
	len = (len << 8) | header[13];
	len = (len << 8) | header[12];
	len = (len << 8) | header[11];

	len -= 0x80;
	while (len)
	{
		n = fgetc(infile);
		if (n == EOF) return "Unexpected end-of-file on input file.";
		fputc(n, outfile);
		--len;
	}
	return NULL;
}

char *cnv_progname = "unspecform";

