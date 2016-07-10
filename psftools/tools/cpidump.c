/*
    psftools: Manipulate console fonts in the .PSF format
    Copyright (C) 2005  John Elliott

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
#include "cpi.h"

void cpi_dump(CPI_FILE *f)
{
	CP_HEAD *cph;
	CP_FONT *cpf;
	int n, nchars;

	printf("CPI_FILE\n========\n");
	printf("Magic0: 0x%02x\n", f->magic0);
	printf("Format: \"%s\"\n", f->format);
	printf("Count of DRfonts: %d\n\n", f->drcount);

	for (n = 0; n < f->drcount; n++)
	{
		printf("DRFONT\n======\n");
		printf("Char size:   %d\n", f->drfonts[n].char_len);
		printf("Offset:      %ld\n", f->drfonts[n].offset);
		printf("Bitmap len:  %d\n", f->drfonts[n].bitmap_len);
		printf("Bitmap data: %p\n\n", f->drfonts[n].bitmap);
	}
	for (cph = f->firstpage; cph != NULL; cph = cph->next)	
	{
		printf("CP_HEAD\n=======\n");
		printf("Header size: %d\n", cph->headsize);
		printf("Device type: %d\n", cph->dev_type);
		printf("Device name: \"%s\"\n", cph->dev_name);
		printf("Codepage:    %d\n", cph->codepage);
		printf("Fonts:       %d\n", cph->font_count);
		printf("Next hdr at  %ld\n", cph->next_offset);
		printf("Font data at %ld\n", cph->cpih_offset);
		printf("This hdr at  %ld\n", cph->self_offset);
		printf("Font len     %ld\n", cph->font_len);
		nchars = 0;
		for (cpf = cph->fonts; cpf != NULL; cpf = cpf->next)
		{
			printf("  CP_FONT\n  -------\n");
			printf("  PR type:  %d\n", cpf->pr_type);
			printf("  PR seqsz: %d\n", cpf->pr_seqsize);
			printf("  Height:   %d\n", cpf->height);
			printf("  Width:    %d\n", cpf->width);
			printf("  Chars:    %d\n", cpf->nchars);
			printf("  Aspect X: %d\n", cpf->xaspect);
			printf("  Aspect Y: %d\n", cpf->yaspect);
			printf("  Data:     %p\n", cpf->data);
			printf("  Data len: %d\n", cpf->fontsize);
			printf("  Offset:   %ld\n", cpf->self_offset);
			if (cpf->nchars > nchars) nchars = cpf->nchars;
		}
		if (cph->dr_lookup)
		{
			printf("Character table: ");
			for (n = 0; n < nchars; n++)
			{
				printf("%d,", cph->dr_lookup[n]);
			}
			printf("\n");
		}
		printf("\n");
	}
	if (f->comment)
	{
		printf("Comment: \"%-*.*s\"\n", f->comment_len, f->comment_len,
			f->comment);
	}

}

