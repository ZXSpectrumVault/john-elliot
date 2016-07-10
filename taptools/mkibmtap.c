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
#include "tapeio.h"

#ifdef __PACIFIC__
#define ARG0 "MKIBMTAP"
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
	BASIC,
	LISTING,
	PROTECTED,
	MEMORY,
	TEXT
} FILETYPE;

static const char *ftype[] = { "BASIC", "ASCII Listing", "Protected", 
				"Memory", "Text" };

static char *st_outname = NULL;
static int st_newfile = 0;
static TIO_FORMAT st_format = TIOF_TZX_IBM;
static FILETYPE st_filetype;
static unsigned short st_seg, st_off;
static TIO_PFILE st_tapefile;

void help(const char *arg0)
{
	fprintf(stderr, "Syntax: %s { options } outputfile { options } "
			"input1 { options } input2 ...\n\n"
			"Options for output file are :-\n"
			"  -N      Force new file rather than appending\n"
			"  -ftzxi  Output in .TZX format, IBM timings (default)\n"
			"  -ftzx   Output in .TZX format, Spectrum timings\n"
			"  -ftap   Output in .TAP format\n"
			"  -fzxt   Output in .ZXT format\n"
			"Options for input file(s) are :-\n"
			"  -a      File is ASCII listing\n"
			"  -b      File is tokenised BASIC\n"
			"  -d      File is ASCII data file\n"
			"  -m      File is memory block\n"
			"  -mSSSS:OOOO File is memory block with load address\n"
			"  -p      File is protected tokenised BASIC\n", arg0);
}

void append(const char *filename)
{
	unsigned char header[17];
	FILE *fp;
	long size;
	char *bname;
	char namebuf[9];
	unsigned char *buf = NULL;
	const char *boo;
	int chunked = (st_filetype == LISTING || st_filetype == TEXT);

	fp = fopen(filename, "rb");
	if (fp == NULL)
	{
		perror(filename);
		return;
	}
	fseek(fp, 0, SEEK_END);
	size = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	if (size > 0xFFFFL && !chunked)
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
	/* Hack off any filetype */
	sprintf(namebuf, "%-8.8s", bname);
	bname = strchr(namebuf, '.');
	if (bname) *bname = 0;

	/* Generate the header */
	memset(header, 0xFF, sizeof(header));
	header[0] = 0xA5;	/* IBM BASIC */
	sprintf((char *)(header + 1), "%-8.8s", namebuf);

	switch (st_filetype)
	{
		case BASIC:	header[ 9] = 0x80; 
				header[12] = 0x60;	/* These four values */
				header[13] = 0x00;	/* (0060:081E) */
				header[14] = 0x1E;	/* are probably fixed */
				header[15] = 0x08;	/* for all versions */
				break;			/* of Cassette BASIC */
		case LISTING:	header[ 9] = 0x40; 
				break;
		case PROTECTED:	header[ 9] = 0xA0; 
				header[12] = 0x60;
				header[13] = 0x00;
				header[14] = 0x1E;
				header[15] = 0x08;
				break;
		case MEMORY:	header[ 9] = 0x01; 
				header[12] = st_seg & 0xFF;
				header[13] = (st_seg >> 8) & 0xFF;	
				header[14] = st_off & 0xFF;
				header[15] = (st_off >> 8) & 0xFF;	
				break;
		case TEXT:	header[ 9] = 0x00; 
				break;
	}
	header[10] = size & 0xFF;
	header[11] = (size >> 8) & 0xFF;

	if (!chunked)
	{
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
	}
	boo = tio_writeibm(st_tapefile, header, sizeof(header));
	if (boo)
	{
		fprintf(stderr, "%s\n", boo);
		if (buf) free(buf);
		return;
	}
	if (!chunked)
	{
		boo = tio_writeibm(st_tapefile, buf, size);
		if (boo)
		{
			fprintf(stderr, "%s\n", boo);
			if (buf) free(buf);
			return;
		}
	}
	else
	{
		int ch, chunkc;
		unsigned char chunkbuf[256];
		chunkc = 1;

		memset(chunkbuf, 0x1A, 256);
		while ( (ch = fgetc(fp)) != EOF)
		{
/* Convert CRLF and LF to CR */
			if (ch == '\r') continue;
			if (ch == '\n') ch = '\r';
			chunkbuf[chunkc++] = ch;
			if (chunkc == 256)
			{
				chunkbuf[0] = 0;
				boo = tio_writeibm(st_tapefile, 
						chunkbuf, 256);
				if (boo) 
				{
					fprintf(stderr, "%s\n", boo);
					return;
				}
				memset(chunkbuf, 0x1A, 256);
				chunkc = 1;
			}
		}
		chunkbuf[0] = chunkc;
		boo = tio_writeibm(st_tapefile, chunkbuf, 256);
		if (boo) 
		{
			fprintf(stderr, "%s\n", boo);
			return;
		}		
	}
	if (buf) free(buf);
	fclose(fp);
	fprintf(stderr, "Appended %s: %s [%ld bytes]\n",
			ftype[st_filetype], namebuf, size);	

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
	unsigned seg, off;
	char *hex;
	const char *boo;

	for (n = 1; n < argc; n++)
	{
		if (argv[n][0] == '-')
		{
			switch(argv[n][1])
			{
				case 'a': case 'A':	
					st_filetype = LISTING; 
					break;
				case 'b': case 'B':	
					st_filetype = BASIC; 
					break;
				case 'd': case 'D':	
					st_filetype = TEXT; 
					break;
				case 'f': case 'F':
					boo = tio_parse_format(&argv[n][2], &st_format);
					if (boo) 
					{
						fputs(boo, stderr);
						exit(1);
					}
					break;
				case 'p': case 'P':	
					st_filetype = PROTECTED;
					break;
				case 'n': case 'N':
					st_newfile = 1;
					break;
				case 'm': case 'M':	
					st_filetype = MEMORY;
					hex = &argv[n][2];
					if (hex[0] == '=') ++hex;
					if (sscanf(hex, "%x:%x", &seg, &off) == 2)
					{
						st_seg = seg;
						st_off = off;
					}
					else if (sscanf(hex, "%x", &seg) == 1)
					{
						st_seg = seg;
						st_off = 0;
					}
					else if (hex[0] != 0)
					{
						fprintf(stderr, "%s: Cannot interpret '%s' as hexadecimal address", argv[0], argv[n]);
						return 1;
					}
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

