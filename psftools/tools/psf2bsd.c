/*
    psftools: Manipulate console fonts in the .PSF format
    Copyright (C) 2000, 2005, 2007  John Elliott

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

/* Convert a PSF file to a C include file in the format used by the 
 * NetBSD kernel. */

#include "cnvshell.h"
#include "psflib.h"

char *cnv_progname = "PSF2BSD";

static char helpbuf[2000];
static char *cpbuf;
static char *fnbuf;
static char *tnbuf;
static char *fontname = NULL;
static char *typename = NULL;
static char *codepage = NULL;
static int first = -1;
static int last = -1;

char *cnv_set_option(int ddash, char *variable, char *value)
{
	if (!strcmp(variable, "name"))		
	{
		fnbuf = malloc(1 + strlen(value));
		if (!fnbuf) return "Out of memory";
		strcpy(fnbuf, value);
		fontname = fnbuf;
		return NULL;
	}
	if (!strcmp(variable, "typeface"))		
	{
		tnbuf = malloc(1 + strlen(value));
		if (!tnbuf) return "Out of memory";
		strcpy(tnbuf, value);
		typename = tnbuf;
		return NULL;
	}
	if (!strcmp(variable, "encoding")) 
	{
		if (!strcmp(value, "437"))   
			codepage = "WSDISPLAY_FONTENC_IBM";
		else if (!strcmp(value, "819"))   
			codepage = "WSDISPLAY_FONTENC_ISO";
		else if (!strcmp(value, "28591")) 
			codepage = "WSDISPLAY_FONTENC_ISO";
		else
		{
			cpbuf = malloc(1 + strlen(value));
			if (!cpbuf) return "Out of memory";
			strcpy(cpbuf, value);
			codepage = cpbuf;
		}
		return NULL;
	}
	if (!strcmp(variable, "first"))
	{
		first = atoi(value);
		return NULL;
	}
	if (!strcmp(variable, "last"))
	{
		last = atoi(value);
		return NULL;
	}
	if (strlen(variable) > 2000) variable[2000] = 0;
	if (strlen(variable) > 2000) variable[2000] = 0;
	sprintf(helpbuf, "Unknown option: %s\n", variable);
	return helpbuf;
}

/* Return help string */
char *cnv_help(void)
{
	sprintf(helpbuf, "Syntax: %s psffile incfile { options }\n\n", 
			cnv_progname);
	strcat (helpbuf, "Options: \n"
                     "    --name=name     Set structure name (default: 'font')\n"
                     "    --typeface=name  Set typeface name (default: match structure name)\n"
                     "    --encoding=437  Select IBM encoding\n"
                     "    --encoding=819  Select ISO-8859-1 encoding\n"
		     "    --encoding=...  Set literal encoding name\n" 
		     "    --first=nnn     Set first character to include\n"
		     "    --last=nnn      Set last character to include\n"
		     );

	return helpbuf;
}

void dumpbyte(FILE *fp, psf_byte value)
{
	int mask;

	for (mask = 0x80; mask != 0; mask = mask >> 1)
	{
		fprintf(fp, "%c", (value & mask) ? '#' : '-');
	}
}

char *cnv_execute(FILE *infile, FILE *outfile)
{	
	int rv, stride, nstr;
	psf_dword nchar, nrow;
	PSF_FILE psf;

	if (fontname == NULL)
	{
		fontname = "font";
	}
	if (codepage == NULL)
	{
		codepage = "WSDISPLAY_FONTENC_ISO";
	}
	if (typename == NULL)
	{
		typename = fontname;
	}
	psf_file_new(&psf);
	rv = psf_file_read(&psf, infile);	

	if (rv != PSF_E_OK) return psf_error_string(rv);
	if (first == -1 || first >= psf.psf_length)
	{
		first = 0;
	}
	if (last == -1 || last >= psf.psf_length)
	{
		last = psf.psf_length - 1;
	}
	if (last < first)
	{
		last = first;
	}

	stride = (psf.psf_width + 7) / 8;
	fprintf(outfile, "static u_char %s_data[];\n\n", fontname);
	fprintf(outfile, "static struct wsdisplay_font %s = {\n", fontname);
	fprintf(outfile, "\t\"%s\",\t\t\t\t/* typeface name */\n", typename);
	fprintf(outfile, "\t%d,\t\t\t\t/* firstchar */\n", first);
	fprintf(outfile, "\t%d,\t\t\t\t/* numchars */\n", last - first + 1);
	fprintf(outfile, "\t%s,\t\t/* encoding */\n", codepage);
	fprintf(outfile, "\t%ld,\t\t\t\t/* width */\n", psf.psf_width);
	fprintf(outfile, "\t%ld,\t\t\t\t/* height */\n", psf.psf_height);
	fprintf(outfile, "\t%d,\t\t\t\t/* stride */\n", stride);
	fprintf(outfile, "\tWSDISPLAY_FONTORDER_L2R,\t/* bit order */\n");
	fprintf(outfile, "\tWSDISPLAY_FONTORDER_L2R,\t/* byte order */\n");
	fprintf(outfile, "\t%s_data\t\t\t/* data */\n", fontname);
	fprintf(outfile, "};\n\n");

	fprintf(outfile, "static u_char %s_data[] = {\n", fontname);
	for (nchar = first; nchar <= last; nchar++)
	{
		if (stride > 1)	fprintf(outfile, "\t");
		fprintf(outfile, "\t\t/* 0x%02lx  ", nchar);
		if (nchar >= 0x20 && nchar < 0x7F) 
			fprintf(outfile, "('%c')", (int)nchar);
		else	fprintf(outfile, "     ");
		fprintf(outfile, " */\n");
		for (nrow = 0; nrow < psf.psf_height; nrow++)
		{
			psf_byte *ptr = psf.psf_data + nchar * psf.psf_charlen;
			
			ptr += nrow * stride;
			fprintf(outfile, "\t");
			for (nstr = 0; nstr < stride; nstr++)
			{
				fprintf(outfile, "0x%02x, ", ptr[nstr]);
			}
			fprintf(outfile, "\t/* ");
			for (nstr = 0; nstr < stride; nstr++)
			{
				dumpbyte(outfile, ptr[nstr]);
			}
			fprintf(outfile, " */\n");
		}
		fprintf(outfile, "\n");
	}
	fprintf(outfile, "};\n");
	psf_file_delete(&psf);
	return NULL;
}

