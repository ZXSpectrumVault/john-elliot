/***************************************************************************
 *                                                                         *
 *    LIBDSK: General floppy and diskimage access library                  *
 *    Copyright (C) 2001-2,2017  John Elliott <seasip.webmaster@gmail.com> *
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

/* Convert one disk image format to another -- both must be based on LDBS */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include "config.h"
#ifdef HAVE_LIBGEN_H
# include <libgen.h>
#endif
#include "libdsk.h"
#include "utilopts.h"
#include "formname.h"
#include "apriboot.h"

#ifdef __PACIFIC__
# define AV0 "DSKCONV"
#else
# ifdef HAVE_BASENAME
#  define AV0 (basename(argv[0]))
# else
#  define AV0 argv[0]
# endif
#endif

int do_copy(char *infile, char *outfile);

static dsk_format_t format = -1;
static char *intyp = NULL, *outtyp = NULL;
static char *incomp = NULL, *outcomp = NULL;

static void report(const char *s)
{
        printf("%-79.79s\r", s);
        fflush(stdout);
}

static void report_end(void)
{
        printf("\r%-79.79s\r", "");
        fflush(stdout);
}


int help(int argc, char **argv)
{
	fprintf(stderr, "Syntax: \n"
                       "      %s {options} in-image out-image\n",
			AV0);
	fprintf(stderr,"\nOptions are:\n"
		       "-itype <type>   type of input disc image\n"
                       "-otype <type>   type of output disc image\n"
                       "                '%s -types' lists valid types.\n"
		       "-format         Force a specified format name\n"
                       "                '%s -formats' lists valid formats.\n",
			AV0, AV0);

	fprintf(stderr,"\nDefault in-image type is autodetect."
		               "\nDefault out-image type is LDBS.\n\n");
		
	fprintf(stderr, "eg: %s diag800b._01 diag800b.ldbs\n"
                        "    %s -otype raw diag800b._01 diag800b.ufi\n", 
			AV0, AV0);
	return 1;
}



int main(int argc, char **argv)
{
	int stdret;

        stdret = standard_args(argc, argv); if (!stdret) return 0;
	if (argc < 3) return help(argc, argv);
	if (find_arg("--help",    argc, argv) > 0) return help(argc, argv);

	ignore_arg("-type", 2, &argc, argv);
	ignore_arg("-comp", 2, &argc, argv);

	intyp     = check_type("-itype", &argc, argv); 
        outtyp    = check_type("-otype", &argc, argv);
	incomp    = check_type("-icomp", &argc, argv); 
        outcomp   = check_type("-ocomp", &argc, argv);
	if (!outtyp) outtyp = "ldbs";
        format    = check_format("-format", &argc, argv);
	args_complete(&argc, argv);
	return do_copy(argv[1], argv[2]);
}



int do_copy(char *infile, char *outfile)
{
	DSK_PDRIVER indr = NULL, outdr = NULL;
	DSK_GEOMETRY *dg;
	DSK_GEOMETRY tmp;
	dsk_err_t e;
	char *op = "Opening";

        dsk_reportfunc_set(report, report_end);

	        e = dsk_open (&indr,  infile,  intyp, incomp);
	if (!e) e = dsk_creat(&outdr, outfile, outtyp, outcomp);

	printf("Input driver: %s\nOutput driver:%s\n",
                        dsk_drvdesc(indr), dsk_drvdesc(outdr));

	if (format == -1) 
	{
		dg = NULL;
	}
	else if (!e)
	{
		e = dg_stdformat(dg = &tmp, format, NULL, NULL);
	}
	op = "Converting";
	if (!e)
	{
		e = dsk_copy(dg, indr, outdr);
		if (e == DSK_ERR_BADFMT && !dg)
		{
			printf("\r                                     \r");
			if (indr)  dsk_close(&indr);
			if (outdr) dsk_close(&outdr);
			fprintf(stderr, 
"This conversion requires the format to be specified manually with the\n"
"-format option.\n"); 
			return 1;
		}
		if (e == DSK_ERR_NOTIMPL)
		{
			printf("\r                                     \r");
			if (indr)  dsk_close(&indr);
			if (outdr) dsk_close(&outdr);
			fprintf(stderr, 
"This program can only convert disc image files, not other drive types such\n"
"as real floppy drives, remote drives or RCPMFS directories. Additionally, \n"
"some image file formats (such as DSK) do not yet have a conversion routine.\n"
"To convert an unsupported file type, you will need to use dsktrans to do a\n"
"sector-by-sector copy.\n");
			return 1;
		}
	}
	if (!e) op = "Finalizing";
	
	printf("\r                                     \r");
	if (outdr) 
	{
		if (!e) e = dsk_close(&outdr); else dsk_close(&outdr);
	}
	if (indr) 
	{
		if (!e) e = dsk_close(&indr); else dsk_close(&indr);
	}
	if (e)
	{
		fprintf(stderr, "\n%s: %s\n", op, dsk_strerror(e));
		return 1;
	}
	return 0;
}

