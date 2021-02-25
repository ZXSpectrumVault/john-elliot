/*
    psftools: Manipulate console fonts in the .PSF format
    Copyright (C) 2005-6  John Elliott

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
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "psflib.h"

psf_errno_t dump_mapping(PSF_MAPPING *m)
{
	FILE *fp;
	char *filename, *tmp;
	int n, p;
	long total;
	unsigned char header[128];
	unsigned char data[4];

	memset(header, 0, sizeof header);
	strcpy((char *)header, "PSFTOOLS CODEPAGE MAP 2\r\n\032");

	filename = malloc(5 + strlen(m->psfm_name));
	if (!filename) return PSF_E_NOMEM;
	strcpy(filename, m->psfm_name);
	tmp = filename + strlen(filename) - 1;
/* Chop off any filename extension, but leave the path untouched */
	while (tmp > filename)
	{
		if (*tmp == '/' || *tmp == '\\' || *tmp == ':') break;	
		if (*tmp == '.')
		{
			*tmp = 0;
			break;
		}
		--tmp;
	}
	strcat(filename, ".cp2");
	fprintf(stderr, "Dumping %s as %s\n", m->psfm_name, 
			filename);

	fp = fopen(filename, "wb");
	if (!fp)
	{
		perror(filename);
		return PSF_E_ERRNO;
	}
	total = 0;
	for (n = 0; n < m->psfm_count; n++)
	{
		for (p = 0; m->psfm_tokens[n][p] != 0xFFFF; p++);
		total += (p + 1);	
	}
	header[0x40] = (total) & 0xFF;
	header[0x41] = (total >> 8) & 0xFF;
	header[0x42] = (total >> 16) & 0xFF;
	header[0x43] = (total >> 24) & 0xFF;
	header[0x44] = (m->psfm_count) & 0xFF;
	header[0x45] = (m->psfm_count >> 8) & 0xFF;
	header[0x46] = (m->psfm_count >> 16) & 0xFF;
	header[0x47] = (m->psfm_count >> 24) & 0xFF;
	fwrite(header, 1, sizeof(header), fp);
	for (n = 0; n < m->psfm_count; n++)
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
	return PSF_E_OK;
}

#ifdef __PACIFIC__
#define AV0 "cp2page"
#else
#define AV0 argv[0]
#endif

void syntax(char *av0)
{
	fprintf(stderr, "Syntax: %s codepage { codepage codepage ... }\n",
			av0);
	fprintf(stderr, "\nWrites the specified codepage(s) as .CP2 file(s).\n"
			"Built-in pages get saved in the current directory.\n"
			"Pages loaded from files get written in the same "
			"directory as those files.\n");
}

int main(int argc, char **argv)
{
	PSF_MAPPING *m;
	psf_errno_t err;
	int n;

	if (argc < 2)
	{
		syntax(AV0);
		return 0;
	}
	for (n = 1; n < argc; n++)
	{
		if (!strcmp(argv[n], "--help"))
		{
			syntax(AV0);
			return 0;
		}
		m = psf_find_mapping(argv[n]);
		if (!m)
		{
			fprintf(stderr, "Cannot find codepage %s\n", 
					argv[n]);
			return 1;
		}
		err = dump_mapping(m);
		if (err) 
		{
			fprintf(stderr, "%s: %s", argv[n],
				psf_error_string(err));
			return 1;	
		}
	}
	return 0;
}

