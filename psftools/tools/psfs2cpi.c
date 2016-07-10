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

/* Merge one or more PSF files into a CPI file */

#include "cnvmulti.h"
#include "psflib.h"
#include "cpi.h"
#include <ctype.h>


static char helpbuf[2048];
static char msgbuf[1000];
static int drfont;
static int nt;
static int verbose;
static int inter = 1;
static char device[9] = "EGA     ";


/* Program name */
char *cnv_progname = "PSFS2CPI";

/* ddash = 1 if option started with a double-dash; else 0 */
/* Return NULL if OK, else error string */
char *cnv_set_option(int ddash, char *variable, char *value)
{
	if (!stricmp(variable, "drfont")) 
	{
		drfont = 1;
		return NULL;
	}
	if (!stricmp(variable, "nt"))
	{
		nt = 1;
		return NULL;
	}
	if (!strcmp(variable, "verbose"))
	{
		verbose = 1;
		return NULL;
	}
	if (!strcmp(variable, "device"))
	{
		sprintf(device, "%-8.8s", value);
		return NULL;
	}
	if (!strcmp(variable, "comment"))
	{
		strncpy(msgbuf, value, sizeof(msgbuf)-1);
		msgbuf[sizeof(msgbuf)-1] = 0;
		return NULL;
	}
	if (strlen(variable) > 2000) variable[2000] = 0;
	sprintf(helpbuf, "Unknown option: %s\n", variable);
	return helpbuf;
}


/* Return help string */
char *cnv_help(void)
    {
    sprintf(helpbuf, "Builds a .CPI codepage from one or more .PSF fonts\n\n"
		     "Syntax: %s +nnn source.psf source.psf +nnn "
		            "source.psf ... target.cpi \n\n", cnv_progname);
    strcat(helpbuf,  "Options are: --device=<device name>\n");
    strcat(helpbuf,  "             --comment=<comment>\n");
    strcat(helpbuf,  "             --drfont: DRFONT output format\n");
    strcat(helpbuf,  "             --nt: FONT.NT output format\n");
    strcat(helpbuf,  "             --verbose: List each file as it is processed\n");
    return helpbuf;
    }


/* Do the conversion */
char *cnv_multi(int nfiles, char **infiles, FILE *outfile)
{        
        int rv, nf, nchars, nc;
	psf_dword mc;
	CPI_FILE cpi;
	CP_HEAD *cph;
	CP_FONT *cpf, *cpf2;
	PSF_FILE psf;
	long codepage = -1;
	PSF_MAPPING *mapping = NULL;
	char mapname[20];
        FILE *fp;

	if (drfont && nt) return "Output format can't be both DRFONT and FONT.NT";
/* Always create the file as FONT. We will convert to FONT.NT or DRFONT 
 * later if required */
	cpi_new(&cpi, CPI_FONT);
        for (nf = 0; nf < nfiles; nf++)
        { 
/* The syntax "+nnn" denotes a codepage number */
	 	if (infiles[nf][0] == '+' && isdigit(infiles[nf][1]))
		{
			char *equals;

			if (!sscanf(&infiles[nf][1], "%lu", &codepage))
				codepage = -1;

/* Doesn't handle 65280 properly in 16-bit DOS 
 * codepage = atoi(&infiles[nf][1]); */
			equals = strchr(infiles[nf], '=');
			if (equals) strcpy(mapname, equals + 1);
			else sprintf(mapname, "CP%ld", codepage);
			mapping = psf_find_mapping(mapname);
			if (!mapping)
			{
				fprintf(stderr, "Warning: This program contains"
				" no mapping for codepage %ld.\nThe first 256"
				" characters of provided PSFs will be used.\n",
				codepage);
			}
			else if (verbose)
				fprintf(stderr, "Processing codepage %ld\n", 
						codepage);
			continue;
		}
		if (codepage < 0) 
			return "No codepage specified for first PSF file.";
		if (verbose)
			fprintf(stderr,"Processing file: '%s'\n", 
					infiles[nf]);
                if (!strcmp(infiles[nf], "-")) fp = stdin;
                else  fp = fopen(infiles[nf], "rb");
          
                if (!fp) 
		{ 
			perror(infiles[nf]); 
			return "Cannot open font file."; 
		}
		psf_file_new(&psf);
		rv = psf_file_read(&psf, fp);
                if (fp != stdin) fclose(fp);
		if (rv != PSF_E_OK) return psf_error_string(rv);
/* PSF loaded. Get the codepage & font to load into. */	
		cph = cpi_add_page(&cpi, (unsigned)codepage);
		if (!cph) return "Out of memory adding codepage to file";
		for (cpf = cph->fonts; cpf != NULL; cpf = cpf->next)
		{
			if (cpf->height == psf.psf_height &&
			    cpf->width  == psf.psf_width)
			{
				fprintf(stderr, "Codepage %ld already has a %dx%d font; it will be replaced\n", codepage, cpf->height, cpf->width);
				if (cpf->data) free(cpf->data);
				cpf->data = NULL;
				cpf->fontsize = 0;
				break;
			}
		}
		if (cpf == NULL)
		{
			cpf = malloc(sizeof(CP_FONT));
			if (cpf == NULL) return "Out of memory adding font to codepage";
			memset(cpf, 0, sizeof(CP_FONT));
			if (!cph->fonts) cph->fonts = cpf;
			else for (cpf2 = cph->fonts; cpf2 != NULL; cpf2 = cpf2->next)
			{
				if (cpf2->next == NULL)
				{
					cpf2->next = cpf;
					break;
				}
			}
		}
		strcpy(cph->dev_name, device);
		cph->dev_type = 1;
/* OK. cpf is the data structure for the new font, and it's in the chain */
		if (psf.psf_length < 256) 
		{
			fprintf(stderr, "Warning: %s has fewer than 256 characters.\n", infiles[nf]);
			nchars = psf.psf_length;
		}
		else    nchars = 256;
		cpf->nchars = nchars;
		cpf->height = psf.psf_height;
		cpf->width  = psf.psf_width;
		if (psf.psf_width > 8)
		{
			fprintf(stderr, "Warning: %s has a width of %ld\n",
					infiles[nf], psf.psf_width);
		}
		cpf->fontsize = nchars * psf.psf_charlen;
		cpf->data = malloc(cpf->fontsize);
		if (cpf->data == NULL) return "Out of memory adding bitmap to font";
		memset(cpf->data, 0, cpf->fontsize);
		if (mapping && !psf_is_unicode(&psf))
		{
			fprintf(stderr, "Warning: File %s has no "
			"Unicode table.\nThe first 256 "
			"characters will be used.\n", infiles[nf]);
		}
		if (mapping && psf_is_unicode(&psf))
		{
			psf_dword codepoint;

			for (nc = 0; nc < 256; nc++)
			{
				codepoint = mapping->psfm_tokens[nc][0];
				if (psf_unicode_lookupmap(&psf,mapping,nc,&mc, &codepoint))
				{
					/* Not found */
					if (!psf_unicode_banned(codepoint)) fprintf(stderr, "Warning: %s has no bitmap for U+%04lx\n", infiles[nf], codepoint);
				}
				else
				{
					memcpy(cpf->data + nc * psf.psf_charlen,
					  psf.psf_data + mc * psf.psf_charlen,
					  psf.psf_charlen);
				}
			}
		}	
		else	/* No mapping, just grab the font as it is */
		{
			memcpy(cpf->data, psf.psf_data, cpf->fontsize);
		}
		psf_file_delete(&psf);
        }
	if (drfont) rv = cpi_crush(&cpi);
	else
	{
		rv = cpi_bloat(&cpi);
		if (nt) strcpy(cpi.format, "FONT.NT");
	}

	if (msgbuf[0])
	{
		cpi.comment = malloc(strlen(msgbuf));
		if (cpi.comment) 
		{
			cpi.comment_len = strlen(msgbuf);
			memcpy(cpi.comment, msgbuf, cpi.comment_len);
		}
		else	rv = CPI_ERR_NOMEM;
	}
	if (!rv) rv = cpi_savefile(&cpi, outfile, inter);
	if (!rv) return NULL;
        return psf_error_string(rv);
}

