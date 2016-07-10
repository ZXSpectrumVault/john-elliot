
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cpi.h"

/* Temporary data - one of these for each size of characters in the
 * resulting CPI. Like a CP_DRFONT header but with more fields */
typedef struct crushtmp
{
	unsigned width;		/* Character width */
	unsigned height;	/* Character height */
	unsigned chsize;	/* Character size = ((width+7)/8) * height */
	unsigned nchars;	/* Number of characters, including 
				 * duplicates */
	cpi_byte *charset;	/* Character set itself */
	cpi_byte *glyph;	/* Current glyph */
	struct crushtmp *next;
} CRUSHTMP;

/* Free the chain of CRUSHTMP structures */
static void free_ctmp(CRUSHTMP *ct)
{
	void *mem;

	while (ct)
	{
		if (ct->charset) free(ct->charset);
		mem = ct;
		ct = ct->next;
		free(mem);
	}
}

/* Given a CP_FONT, find out which CRUSHTMP its characters should go to */
static CRUSHTMP *find_ctmp(CRUSHTMP *ct, CP_FONT *cpf)
{
	int chsize;

	for(; ct != NULL; ct = ct->next)
	{
		chsize = ((cpf->width + 7) / 8) * cpf->height;		
		if (cpf->height == ct->height && chsize == ct->chsize)
			return ct;
	}
	return NULL;
}


int cpi_crush(CPI_FILE *f)
{
	CP_HEAD *cph;
	CP_FONT *cpf;
	int chindex;
	int nsizes, chcount, chlen;
	int ch, n;
	CRUSHTMP *tmp, *ct;
	
	tmp = NULL;
	if (!strcmp(f->format, "DRFONT ")) 
		return 0;	/* Already DRFONT */

	/* Start to construct a DRFONT header. Firstly, work out how many 
	 * font sizes there are */
	nsizes = 0;
	for (cph = f->firstpage; cph != NULL; cph = cph->next)
	{	
/* Skip printer codepages */
		if (cph->dev_type != 1) continue;
/* Allocate all the lookup tables. As each character is migrated, we 
 * will populate the appropriate lookup table. */
		if (!cph->dr_lookup)
		{
			chcount = cpi_count_chars(cph);
			cph->dr_lookup = malloc(chcount * sizeof(unsigned short));
			if (!cph->dr_lookup) return CPI_ERR_NOMEM;
			memset(cph->dr_lookup, 0, chcount * sizeof(unsigned short));
		}
		for (cpf = cph->fonts; cpf != NULL; cpf = cpf->next)
		{
			chlen = ((cpf->width + 7) / 8) * cpf->height;
			ct = find_ctmp(tmp, cpf);
			if (ct)	/* Just add current char count */
			{
				ct->nchars += cpf->nchars;
			}
			else	/* Not found */
			{
				ct = malloc(sizeof(CRUSHTMP));
				if (ct == NULL) 
				{
					free_ctmp(tmp);
					return CPI_ERR_NOMEM;
				}
				ct->height = cpf->height;
				ct->width  = cpf->width;
				ct->nchars = cpf->nchars;
				ct->chsize = chlen;
				ct->charset= NULL;
				ct->glyph  = NULL;
				++nsizes;
/* Add this temp structure to the list */
				ct->next = tmp;
				tmp = ct;
			}
/* Can't have more than 256 sizes, because DRFONT uses a single byte to 
  count them */
			if (nsizes >= 256) break;
		}
	}
	if (!nsizes) return CPI_ERR_EMPTY;
	f->drfonts = malloc(nsizes * sizeof(CP_DRFONT));
	if (!f->drfonts) return CPI_ERR_NOMEM;
	f->drcount = nsizes;
/* Allocate memory for all the font bitmaps */
	for (ct = tmp; ct != NULL; ct = ct->next)
	{
		ct->charset = malloc(ct->chsize * ct->nchars);
		if (!ct->charset)
		{
			free_ctmp(tmp);
			return CPI_ERR_NOMEM;
		}	
	}
/* Now merge the characters */
	chindex = 0;
	for (cph = f->firstpage; cph != NULL; cph = cph->next)
	{
		int match;

/* For each character in this codepage, find all the glyphs that represent
 * it */
		chcount = cpi_count_chars(cph);
		for (ch = 0; ch < chcount; ch++)
		{
/* Set the current glyph in each temp structure */
			for (cpf = cph->fonts; cpf != NULL; cpf = cpf->next)
			{
				ct = find_ctmp(tmp, cpf);
				if (!ct || !cpf->data) continue;	
				ct->glyph = cpf->data + (ch * ct->chsize);
			}
/* See if that glyph is already present */

			match = 0;
			for (n = 0; n < chindex; n++)
			{
				match = 1;
/* At each size: Compare current glyph with glyph n */
				for (ct = tmp; ct != NULL; ct = ct->next)
				{
					if (memcmp(ct->glyph,
					   ct->charset + n * ct->chsize,
					   ct->chsize))
					{
						match = 0;	
					}
				}
				if (match)	/* Got one! */
				{
					cph->dr_lookup[ch] = n;
					break;
				}
			}
/* No luck. Have to add it */
			if (!match) /* Doesn't already exist */
			{
				cph->dr_lookup[ch] = chindex;
/* Add character to the same position in each table */
				for (ct = tmp; ct != NULL; ct = ct->next)
				{
					memcpy(ct->charset+ct->chsize*chindex, 
					       ct->glyph, ct->chsize);
				}
				++chindex;
			}
		}
	}
/* Construct DRFONT headers... */
	for (n = 0, ct = tmp; ct != NULL; ct = ct->next, n++)
	{
		f->drfonts[n].char_len   = ct->chsize;
		f->drfonts[n].bitmap_len = ct->chsize * chindex;
		f->drfonts[n].offset = 0;

		f->drfonts[n].bitmap = malloc(f->drfonts[n].bitmap_len);
		if (!f->drfonts[n].bitmap) 
		{
			free_ctmp(tmp);
			return CPI_ERR_NOMEM;
		}
		memcpy(f->drfonts[n].bitmap, ct->charset, f->drfonts[n].bitmap_len);
		free(ct->charset);
		ct->charset = NULL;
	}
/* Free the original font data */
	for (cph = f->firstpage; cph != NULL; cph = cph->next)
	{	
		for (cpf = cph->fonts; cpf != NULL; cpf = cpf->next)
		{
			if (cpf->data) free(cpf->data);
			cpf->data = NULL;
		}
	}	
	f->magic0 = 0x7F;
	strcpy(f->format, "DRFONT ");
	free_ctmp(tmp);
	return CPI_ERR_OK;
}
