/************************************************************************

    TAPTOOLS v1.1.0 - Tapefile manipulation utilities

    Copyright (C) 1996, 2014  John Elliott <jce@seasip.demon.co.uk>

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
#include "dskbits.h"
#include "plus3fs.h"
#include "tapeio.h"

#ifdef __PACIFIC__
#define AV0 "TAP2DSK"
#else
#define AV0 argv[0]
#endif

typedef unsigned char byte;

static TIO_PFILE st_tape;
static TIO_FORMAT st_format = TIOF_TAP;
unsigned int blklen;
byte p3hdr[128];
byte p3sig[] = "PLUS3DOS\032\001";
int uctr = 1;
int nctr = 1;
const char *dsktype = "dsk";
const char *compress = NULL;
const char *label = NULL;
int timestamp = 1;
int format = 180;
char fcbname[12];
PLUS3FS gl_fs;
int gl_uid = 0;
int cpmonly = 0;
int dosonly = 0;
unsigned long expected_len = 0;

static unsigned char boot_180[] =
{
        0x00, /* Disc type */
        0x00, /* Disc geometry */
        0x28, /* Tracks */
        0x09, /* Sectors */
        0x02, /* Sector size */
        0x01, /* Reserved tracks */
        0x03, /* ?Sectors per block */
        0x02, /* ?Directory blocks */
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
        0x04, /* ?Sectors per block */
        0x04, /* ?Directory blocks */
        0x2A, /* Gap length (R/W) */
        0x52  /* Gap length (format) */
};


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

void diewith(dsk_err_t err)
{
	fprintf(stderr, "Cannot write %-8.8s.%-3.3s: %s\n",
			fcbname, fcbname + 8, p3fs_strerror(err));

	p3fs_umount(gl_fs);
	exit(1);
}

void diewith_boo(const char *s)
{
	fprintf(stderr, "Cannot write %-8.8s.%-3.3s: %s\n",
			fcbname, fcbname + 8, s);

	p3fs_umount(gl_fs);
	exit(1);
}



void x_header(TIO_HEADER *hdr)
{
	char namebuf[11];

	memcpy(namebuf, hdr->header + 2, 10);
	namebuf[10] = 0;
	p3fs_83name(namebuf, fcbname);
	memset(p3hdr, 0, sizeof(p3hdr));
	memcpy(p3hdr, p3sig, sizeof(p3sig));

	p3hdr[15] = hdr->header[1];
	memcpy(p3hdr + 16, hdr->header + 12, 6);
	expected_len = hdr->header[12] + 256 * hdr->header[13];
}

void x_ibm_header(TIO_HEADER *hdr)
{
	int n;

	memset(p3hdr, 0, sizeof(p3hdr));
	memcpy(p3hdr, p3sig, sizeof(p3sig));
	for (n = 0; n < 8; n++)
	{
		if (hdr->header[2 + n] < ' ' || hdr->header[2 + n] > 0x7E)
		{
			fcbname[n] = '_';	
		}
		else 	fcbname[n] = toupper(hdr->header[2 + n]);
	}
	if (hdr->header[10] & 0x20)
	{
		strcpy(fcbname + 8, "P  ");	
		p3hdr[15] = 3;
		memcpy(p3hdr + 16, hdr->header + 11, 2);
	}
	else if (hdr->header[10] & 0x80)
	{
		strcpy(fcbname + 8, "B  ");	
		p3hdr[15] = 3;
		memcpy(p3hdr + 16, hdr->header + 11, 2);
	}
	else if (hdr->header[10] & 0x40)
	{
		strcpy(fcbname + 8, "A  ");	
	}
	else if (hdr->header[10] & 0x01)
	{
		strcpy(fcbname + 8, "M  ");	
		p3hdr[15] = 3;
		memcpy(p3hdr + 16, hdr->header + 11, 2);
		memcpy(p3hdr + 18, hdr->header + 15, 2);
		memcpy(p3hdr + 20, hdr->header + 13, 2);
	}
	else 
	{
		strcpy(fcbname + 8, "D  ");	
	}
	expected_len = 0;
}

static char *do_write(void *param, int c)
{
	PLUS3FILE fpo = (PLUS3FILE)param;

	int err = p3fs_putc(c, fpo);
	if (err) diewith(err);
	return NULL;
}



void x_data(TIO_HEADER *hdr, int read_data)
{
	unsigned long blk2;
	PLUS3FILE fpo;
	int n, err;
	byte csum;

	if (!read_data)
	{
		tio_compact(hdr);
	}
	blk2 = expected_len + 128;
	
	p3hdr[11] =  blk2        & 0xFF;
	p3hdr[12] = (blk2 >>  8) & 0xFF;	
	p3hdr[13] = (blk2 >> 16) & 0xFF;
	p3hdr[14] = 0;

	for (n = csum = 0; n < 127; n++) csum += p3hdr[n];
	p3hdr[n] = csum;

	if (fcbname[0])
	{
		if (fcbname[0] < 32)
			sprintf(fcbname, "UNNAMED %03d", nctr++);

		do
		{
			if (!p3fs_exists(gl_fs, gl_uid, fcbname)) break;
			n = atoi(fcbname + 8);
			sprintf(fcbname + 8, "%03d", n + 1);
		} while(1);

		printf("Writing %-8.8s.%-3.3s\n", fcbname, fcbname+8);

		err = p3fs_creat(gl_fs, &fpo, gl_uid, fcbname);
		if (err) diewith(err);
		if (hdr->status != TIO_IBM_HEADER)
		{
			for (n = 0; n < 128; n++) 
			{
				err = p3fs_putc(p3hdr[n], fpo);
				if (err) diewith(err);
			}
		}
	}
	else if (hdr->status == TIO_ZX_SNA)
	{
		sprintf(fcbname, "BLOCK%03dSNA", uctr++);
		printf("Writing snapshot %-8.8s.%-3.3s\n", 
				fcbname, fcbname+8);
		err = p3fs_creat(gl_fs, &fpo, gl_uid, fcbname);
		if (err) diewith(err);
	}
	else
	{
		sprintf(fcbname, "BLOCK   %03d", uctr++);
		printf("Writing headerless %-8.8s.%-3.3s\n", 
				fcbname, fcbname+8);
		err = p3fs_creat(gl_fs, &fpo, gl_uid, fcbname);
		if (err) diewith(err);
	}
	if (read_data)
	{
		const char *boo = tio_readdata(st_tape, hdr, fpo, do_write);
		if (boo) 
		{
			fprintf(stderr, "%s\n", boo);
			exit(1);
		}
	}
	else for (n = 0; n < hdr->length; n++)
	{
		err = p3fs_putc(hdr->data[n], fpo);
		if (err) diewith(err);
	}
	err = p3fs_close(fpo);
	if (err) diewith(err);
	fcbname[0] = 0;
}

void syntax(const char *av0)
{
	fprintf(stderr, "Syntax: %s {options} tapfile dskfile\n\n"
			"Options are:\n"
			"  -720: Write a 720k image (default)\n"
			"  -180: Write a 180k image\n"
			"  -cpmonly: Write a disc only usable by +3DOS\n"
			"  -dosonly: Write a disc only usable by PCDOS\n"
			"  -type type: Output file type (default=dsk)\n"
			"  -compress cmp: Output compression (default=none)\n"
			"  -label lbl: Set disk label (default=tapfile name)\n"
			"  -nostamps: Do not include file date stamps\n", av0);
      	 
	exit(1);
}

int main(int argc, char **argv)
{
	dsk_err_t err;
	int n, optend = 0;
	const char *tapfile = NULL;
	const char *dskfile = NULL;
	const char *option, *value;
	const char *boo;
	TIO_HEADER hdr, data;

	for (n = 1; n < argc; n++)
	{
		if (!strcmp(argv[n], "--"))
		{
			optend = 1;
			continue;
		}
		if (optend || argv[n][0] != '-')
		{
			if (tapfile == NULL) tapfile = argv[n];
			else if (dskfile == NULL) dskfile = argv[n];
			else syntax(AV0);
			continue;
		}
		option = argv[n] + 1;
		if (option[0] == '-') ++option;

		if      (!strcmp (option, "180")) 
		{
			format = 180;
			continue;
		}
		else if (!strcmp (option, "720"))
		{
			format = 720;
			continue;
		}
		else if (!strcmp (option, "nostamps")) 
		{
			timestamp = 0;
			continue;
		}
		else if (!strcmp (option, "cpmonly")) 
		{
			cpmonly = 1;	
			continue;
		}
		else if (!strcmp (option, "dosonly")) 
		{
			dosonly = 1;
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
		if (!strncmp(option, "type", 4)) dsktype = value;
		else if (!strncmp(option, "label", 5)) label = value;
		else if (!strncmp(option, "compress", 8)) compress = value;
		else syntax(AV0);
	}
	if (tapfile == NULL || dskfile == NULL) 
	{
		syntax(AV0);
	}
	if (cpmonly && dosonly)
	{
		fprintf(stderr, "-cpmonly and -dosonly both present; -cpmonly "
				"takes priority\n");
	}
	if (label == NULL) label = tapfile;
        dsk_reportfunc_set(report, report_end);

	boo = tio_open(&st_tape, tapfile, &st_format);
	if (boo)
	{
		fprintf(stderr, "%s\n", boo);
		exit(1);
	}
	err = p3fs_mkfs(&gl_fs, dskfile, dsktype, compress, 
			(format == 180) ? boot_180 : boot_720, timestamp);
	if (timestamp && !err)
	{
		p3fs_83name(label, fcbname);
		err = p3fs_setlabel(gl_fs, fcbname);
	}
	if (err)
	{
		fprintf(stderr, "Can't create %s: %s\n",
			dskfile, p3fs_strerror(err));
		tio_close(&st_tape);
		exit(1);
	}
	fcbname[0] = 0;

	do
	{
		memset(&hdr, 0, sizeof(hdr));
		memset(&data, 0, sizeof(data));
		boo = tio_readraw(st_tape, &hdr, 0);
		if (boo) { fprintf(stderr, "%s\n", boo); exit(1); }

		switch (hdr.status)
		{
			case TIO_IBM_HEADER: 
				x_ibm_header(&hdr); 
				x_data(&hdr, 1);
				break;
			case TIO_ZX_HEADER: 
				x_header(&hdr); 
				x_data(&hdr, 1);
				break;
			case TIO_EOF:
			case TIO_TZXOTHER: 
				break;

			default:
				x_data(&hdr, 0);
				break;
		}
		if (hdr.data) free(hdr.data);
		if (data.data) free(data.data);
	}
	while (hdr.status != TIO_EOF);	

	if (cpmonly == 0 && (format == 180 || format == 720))
	{	
		err = p3fs_dossync(gl_fs, format, dosonly);
		if (err)
		{
			fprintf(stderr, "Cannot do DOS sync on %s: %s\n",
					dskfile, p3fs_strerror(err));
		
		}
	}
	tio_close(&st_tape);
	err = p3fs_umount(gl_fs);
	if (err) 
	{
		fprintf(stderr, "Cannot close %s: %s\n",
				dskfile, p3fs_strerror(err));
		return 1;
	}
	return 0;
}
