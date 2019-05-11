/***************************************************************************
 *                                                                         *
 *    LIBDSK: General floppy and diskimage access library                  *
 *    Copyright (C) 2001,2019  John Elliott <seasip.webmaster@gmail.com>   *
 *                                                                         *
 *    This library is free software; you can redistribute it and/or        *
 *    modify it under the terms of the GNU Library General Public          *
 *    License as published by the Free Software Foundation; either         *
 *    version 2 of the License, or (at your option) any later version.     *
 *                                                                         *
 *    This library is distributed in the hope that it will be useful,      *
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of       *
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU    *
 *    Library General Public License for more details.                     *
 *                                                                         *
 *    You should have received a copy of the GNU Library General Public    *
 *    License along with this library; if not, write to the Free           *
 *    Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,      *
 *    MA 02111-1307, USA                                                   *
 *                                                                         *
 ***************************************************************************/

/* Used to list the contents of a Gotek device */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "config.h"
#ifdef HAVE_LIBGEN_H
# include <libgen.h>
#endif
#include "libdsk.h"
#include "utilopts.h"
#include "labelopt.h"

#ifdef __PACIFIC__
# define AV0 "DSKID"
#else
# ifdef HAVE_BASENAME
#  define AV0 (basename(argv[0]))
# else
#  define AV0 argv[0]
# endif
#endif

static unsigned first = 0;
static unsigned last  = 999;

int help(int argc, char **argv)
{
	fprintf(stderr, "Syntax: \n"
                "      %s { options} dskimage \n\n"
		"Options:\n"
		"  -first <n>  First image to list (default is 0)\n"
		"  -last  <n>  Last image to list (default is 0)\n",
		AV0);

	fprintf(stderr, "eg: %s /dev/sdh1\n"
                        "    %s -last 99 imagefile \n", AV0, AV0);
	return 1;
}

static void report(const char *s)
{
	fprintf(stderr,"%-79.79s\r", s);
	fflush(stderr);
}

static void report_end(void)
{
	fprintf(stderr,"\r%-79.79s\r", "");
	fflush(stderr);
}

void printable(char *dest)
{
	while (*dest)
	{
		if (!isprint(*dest)) *dest = '.';
		++dest;
	}
}




int lsgotek(int argc, const char *filename)
{
	char *buf = dsk_malloc(20 + strlen(filename));
	unsigned n, size;
	DSK_PDRIVER pdriver;	
	DSK_GEOMETRY geom;
	dsk_err_t err;

	dsk_reportfunc_set(report, report_end);	
	if (!buf)
	{
		fprintf(stderr, "Out of memory\n");
		return 1;
	}
	if (argc > 1) printf("%s:\n", filename);
	printf("    No.\tSize\tFilesystem\tOEM ID  \tBoot label\tLabel\n");
	for (n = first; n <= last; n++)
	{
		sprintf(buf, "gotek:%s,%u", filename, n);
		err = dsk_open(&pdriver, buf, "gotek", NULL);
		if (err)
		{
			fprintf(stderr, "%s: %s\n", filename,
				dsk_strerror(err));
			return 1;
		}
		printf("    %03u\t", n);
		err = dsk_getgeom(pdriver, &geom);
		size = 0;
		if (!err)
		{
			size = geom.dg_sectors * geom.dg_cylinders * 
				geom.dg_heads / 2;
			printf("%4dK\t", size);
		}
		else
		{
			printf("\t");
		}
		if (geom.dg_secsize == 512)	/* May contain a readable FS */
		{
			char oemid[9];
			char fsid[9];
			char bootlabel[13];
			char dirlabel[13];

			err = get_labels(pdriver, &geom, oemid, fsid, 
						bootlabel, dirlabel);

			printable(oemid);
			printable(fsid);
			printable(bootlabel);
			printable(dirlabel);
			if (!err)
			{
				printf("%-8.8s\t%-8.8s\t%-11.11s\t%-11.11s",
					fsid, oemid, bootlabel, dirlabel);
			}
		}
		putchar('\n');
		dsk_close(&pdriver);
	}
	return 0;
}


static void parse_number(char *arg, int *argc, char **argv, unsigned *result)
{
	int n = find_arg(arg, *argc, argv);
	if (n >= 0)
	{
		excise_arg(n, argc, argv);
		if (n > *argc || !isdigit(argv[n][0]))
		{
			fprintf(stderr, "Syntax error: %s must be followed by a number\n", arg);
			exit(1);
		}
		*result = atoi(argv[n]);
		excise_arg(n, argc, argv);
	}
}

int main(int argc, char **argv)
{
	int n, err;
        int stdret = standard_args(argc, argv); if (!stdret) return 0;

	parse_number("-first", &argc, argv, &first);
	parse_number("-last",  &argc, argv, &last);

	if (argc < 2) return help(argc, argv);

        if (find_arg("--help",    argc, argv) > 0) return help(argc, argv);
	args_complete(&argc, argv);

	err = 0;
	for (n = 1; n < argc; n++)
	{
		if (lsgotek(argc, argv[n]))
			++err;
	}
	return err;
}


