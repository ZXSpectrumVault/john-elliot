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
#include <stdio.h>
#include "cnvshell.h"
#include "psflib.h"

/* Convert a PSF file to a text format */

char *cnv_progname = "PSF2TXT";


char *cnv_set_option(int ddash, char *variable, char *value)
    {
    return "This program does not have options.";
    }

char *cnv_help(void)
    {
    static char buf[150];

    sprintf(buf, "Syntax: %s psf_file txt_file\n", cnv_progname);
    return buf;
    }



char *cnv_execute(FILE *fpin, FILE *fpout)
{	
	int rv;
	PSF_FILE psf;
	psf_dword ch;
	psf_byte pix;
	psf_dword x, y;
	char *ucs;

	psf_file_new(&psf);
	rv = psf_file_read(&psf, fpin);	
	if (rv != PSF_E_OK) return psf_error_string(rv);

	fprintf(fpout, "%%PSF2\n");
	fprintf(fpout, "Version: %ld\n", psf.psf_version);
	fprintf(fpout, "Flags: %ld\n", psf.psf_flags);
	fprintf(fpout, "Length: %ld\n", psf.psf_length);
	fprintf(fpout, "Width: %ld\n", psf.psf_width);
	fprintf(fpout, "Height: %ld\n", psf.psf_height);
	rv = 0;
	for (ch = 0; ch < psf.psf_length; ch++)
	{
		fprintf(fpout, "%%\n// Character %ld\n", ch);
		for (y = 0; y < psf.psf_height; y++)
		{
			if (y == 0) fprintf(fpout, "Bitmap: ");
			else	    fprintf(fpout, "        ");

			for (x = 0; x < psf.psf_width; x++)
			{
				rv = psf_get_pixel(&psf, ch, x, y, &pix);
				if (rv) break;
				if (pix) fputc('#', fpout);
				else	 fputc('-', fpout);
			}
			if (rv) break;
			if (y < psf.psf_height - 1) fprintf(fpout, " \\");
			fputc('\n', fpout);
		}
		if (rv) break;
		if (psf_is_unicode(&psf))
		{
			rv = psf_unicode_to_string(psf.psf_dirents_used[ch], 
					&ucs);
			if (!rv)
			{
				fprintf(fpout, "Unicode: %s\n", ucs);
				free(ucs);
			}

		}
		if (rv) break;
	}

	psf_file_delete(&psf);
	return 0;
}

