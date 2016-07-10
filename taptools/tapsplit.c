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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tapeio.h"

#ifdef __PACIFIC__
#define AV0 "TAPSPLIT"
#else
#define AV0 argv[0]
#endif

typedef unsigned char byte;

TIO_PFILE tapefile;
byte p3hdr[128];
byte p3sig[] = "PLUS3DOS\032\001";
int uctr = 1;
int nctr = 1;
char *outdir = NULL;
unsigned long expected_len = 0;

#define NAMELENGTH 100	/* Maximum filename length. 100 is massive overkill
			   when normal Spectrum names don't exceed 10 chars */

char lfname[NAMELENGTH];
char *lfpath;

void mkname(char *name)
{
	if (outdir) sprintf(lfpath, "%s/%s", outdir, name);
	else	strncpy(lfpath, name, NAMELENGTH - 1);
	lfpath[NAMELENGTH - 1] = 0;
}

FILE *lfopen(char *name, char *mode)
{
	mkname(name);
	return fopen(lfpath, mode);
}

void progress(char *what, char *name)
{
	mkname(name);
	printf("%s %s\n", what, lfpath);
}

void x_header(TIO_HEADER *hdr)
{
	char *p;

	sprintf(lfname, "%-10.10s", hdr->header + 2);
	lfname[10] = 0;
	/* Chop off any trailing spaces */
	for (p = lfname + 9; p >= lfname; p--)
	{
		if (p[0] != ' ') break;
		*p = 0;
	}

	memset(p3hdr, 0, sizeof(p3hdr));
	memcpy(p3hdr, p3sig, sizeof(p3sig));

	p3hdr[15] = hdr->header[1];
	memcpy(p3hdr + 16, hdr->header + 12, 6);
	expected_len = hdr->header[12] + 256 * hdr->header[13];
}

void x_ibm_header(TIO_HEADER *hdr)
{
	char *p;

	sprintf(lfname, "%-8.8s", hdr->header + 2);
	lfname[8] = 0;
	/* Chop off any trailing spaces */
	for (p = lfname + 7; p >= lfname; p--)
	{
		if (p[0] != ' ') break;
		*p = 0;
	}
	if      (hdr->header[10] & 0x20)  strcat(lfname, ".p"); 
	else if (hdr->header[10] & 0x80)  strcat(lfname, ".b"); 
	else if (hdr->header[10] & 0x40)  strcat(lfname, ".a"); 
	else if (hdr->header[10] & 0x01)  strcat(lfname, ".m"); 
	else                              strcat(lfname, ".d"); 
	expected_len = 0;
}

static char *do_write(void *param, int c)
{
	if (fputc(c, param) == EOF) 
		return "Write error on output file.";
	return NULL;
}

void x_data(TIO_HEADER *hdr, int read_data)
{
	unsigned long blk2;
	FILE *fpo = NULL;			
	int n;
	byte csum;

	if (!read_data)
	{
		tio_compact(hdr);
	}
	if (!lfname[0])
	{
		if (hdr->status == TIO_ZX_SNA)
		{
			sprintf(lfname, "Block%d.sna", uctr++);
		}
		else
		{
			sprintf(lfname, "Block%d", uctr++);
		}
	}
	if (lfname[0] < 32) 
	{
		sprintf(lfname, "unnamed.%d", nctr++);
	}
	do
	{
		fpo = lfopen(lfname, "r");
		if (fpo == NULL) break;
		strcat(lfname, ".1");
		fclose(fpo);
	} while(1);

	progress("Writing", lfname);

	fpo = lfopen(lfname, "wb");
	if (fpo == NULL)
	{
		perror(lfpath);
		exit(1);
	}
	/* If this came from a Spectrum file, give it a +3DOS header */
	if (read_data && hdr->status == TIO_ZX_HEADER)
	{
		blk2 = expected_len + 128;

		p3hdr[11] =  blk2        & 0xFF;
		p3hdr[12] = (blk2 >>  8) & 0xFF;	
		p3hdr[13] = (blk2 >> 16) & 0xFF;
		p3hdr[14] = (blk2 >> 24) & 0xFF;

		for (n = csum = 0; n < 127; n++) csum += p3hdr[n];
		p3hdr[n] = csum;

		if (fwrite(p3hdr, 1, 128, fpo) < 128)
		{
			perror(lfpath);
			exit(1);
		}
	}
	if (read_data)
	{
		const char *boo = tio_readdata(tapefile, hdr, fpo, do_write);
		if (boo) 
		{
			fprintf(stderr, "%s\n", boo);
			exit(1);
		}
	}
	else if (fwrite(hdr->data, 1, hdr->length, fpo) < hdr->length)
	{
		perror(lfpath);
		exit(1);
	}
	lfname[0] = 0;
}


int main(int argc, char **argv)
{
	TIO_FORMAT fmt = TIOF_TAP;
	TIO_HEADER hdr;
	const char *boo;

	if (argc < 2)
	{
		fprintf(stderr, "Syntax: %s tapfile { <output-dir> }\n", AV0);
		return 1;
	}
	if (argc > 2)
	{
		outdir = argv[2];
		lfpath = malloc(NAMELENGTH + 1 + strlen(outdir));
		printf("Output directory is %s\n", outdir);
	}
	else
	{
		lfpath = malloc(NAMELENGTH);
	}
	boo = tio_open(&tapefile, argv[1], &fmt);	
	if (boo) 
	{
		fprintf(stderr, "%s\n", boo);
		exit(1);
	}
	lfname[0] = 0;

	do
	{
		memset(&hdr,  0, sizeof(hdr));
		boo = tio_readraw(tapefile, &hdr, 0);
		if (boo) 
		{
			fprintf(stderr, "%s\n", boo);
			exit(1);
		}
		switch (hdr.status)
		{
			case TIO_EOF: break;
			case TIO_TZXOTHER: break;

			case TIO_ZX_HEADER:
				x_header(&hdr);
				x_data(&hdr, 1);
				break;
			case TIO_IBM_HEADER:
				x_ibm_header(&hdr);
				x_data(&hdr, 1);
				break;
			default:
				x_data(&hdr, 0);
				break;
		}
	
		if (hdr.data)  free(hdr.data); 	
	}
	while (hdr.status != TIO_EOF);
	tio_close(&tapefile);
	free(lfpath);
	return 0;
}
