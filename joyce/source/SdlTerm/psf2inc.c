/*
    psftools: Manipulate console fonts in the .PSF format
    Copyright (C) 2000  John Elliott

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

/* Convert a PSF file to a C include file. Used while building zx2psf. */

#include "cnvshell.h"
#include "psflib.h"

char *cnv_progname = "PSF2INC";


char *cnv_set_option(int ddash, char *variable, char *value)
    {
    return "This program does not have options.";
    }

char *cnv_help(void)
    {
    static char buf[150];

    sprintf(buf, "Syntax: %s psf_file inc_file\n", cnv_progname);
    return buf;
    }


char *cnv_execute(FILE *infile, FILE *outfile)
{	
	int rv;
	PSF_FILE psf;
	int x,y;
	int count, max;

	psf_file_new(&psf);
	rv = psf_file_read(&psf, infile);	

	if (rv != PSF_E_OK) return psf_error_string(rv);
	fprintf(outfile, "    0x%02x, 0x%02x,   /* Magic  */\n", psf.psf_magic & 0xFF,
					 (psf.psf_magic >> 8) & 0xFF);
	fprintf(outfile, "    0x%02x,         /* Type   */\n",   psf.psf_type);
	fprintf(outfile, "    0x%02x,         /* Height */\n", psf.psf_height);

	max = psf_count_chars(&psf);
	count = 0;
	for (y = 0; y < max; y++)
	{
		fprintf(outfile, "    ");	
		for (x = 0; x < psf.psf_height; x++)
		{
			fprintf(outfile, "0x%02x, ", psf.psf_data[count]);
			++count;
		}
		fprintf (outfile, "  /* %03d */\n", y); 
	}
/* If there's a Unicode directory, dump that out */
	if (psf_is_unicode(&psf))
	{
		fprintf(outfile, "   /* Unicode directory: */\n");
		for (y = 0; y < max; y++)
		{
                        psf_unicode_dirent *e = psf.psf_dirents_used[y];
			fprintf(outfile, "    ");
			while (e)
			{
				fprintf(outfile, "0x%02x, 0x%02x, ",
					(e->psfu_token & 0xFF),
					((e->psfu_token >> 8) & 0xFF));
	
				e = e->psfu_next;
			}
			fprintf(outfile, "0xFF, 0xFF,  /* %03d */\n", y);
		}	

	}

	psf_file_delete(&psf);
	return NULL;
}

