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

/* Convert a PSF to an X11 Bitmap Distribution Format font.
 *
 */

static char helpbuf[2048];
static int first  = -1;
static int last   = -1;
static char fontname[256] = { 'p', 's', 'f', 0 };
static int descent = -1;
static int def_char = 255;
static int iso10646 = 0;

static PSF_FILE psf;
static PSF_MAPPING *codepage;

/* Program name */
char *cnv_progname = "PSF2BDF";

/* ddash = 1 if option started with a double-dash; else 0 */
/* Return NULL if OK, else error string */
char *cnv_set_option(int ddash, char *variable, char *value)
{
	if (!stricmp(variable, "first"))  { first = atoi(value); return NULL; }
	if (!stricmp(variable, "last"))   { last = atoi(value); return NULL; }
	if (!stricmp(variable, "descent")) { descent = atoi(value); return NULL; }
	if (!stricmp(variable, "defchar")) { def_char = atoi(value); return NULL; }
	if (!stricmp(variable, "fontname")) { strncpy(fontname, value, 255); return NULL; }
	if (!stricmp(variable, "256"))	{ first = 0; last = 255; return NULL; }
	if (!stricmp(variable, "iso10646")) { iso10646 = 1; return NULL; }
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
    sprintf(helpbuf, "Syntax: %s psffile bdffile { options }\n\n", cnv_progname);
    strcat (helpbuf, "Options: \n"
		     "    --first=n       Start with character n\n"
                     "    --last=n        Finish with character n\n"
		     "    --fontname=str  Font name to use\n"
		     "    --defchar=n     Set default character\n"
		     "    --descent=n     Set descent (distance between baseline of \n"
		     "                    characters and bottom of font)\n"
                     "    --codepage=xxx  Extract characters in the specified codepage\n"
                     "    --256           Equivalent to --first=0 --last=255\n"
                     "    --iso10646      Use ISO10646 encoding\n");
            
    return helpbuf;
    }


char *cnv_execute(FILE *infile, FILE *outfile)
{	
	int rv, y, x, wb;
	psf_dword ch, f, l, glyph, codept;
	unsigned char *d;

	psf_file_new(&psf);
	rv = psf_file_read(&psf, infile);

	if (rv != PSF_E_OK) return psf_error_string(rv);
	if (codepage && !psf_is_unicode(&psf))
	{
		psf_file_delete(&psf);
		return "Cannot extract by codepage; source file has no Unicode table";
	}

	if (descent == -1) descent = psf.psf_height / 4;

	fprintf(outfile, "STARTFONT 2.1\n");
	fprintf(outfile, "FONT %s\n", fontname);
	fprintf(outfile, "SIZE %ld 75 75\n", psf.psf_height);
	fprintf(outfile, "FONTBOUNDINGBOX %ld %ld 0 %d\n", psf.psf_width, 
					psf.psf_height, -descent);
	fprintf(outfile, "STARTPROPERTIES 3\n");
	fprintf(outfile, "FONT_DESCENT %d\n", descent);
	fprintf(outfile, "FONT_ASCENT %ld\n", psf.psf_height - descent);
	fprintf(outfile, "DEFAULT_CHAR %d\n", def_char);
	fprintf(outfile, "ENDPROPERTIES\n");

	f = (first >= 0) ? first : 0;
	l = (last  >= 0) ? last  : (psf.psf_length - 1);
	if (codepage && l >= 256) l = 255;
	if (l >= psf.psf_length) l = psf.psf_length - 1;
	fprintf(outfile, "CHARS %ld\n", l + 1 - f);

	/* Emit header */
	wb = (psf.psf_width + 7) / 8;
	for (ch = f; ch <= l; ch++)
	{
		if (codepage)
		{
			if (ch < 256 && !psf_unicode_lookupmap(&psf,
					codepage, ch, &glyph, &codept))
			{
				d = psf.psf_data + glyph * psf.psf_charlen;
			}
			else
			{
				if (ch < 256 && !psf_unicode_banned(codepage->psfm_tokens[ch][0])) fprintf(stderr, "Warning: U+%04lx not found in font\n", codepage->psfm_tokens[ch][0]);
				codept = 0xFFFD;
				d = NULL;
			}
		}
		else	
		{
			d = psf.psf_data + ch * psf.psf_charlen;
			if (psf_is_unicode(&psf))
			{
				if (psf.psf_dirents_used[ch] &&
				    psf.psf_dirents_used[ch]->psfu_token != 0xFFFE)
				{
					codept = psf.psf_dirents_used[ch]->psfu_token;
				}
				else codept = 0xFFFD;
			}
			else codept = ch;
		}
		fprintf(outfile, "STARTCHAR C%04lx\n", ch);
		fprintf(outfile, "ENCODING %ld\n", iso10646 ? codept : ch);
		fprintf(outfile, "SWIDTH 666 0\n");
		fprintf(outfile, "DWIDTH %ld 0\n", psf.psf_width);
		fprintf(outfile, "BBX %ld %ld 0 %d\n", psf.psf_width, psf.psf_height, 
					-descent);
		fprintf(outfile, "BITMAP\n");
	
		for (y = 0; y < psf.psf_height; y++)
		{
			for (x = 0; x < wb; x++)
			{
				fprintf(outfile, "%02x", d ? *d : 0); 
				if (d) d++;	
			}
			fprintf(outfile, "\n");
		}
		fprintf(outfile, "ENDCHAR\n");
	}	
	fprintf(outfile, "ENDFONT\n");
	psf_file_delete(&psf);	
	return NULL;
}

