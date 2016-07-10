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
#include "zxflib.h"

/* Convert a PSF to a Spectrum font. Optionally re-map the characters
 * from ISO Latin-1 */

#define MD_BARE   0	/* Just save chars 32-127 as Spectrum font */
#define MD_MERGE1 1	/* Get characters 94, 96 and 127 from their
                         * Latin-1 counterparts */

static int mode     = -1;
static ZXF_FORMAT zxf_format = ZXF_P3DOS;
static char *auxfile = NULL;

static char helpbuf[2048];

/* Program name */
char *cnv_progname = "PSF2ZX";

/* ddash = 1 if option started with a double-dash; else 0 */
/* Return NULL if OK, else error string */
char *cnv_set_option(int ddash, char *variable, char *value)
{
	if (!stricmp(variable, "mode")) 
        {
		if      (!stricmp(value, "raw"  )) mode = MD_BARE;
		else if (!stricmp(value, "merge1")) mode = MD_MERGE1;
		else return "Invalid value for --mode option. Use the --help option for the help screen.";
		return NULL;
	}
	if (!stricmp(variable, "p3dos")) { zxf_format=ZXF_P3DOS; return NULL; }
	if (!stricmp(variable, "tap"))   { zxf_format=ZXF_TAP; return NULL; }
	if (!stricmp(variable, "naked")) { zxf_format=ZXF_NAKED; return NULL; }
	if (!stricmp(variable, "sna"))   
	{ 
		auxfile = malloc(1 + strlen(value));
		if (!auxfile) return "Out of memory";
		strcpy(auxfile, value);
		zxf_format = ZXF_SNA;
	 	return NULL; 
	}
	if (!stricmp(variable, "rom"))   
	{ 
		auxfile = malloc(1 + strlen(value));
		if (!auxfile) return "Out of memory";
		strcpy(auxfile, value);
		zxf_format = ZXF_ROM;
	 	return NULL; 
	}
	if (strlen(variable) > 2000) variable[2000] = 0;
	sprintf(helpbuf, "Unknown option: %s\n", variable);
	return helpbuf;
}


/* Return help string */
char *cnv_help(void)
{
	sprintf(helpbuf, "Syntax: %s psffile zxfont { options }\n\n", 
			cnv_progname);
	strcat (helpbuf, "Options: \n"
		    "        --p3dos          - Output in +3DOS format (default).\n"
		    "        --tap            - Output in TAP format.\n"
		    "        --rom=romfile    - Output a ROM based on romfile.\n"
		    "        --sna=snafile    - Output a snapshot based on snafile.\n"
		    "        --naked          - Output just the font bytes.\n"
                    "        --mode=raw       - Don't translate pound sign,\n"
		    "                           arrow and copyright symbol.\n"
                    "        --mode=merge1    - Translate pound sign, arrow \n"
		    "                           and copyright symbol.\n"
		    "(default is merge1 if the PSF is Unicode, raw if not)\n");
	return helpbuf;
}


/* Plain copy of character bitmap PSF->ZX */
static void copychar(PSF_FILE *src, ZXF_FILE *dst, int from, int to)
{
	psf_dword idx;
	psf_byte *sptr;
	zxf_byte *dptr;
	static psf_byte nil8[8];

/* If the PSF has a Unicode directory, look up the tokens in it */
	if (psf_is_unicode(src))
	{
		sptr = nil8;
		if (!psf_unicode_lookup(src, from, &idx))
			sptr = src->psf_data + 8 *  idx;
/* I don't expect many fonts to have the uparrow. So if that can't be found, 
 * fall back on 0x5E */
		else if (from == 0x2191 && !psf_unicode_lookup(src, 0x5E, &idx))
			sptr = src->psf_data + 8 *  idx;
	}
	else
	{
		/* 0x18 is the most likely place to find an uparrow in a
		 * non-unicode PSF. */
		if (from == 0x2191) from = 0x18;
		sptr = src->psf_data + 8 *  from;
	}
	dptr = dst->zxf_data + 8 * (to - 32);
    
	memcpy(dptr, sptr, 8);
}

/* Do the conversion */
char *cnv_execute(FILE *infile, FILE *outfile)
{	
	int rv, x;
	PSF_FILE psf;
	ZXF_FILE zxf;

	psf_file_new(&psf);
	rv = psf_file_read(&psf, infile);	
	if (rv != PSF_E_OK) return psf_error_string(rv);

	if (psf.psf_height != 8 || psf.psf_width != 8)
	{
		psf_file_delete(&psf);
		return "PSF characters must be 8x8 for a Spectrum font.";
	}
	if (mode == -1)
	{
		if (psf_is_unicode(&psf)) mode = MD_MERGE1;
		else			  mode = MD_BARE;
	}

	zxf_file_new(&zxf);
	if (rv == PSF_E_OK) rv = zxf_file_create(&zxf);	
	if (rv == PSF_E_OK)
	{
            if (mode == MD_BARE)
		 for (x = 32; x <= 127; x++) copychar(&psf, &zxf, x, x);
            else for (x = 32; x <= 127; x++) switch(x)
                {
                case 0x5E: copychar(&psf, &zxf, 0x2191, x); break; 
                case 0x60: copychar(&psf, &zxf, 0xA3, x); break;
                case 0x7F: copychar(&psf, &zxf, 0xA9, x); break;
                default:   copychar(&psf, &zxf, x, x);    break;
                }
	    rv = zxf_file_write(&zxf, outfile, zxf_format, auxfile);
	}
	psf_file_delete(&psf);
	
	if (rv != ZXF_E_OK)
	{
		zxf_file_delete(&zxf);
		return zxf_error_string(rv);
	}
	zxf_file_delete(&zxf);
	if (auxfile) free(auxfile);
	return NULL;
}

