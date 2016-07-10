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

/* Merge one or more PSF files into a new PSF file */

#include "cnvmulti.h"
#include "psflib.h"
#include <ctype.h>


static char helpbuf[2048];
static char msgbuf[1000];
static int v1 = 0;
static PSF_FILE *psf_in, psf_out;

/* Program name */
char *cnv_progname = "PSFJOIN";

/* ddash = 1 if option started with a double-dash; else 0 */
/* Return NULL if OK, else error string */
char *cnv_set_option(int ddash, char *variable, char *value)
{
	if (!stricmp(variable, "psf1")) { v1 = 1; return NULL; }
	if (!stricmp(variable, "psf2")) { v1 = 0; return NULL; }
	if (strlen(variable) > 2000) variable[2000] = 0;
	sprintf(helpbuf, "Unknown option: %s\n", variable);
	return helpbuf;
}


/* Return help string */
char *cnv_help(void)
{
    sprintf(helpbuf, "Concatenates .PSF fonts\n\n"
		     "Syntax: %s source.psf source.psf "
		            " ... target.psf \n\n", cnv_progname);
    strcat(helpbuf,  "Options are:\n");
    strcat(helpbuf,  "             --psf1: Output in PSF1 format\n");
    strcat(helpbuf,  "             --psf2: Output in PSF2 format (default)\n");
    return helpbuf;
} 

/* Do the conversion */
char *cnv_multi(int nfiles, char **infiles, FILE *outfile)
{ 
        int rv, nf, nc;
	psf_dword mc, nchars, charlen = 0;
        FILE *fp;
	int w = -1, h = -1;
	psf_unicode_dirent *ude;

/* Load all the source files */
	nchars = 0;
	psf_in = malloc(nfiles * sizeof(PSF_FILE));
	if (!psf_in) return "Out of memory";
        for (nf = 0; nf < nfiles; nf++)
        { 
		psf_file_new(&psf_in[nf]);
                if (!strcmp(infiles[nf], "-")) fp = stdin;
                else  fp = fopen(infiles[nf], "rb");
          
                if (!fp) 
		{ 
			perror(infiles[nf]); 
			return "Cannot open font file."; 
		}
		rv = psf_file_read(&psf_in[nf], fp);
                if (fp != stdin) fclose(fp);
		if (rv != PSF_E_OK) 
		{
			sprintf(msgbuf, "%s: %s", infiles[nf],
					psf_error_string(rv));
			return msgbuf;
		}
		if (w == -1 && h == -1)
		{
			w = psf_in[nf].psf_width;
			h = psf_in[nf].psf_height;
			charlen = psf_in[nf].psf_charlen;
		}
/* Allow a little slackness in the width; we can concatenate say 6x8 and 8x8
 * fonts */
		else if (h != psf_in[nf].psf_height || charlen != psf_in[nf].psf_charlen)
		{
			return "All fonts must have the same dimensions.";
		}
		nchars += psf_in[nf].psf_length;
	}
	rv = psf_file_create(&psf_out, w, h, nchars, 0);
	if (rv) return psf_error_string(rv);
	
	mc = 0;
        for (nf = 0; nf < nfiles; nf++)
        { 
		for (nc = 0; nc < psf_in[nf].psf_length; nc++)
		{
			memcpy(psf_out.psf_data + mc * psf_out.psf_charlen,
			       psf_in[nf].psf_data + nc * psf_in[nf].psf_charlen,
			       psf_out.psf_charlen);
			mc++;
		}
	}
	/* Copy the Unicode directory from the first font only */
	rv = 0;
	if (psf_is_unicode(&psf_in[0]))
	{
		rv = psf_file_create_unicode(&psf_out);
	}
	if (!rv) for (nc = 0; nc < psf_in[0].psf_length; nc++)
	{
		for (ude = psf_in[0].psf_dirents_used[nc]; ude != NULL; ude = ude->psfu_next)
		{
			rv = psf_unicode_add(&psf_out, nf, ude->psfu_token);
			if (rv) break;
		}
		if (rv) break;
	}
	if (!rv) rv = psf_file_write(&psf_out, outfile);
	psf_file_delete(&psf_out);
        for (nf = 0; nf < nfiles; nf++)
        { 
		psf_file_delete(&psf_in[nf]);
	}
	free(psf_in);
	if (rv) return psf_error_string(rv);
	return NULL;
}

