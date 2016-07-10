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
#include "config.h"
#include "dskbits.h"
#include "plus3fs.h"

#ifdef __PACIFIC__
#define AV0 "MKP3FS"
#else
#define AV0 argv[0]
#endif


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

typedef struct queued_file
{
	struct queued_file *next;
	const char *pathname;
	char filename[13];
} QFILE;

QFILE *qf_head = NULL;

void add_filename(const char *s)
{
	QFILE *qf = malloc(sizeof(QFILE));
	QFILE *qf2;
	char *sep;

	if (!qf)
	{
		fprintf(stderr, "Out of memory\n");
		exit(1);
	}
	qf->pathname = s;
	sep = strrchr(s, '/');
	if (sep) s = sep + 1;
#if defined(__MSDOS__) || defined(WIN32)
	sep = strrchr(s, '\\');
	if (sep) s = sep + 1;
	sep = strrchr(s, ':');
	if (sep) s = sep + 1;
#endif
	strncpy(qf->filename, s, 12);
	qf->filename[12] = 0;
	qf->next = NULL;
/* Add this entry to the end of the linked list */
	if (!qf_head)
	{
		qf_head = qf;
	}
	else
	{
		qf2 = qf_head;
		while (qf2->next != NULL) qf2 = qf2->next;
		qf2->next = qf;
	}
}

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


dsk_err_t put_file(QFILE *qf)
{
	FILE *fp;
	dsk_err_t err;
	PLUS3FILE fpo;
	int c;

	p3fs_83name(qf->filename, fcbname);
	fp = fopen(qf->pathname, "rb");
	if (!fp) return DSK_ERR_SYSERR;

	printf("Writing %-8.8s.%-3.3s\n", fcbname, fcbname+8);

	err = p3fs_creat(gl_fs, &fpo, gl_uid, fcbname);
	if (err) return err;

	while ((c = fgetc(fp)) != EOF)
	{
		err = p3fs_putc(c, fpo);
		if (err) return err;
	}
	fclose(fp);
	return p3fs_close(fpo);
}

void syntax(const char *av0)
{
	fprintf(stderr, "Syntax: %s {options} dskfile file file ...\n\n"
			"Options are:\n"
			"  -180: Write a 180k image (default)\n"
			"  -720: Write a 720k image\n"
			"  -cpmonly: Write a disc only usable by +3DOS\n"
			"  -dosonly: Write a disc only usable by PCDOS\n"
			"  -type type: Output file type (default=dsk)\n"
			"  -compress cmp: Output compression (default=none)\n"
			"  -label lbl: Set disk label (default=none)\n"
			"  -nostamps: Do not include file date stamps\n", av0);
      	 
	exit(1);
}

int main(int argc, char **argv)
{
	dsk_err_t err;
	int n, optend = 0;
	const char *dskfile = NULL;
	const char *option, *value;
	QFILE *qf;

	for (n = 1; n < argc; n++)
	{
		if (!strcmp(argv[n], "--"))
		{
			optend = 1;
			continue;
		}
		if (optend || argv[n][0] != '-')
		{
			if (dskfile == NULL) dskfile = argv[n];
			else add_filename(argv[n]);	
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
	if (dskfile == NULL || qf_head == NULL) 
	{
		syntax(AV0);
	}
	if (cpmonly && dosonly)
	{
		fprintf(stderr, "-cpmonly and -dosonly both present; -cpmonly "
				"takes priority\n");
	}

        dsk_reportfunc_set(report, report_end);

	err = p3fs_mkfs(&gl_fs, dskfile, dsktype, compress, 
			(format == 180) ? boot_180 : boot_720, timestamp);
	if (timestamp && !err && label)
	{
		p3fs_83name(label, fcbname);
		err = p3fs_setlabel(gl_fs, fcbname);
	}
	if (err)
	{
		fprintf(stderr, "Can't create %s: %s\n",
			dskfile, p3fs_strerror(err));
		exit(1);
	}
	for (qf = qf_head; qf != NULL; qf = qf->next)
	{
		err = put_file(qf);
		if (err) diewith(err);
	}
	if (cpmonly == 0 && (format == 180 || format == 720))
	{	
		err = p3fs_dossync(gl_fs, format, dosonly);
		if (err)
		{
			fprintf(stderr, "Cannot do DOS sync on %s: %s\n",
					dskfile, p3fs_strerror(err));
		
		}
	}
	err = p3fs_umount(gl_fs);
	if (err) 
	{
		fprintf(stderr, "Cannot close %s: %s\n",
				dskfile, p3fs_strerror(err));
		return 1;
	}
	return 0;
}
