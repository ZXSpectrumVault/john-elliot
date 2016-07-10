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
#include "cnvshell.h"
#include "psflib.h"

/* Convert a PSF font to a Hercules WriteOn font.
 *
 */

static char helpbuf[2048];
static int first  = -1;
static int last   = -1;

static PSF_FILE psf;
static PSF_MAPPING *codepage = NULL;

/* Program name */
char *cnv_progname = "PSF2WOF";

/* ddash = 1 if option started with a double-dash; else 0 */
/* Return NULL if OK, else error string */
char *cnv_set_option(int ddash, char *variable, char *value)
{
	if (!stricmp(variable, "first")) { first = atoi(value); return NULL; }
	if (!stricmp(variable, "last"))  { last = atoi(value); return NULL; }
	if (!stricmp(variable, "256"))   { first = 0; last = 255; return NULL; }
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
    sprintf(helpbuf, "Converts a PSF font to a Hercules WriteOn font.\n"
		     "Syntax: %s psffile woffile { options }\n\n", 
		     cnv_progname);
    strcat (helpbuf, "Options: \n"
		     "    --first=n Start with character n\n"
                     "    --last=n  Finish with character n\n"
                     "    --256     Equivalent to --first=0 --last=255\n");
            
    return helpbuf;
    }


unsigned char wof_head[] =
{
	0x45, 0x53, 0x01, 0x00,	/* magic */
	0x16, 0x00,		/* sizeof(wof_head) */
	0x00, 0x00,		/* width */	
	0x00, 0x00,		/* height */	
	0x0E, 0x00,		/* 14 in all WOF files */	
	0x00, 0x00,		/* first char */
	0x00, 0x00,		/* last char */
	0x00, 0x00,		/* Always 0? */
	0x00, 0x00,		/* width again? */	
	0x00, 0x00,		/* Always 0? */
};

char *cnv_execute(FILE *infile, FILE *outfile)
{	
	int rv;
	psf_dword ch, f, l, glyph, z;

	psf_file_new(&psf);
        rv = psf_file_read(&psf, infile);

	if (rv != PSF_E_OK) return psf_error_string(rv);
	if (codepage && !psf_is_unicode(&psf))
	{
		psf_file_delete(&psf);
		return "Cannot extract by codepage; source file has no Unicode table";
	}

	f = (first >= 0) ? first : 0;
	l = (last  >= 0) ? last  : (psf.psf_length - 1);
	if (codepage && l >= 256) l = 255;
	if (l >= psf.psf_length) l = psf.psf_length;

	/* Create .WOF header */
	wof_head[6] = psf.psf_width & 0xFF;
	wof_head[7] = psf.psf_width >> 8;
	wof_head[8] = psf.psf_height & 0xFF;
	wof_head[9] = psf.psf_height >> 8;
	wof_head[12] = f & 0xFF;
	wof_head[13] = f >> 8;
	wof_head[14] = l & 0xFF;
	wof_head[15] = l >> 8;
	wof_head[18] = psf.psf_width & 0xFF;
	wof_head[19] = psf.psf_width >> 8;

	if (fwrite(wof_head, 1, sizeof(wof_head), outfile) < (int)sizeof(wof_head))
	{
		psf_file_delete(&psf);
		return "Could not write to output.";
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

