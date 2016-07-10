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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "tapeio.h"

#ifdef __PACIFIC__
#define ARG0 "TAPCAT"
#else
#define ARG0 argv[0]
#endif

#ifndef SEEK_SET
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2
#endif

typedef enum
{
	BASIC  = 0,
	NUMBER = 1,
	CHAR   = 2,
	MEMORY = 3,
	HEADERLESS = 4
} FILETYPE;

static const char *ftype[] = { "Program", "Number Array", "Character Array", 
				"Bytes" };

static char *st_outname = NULL;
static int st_newfile = 0;
static TIO_FORMAT st_format = TIOF_TAP;
static FILETYPE st_filetype = MEMORY;
static unsigned short st_off = 0;
static TIO_PFILE st_tapefile;

void help(const char *arg0)
{
	fprintf(stderr, "Syntax: %s { options } outputfile { options } "
			"input1 { options } input2 ...\n\n"
			"Options for output file are :-\n"
			"  -N     Force new file rather than appending\n"
			"  -ftap  Output in .TAP format (default)\n"
			"  -fpzx  Output in .PZX format\n"
			"  -ftzx  Output in .TZX format\n"
			"  -fzxt  Output in .ZXT format\n"
			"Options for input file(s) with no +3DOS header are :-\n"
			"  -b      File is BASIC\n"
			"  -bLLLL  File is BASIC with autostart line\n"
			"  -c      File is character array\n"
			"  -cC     File is character array C$()\n"
			"  -d      File is headerless block\n"
			"  -m      File is memory block      (default)\n"
			"  -mAAAA  File is memory block with load address\n"
			"  -u      File is number array\n"
			"  -uC     File is number array C()\n",
			arg0);
			
}

void append(const char *filename)
{
	unsigned char p3hdr[128];
	unsigned char header[17];
	FILE *fp;
	long size;
	char *bname;
	char namebuf[11];
	unsigned char *buf = NULL;
	const char *boo;
	unsigned char cksum;
	int n;
	int plus3dos = 0;

	fp = fopen(filename, "rb");
	if (fp == NULL)
	{
		perror(filename);
		return;
	}
	fseek(fp, 0, SEEK_END);
	size = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	if (size > 128)
	{
		if (fread(p3hdr, 1, sizeof(p3hdr), fp) == sizeof(p3hdr)
		&& !memcmp(p3hdr, "PLUS3DOS\032", 9))
		{
			for (cksum = n = 0; n < 127; n++) cksum += p3hdr[n];
			if (cksum == p3hdr[127]) plus3dos = 1;
		}
		if (plus3dos)
		{
			size = p3hdr[14];
			size = (size << 8) | p3hdr[13];
			size = (size << 8) | p3hdr[12];
			size = (size << 8) | p3hdr[11];
			size -= 128;
		}
	}
	fseek(fp, plus3dos ? 128 : 0, SEEK_SET);

	if (size > 0xFFFFL) 
	{
		fprintf(stderr, "%s: File will be truncated to 65535 bytes\n",
				filename);
		size = 0xFFFFL;
	}
	bname = (char *) filename;
	if (strrchr(bname, '/')) bname = 1 + strrchr(bname, '/');
#ifdef __MSDOS__
	if (strrchr(bname, '\\')) bname = 1 + strrchr(bname, '\\');
	if (strrchr(bname, ':'))  bname = 1 + strrchr(bname, ':');
#endif
	sprintf(namebuf, "%-10.10s", bname);

	/* Generate the header */
	memset(header, 0x0, sizeof(header));

	sprintf((char *)(header + 1), "%-10.10s", namebuf);
	if (plus3dos)
	{
		header[0] = p3hdr[15];
		memcpy(header + 11, p3hdr + 16, 6);
	}
	else 
	{
		header[0]  = st_filetype;
		header[11] = size & 0xFF;
		header[12] = (size >> 8) & 0xFF;
		header[13] = st_off & 0xFF;
		header[14] = (st_off >> 8) & 0xFF;
		header[15] = size & 0xFF;
		header[16] = (size >> 8) & 0xFF;
	}	
	buf = malloc(size);
	if (!buf)
	{
		fprintf(stderr, "%s: Cannot load file into memory.\n", 
				filename);
		return;
	}
	memset(buf, 0x1A, size);
	if (fread(buf, 1, size, fp) < size)
	{
		perror(filename);
	}
	if (plus3dos || st_filetype != HEADERLESS)
	{
		boo = tio_writezx(st_tapefile, 0, header, sizeof(header));
		if (boo)
		{
			fprintf(stderr, "%s\n", boo);
			if (buf) free(buf);
			return;
		}
	}
	boo = tio_writezx(st_tapefile, 0xFF, buf, size);
	if (boo)
	{
		fprintf(stderr, "%s\n", boo);
		if (buf) free(buf);
		return;
	}
	fclose(fp);
	fprintf(stderr, "Appended %s: %s [%ld bytes]\n",
			ftype[header[0]], namebuf, size);	

}

const char * open_output(const char *s)
{
	FILE *fp;
	const char *boo;

	if (st_newfile)
	{
		return tio_creat(&st_tapefile, s, st_format);
	}

	fp = fopen(s, "r+b");
	if (!fp)
	{
		return tio_creat(&st_tapefile, s, st_format);
	}
	fclose(fp);
	/* OK. File exists. */
	boo = tio_open(&st_tapefile, s, &st_format);
	if (boo) return boo;

	fprintf(stderr, "File %s exists: appending to it.\n"
			"Format is %s\n", s, tio_format_desc(st_format));

	return tio_seek_end(st_tapefile);
}


int main(int argc, char **argv)
{
	int n;
	int count = 0;
	unsigned off;
	char *hex;
	const char *boo;

	for (n = 1; n < argc; n++)
	{
		if (argv[n][0] == '-')
		{
			switch(argv[n][1])
			{
				case 'b': case 'B':	
					st_filetype = BASIC; 
					if (isdigit(argv[n][2]))
						st_off = atoi(&argv[n][2]);
					else	st_off = 0x8000;
					break;
				case 'c': case 'C':
					st_filetype = CHAR; 
					if (isalpha(argv[n][2]))
						st_off = toupper(argv[n][2]);
					else	st_off = 'A';
					break;
				case 'f': case 'F':
					boo = tio_parse_format(&argv[n][2], &st_format);
					if (boo) 
					{
						fputs(boo, stderr);
						exit(1);
					}
					break;
				case 'd': case 'D':
					st_filetype = HEADERLESS;
					break;
				case 'm': case 'M':	
					st_filetype = MEMORY;
					hex = &argv[n][2];
					if (hex[0] == '=') ++hex;
					if (sscanf(hex, "%x", &off) == 1)
					{
						st_off = off;
					}
					else if (hex[0] != 0)
					{
						fprintf(stderr, "%s: Cannot interpret '%s' as hexadecimal address", argv[0], argv[n]);
						return 1;
					}
					break;
				case 'n': case 'N':
					st_newfile = 1;
					break;
				case 'u': case 'U':
					st_filetype = NUMBER; 
					if (isalpha(argv[n][2]))
						st_off = toupper(argv[n][2]);
					else	st_off = 'A';
					break;
				case 'h': case 'H': case '?':
					help(ARG0);
					return 0;
				default:
					fprintf(stderr, "Unrecognised option: %s", argv[n]);
					return 1;	
			} 
		}
		else
		{
			if (st_outname == NULL)
			{
				st_outname = argv[n];
				boo = open_output(st_outname);
				if (boo)
				{
					fprintf(stderr, "%s: %s", st_outname, boo);
					return 1;
				}
			}
			else
			{
				++count;
				append(argv[n]);
			}
		}
	}
	if (st_outname == NULL)
	{
		fprintf(stderr, "%s: No output filename provided.\n", ARG0);
		return 1;
	}
	if (!count)
	{
		fprintf(stderr, "%s: No input files provided.\n", ARG0);
		return 1;
	}
	return 0;
}

