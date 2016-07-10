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
#include "cnvshell.h"
#include "psflib.h"
#include "cpi.h"

/* Extract a font from a .CPI file.
 * 
 */

static char helpbuf[2048];
static int height = -1; 
static int cpage = -1;
static int v1 = 0, v2 = 0;
static PSF_MAPPING *codepage = NULL;
static PSF_FILE psf;
static CPI_FILE cpi;

/* Program name */
char *cnv_progname = "CPI2PSF";

/* ddash = 1 if option started with a double-dash; else 0 */
/* Return NULL if OK, else error string */
char *cnv_set_option(int ddash, char *variable, char *value)
{
    if (!stricmp(variable, "srcpage") || !stricmp(variable, "codepage")) 
    { 
	    char buf[20];
	    cpage  = atoi(value); 
	    sprintf(buf, "CP%d", cpage);
	    codepage = psf_find_mapping(buf); 
	    if (codepage == NULL) 
	    {
		    fprintf(stderr, "Warning: No Unicode mapping known for codepage %d\n", cpage);
	    } 
	    return NULL; 
    
    }
    if (!stricmp(variable, "height")) { height = atoi(value); return NULL; }
    if (!stricmp(variable, "psf1"))   { v1 = 1; return NULL; }
    if (!stricmp(variable, "psf2"))   { v2 = 1; return NULL; }
    if (strlen(variable) > 2000) variable[2000] = 0;
    sprintf(helpbuf, "Unknown option: %s\n", variable);
    return helpbuf;
    }


/* Return help string */
char *cnv_help(void)
    {
    sprintf(helpbuf, "Syntax: %s cpifile psffile { options }\n\n", cnv_progname);
    strcat (helpbuf, "Options: \n"
	             "    --height=x     Extract the font with the given height\n"
	             "    --srcpage=x    Only extract codepage x\n"
                     "    --psf1         Output in PSF1 format\n"
                     "    --psf2         Output in PSF2 format (default) \n");
            
    return helpbuf;
}

static psf_dword *unicodes = NULL;
static int unicodes_max = 0, unicodes_count = 0;

static psf_errno_t unicode_add(psf_dword *token)
{
	psf_dword *tmp;
	int n;

	for (n = 0; n < unicodes_count; n++)
	{
		if (unicodes[n] == token[0]) return PSF_E_OK;
	}

	if (unicodes_count >= unicodes_max)
	{
		tmp = calloc(unicodes_max + 256, sizeof(psf_dword));
		if (!tmp) return PSF_E_NOMEM;
		if (unicodes) memcpy(tmp, unicodes,
				unicodes_max * sizeof(psf_dword));
		free(unicodes);
		unicodes = tmp;
		unicodes_max += 256;
	}
	unicodes[unicodes_count++] = token[0];
	return PSF_E_OK;
}

char *cnv_execute(FILE *infile, FILE *outfile)
{
	int rv, m, n, err;
	CP_HEAD *cph;
	CP_FONT *cpf;
	int  nchars, fontw = 0;
	psf_dword glyph;
	char buf[20];

	nchars = 0;
	err = cpi_loadfile(&cpi, infile);
	if (err == CPI_ERR_BADFMT) return "File is not a valid CPI file";
/* This relies on the same error codes being used by the CPI & PSF libs */
	if (err) return psf_error_string(err);

/* If no height given, take the first one we find. If there is a height 
 * given, then look it up anyway so we know the width to use for the 
 * output. */
	fontw = -1;
	for (cph = cpi.firstpage; cph != NULL; cph = cph->next)
		for (cpf = cph->fonts; cpf != NULL; cpf = cpf->next)
	{
		if (height == -1 || cpf->height == height)
		{
			height = cpf->height;
			fontw = cpf->width;
			goto gotheight;	/* Break out of 2 loops */
		}
	}			
gotheight:
	if (fontw < 0) return "Requested font size not found in CPI file.";
/* Prepare for dump... */
	for (cph = cpi.firstpage; cph != NULL; cph = cph->next)
	{
		/* If user requested only one codepage, only do that 
		 * codepage */
		if (cpage != -1 && cph->codepage != cpage) continue;

	  	sprintf(buf, "CP%d", cph->codepage);
	    	codepage = psf_find_mapping(buf); 
	    	if (cpage == -1 && codepage == NULL) 
	    	{
	    		fprintf(stderr, "Warning: No Unicode mapping "
				"known for codepage %d; it will "
				"be skipped.\n", cph->codepage);
			continue;
		}
		if (!cph->fonts)
		{
			fprintf(stderr, "Codepage %d has no fonts; "
					"it will be skipped.\n",
					cph->codepage);
		}
		nchars = cph->fonts->nchars;
		if (codepage)
		{
			if (nchars > 256) nchars = 256;
			for (n = 0; n < nchars; n++)
			{
				if (unicode_add(codepage->psfm_tokens[n]))
					return "Out of memory";
			}
		}
	}
/* OK. There are unicodes_count unique codepoints */
	psf_file_new(&psf);
	if (unicodes_count)
	{
/* 1.0.3: If extracting a single page, create a PSF that will hold 256 
 * characters, rather than merging identical Unicode codepoints */
		if (cpage == -1) 
			rv = psf_file_create(&psf, fontw, height, unicodes_count, 1);
		else	rv = psf_file_create(&psf, fontw, height, nchars, 1);
	}		
	else	rv = psf_file_create(&psf, fontw, height, nchars, 0);
	if (rv) return psf_error_string(rv);
/* Dump the character bitmaps */
	for (cph = cpi.firstpage; cph != NULL; cph = cph->next)
	{
		/* Include this codepage? */
		if (cpage != -1 && cph->codepage != cpage) continue;

		sprintf(buf, "CP%d", cph->codepage);
	    	codepage = psf_find_mapping(buf); 
		if (cpage == -1 && codepage == NULL) continue;
		if (!cph->fonts) continue;
		nchars = cph->fonts->nchars;
		if (nchars > 256) nchars = 256;
		for (n = 0; n < nchars; n++)
		{
			cpi_byte *dst = NULL;
			cpi_byte *src = cpi_find_glyph(&cpi, 
					cph->codepage, height, fontw, n);
			if (!src) continue;
/* psftools 1.0.3: If extracting a single page, then retain the order of 
 * characters that it used. For compatibility with the way --codepage works 
 * in other psftools. */
			if (codepage && (cpage == -1))
			{
				glyph = codepage->psfm_tokens[n][0];
				for (m = 0; m < unicodes_count; m++)
				{
					if (unicodes[m] == glyph)
					{
						dst = psf.psf_data + m * psf.psf_charlen; 
/* Add the entry, if it doesn't exist */
						if (psf_unicode_lookup(&psf, glyph, NULL) == PSF_E_NOTFOUND) psf_unicode_addmap(&psf, m, codepage, n);
						break;
					}
				}
			}
/* Extracting a single codepage, but we can add Unicode mappings */
			else 
			{
				dst = psf.psf_data + n * psf.psf_charlen;
				if (codepage && (n < 256))
				{
					psf_unicode_addmap(&psf, n, codepage, n);
				}
			}
			if (dst) memcpy(dst, src, psf.psf_charlen);
		}
	}
	if (v1) psf_force_v1(&psf);
	if (v2) psf_force_v2(&psf);
	rv = psf_file_write(&psf, outfile);
	psf_file_delete(&psf);	
	if (rv) return psf_error_string(rv);
	return NULL;
}

