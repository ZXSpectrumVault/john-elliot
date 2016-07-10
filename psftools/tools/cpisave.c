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

/* Write a CPI file to disk */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cpi.h"

static void poke4(cpi_byte *address, long value)
{
	address[0] = 0xFF & value;
	address[1] = 0xFF & (value >> 8);
	address[2] = 0xFF & (value >> 16);
	address[3] = 0xFF & (value >> 24);
}

static int write_i1(FILE *fp, unsigned char i1)
{
	if (fputc(i1, fp) == EOF) return CPI_ERR_ERRNO;
	return CPI_ERR_OK;
}

static int write_i2(FILE *fp, unsigned short i2)
{
	cpi_byte c[2];

	c[0] = i2 & 0xFF;
	c[1] = i2 >> 8;

	if (fwrite(c, 1, 2, fp) < 2) return CPI_ERR_ERRNO;
	return CPI_ERR_OK;
}

static int write_i4(FILE *fp, unsigned long i4)
{
	int err;

	err = write_i2(fp,  i4 & 0xFFFF);
	if (err) return err;
	return (write_i2(fp, i4 >> 16));
}


static int write_cph(FILE *fp, CP_HEAD *cph, int ntfont, long endfile)
{
	char buf[9];
	long nxoffset, datoffset;
	int err = CPI_ERR_OK;

	sprintf(buf, "%-8.8s", cph->dev_name);

	if (!err) err = write_i2(fp, 28);

	nxoffset  = cph->next_offset;
	datoffset = cph->cpih_offset;
	if (ntfont)
	{	
		if (!nxoffset) nxoffset = endfile;
		nxoffset  -= cph->self_offset;
		datoffset -= cph->self_offset;
	}
	if (!err) err = write_i4(fp, nxoffset);
	if (!err) err = write_i2(fp, cph->dev_type);	
	if (!err && fwrite(buf, 1, 8, fp) < 8) err = CPI_ERR_ERRNO;
	if (!err) err = write_i2(fp, cph->codepage);
	if (!err) err = write_i4(fp, 0);
	if (!err) err = write_i2(fp, 0);
	if (!err) err = write_i4(fp, datoffset);
	return err;
}

int cpi_save  (CPI_FILE *f, const char *filename, int interleaved)
{
	int err;
	FILE *fp;	

        fp = fopen(filename, "wb");
        if (!fp) return CPI_ERR_ERRNO;
        err = cpi_savefile(f, fp, interleaved);
	if (err)
	{
		fclose(fp);
		remove(filename);
		return err;
	}
        if (fclose(fp)) return CPI_ERR_ERRNO;
	return err;
}

static int compare_drfonts(const void *a, const void *b)
{
	const CP_DRFONT *dra = (CP_DRFONT *)a;
	const CP_DRFONT *drb = (CP_DRFONT *)b;

	return dra->char_len - drb->char_len;
}

/* In DRFONT fonts, sort in ascending order */
static int comp_dcpf(const void *a, const void *b)
{
	const CP_FONT *fna = (CP_FONT *)a;
	const CP_FONT *fnb = (CP_FONT *)b;

	return (fna->height * fna->width) - (fnb->height * fnb->width);
}


/* In FONT fonts, sort in descending order */
static int comp_cpf(const void *a, const void *b)
{
	const CP_FONT *fna = (CP_FONT *)a;
	const CP_FONT *fnb = (CP_FONT *)b;

	return (fnb->height * fnb->width) - (fna->height * fnb->width);
}


int cpi_savefile(CPI_FILE *f, FILE *fp, int interleaved)
{
	int err = CPI_ERR_OK;
	cpi_byte header[23];
	long fpos;
	CP_HEAD *cph;
	CP_FONT *cpf, *cpf2, *cpf3;
	int n, count, nchars;
	int drfont = 0;
	int ntfont = 0;

	/* Write out header */
	memset(header, 0, sizeof(header));
	header[0] = f->magic0;
	sprintf((char *)(header+1), "%-7.7s", f->format);
	header[0x10] = 1;	/* 1 pointer */
	header[0x12] = 1;	/* Type 1 */
	fpos = 0x17;
	if (!strcmp(f->format, "FONT.NT")) 
	{
		ntfont = 1;
	}
	if (f->magic0 == 0x7F)	/* DRFONT */
	{
		drfont = 1;
		fpos += (5 * f->drcount) + 1; 
	}
/* Sort the font sizes within the file */
	if (drfont) qsort(f->drfonts, f->drcount, sizeof(CP_DRFONT),
			compare_drfonts);
	for (cph = f->firstpage; cph != NULL; cph = cph->next)
	{
		n = 0;
		for (cpf = cph->fonts; cpf != NULL; cpf = cpf->next) n++;
		cpf2 = malloc(n * sizeof(CP_FONT));
/* If we have enough memory, sort the fonts as well */
		if (cpf2)
		{
			n = 0;
	/* Copy all the fonts to an array for sorting */
			for (cpf = cph->fonts; cpf != NULL; cpf = cpf->next) 
			{
				memcpy(&cpf2[n], cpf, sizeof(CP_FONT));
				n++;
			}
	/* Sort them */
			if (drfont) qsort(cpf2, n, sizeof(CP_FONT), comp_dcpf);
			else	    qsort(cpf2, n, sizeof(CP_FONT), comp_cpf);
	/* Copy them back */
			n = 0;
			for (cpf = cph->fonts; cpf != NULL; cpf = cpf->next) 
			{
				cpf3 = cpf->next;
				memcpy(cpf, &cpf2[n], sizeof(CP_FONT));
				cpf->next = cpf3;
				n++;
			}
			free(cpf2);
		}
	}

/* Note that this allows us to write a file that's both FONT.NT and DRFONT;
  I don't advise it :-) */
	
	poke4(header + 0x13, fpos);	

	if (fwrite(header, 1, sizeof(header), fp) < (int)sizeof(header))
	{
		return CPI_ERR_ERRNO;
	}
	/* Calculate addresses of all structures. */
	fpos += 2;	/* Skip over count of pages */
	count = 0;
	/* If not interleaved, do all the headers followed by all the data */
	if (!interleaved) for (cph = f->firstpage; cph != NULL; cph = cph->next)
	{
		cph->self_offset = fpos;
		if (cph->next)	cph->next_offset = fpos + 28;
		else		cph->next_offset = 0;

		fpos += 28;	
		++count;
	}
	for (cph = f->firstpage; cph != NULL; cph = cph->next)
	{
		/* If interleaved, do font headers here */
		if (interleaved) 
		{
			cph->self_offset = fpos;
			fpos += 28;	
			++count;
		}
		cph->cpih_offset = fpos;
		fpos += 6;	/* General header */
		for (cpf = cph->fonts; cpf != NULL; cpf = cpf->next)
		{
			if (cph->dev_type == 1)
			{
				fpos += 6;	/* Fontsize header */
				if (!drfont) fpos += cpf->fontsize;
			}
			else if (cph->dev_type == 2)
			{
				fpos += 4;	/* Fontsize header */
				fpos += cpf->fontsize;
			}	
		}
/* Font length in the header does not include the DRFONT index.
 * DRDOS utilities assume the index is always 256 chars. We will always
 * write an index that's at least 256 chars long, but if there are more
 * characters it will be longer. */
		cph->font_len = fpos - cph->cpih_offset;
		if (drfont)
		{
			nchars = cpi_count_chars(cph);
			if (nchars < 256) nchars = 256;
			fpos += 2 * nchars;
		}
					/* DRFONT font index */
		if (interleaved)
		{
			if (cph->next)	cph->next_offset = fpos;
			else		cph->next_offset = 0;
		}
	}
	/* Lastly, the font data (if DRFONT) */
	if (drfont) for (n = 0; n < f->drcount; n++)
	{
		f->drfonts[n].offset = fpos;
		fpos += f->drfonts[n].bitmap_len;
	}
	/* Now start writing the structures. Firstly, the extended
	 * DRFONT header. */
	if (drfont)
	{
		err = write_i1(fp, f->drcount);
		for (n = 0; n < f->drcount; n++)
		{
			if (!err) err = write_i1(fp, f->drfonts[n].char_len);
		}
		for (n = 0; n < f->drcount; n++)
		{
			if (!err) err = write_i4(fp, f->drfonts[n].offset);
		}
	}
	if (!err) err = write_i2(fp, count); 
	/* Do the codepage headers (if not interleaving) */
	if (!err && !interleaved) 
		for (cph = f->firstpage; cph != NULL; cph = cph->next)
	{
		if (!err) err = write_cph(fp, cph, ntfont, fpos);
	}

	/* Next, the codepage bodies */
	if (!err) for (cph = f->firstpage; cph != NULL; cph = cph->next)
	{
		int fcount = 0;

		if (!err && interleaved) err = write_cph(fp, cph, ntfont, fpos);
		for (cpf = cph->fonts; cpf != NULL; cpf = cpf->next)
			++fcount;
		if (!err) err = write_i2(fp, drfont ? 2 : 1);
		if (!err) err = write_i2(fp, fcount);
		if (!err) err = write_i2(fp, cph->font_len - 6);
		if (!err) for (cpf = cph->fonts; cpf != NULL; cpf = cpf->next)
		{
			if (cph->dev_type == 1)
			{
				if (!err) err = write_i1(fp,cpf->height);
				if (!err) err = write_i1(fp,cpf->width);
				if (!err) err = write_i1(fp,cpf->yaspect);
				if (!err) err = write_i1(fp,cpf->xaspect);
				if (!err) err = write_i2(fp,cpf->nchars);
				if (!err && !drfont)
				{
					if (fwrite(cpf->data, 1, 
					  cpf->fontsize, fp) < cpf->fontsize) 
						err = CPI_ERR_ERRNO;
				}
			}
			else if (cph->dev_type == 2)
			{
				if (!err) err = write_i2(fp,cpf->pr_type);
				if (!err) err = write_i2(fp,cpf->pr_seqsize);
				if (!err && fwrite(cpf->data, 1, 
				  cpf->fontsize, fp) < cpf->fontsize) 
					err = CPI_ERR_ERRNO;
			}
		}
		if (!err && drfont)
		{
/* DRDOS always assumes that this table has 256 entries. We will always
 * write an index that's at least 256 chars long, but if there are more
 * characters it will be longer. */
			nchars = cpi_count_chars(cph);
			if (nchars < 256) nchars = 256;
			for (n = 0; n < nchars; n++)
			{
				if (!err) err = write_i2(fp,cph->dr_lookup[n]);
			}
			for (; n < 256; n++)
			{
				if (!err) err = write_i2(fp, 0);
			}
		}
	}
	/* The fonts (if DRFONT) */
	if (!err && drfont)
	{
		for (n = 0; n < f->drcount; n++)
		{
			if (!err && fwrite(f->drfonts[n].bitmap, 1,
			  f->drfonts[n].bitmap_len, fp) < f->drfonts[n].bitmap_len) err = CPI_ERR_ERRNO;
		}	
	}
	/* And any comment */
	if (!err && f->comment && f->comment_len)
	{
		if (fwrite(f->comment, 1, f->comment_len, fp) < f->comment_len)
			err= CPI_ERR_ERRNO;
	}
	return err;
}
