/*
    psftools: Manipulate console fonts in the .PSF format
    Copyright (C) 2005, 2008  John Elliott

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

/* Merge three PSF files into a character ROM */

#include "cnvmulti.h"
#include "psflib.h"


static char helpbuf[2048];
static char msgbuf[1025];
static int compaq;
static unsigned char rom[8192];

/* Program name */
char *cnv_progname = "PSFS2MDA";

/* ddash = 1 if option started with a double-dash; else 0 */
/* Return NULL if OK, else error string */
char *cnv_set_option(int ddash, char *variable, char *value)
{
	if (!stricmp(variable, "compaq")) 
	{
		compaq = 1;
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
    sprintf(helpbuf, "Builds a CGA/MDA/Compaq CGA character ROM from three "
		    "fonts\n\n"
		     "Syntax: %s source14.psf source8.psf { source8a.psf } "
		     	"target.rom\n\n", cnv_progname);
    strcat(helpbuf,  "Options are: --compaq   Output in Compaq CGA format\n");
    strcat(helpbuf,  "             --comment=<comment>\n");
    return helpbuf;
    }

void transform(PSF_FILE *psf, int y0, int dest, int step, int firstc)
{
	int n, m, y;
	psf_dword offset;
	psf_dword bytes_line = (psf->psf_width + 7) / 8;

	for (n = 0; n < 256; n++)
	{
		/* Point at character glyph */
		offset = (firstc % psf->psf_length) * psf->psf_charlen;
		offset += bytes_line * y0;
		y = y0;
		/* Copy the left-hand 8x8 chunk */
		for (m = 0; m < 8; m++)
		{
			if (y >= psf->psf_height)
				rom[dest + m] = 0;
			else	rom[dest + m] = psf->psf_data[offset];
			offset += bytes_line;
			y++;
		}
		dest += step;
		++firstc;
	}
}

/* Do the conversion */
char *cnv_multi(int nfiles, char **infiles, FILE *outfile)
{        
        int rv, nf, nc;
	PSF_FILE psf[3];
        FILE *fp;
	
	if (nfiles != 3 && nfiles != 2) 
		return "Syntax error: Two or three input files must be supplied";

        for (nf = 0; nf < nfiles; nf++)
        { 
		if (!strcmp(infiles[nf], "-")) fp = stdin;
                else  fp = fopen(infiles[nf], "rb");
          
                if (!fp)
                { 
                        perror(infiles[nf]); 
                        return "Cannot open font file."; 
                }

		psf_file_new(&psf[nf]);
		rv = psf_file_read(&psf[nf], fp);
                if (fp != stdin) fclose(fp);
		if (rv != PSF_E_OK) return psf_error_string(rv);
/* PSF loaded. */
	}
	if (psf[0].psf_height != 14)
		fprintf(stderr, "Warning: %s is not 14 pixels high\n", infiles[0]);
	if (psf[1].psf_height != 8)
		fprintf(stderr, "Warning: %s is not 8 pixels high\n", infiles[1]);
	if (nfiles == 3 && psf[2].psf_height != 8)
		fprintf(stderr, "Warning: %s is not 8 pixels high\n", infiles[2]);
	/* Now generate the ROM */
	memset(rom, 0, sizeof(rom));
	if (compaq)
	{
		transform(&psf[0], 8, 0x0000, 16, 0);
		transform(&psf[0], 0, 0x0008, 16, 0);
		transform(&psf[1], 0, 0x1000, 16, 0);
		if (nfiles == 2)
			transform(&psf[1], 0, 0x1008, 16, 256);
		else	transform(&psf[2], 0, 0x1008, 16,   0);
/* Comment, if any */
		for (nc = 0; nc < 1024; nc++)
		{
			if (msgbuf[nc] == 0) break;
			rom[nc * 16 + 6] = msgbuf[nc];
		}
	}
	else
	{
		transform(&psf[0], 0, 0x0000, 8, 0);
		transform(&psf[0], 8, 0x0800, 8, 0);
		if (nfiles == 2)
			transform(&psf[1], 0, 0x1000, 8, 256);
		else	transform(&psf[2], 0, 0x1000, 8, 0);
		transform(&psf[1], 0, 0x1800, 8, 0);
	}
	if (fwrite(rom, 1, sizeof(rom), outfile) < (int)sizeof(rom))
	{
		return "Error writing output file";
	}
	for (nf = 0; nf < nfiles; nf++)
	{
		psf_file_delete(&psf[nf]);
	}
	return NULL;
}

