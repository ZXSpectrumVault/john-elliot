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

/* This fine deals with the management of the in-memory structures associated
 * with a codepage (CPI) file */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cpi.h"

int cpi_new   (CPI_FILE *f, cpi_format format)
{
	switch(format)
	{
		case CPI_NAKED:
		case CPI_FONT: 
			f->magic0 = 0xFF;
			strcpy(f->format, "FONT   ");
			break;
		case CPI_FONTNT: 
			f->magic0 = 0xFF;
			strcpy(f->format, "FONT.NT");
			break;
		case CPI_DRFONT:
			f->magic0 = 0x7F;
			strcpy(f->format, "DRFONT ");
			break;
	}
	f->firstpage = NULL;
	f->drfonts   = NULL;
	f->drcount   = 0;
	f->comment   = NULL;
	f->comment_len=0;
	return 0;
}

int cpi_delete(CPI_FILE *f)
{
	CP_HEAD *h,  *h1;
	CP_FONT *fn, *fn1;

	for (h = f->firstpage; h != NULL; )
	{
		for (fn = h->fonts; fn != NULL; )
		{
			if (fn->data) free(fn->data);
			fn1 = fn;
			fn = fn->next;
			free(fn1);	
		}
		if (h->dr_lookup) free(h->dr_lookup); 
		h1 = h;
		h = h->next;
		free(h1);
	}
	if (f->drfonts) 
	{
		int n;
		for (n = 0; n < f->drcount; n++)
		{
			if (f->drfonts[n].bitmap) free(f->drfonts[n].bitmap);
		}
		free(f->drfonts);
	}
	if (f->comment) free(f->comment);
	f->comment = NULL;
	f->comment_len = 0;
	f->drcount = 0;
	f->drfonts = NULL;
	f->firstpage = NULL; 
	return 0;
}


CP_HEAD *cpi_lookup(CPI_FILE *f, unsigned number)
{
	CP_HEAD *h;
	
	for (h = f->firstpage; h != NULL; h = h->next)
	{
		if (h->codepage == number) return h;
	}
	return NULL;
}


CP_HEAD *cpi_add_page(CPI_FILE *f, unsigned number)
{
	CP_HEAD *cph;
	CP_HEAD *h;

	cph = cpi_lookup(f, number);
	if (cph) return cph;

 	cph = malloc(sizeof(*cph));
	if (!cph) return NULL;
	memset(cph, 0, sizeof(*cph));	
	cph->codepage = number;
	if (!f->firstpage)
	{
		f->firstpage = cph;
		return cph;
	}	
	for (h = f->firstpage; h->next != NULL; h = h->next);
	h->next = cph;
	return cph;
}



CP_FONT *cph_add_font(CP_HEAD *h)
{
	CP_FONT *cpf = malloc(sizeof(CP_FONT));
	CP_FONT *f;

	if (!cpf) return NULL;
	memset(cpf, 0, sizeof(*cpf));
	if (!h->fonts)
	{
		h->fonts = cpf;
		return cpf;
	}
	for (f = h->fonts; f->next != NULL; f = f->next);
	f->next = cpf;
	return cpf;	
}


int cpi_count_chars(CP_HEAD *cph)
{
	int count = -1;
	CP_FONT *cpf;

/* Let count = the number of characters in the shortest font in this
   codepage (all fonts in a codepage ought to have the same number 
   of characters anyway!) */	
	for (cpf = cph->fonts; cpf != NULL; cpf = cpf->next)
	{
		if (count < 0 || cpf->nchars > count) count = cpf->nchars;
	}
	return count;
}


cpi_byte *cpi_find_glyph(CPI_FILE *f, int codepage, int height, int width,
		int glyph)
{
	CP_FONT *cpf;
	CP_HEAD *cph;
	CP_DRFONT *cpd;
	int n, char_len;
	cpi_byte *result = NULL;

	char_len = ((width + 7) / 8) * height;
	/* If this is a DRFONT, see if we cover this size */
	cpd = NULL;
	if (!strcmp(f->format, "DRFONT "))
	{
		for (n = 0; n < f->drcount; n++)
		{
			if (f->drfonts[n].char_len == char_len)
			{
				cpd = &f->drfonts[n];
				break;
			}
		}
		if (!cpd) return NULL;
	}	

	for (cph = f->firstpage; cph != NULL; cph = cph->next)
	{
		if (cph->codepage != codepage) continue;
		for (cpf = cph->fonts; cpf != NULL; cpf = cpf->next)
		{
			if (cpf->height != height) continue;
			if (cpf->width  != width ) continue;
			if (glyph < 0 || glyph >= cpf->nchars) continue;

			if (cpd) /* DRFONT file */
			{
				result = cpd->bitmap + cph->dr_lookup[glyph] * char_len;
			}
			else
			{
				result = cpf->data + glyph * char_len;
			}
			return result;
		}
	}	
	return result;
}
