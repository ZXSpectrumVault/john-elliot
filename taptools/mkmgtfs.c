/************************************************************************

    TAPTOOLS v1.0.9 - Tapefile manipulation utilities

    Copyright (C) 1996, 2012  John Elliott <jce@seasip.demon.co.uk>

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
#include "mgtfs.h"

#ifdef __PACIFIC__
#define AV0 "MKMGTFS"
#else
#define AV0 argv[0]
#endif

typedef unsigned char byte;

const char *dsktype = "dsk";
const char *compress = NULL;
const char *label = NULL;
int format = 800;
char fcbname[12];
MGTFS gl_fs;

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
	fprintf(stderr, "Cannot write %-10.10s: %s\n",
			fcbname, mgtfs_strerror(err));

	mgtfs_umount(gl_fs);
	exit(1);
}


dsk_err_t put_file(QFILE *qf)
{
	FILE *fp;
	dsk_err_t err;
	MGTFILE fpo;
	int c;

	mgtfs_makename(qf->filename, fcbname);
	fp = fopen(qf->pathname, "rb");
	if (!fp) return DSK_ERR_SYSERR;

	printf("Writing %-10.10s\n", fcbname);

	err = mgtfs_creat(gl_fs, &fpo, fcbname);
	if (err) return err;

	while ((c = fgetc(fp)) != EOF)
	{
		err = mgtfs_putc(c, fpo);
		if (err) return err;
	}
	fclose(fp);
	return mgtfs_close(fpo);
}

void syntax(const char *av0)
{
	fprintf(stderr, "Syntax: %s {options} dskfile file file ...\n\n"
			"Options are:\n"
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

		if      (!strcmp (option, "400")) 
		{
			format = 400;
			continue;
		}
		else if (!strcmp (option, "800"))
		{
			format = 800;
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

        dsk_reportfunc_set(report, report_end);

	err = mgtfs_mkfs(&gl_fs, dskfile, dsktype, compress, format);
	if (err)
	{
		fprintf(stderr, "Can't create %s: %s\n",
			dskfile, mgtfs_strerror(err));
		exit(1);
	}
	if (label) 
	{
		mgtfs_makename(label, fcbname);
		mgtfs_setlabel(gl_fs, fcbname);
	}
	for (qf = qf_head; qf != NULL; qf = qf->next)
	{
		err = put_file(qf);
		if (err) diewith(err);
	}
	err = mgtfs_umount(gl_fs);
	if (err) 
	{
		fprintf(stderr, "Cannot close %s: %s\n",
				dskfile, mgtfs_strerror(err));
		return 1;
	}
	return 0;
}
