/*
    psftools: Manipulate console fonts in the .PSF format
    Copyright (C) 2003  John Elliott

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
#include "psflib.h"

#ifndef EXIT_FAILURE	/* Pacifc */
#define EXIT_FAILURE 1
#endif

int main(int argc, char **argv)
{
	int n, p;
	PSF_MAPPING *m;
	psf_dword *sym;
	char u;

	if (argc < 2)
	{
		printf("Code pages supported are:\n");
		psf_list_mappings(stdout);
		printf("\n");
	}
	else
	{
		for (n = 1; n < argc; n++)
		{
			m = psf_find_mapping(argv[n]);
			if (!m)
			{
				fprintf(stderr, "Codepage not found: %s\n", 
						argv[n]);
				return EXIT_FAILURE;
			}
			if (argc > 2)
			{
				for (p = 1 + strlen(argv[n]); p >= 0; p--)
					putchar('=');
				printf("\n%s:\n", argv[n]);	
				for (p = 1 + strlen(argv[n]); p >= 0; p--)
					putchar('=');
				putchar('\n');
			}
			for (p = 0; p < 256; p++)
			{
				u = '+';
				printf("0x%02x", p);
				sym = m->psfm_tokens[p];
				while (*sym != 0xFFFFL)
				{
					if (*sym == 0x1FFFFL) 
					{
						u = '*';
						++sym;
						continue;
					}
					printf("\tU%c%04lx", u, *sym);
					++sym;
				}
				printf("\n");
			}
		}
	}
	return 0;
}

