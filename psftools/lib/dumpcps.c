/*
    psftools: Manipulate console fonts in the .PSF format
    Copyright (C) 2005, 2007  John Elliott

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
#include "psfio.h"
#include <ctype.h>

/* This is an internal version of page2cp2, which dumps all pages in one go.
 */
#include "mappings.c"

int dump_mapping(PSF_MAPPING *m)
{
	FILE *fp;
	char filename[20];
	int n, p, total;
	unsigned char header[128];
	unsigned char data[4];

	memset(header, 0, sizeof header);
	strcpy(header, "PSFTOOLS CODEPAGE MAP\r\n\032");

	sprintf(filename, "%s.cp2", m->psfm_name);
	fp = fopen(filename, "wb");
	if (!fp)
	{
		perror(filename);
		return -1;
	}
	total = 0;
	for (n = 0; n < 256; n++)
	{
		for (p = 0; m->psfm_tokens[n][p] != 0xFFFF; p++);
		total += (p + 1);	
	}
	header[0x40] = (total) & 0xFF;
	header[0x41] = (total >> 8) & 0xFF;
	header[0x42] = (total >> 16) & 0xFF;
	header[0x43] = (total >> 24) & 0xFF;
	fwrite(header, 1, sizeof(header), fp);
	for (n = 0; n < 256; n++)
	{
		for (p = 0; m->psfm_tokens[n][p] != 0xFFFF; p++)
		{
			data[0] = (m->psfm_tokens[n][p]) & 0xFF;
			data[1] = (m->psfm_tokens[n][p] >> 8) & 0xFF;
			data[2] = (m->psfm_tokens[n][p] >> 16) & 0xFF;
			data[3] = (m->psfm_tokens[n][p] >> 24) & 0xFF;
			fwrite(data, 1, sizeof(data), fp);
		}
		data[0] = data[1] = 0xFF;
		data[2] = data[3] = 0;
		fwrite(data, 1, sizeof(data),fp);
	}
	fclose(fp);
	return 0;
}

int main(int argc, char **argv)
{
	PSF_MAPPING **m;

	for (m = builtin_codepages; *m; m++) 
	{
		dump_mapping(*m);
	}
	return 0;
}

