/*
    psftools: Manipulate console fonts in the .PSF format
    Copyright (C) 2003, 2005  John Elliott

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

/* Convert a PSF font to a raw font (as used by DOS font editors etc.) 
 */

static char helpbuf[2048];
static int first  = -1;
static int last   = -1;
static int doflip = 0;
static PSF_MAPPING *codepage = NULL;
static PSF_FILE psf;

/* Program name */
char *cnv_progname = "PSF2RAW";

/* ddash = 1 if option started with a double-dash; else 0 */
/* Return NULL if OK, else error string */
char *cnv_set_option(int ddash, char *variable, char *value)
{
	if (!stricmp(variable, "first"))  { first = atoi(value); return NULL; }
	if (!stricmp(variable, "last"))   { last = atoi(value); return NULL; }
	if (!stricmp(variable, "256"))    { first = 0; last = 255; return NULL; }
	if (!stricmp(variable, "flip"))   { doflip = 1; return NULL; }
	if (!stricmp(variable, "codepage"))  
	{
		codepage = psf_find_mapping(value);
		if (codepage == NULL) return "Code page name not recognised.";
			return NULL;
	}
	if (strlen(variable) > 2000) variable[2000] = 0;
	sprintf(helpbuf, "Unknown option: %s\n", variable);
	return helpbuf;
}


/* Return help string */
char *cnv_help(void)
    {
    sprintf(helpbuf, "Syntax: %s psffile rawfnt { options }\n\n", cnv_progname);
    strcat (helpbuf, "Options: \n"
		     "    --first=n      Start with character n\n"
                     "    --last=n       Finish with character n\n"
		     "    --codepage=xxx Extract codepage xxx\n"
                     "    --flip         Mirror all bytes as they are written\n"
                     "    --256          Equivalent to --first=0 --last=255\n");
            
    return helpbuf;
    }


char *cnv_execute(FILE *infile, FILE *outfile)
{	
	int rv, n, max;
	psf_dword ch, f, l, z, glyph;

	psf_file_new(&psf);
        rv = psf_file_read(&psf, infile);

	if (rv != PSF_E_OK) return psf_error_string(rv);
	if (codepage && !psf_is_unicode(&psf))
	{
		psf_file_delete(&psf);
		return "Cannot extract by codepage; source file has no Unicode table";
	}

	/* Most systems that use raw fonts depend on the font being 1 byte 
	 * wide */
	if (psf.psf_width > 8) 
		fprintf(stderr, "Warning: Font is wider than 8 bits.\n");

	f = (first >= 0) ? first : 0;
	l = (last  >= 0) ? last  : (psf.psf_length - 1);
	if (codepage && l >= 256) l = 255;
	if (l >= psf.psf_length) l = psf.psf_length;

	if (doflip)
	{
		max = (last+1) * psf.psf_charlen;
		for (n = first * psf.psf_charlen; n < max; n++)
		{
			psf.psf_data[n] = flip(psf.psf_data[n]);
		}
	}
	for (ch = f; ch <= l; ch++)
	{
		psf_byte *src;

		if (codepage)
		{
			if (ch < 256 && !psf_unicode_lookupmap(&psf, codepage, 
						ch, &glyph, NULL))
			{
				src = psf.psf_data + glyph * psf.psf_charlen;
			}
			else
			{
				if (ch < 256 && !psf_unicode_banned(codepage->psfm_tokens[ch][0])) fprintf(stderr, "Warning: U+%04lx not found in font\n", codepage->psfm_tokens[ch][0]);
				src = NULL;
			}
		}
		else	src = psf.psf_data + ch * psf.psf_charlen;
		
		if (!src) for (z = 0; z < psf.psf_charlen; z++)
		{
			if (fputc(0, outfile) == EOF)
			{
				psf_file_delete(&psf);
				return "Could not write to output.";
			}	
		}	
		else if (fwrite(src, 1, psf.psf_charlen, outfile) < 
				psf.psf_charlen)
		{
			psf_file_delete(&psf);
			return "Could not write to output.";
		}
	}	
	psf_file_delete(&psf);	
	return NULL;
}

