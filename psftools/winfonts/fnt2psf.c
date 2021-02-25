/*
    psftools: Manipulate console fonts in the .PSF format
    Copyright (C) 2000, 2005  John Elliott

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
#include "mswfnt.h"

#ifndef SEEK_SET
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2
#endif

/* Convert a Windows .FNT font to PSF
 * Note that this is not the same as the .FON fonts distributed with 
 * MS-Windows. You have to extract the .FNT fonts from the .FON using 
 * a resource editor. */

static char helpbuf[2048];

static PSF_FILE psf;
static PSF_MAPPING *codepage = NULL;
static MSW_FONTINFO msw;
static int v1 = 0, v2 = 0;
/* Program name */
char *cnv_progname = "FNT2PSF";

/* ddash = 1 if option started with a double-dash; else 0 */
/* Return NULL if OK, else error string */
char *cnv_set_option(int ddash, char *variable, char *value)
{
	if (!stricmp(variable, "psf1"))   { v1 = 1; return NULL; }
	if (!stricmp(variable, "psf2"))   { v2 = 1; return NULL; }
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
	sprintf(helpbuf, "Syntax: %s winfnt psffile { options }\n\n", 
			cnv_progname);
	strcat (helpbuf, "Options: \n"
	                 "    --psf1 - Output in PSF1 format\n"
	                 "    --psf2 - Output in PSF2 format (default)\n"
	                 "    --codepage=x   Create a Unicode directory "
			 "from the specified code page\n");
	return helpbuf;
}


static msw_word offset_table[768];
static psf_dword stdpos;

static int maybe_fseek(FILE *fp, long pos)
{
	if (fp != stdin) return fseek(fp, pos, SEEK_SET);

	while (pos > stdpos) { getchar(); ++stdpos; }
	return 0;
}

/* Convert Windows 1.x font */
char *msw_conv_1(FILE *fp)
{
	int y, x, vx, x1, ch, chx;
	psf_byte b, mask;
	
	/* v1 file only has a lengths table for proportional fonts, and all 
         * the characters are stored side-by-side in a bitmap. */
	if (msw.dfPixWidth == 0) 
	{
		for (ch = msw.dfFirstChar; ch <= msw.dfLastChar+1; ch++)
		{
			if (read_word(fp, &offset_table[ch])) 
			{
				return "Unexpected end of file";
			}
		}
	}
	else
	{
		vx = 0;
		for (ch = msw.dfFirstChar; ch <= msw.dfLastChar+1; ch++)
		{
			offset_table[ch] = vx;
			vx += msw.dfPixWidth;
		}
	}
	/* Seek to font bitmap */
	if (maybe_fseek(fp, msw.dfBitsOffset)) 
		return "Unexpected end of file";	
	for (y = 0; y < msw.dfPixHeight; y++)
	{
		for (x = 0; x < msw.dfWidthBytes; x++)
		{
			if (read_byte(fp, &b)) return "Unexpected end of file";
			vx = x * 8;
			mask = 0x80;
			for (x1 = 0; x1 < 8; x1++,vx++)
			{
				for (ch = msw.dfFirstChar; ch <= msw.dfLastChar; ch++) 
					if (offset_table[ch] <= vx &&
					    offset_table[ch+1] > vx) break;

				chx = vx - offset_table[ch];

				psf_set_pixel(&psf, ch, chx, y, b & mask);	
				mask = mask >> 1;
			}
		}
	}
	return NULL;
}

/* Convert Windows 2.x font */

char *msw_conv_2(FILE *fp)
{
	int ch, y, w, ww, x;
	psf_byte *b;
	int c;

	w = (psf.psf_width + 7) / 8;
	stdpos = 0x76;
	for (ch = msw.dfFirstChar; ch <= msw.dfLastChar; ch++)
	{
		if (read_word(fp, &offset_table[ch*2])   ||
		    read_word(fp, &offset_table[ch*2+1])) 
			return "Unexpected end of file";
		stdpos += 4;
	}
	for (ch = msw.dfFirstChar; ch <= msw.dfLastChar; ch++)
	{
		ww = (offset_table[ch*2] + 7) / 8;
		maybe_fseek(fp, offset_table[ch*2+1]);
		b = psf.psf_data + ch * psf.psf_charlen;
		for (x = 0; x < ww; x++)
		{
			for (y = 0; y < msw.dfPixHeight; y++)
			{
				c = fgetc(fp);	
				++stdpos;
				if (c == EOF) return "Unexpected end of file";
				b[y * w + x] = c;
			}
		}
	}
	return NULL;
}

/* Convert Windows 3.x font */

char *msw_conv_3(FILE *fp)
{
	int ch, y, w, ww, x;
	psf_byte *b;
	int c;
	long offset;

	stdpos = 0x9A;
	w = (psf.psf_width + 7) / 8;
        for (ch = msw.dfFirstChar; ch <= msw.dfLastChar; ch++)
        {
                if (read_word(fp, &offset_table[ch*3])   ||
                    read_word(fp, &offset_table[ch*3+1]) ||
		    read_word(fp, &offset_table[ch*3+2]))
                        return "Unexpected end of file";
		stdpos += 6;
        }
        for (ch = msw.dfFirstChar; ch <= msw.dfLastChar; ch++)
        {
		ww = (offset_table[ch*3] + 7) / 8;
		offset = offset_table[ch*3+2];
		offset = (offset << 16) | offset_table[ch*3+1];
                maybe_fseek(fp, offset);
		b = psf.psf_data + ch * psf.psf_charlen;
		for (x = 0; x < ww; x++)
		{
			for (y = 0; y < msw.dfPixHeight; y++)
			{
				c = fgetc(fp);	
				++stdpos;
				if (c == EOF) return "Unexpected end of file";
				b[y * w + x] = c;
			}
		}
        }
        return NULL;
}




/* Do the conversion */
char *cnv_execute(FILE *infile, FILE *outfile)
{	
	int rv;
	char *rvs;

	psf_file_new(&psf);

	rv = msw_fontinfo_read(&msw, infile);

	if (rv == -1) return "Read error on input file";
	if (rv == -2) return ".FNT file is not in a recognised Windows format";

	if (msw.dfType & 0x7F) return ".FNT file is not a bitmap font";
	if (msw.dfPixWidth == 0) fprintf(stderr, "Warning: Variable-width!\n");

	rv = psf_file_create(&psf, msw.dfMaxWidth, 
                                    msw.dfPixHeight, 
		 		    msw.dfLastChar + 1, 0);
	if (rv) return psf_error_string(rv);
	
	rvs = "Unsupported Windows font format";
	switch(msw.dfVersion)
	{
		case 0x100: rvs = msw_conv_1(infile); break;
		case 0x200: rvs = msw_conv_2(infile); break;
		case 0x300: rvs = msw_conv_3(infile); break;
	}
	if (!codepage)
	{
		if (msw.dfCharSet == 0xFF)   codepage = psf_find_mapping("CP437");
		else if (msw.dfCharSet == 0) codepage = psf_find_mapping("CP1252");
	}
        if (codepage)
        {
		int n;
                psf_file_create_unicode(&psf);
                for (n = msw.dfFirstChar; n < msw.dfLastChar; n++) if (n < 256)
		{
                	psf_unicode_addmap(&psf, n, codepage, n);
		}
	}

        if (v1) psf_force_v1(&psf);
        if (v2) psf_force_v2(&psf);

	if (!rvs) rv = psf_file_write(&psf, outfile);
	psf_file_delete(&psf);	
	if (rvs) return rvs;
	if (rv) return psf_error_string(rv);
	return NULL;
}

