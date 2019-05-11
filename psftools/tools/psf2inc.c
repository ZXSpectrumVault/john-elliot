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

/* Convert a PSF file to a C include file. Used while building zx2psf. */

#include "cnvshell.h"
#include "psflib.h"

char *cnv_progname = "PSF2INC";

static char helpbuf[2000];
static int v1 = 0, v2 = 0, fasm = 0;

char *cnv_set_option(int ddash, char *variable, char *value)
{
    if (!stricmp(variable, "psf1"))   { v1 = 1; return NULL; }
    if (!stricmp(variable, "psf2"))   { v2 = 1; return NULL; }
    if (!stricmp(variable, "asm"))    { fasm = 1; return NULL; }
    if (strlen(variable) > 2000) variable[2000] = 0;
    sprintf(helpbuf, "Unknown option: %s\n", variable);
    return helpbuf;
}

/* Return help string */
char *cnv_help(void)
    {
    sprintf(helpbuf, "Syntax: %s psffile incfile { options }\n\n", cnv_progname);
    strcat (helpbuf, "Options: \n"
                     "    --psf1         Force PSF1 format\n"
                     "    --psf2         Force PSF2 format\n"
		     "    --asm          Output in RMAC/RASM86 format\n");

    return helpbuf;
    }

static psf_dword filepos;

static int startline;

static void put_comment(PSFIO *io, char *s)
{
	if (fasm) fprintf(io->data.fp, "  ; %s\n", s);
	else	  fprintf(io->data.fp,"  /* %s */\n   ", s);
	startline = 1;
}

static void put_newline(PSFIO *io)
{
	if (fasm) 	fprintf(io->data.fp, "\n");
	else		fprintf(io->data.fp, "\n   ");
	startline = 1;
}


int write_printf(PSFIO *io, psf_byte b)
{
	static psf_dword ch, x, cb;
	int ct = 0;
	static psf_byte pch;

	if (!filepos) 
	{
		put_newline(io);
		ch = 0;
		x = 0;
		cb = 0;
	}
	if (fasm)
	{
		if (startline)
		{
			fprintf(io->data.fp, "\tdb\t");
			startline = 0;
		}
		else 
		{
			fprintf(io->data.fp, ", ");
		}
		fprintf(io->data.fp, "0%02xh", b);
	}
	else	  fprintf(io->data.fp, "0x%02x, ", b);
	if (io->psf->psf_magic == PSF1_MAGIC) switch(filepos)
	{
		case 1: put_comment(io, "Magic"); x = 0; cb = 0; ct = 1; break;
		case 2: put_comment(io, "Type");  x = 0; cb = 0; ct = 1; break;
		case 3: put_comment(io, "Char size"); x = 0; cb = 0; ct = 1; break;
	}
	if (io->psf->psf_magic == PSF_MAGIC) switch(filepos)
	{
		case  3: put_comment(io, "Magic");        x = 0; cb = 0; ct = 1; break;
		case  7: put_comment(io, "Version");      x = 0; cb = 0; ct = 1; break;
		case 11: put_comment(io, "Header size");  x = 0; cb = 0; ct = 1; break;
		case 15: put_comment(io, "Flags");        x = 0; cb = 0; ct = 1; break;
		case 19: put_comment(io, "No. of chars"); x = 0; cb = 0; ct = 1; break;
		case 23: put_comment(io,"Char length");  x = 0; cb = 0; ct = 1; break;
		case 27: put_comment(io,"Char width");   x = 0; cb = 0; ct = 1; break;
		case 31: put_comment(io, "Char height");  x = 0; cb = 0; ct = 1; break;
	}
	++filepos;
	if (ct) { pch = b; return PSF_E_OK; }
	if (ch >= io->psf->psf_length)	/* Unicode dir */
	{
		if (io->psf->psf_magic == PSF_MAGIC && b == 0xFF)
		{
			put_newline(io);
			x = 0;
		}
		if (io->psf->psf_magic == PSF1_MAGIC && b == 0xFF && pch == 0xFF)
		{
			put_newline(io);
			x = 0;
		}
	}

	cb++;
	if (cb == io->psf->psf_charlen && ch < io->psf->psf_length)
	{
		char buf[30];
		sprintf(buf, "%ld", ch++);
		put_comment(io, buf);
		cb = x = 0;
	}
	else x++;
	if (x == 8)
	{
		put_newline(io);
		x = 0;
	}	

	pch = b;
	return PSF_E_OK;;
}




char *cnv_execute(FILE *infile, FILE *outfile)
{	
	int rv;
	PSF_FILE psf;
	PSFIO psfio;
	char *s = NULL;

	filepos = 0;
	psf_file_new(&psf);
	rv = psf_file_read(&psf, infile);	

	if (rv != PSF_E_OK) return psf_error_string(rv);

	psfio.psf = &psf;
	psfio.readfunc  = NULL;
	psfio.writefunc = write_printf;
	psfio.data.fp = outfile; 

	if (v1) psf_force_v1(&psf);
	if (v2) psf_force_v2(&psf);

	if ((rv = psf_write(&psfio))) s = psf_error_string(rv);
	else fprintf(outfile, "\n");

	psf_file_delete(&psf);
	return s;
}

