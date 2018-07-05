/*
    psftools: Manipulate console fonts in the .PSF format
    Copyright (C) 2005,2006,2007  John Elliott

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

/* Load a CPI file into memory */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cpi.h"

#ifndef SEEK_SET
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2
#endif

static const cpi_byte font_magic[8]   = { 0xFF,'F','O','N','T',' ',' ',' ' };
static const cpi_byte fontnt_magic[8] = { 0xFF,'F','O','N','T','.','N','T' };
static const cpi_byte drfont_magic[8] = { 0x7F,'D','R','F','O','N','T',' ' };

static unsigned short peek2(cpi_byte *p)
{
	return (((unsigned short)p[1]) << 8) | p[0];
}
static unsigned long peek4(cpi_byte *p)
{
	return (((unsigned long)p[3]) << 24) | 
	       (((unsigned long)p[2]) << 16) |
	       (((unsigned long)p[1]) <<  8) | p[0];
}

/* 1.0.5: Support CPI files with segment:offset pointers */
static unsigned long peek4so(cpi_byte *p)
{
	return peek2(p+2) * 16L + peek2(p);
}

static int read2(FILE *fp, unsigned short *s)
{
	cpi_byte b[2];
	if (fread(b, 1, 2, fp) < 2) return CPI_ERR_ERRNO;
	*s = peek2(b);
	return 0;
}

static int read4(FILE *fp, unsigned long *l)
{
	cpi_byte b[4];
	if (fread(b, 1, 4, fp) < 4) return CPI_ERR_ERRNO;
	*l = peek4(b);
	return 0;
}


static int load_fontdata(FILE *fp, CP_HEAD *cph, cpi_byte *fonts_head)
{
	unsigned fonttype, numfonts, m;
	CP_FONT *cpf;
	cpi_byte font_blk[6];
	long pos;

	fonttype      = peek2(fonts_head);
	numfonts      = peek2(fonts_head + 2);
	cph->font_len = peek2(fonts_head + 4);
	/* Can't have DRFONT fonts in a FONT CPI */
	/* Toshiba LCD.CPI has FONTs of type 0, just to be awkward */
	if (fonttype != 1 && fonttype != 0) return CPI_ERR_BADFMT;
/* Load the individual fonts */
/* Fix for some DRDOS CPIs: printer CPIs report 2 but there is only one font */
	if (cph->dev_type == 2) numfonts = 1;
 	cph->font_count = numfonts;
	for (m = 0; m < numfonts; m++)
	{
		if (cph->dev_type == 1)
		{
			pos = ftell(fp);
			if (fread(font_blk, 1, sizeof(font_blk), fp) < (int)sizeof(font_blk))
				return CPI_ERR_BADFMT;	
			cpf = cph_add_font(cph);
			if (!cpf) return CPI_ERR_NOMEM;
/* Codepage header created. */
			cpf->height = font_blk[0];
			cpf->width  = font_blk[1];
			cpf->yaspect= font_blk[2];
			cpf->xaspect= font_blk[3];
			cpf->nchars = peek2(font_blk + 4);
			cpf->fontsize = ((cpf->width + 7) / 8) * cpf->height * cpf->nchars;
			cpf->self_offset = pos;
			cpf->data = malloc(cpf->fontsize);
			if (cpf->data == NULL) 
			{
				cpf->height = cpf->width = cpf->nchars = 0;
				return CPI_ERR_NOMEM;
			}
			if (fread(cpf->data, 1, cpf->fontsize, fp) < cpf->fontsize)
				return CPI_ERR_BADFMT;
		}
		else if (cph->dev_type == 2)
		{
			pos = ftell(fp);
			if (fread(font_blk, 1, 4, fp) < 4)
				return CPI_ERR_BADFMT;	
			cpf = cph_add_font(cph);
			if (!cpf) return CPI_ERR_NOMEM;
			cpf->pr_type    = peek2(font_blk);
			cpf->pr_seqsize = peek2(font_blk + 2); 
			cpf->self_offset = pos;
			cpf->fontsize = cph->font_len - 4;
			cpf->data = malloc(cpf->fontsize);
			if (cpf->data == NULL) 
			{
				cpf->fontsize = 0;
				return CPI_ERR_NOMEM;
			}
			if (fread(cpf->data, 1, cpf->fontsize, fp) < cpf->fontsize)
				return CPI_ERR_BADFMT;
		}
	}	
	return CPI_ERR_OK;
}


static int cpi_loadfont(CPI_FILE *f, FILE *fp, cpi_byte *header)
{
	unsigned short count;
	unsigned n;
	int err;
	cpi_byte fonts_head[6], cphead[28];
	CP_HEAD *cph;
	long pos, maxpos;
	int fontnt = 0;
	long filesize;
	int use_so = 0;

	if (!memcmp(header, fontnt_magic, 8)) fontnt = 1;


	maxpos = 0;
	cpi_new(f, fontnt ? CPI_FONTNT : CPI_FONT);
/* Get the file size. If there's a pointer pointing beyond the end of 
 * the file, it may indicate that we need to change to segment:offset
 * mode. */
	if (fseek(fp, 0, SEEK_END)) return CPI_ERR_ERRNO;
	filesize = ftell(fp);
	if (fseek(fp, peek4(header + 19), SEEK_SET)) return CPI_ERR_ERRNO;
	err = read2(fp, &count);
	if (err) return err;

	for (n = 0; n < count; n++)
	{
		pos = ftell(fp);
		if (fread(cphead, 1, sizeof(cphead), fp) < (int)sizeof(cphead))
			return CPI_ERR_BADFMT;	
		if (peek2(cphead) > 28)
		{
			/* More data in the header. May want to do something
			 * with this, later. */
		}
		cph = cpi_add_page(f, peek2(cphead + 16));
		if (!cph) return CPI_ERR_NOMEM;
		cph->headsize    = peek2(cphead);
		cph->self_offset = pos;
		cph->dev_type    = peek2(cphead + 6);
		sprintf(cph->dev_name, "%-8.8s", cphead + 8);

		if (use_so)
		{
			cph->next_offset = peek4so(cphead + 2);
			cph->cpih_offset = peek4so(cphead + 24);
		}
		else
		{
			cph->next_offset = peek4(cphead + 2);
			cph->cpih_offset = peek4(cphead + 24);
/* 1.0.5: Check for segment:offset; if it's in use, then one or other
 *       pointer (interpreted as a 32-bit integer) will almost certainly 
 *       be pointed off the end of the file. 
 *
 * 	 Note that psftools cannot save CPIs with segment:offset pointers,
 * 	 so the fact that segment:offset pointers are in use is not recorded
 * 	 in the memory structure.
 *
 * 	 Also don't allow a next_offset of FFFF:FFFF to force segment:offset, 
 * 	 because that's an end-of-file marker.
 *       */
			if (cph->cpih_offset > filesize ||
			    (cph->next_offset > filesize && 
			     cph->next_offset != 0xFFFFFFFFL))
			{
				use_so = 1;
				cph->next_offset = peek4so(cphead + 2);
				cph->cpih_offset = peek4so(cphead + 24);
			}
		}
/* FONT.NT fonts use relative offsets */
		if (fontnt) 
		{
			cph->cpih_offset += pos;
			cph->next_offset += pos;
		}
/* Codepage header created. */
		if (fseek(fp, cph->cpih_offset, SEEK_SET)) 
			return CPI_ERR_ERRNO;
		if (fread(fonts_head, 1, sizeof(fonts_head), fp) < (int)sizeof(fonts_head))
			return CPI_ERR_BADFMT;	

		err = load_fontdata(fp, cph, fonts_head);
		if (err) return err;

		if (ftell(fp) > maxpos) maxpos = ftell(fp);

		if (cph->next_offset == 0xFFFFFFFFL || cph->next_offset == 0)
			break;
		if (fseek(fp, cph->next_offset, SEEK_SET)) 
			return CPI_ERR_ERRNO;
		
	}
	fseek(fp, 0, SEEK_END);
	if (ftell(fp) > maxpos)
	{
		f->comment_len = ftell(fp) - maxpos;
		f->comment = malloc(f->comment_len);
		if (!f->comment) return CPI_ERR_NOMEM;
		if (fseek(fp, maxpos, SEEK_SET)) return CPI_ERR_ERRNO;
		if (fread(f->comment, 1, f->comment_len, fp) < f->comment_len)
			return CPI_ERR_ERRNO;	
	}
	return 0;
}


int cpi_loaddrfont(CPI_FILE *f, FILE *fp, cpi_byte *header)
{
	unsigned short count;
	unsigned m, n, q;
	cpi_byte cphead[28], fonts_head[6], font_blk[6];
	CP_HEAD *cph;
	CP_FONT *cpf;
	long pos, maxpos;
	unsigned fonttype, numfonts, maxchar;
	int c, d, err, nchars;

	maxpos = 0;
	cpi_new(f, CPI_DRFONT);
	c = fgetc(fp);	/* Length of extended header */
	if (c == EOF) return CPI_ERR_BADFMT;
	f->drcount = c;
	f->drfonts = malloc(c * sizeof(CP_DRFONT));
	if (!f->drfonts) return CPI_ERR_NOMEM;	
	for (n = 0; n < c; n++)
	{
		d = fgetc(fp);
		if (d == EOF) return CPI_ERR_BADFMT;
		f->drfonts[n].char_len = d;
	}
	for (n = 0; n < c; n++)
	{
		err = read4(fp, (unsigned long *)&f->drfonts[n].offset);
		if (err) return err;
	}
	/* Header read. Now seek to start of data. */
	if (fseek(fp, peek4(header + 19), SEEK_SET)) return CPI_ERR_ERRNO;
	err = read2(fp, &count);
	if (err) return err;

	maxchar = 0;
	for (n = 0; n < count; n++)
	{
		pos = ftell(fp);
		if (fread(cphead, 1, sizeof(cphead), fp) < (int)sizeof(cphead))
			return CPI_ERR_BADFMT;	
		if (peek2(cphead) > 28)
		{
			/* More data in the header. May want to do something
			 * with this, later. */
		}
		cph = cpi_add_page(f, peek2(cphead + 16));
		if (!cph) return CPI_ERR_NOMEM;
		cph->headsize    = peek2(cphead);
		cph->self_offset = pos;
		cph->next_offset = peek4(cphead + 2);
		cph->dev_type    = peek2(cphead + 6);
		sprintf(cph->dev_name, "%-8.8s", cphead + 8);
		cph->cpih_offset = peek4(cphead + 24);
/* Codepage header created. */
		if (fseek(fp, cph->cpih_offset, SEEK_SET)) 
			return CPI_ERR_ERRNO;
		if (fread(fonts_head, 1, sizeof(fonts_head), fp) < (int)sizeof(fonts_head))
			return CPI_ERR_BADFMT;	
		fonttype      = peek2(fonts_head);
		numfonts      = peek2(fonts_head + 2);
		cph->font_len = peek2(fonts_head + 4);
		if (fonttype == 1) /* Wow. FONT record in a DRFONT. DRDOS 
	       mode doesn't seem to mind this, so...	*/
		{
			err = load_fontdata(fp, cph, fonts_head);
			if (err) return err;	
		}
		else if (fonttype != 2) return CPI_ERR_BADFMT;
		else 
		{
			for (m = 0; m < c; m++)
/* Load the individual fonts */
			{
				if (fread(font_blk, 1, sizeof(font_blk), fp) < (int)sizeof(font_blk))
					return CPI_ERR_BADFMT;	
				cpf = cph_add_font(cph);
				if (!cpf) return CPI_ERR_NOMEM;
				cpf->height = font_blk[0];
				cpf->width  = font_blk[1];
				cpf->yaspect= font_blk[2];
				cpf->xaspect= font_blk[3];
				cpf->nchars = peek2(font_blk + 4);
				cpf->fontsize = 0;
				cpf->self_offset = pos;
				cpf->data = NULL;
			}
			nchars = cpi_count_chars(cph);
/* Now load the lookup table into the cph.
 * DRDOS utilities all assume the table is 256 chars long. We will be liberal
 * in what we read and conservative in what we write: the mallocced table will
 * always be at least 256 chars long, but we will only read from the file for
 * characters that are actually there. */
			if (nchars < 256) nchars = 256;
			cph->dr_lookup = malloc(nchars * sizeof(unsigned short));
			if (!cph->dr_lookup) return CPI_ERR_NOMEM;
			nchars = cpi_count_chars(cph);
			for (q = 0; q < nchars; q++)
			{
				err = read2(fp, &cph->dr_lookup[q]);
				if (err) return err;		
				if (cph->dr_lookup[q] > maxchar) 
					maxchar = cph->dr_lookup[q];
			}
			for (;q < 256; q++)
			{
				cph->dr_lookup[q] = 0;
			}
		}
/* Go to next codepage */
		if (maxpos < ftell(fp)) maxpos = ftell(fp);
		if (cph->next_offset == 0xFFFFFFFFL || cph->next_offset == 0)
			break;
		if (fseek(fp, cph->next_offset, SEEK_SET)) 
			return CPI_ERR_ERRNO;
	}
/* Now we need to load the font bitmaps */
	for (n = 0; n < f->drcount; n++)
	{
		if (fseek(fp, f->drfonts[n].offset, SEEK_SET))
			return CPI_ERR_ERRNO;
		f->drfonts[n].bitmap_len = (maxchar + 1) * f->drfonts[n].char_len;
		f->drfonts[n].bitmap = malloc(f->drfonts[n].bitmap_len);
		if (!f->drfonts[n].bitmap) return CPI_ERR_NOMEM;
		if (fread(f->drfonts[n].bitmap, 1, f->drfonts[n].bitmap_len, fp) < f->drfonts[n].bitmap_len) return CPI_ERR_BADFMT;
		if (maxpos < ftell(fp)) maxpos = ftell(fp);
	}
	fseek(fp, 0, SEEK_END);
	if (ftell(fp) > maxpos)
	{
		f->comment_len = ftell(fp) - maxpos;
		f->comment = malloc(f->comment_len);
		if (!f->comment) return CPI_ERR_NOMEM;
		if (fseek(fp, maxpos, SEEK_SET)) return CPI_ERR_ERRNO;
		if (fread(f->comment, 1, f->comment_len, fp) < f->comment_len)
			return CPI_ERR_ERRNO;	
	}
	return 0;
}


int cpi_loadnaked(CPI_FILE *f, FILE *fp)
{
	int err;
	cpi_byte cphead[28], fonts_head[6];
	CP_HEAD *cph;
	long maxpos;

	maxpos = 0;
	cpi_new(f, CPI_NAKED);

	if (fread(cphead, 1, sizeof(cphead), fp) < (int)sizeof(cphead))
		return CPI_ERR_BADFMT;	
	if (peek2(cphead) > 28)
	{
		/* This length doesn't seem to make any difference in a 
		 * naked CP file */
	}
	cph = cpi_add_page(f, peek2(cphead + 16));
	if (!cph) return CPI_ERR_NOMEM;
	cph->headsize    = peek2(cphead);
/* These pointers are meaningless in a .CP file, but do them anyway... */
	cph->self_offset = 0;
	cph->next_offset = peek4(cphead + 2);
	cph->dev_type    = peek2(cphead + 6);
	sprintf(cph->dev_name, "%-8.8s", cphead + 8);
	cph->cpih_offset = peek4(cphead + 24);
/* Codepage header created. Don't bother to seek to the image header; it 
 * must follow this header immediately. */
	if (fread(fonts_head, 1, sizeof(fonts_head), fp) < (int)sizeof(fonts_head))
		return CPI_ERR_BADFMT;	
/* Force the type in the header to 1; some files have 0. */
	fonts_head[0] = 1;
	fonts_head[1] = 0;
	err = load_fontdata(fp, cph, fonts_head);
	if (err) return err;

	if (ftell(fp) > maxpos) maxpos = ftell(fp);
	fseek(fp, 0, SEEK_END);
	if (ftell(fp) > maxpos)
	{
		f->comment_len = ftell(fp) - maxpos;
		f->comment = malloc(f->comment_len);
		if (!f->comment) return CPI_ERR_NOMEM;
		if (fseek(fp, maxpos, SEEK_SET)) return CPI_ERR_ERRNO;
		if (fread(f->comment, 1, f->comment_len, fp) < f->comment_len)
			return CPI_ERR_ERRNO;	
	}
	return 0;
}



int cpi_load(CPI_FILE *f, const char *filename)
{
	int err;
	FILE *fp;

	fp = fopen(filename, "rb");
	if (!fp) return CPI_ERR_ERRNO;
	err = cpi_loadfile(f, fp);
	if (fclose(fp)) return CPI_ERR_ERRNO;
	return err;
}

int cpi_loadfile(CPI_FILE *f, FILE *fp)
{
	cpi_byte magic[23];
	int err;

	if (fread(magic, 1, 23, fp) < 23) 
	{
		fclose(fp);
		return CPI_ERR_BADFMT;
	}
	if (!memcmp(magic, font_magic, 8) || !memcmp(magic, fontnt_magic, 8))
	{
		f->magic0 = magic[0];
		sprintf(f->format, "%-7.7s", magic + 1);
		err = cpi_loadfont(f, fp, magic);
	}
	else if (!memcmp(magic, drfont_magic, 8))
	{
		f->magic0 = magic[0];
		sprintf(f->format, "%-7.7s", magic + 1);
		err = cpi_loaddrfont(f, fp, magic);
	}
/* .CP: "Naked" font extracted from a CPI file */
	else if (magic[0] >= 0x1A && magic[0] <= 0x20 && magic[1] == 0)
	{
		f->magic0 = 0xFF;
		sprintf(f->format, "%-7.7s", "FONT");
		fseek(fp, 0, SEEK_SET);
		err = cpi_loadnaked(f, fp);
	}
	else err = CPI_ERR_BADFMT;
	return err;
}
	
