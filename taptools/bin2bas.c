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
#include "tapeio.h"

#ifndef SEEK_SET
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2
#endif

static int st_plus3dos = 1;
static TIO_FORMAT st_format = TIOF_TAP;

static char st_filename[11] = "Default   ";

unsigned char basic1[] =
{
	0x00, 0x0A, 0xFF, 0xFF, 0xEA,	/* 10 <len xx> REM */
};

unsigned char basic2[] = 
{
	0x0D,				/* End of line 10 */
	0x00, 0x14, 			/* Line 20 */
	0xFF, 0xFF,			/* <len xx> */
	0xF1, 'x', '=', 0xC0, '(', '5',	/* LET x=USR (5 */
/* For testing: Print execution address rather than jumping to it */
/*	0xF5, '(', '5',		*/	/* PRINT (5 */
	0x0E, 0, 0, 5, 0, 0, '+', 	/* [number 5] + */
	0xBE, '2', '3', '6', '3', '5',	/* PEEK 23635 */
	0x0E, 0, 0, 0x53, 0x5C, 0, '+',	/* [number 23635] + */
	'2', '5', '6',			/* + 256 */
	0x0E, 0, 0, 0, 1, 0, '*',	/* [number 256] * */
	0xBE, '2', '3', '6', '3', '6',	/* PEEK 23636 */
	0x0E, 0, 0, 0x54, 0x5C, 0,	/* [number 23636] */
	')', 0x0D	
};

char *cnv_set_option(int ddash, char *variable, char *value)
{
	char *boo;

	if (variable[0] == 'f' || variable[0] == 'F')
	{
		if ((variable[1] == 'P' || variable[1] == 'p') &&
		    variable[2] == '3')
		{
			st_plus3dos = 1;
			return NULL;
		}
		boo = (char *)tio_parse_format(variable + 1, &st_format);
		if (boo) return boo;
		st_plus3dos = 0;
		if (value && value[0]) sprintf(st_filename, "%-10.10s", value);
		return NULL;
	}
	if (variable[0] == 't' || variable[0] == 'T')
	{
		st_plus3dos = 0;
		st_format = TIOF_TAP;
		if (value && value[0]) sprintf(st_filename, "%-10.10s", value);
		return NULL;
	}
	if (variable[0] == 'z' || variable[0] == 'Z')
	{
		st_plus3dos = 0;
		st_format = TIOF_ZXT;
		if (value && value[0]) sprintf(st_filename, "%-10.10s", value);
		return NULL;
	}
	if (variable[0] == 'p' || variable[0] == 'P')
	{
		st_plus3dos = 1;
		return NULL;
	}
	
	return "Unrecognised option";
}

char *cnv_help(void)
{
	return "Syntax: bin2bas { options } infile outfile\n\n"
		"Options:\n"
		"-ftap=filename: Output in .TAP format\n"
		"-fzxt=filename: Output in .ZXT format\n"
		"-ftzx=filename: Output in .TZX format\n"
		"-fp3: Output in +3DOS format (default)\n";
}


char *cnv_execute(FILE *infile, FILE *outfile)
{
	unsigned char p3header[128];
	unsigned char tapheader[17];
	unsigned char sum;
	int n;
	long size, base, bassize, outsize = 0L;
	unsigned char *buf;
	TIO_PFILE tape;

	if (fseek(infile, 0, SEEK_END)) return "Cannot fseek in input file";
	size = ftell(infile);
	if (fseek(infile, 0, SEEK_SET)) return "Cannot fseek in input file";
	base = 0;
	if (fread(p3header, 1, 128, infile) == 128)
	{
		for (sum = n = 0; n < 127; n++) sum += p3header[n];
		if (sum == p3header[n] && !memcmp(p3header, "PLUS3DOS\032", 9)) 
		{
			base = 128;		 
		}
	}
	if (fseek(infile, base, SEEK_SET)) return "Cannot fseek in input file";
	memset(p3header, 0, sizeof(p3header));
	memcpy(p3header, "PLUS3DOS\032\001\000", 11);

	bassize = (size - base) + sizeof(basic1) + sizeof(basic2);
	buf = malloc(bassize);
	if (!buf)
	{
		return "Not enough memory to load input file";
	}
	if (fread(buf + sizeof(basic1), 1, size - base, infile) < size - base)
	{
		return "Unexpected end of file on input file";
	}

	/* BASIC line 10 has REM + code + newline */
	basic1[2] = (size + 2 - base) & 0xFF;
	basic1[3] = (size + 2 - base) >> 8;

	/* BASIC line 20: Length is sizeof array less 5 */
	basic2[3] = (sizeof(basic2) - 5) & 0xFF;
	basic2[4] = (sizeof(basic2) - 5) >> 8;

	memcpy(buf, basic1, sizeof(basic1));
	memcpy(buf + size - base + sizeof(basic1), basic2, sizeof(basic2));
	/* Program image now generated in buf */

	p3header[15] = 0;			/* BASIC */
	p3header[16] = (bassize) & 0xFF;	/* Size of program+vars */
	p3header[17] = (bassize >> 8) & 0xFF;
	p3header[18] = 0;			/* Autostart line */
	p3header[19] = 0;
	p3header[20] = (bassize) & 0xFF;	/* Size of program */
	p3header[21] = (bassize >> 8) & 0xFF;

	/* Copy TAP header from +3DOS header */
	tapheader[0] = 0;	/* BASIC */
	sprintf((char *)tapheader + 1, "%-10.10s", st_filename);
	memcpy(tapheader + 11, p3header + 16, 6);

	outsize = bassize + 128;
	
	p3header[11] = (outsize) & 0xFF;
	p3header[12] = (outsize >>  8) & 0xFF;
	p3header[13] = (outsize >> 16) & 0xFF;
	p3header[14] = (outsize >> 24) & 0xFF;

	for (sum = n = 0; n < 127; n++) sum += p3header[n];
	p3header[127] = sum;

	if (st_plus3dos)
	{
		if (fwrite(p3header, 1, sizeof(p3header), outfile) < (int)sizeof(p3header))
			return "Failed to write +3DOS header";
		if (fwrite(buf, 1, bassize, outfile) < (int)bassize)
			return "Failed to write file contents";
	}
	else
	{
		const char *boo;

		boo = tio_mktemp(&tape, st_format);
		if (boo) return (char *)boo;
		boo = tio_writezx(tape, 0, tapheader, 17);
		if (boo) return (char *)boo;
		boo = tio_writezx(tape, 0xFF, buf, bassize);
		if (boo) return (char *)boo;
		boo = tio_closetemp(&tape, outfile);
		if (boo) return (char *)boo;
	}
	return NULL;
}

char *cnv_progname = "bin2bas";

