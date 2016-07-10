
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cpi.h"


/* Given a DRFONT, turn it into a conventional font */
int cpi_bloat(CPI_FILE *f)
{
	CP_HEAD *cph;
	CP_FONT *cpf;
	CP_DRFONT *cpd;
	int nsizes, chcount, chlen;
	int n, m;
	cpi_byte *src, *dest;

	if (strcmp(f->format, "DRFONT ")) 
		return 0;	/* Not DRFONT */

	nsizes = 0;
/* Allocate all the font bitmaps */
	for (cph = f->firstpage; cph != NULL; cph = cph->next)
	{	
		for (cpf = cph->fonts; cpf != NULL; cpf = cpf->next)
		{
			chlen = ((cpf->width + 7) / 8) * cpf->height;
			if (cpf->data) { free(cpf->data); cpf->data = NULL; }
			cpf->data = malloc(chlen * cpf->nchars);
			if (!cpf->data) return CPI_ERR_NOMEM;
			cpf->fontsize = chlen * cpf->nchars;
		}
	}
/* Populate the font bitmaps */
	for (cph = f->firstpage; cph != NULL; cph = cph->next)
	{	
		chcount = cpi_count_chars(cph);
		for (m = 0, cpf = cph->fonts; cpf != NULL; cpf = cpf->next, m++)
		{
			cpd = NULL;
			chlen = ((cpf->width + 7) / 8) * cpf->height;

/* Hopefully the mth drfont header is the one we want. */
			if (f->drfonts[m].char_len == chlen)
			{
				cpd = &f->drfonts[m];
			}
/* If not, try to find one with the right character size. This could, of
 * course, be fooled by a DRFONT which has (eg) 8x16 and 16x8 characters */
			else for (n = 0; n < f->drcount; n++)
			{
				if (f->drfonts[n].char_len == chlen)
				{
					cpd = &f->drfonts[n];
					break;
				}	
			}
			if (!cpd) continue;
			for (n = 0; n < chcount; n++)
			{
				src  = cpd->bitmap + chlen * cph->dr_lookup[n];
				dest = cpf->data   + chlen * n;
				memcpy(dest, src, chlen);
			}
		}
		/* Codepage migrated. Free its dr_lookup table */
		free(cph->dr_lookup);
		cph->dr_lookup = NULL;
	}
/* Switch font header to FONT */
	f->magic0 = 0xFF;
	strcpy(f->format, "FONT   ");
	free(f->drfonts);
	f->drfonts = NULL;
	f->drcount = 0;
	return CPI_ERR_OK;
}
