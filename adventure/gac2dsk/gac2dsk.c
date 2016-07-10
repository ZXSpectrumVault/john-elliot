/************************************************************************

    GAC2DSK 1.0.0 - Convert GAC tape files to +3DOS

    Copyright (C) 2007, 2009  John Elliott <jce@seasip.demon.co.uk>

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

*************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "config.h"
#include "libdsk.h"
#include "plus3fs.h"
#include "gacread.h"

#ifdef __PACIFIC__
#define AV0 "GAC2DSK"
#else
#define AV0 argv[0]
#endif

static const char *gl_otype = "dsk";
static const char *gl_ocomp = NULL;
static int gl_format = 180;
static int gl_dos = 0;

PLUS3FS gl_fs;

static void report(const char *s)
{
        printf("%s\r", s);
        fflush(stdout);
}

static void report_end(void) 
{
        printf("\r%-79.79s\r", "");
        fflush(stdout);
}

static unsigned char basic_loader[] =
{
	0x00, 0x0A, 0x0D, 0x00,			/* 0000 line no & length */
		0xFD, '0', '0', '0', '0', '0',	/* 0004 CLEAR nnnn */
		0x0E,   0,   0,   0,   0,  0,	/* 000A number */
		0x0D,				/* 0010 CR */
	0x00, 0x14, 0x14, 0x00,			/* 0011 line no & length */
		0xEF, 0x22, 'd', 'a', 't', 'a', 0x22, /* 0015 LOAD "data" */
		0xAF, '0', '0', '0', '0', '0',	/* 001C CODE nnnn */
		0x0E,   0,   0,   0,   0,  0,	/* 0022 number */
		0x0D,				/* 0028 CR */
	0x00, 0x1E, 0x0E, 0x00,			/* 0029 line no & length */
		0xF9, 0xC0,			/* 002D RAND USR */
		'0', '0', '0', '0', '0',	/* 002F nnnnn */
		0x0E,   0,   0,   0,   0,  0,	/* 0034 number */
		0x0D,				/* 0035 CR */
};

void poke2(byte *array, int offset, int v)
{
	array[offset] = v & 0xFF;
	array[offset+1] = (v >> 8) & 0xFF;
}


dsk_err_t do_convert()
{
	char buf[6];
	byte header[128];
	unsigned n;
	byte cksum;
	dsk_err_t err;
	PLUS3FILE p3fp;

	sprintf(buf, "%5u", gac_sp); memcpy(basic_loader + 0x05, buf, 5);
	poke2(basic_loader, 0x0d, gac_sp);
	sprintf(buf, "%5u", gac_ix); memcpy(basic_loader + 0x1d, buf, 5);
	poke2(basic_loader, 0x25, gac_ix);
	sprintf(buf, "%5u", gac_pc); memcpy(basic_loader + 0x2f, buf, 5);
	poke2(basic_loader, 0x37, gac_pc);

	/* Generate loader */
	memset(header, 0, sizeof(header));
	memcpy(header, "PLUS3DOS\032\001\000", 11);
	poke2(header, 11, sizeof(basic_loader) + 128);
	poke2(header, 16, sizeof(basic_loader));
	poke2(header, 18, 0x000);
	poke2(header, 20, sizeof(basic_loader));
	cksum = 0;
	for (n = 0; n < 127; n++) cksum += header[n];
	header[127] = cksum;

	err = p3fs_creat(gl_fs, &p3fp, 0, "DISK       ");
	if (err) return err;
	for (n = 0; n < 128; n++)
	{
		err = p3fs_putc(header[n], p3fp);
		if (err) return err;
	}
	for (n = 0; n < sizeof(basic_loader); n++)
	{
		err = p3fs_putc(basic_loader[n], p3fp);
		if (err) return err;
	}
	err = p3fs_close(p3fp);
	if (err) return err;

	/* Generate datafile */
	memset(header, 0, sizeof(header));
	memcpy(header, "PLUS3DOS\032\001\000", 11);
	poke2(header, 11, gac_de + 128);
	header[15] = 3;
	poke2(header, 16, gac_de);
	poke2(header, 18, gac_ix);
	cksum = 0;
	for (n = 0; n < 127; n++) cksum += header[n];
	header[127] = cksum;
	err = p3fs_creat(gl_fs, &p3fp, 0, "DATA       ");
	if (err) return err;
	for (n = 0; n < 128; n++)
	{
		err = p3fs_putc(header[n], p3fp);
		if (err) return err;
	}
	for (n = 0; n < gac_de; n++)
	{
		err = p3fs_putc(datablock[n+1], p3fp);
		if (err) return err;
	}
	return p3fs_close(p3fp);
}

static unsigned char boot_180[] =
{
        0x00, /* Disc type */
        0x00, /* Disc geometry */
        0x28, /* Tracks */
        0x09, /* Sectors */
        0x02, /* Sector size */
        0x01, /* Reserved tracks */
        0x03, /* Block shift */
        0x02, /* Directory blocks */
        0x2A, /* Gap length (R/W) */
        0x52  /* Gap length (format) */
};

static unsigned char boot_720[] =
{
        0x03, /* Disc type */
        0x81, /* Disc geometry */
        0x50, /* Tracks */
        0x09, /* Sectors */
        0x02, /* Sector size */
        0x02, /* Reserved tracks */
        0x04, /* Block shift */
        0x04, /* Directory blocks */
        0x2A, /* Gap length (R/W) */
        0x52  /* Gap length (format) */
};
void syntax(const char *arg)
{
	fprintf(stderr, "Syntax: %s {options} infile.tap outfile.dsk\n\n", arg);
	fprintf(stderr, "Options:\n"
			"-otype type: Output file type (default=dsk)\n"
			"-ocomp type: Output compression (default=none)\n"
			"-180:        Output 180k disc image\n"
			"-720:        Output 720k disc image\n"
			"-dos:        Output DOS disc image\n");

	exit(1);
}

int main(int argc, char **argv)
{
	char *inname = NULL;
	char *outname = NULL;
	int n, optend = 0;
	const char *option, *value;
	dsk_err_t err;

	for (n = 1; n < argc; n++)
	{
		if (!strcmp(argv[n], "--"))
		{
			optend = 1;
			continue;
		}
		if (optend || argv[n][0] != '-')
		{
			if      (inname  == NULL) inname = argv[n];
			else if (outname == NULL) outname = argv[n];
			else    syntax(AV0);
			continue;
		}
		option = argv[n] + 1;
		if (option[0] == '-') ++option;

		if      (!strcmp (option, "180")) 
		{
			gl_format = 180;
			continue;
		}
		else if (!strcmp (option, "720"))
		{
			gl_format = 720;
			continue;
		}
		else if (!strcmp (option, "dos")) 
		{
			gl_dos = 1;	
			gl_format = 720;
			continue;
		}

/* Check for --variable=value and -variable value */
		value = strchr(option, '=');
		if (value) ++value;
		else
		{
			++n;
			if (n >= argc) syntax(AV0);
			value = argv[n];
		}
		if (!strncmp(option, "otype", 5)) gl_otype = value;
		else if (!strncmp(option, "ocomp", 5)) gl_ocomp = value;
		else syntax(AV0);
	}
	if (inname == NULL || outname == NULL)
	{
		syntax(AV0);
	}
        dsk_reportfunc_set(report, report_end);

	err = load_gac(inname);
	if (err)
	{
		exit(1);
	}

	switch(gl_format)
	{
		case 180: err = p3fs_mkfs(&gl_fs, outname, gl_otype, gl_ocomp,
					boot_180, 0); break;
		case 720: err = p3fs_mkfs(&gl_fs, outname, gl_otype, gl_ocomp,
					boot_720, 1); break;
	}

	if (err)
	{
		fprintf(stderr, "Can't create %s: %s\n",
			outname, p3fs_strerror(err));
		exit(1);
	}
	err = do_convert();
	if (err)
	{
		dsk_strerror(err);
	}
	else if (gl_dos && (gl_format == 180 || gl_format == 720))
	{	
		err = p3fs_dossync(gl_fs, gl_format, gl_dos);
		if (err)
		{
			fprintf(stderr, "Cannot do DOS sync on %s: %s\n",
					outname, p3fs_strerror(err));
		
		}
	}
	err = p3fs_umount(gl_fs);
	if (err) 
	{
		fprintf(stderr, "Cannot close %s: %s\n",
				outname, p3fs_strerror(err));
		return 1;
	}
	return 0;
}
