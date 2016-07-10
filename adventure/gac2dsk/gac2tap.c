/************************************************************************

    GAC2TAP 1.0.0 - Convert GAC tape files to use standard BASIC loader

    Copyright (C) 2007, 2009  John Elliott <jce@seasip.demon.co.uk>

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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gacread.h"

#ifdef __PACIFIC__
#define AV0 "GAC2TAP"
#else
#define AV0 argv[0]
#endif

#ifndef EXIT_SUCCESS
#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1
#endif

static unsigned char tap_begin[] = 
{
	0x13, 0x00,	/* 0000 Block length [TAP] */
	0x00,		/* 0002 Block type */
	0x00,		/* 0003 BASIC loader */
	'P', 'R', 'O', 'G', 'N', 'A', 'M', 'E', '_', '_',	/* 0004 name */
	0x00, 0x00,	/* 000E Length */
	0x00, 0x00,	/* 0010 Line number */
	0x00, 0x00,	/* 0012 Variables */
	0x00		/* 0013 bitsum */
};

static unsigned char code_begin[] = 
{
	0x13, 0x00,	/* 0000 Block length [TAP] */
	0x00,		/* 0002 Block type */
	0x03,		/* 0003 Code header */
	'd', 'a', 't', 'a', ' ', ' ', ' ', ' ', ' ', ' ',	/* 0004 name */
	0x00, 0x00,	/* 000E Length */
	0x00, 0x00,	/* 0010 Load address */
	0x00, 0x00,	/* 0012 unused */
	0x00		/* 0013 bitsum */
};


static unsigned char basic_loader[] =
{
	0x00, 0x0A, 0x0D, 0x00,			/* 0000 line no & length */
		0xFD, '0', '0', '0', '0', '0',	/* 0004 CLEAR nnnn */
		0x0E,   0,   0,   0,   0,  0,	/* 000A number */
		0x0D,				/* 0010 CR */
	0x00, 0x14, 0x14, 0x00,			/* 0011 line no & length */
		0xEF, 0x22, 'd', 'a', 't', 'a', 0x22, /* 0015 LOAD "data" */
		0xAF, '0', '0', '0', '0', '0',	/* 001C CODE nnnn */
		0x0E,   0,   0,   0,   0,  0,	/* 0022 number */
		0x0D,				/* 0028 CR */
	0x00, 0x1E, 0x0E, 0x00,			/* 0029 line no & length */
		0xF9, 0xC0,			/* 002D RAND USR */
		'0', '0', '0', '0', '0',	/* 002F nnnnn */
		0x0E,   0,   0,   0,   0,  0,	/* 0034 number */
		0x0D,				/* 0035 CR */
};

void poke2(byte *array, int offset, int v)
{
	array[offset] = v & 0xFF;
	array[offset+1] = (v >> 8) & 0xFF;
}

void check_fwrite(const char *name, byte *buf, unsigned count, FILE *fp)
{
	if (fwrite(buf, 1, count, fp) < count)
	{
		fclose(fp);
		remove(name);
		fprintf(stderr, "Write error on %s\n", name);
		exit(EXIT_FAILURE);
	}
}

void write_newfile(const char *name)
{
	FILE *fp = fopen(name, "wb");
	char buf[6];
	byte head[3];
	unsigned n;
	byte cksum;

	if (!fp)
	{
		perror(name);
		exit(EXIT_FAILURE);
	}
	sprintf(buf, "%5u", gac_sp); memcpy(basic_loader + 0x05, buf, 5);
	poke2(basic_loader, 0x0d, gac_sp);
	sprintf(buf, "%5u", gac_ix); memcpy(basic_loader + 0x1d, buf, 5);
	poke2(basic_loader, 0x25, gac_ix);
	sprintf(buf, "%5u", gac_pc); memcpy(basic_loader + 0x2f, buf, 5);
	poke2(basic_loader, 0x37, gac_pc);

	/* Generate TAP file */
	memcpy(tap_begin + 4, progname, 10);
	poke2(tap_begin, 14, sizeof(basic_loader));
	poke2(tap_begin, 16, 0x0000);
	poke2(tap_begin, 18, sizeof(basic_loader));
	cksum = 0;
	for (n = 2; n < (unsigned)(sizeof(tap_begin) - 1); n++) 
		cksum ^= tap_begin[n];

	tap_begin[sizeof(tap_begin)-1] = cksum;

	check_fwrite(name, tap_begin, sizeof(tap_begin), fp);
	poke2(head, 0, sizeof(basic_loader) + 2);
	head[2] = 0xFF;
	check_fwrite(name, head, 3, fp);
	check_fwrite(name, basic_loader, sizeof(basic_loader), fp);
	cksum = 0xff;
	for (n = 0; n < (unsigned)sizeof(basic_loader); n++) 
		cksum ^= basic_loader[n];
	check_fwrite(name, &cksum, 1, fp);

	/* Write code file */
	poke2(code_begin, 14, gac_de);
	poke2(code_begin, 16, gac_ix);
	cksum = 0;
	for (n = 2; n < (unsigned)sizeof(code_begin) - 1; n++) 
		cksum ^= code_begin[n];
	code_begin[sizeof(code_begin)-1] = cksum;
	check_fwrite(name, code_begin, sizeof(code_begin), fp);
	poke2(head, 0, gac_de + 2);
	head[2] = 0xFF;
	check_fwrite(name, head, 3, fp);
	check_fwrite(name, datablock + 1, gac_de, fp);
	cksum = 0xff;
	for (n = 0; n < gac_de; n++) cksum ^= datablock[n+1];
	check_fwrite(name, &cksum, 1, fp);

	fclose(fp);
}

int main(int argc, char **argv)
{
	int err;

	if (argc < 3)
	{
		fprintf(stderr, "Syntax: %s <filename.tap> <newfile>\n", AV0);
		return EXIT_FAILURE;
	}
	err = load_gac(argv[1]);
	if (err) return EXIT_FAILURE;
	write_newfile(argv[2]);
	return EXIT_SUCCESS;
}
