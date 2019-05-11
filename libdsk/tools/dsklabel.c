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

/* This program can display or change the label, filesystem ID and OEM ID 
 * of a FAT-format floppy disc. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "config.h"
#ifdef HAVE_LIBGEN_H
# include <libgen.h>
#endif
#include "libdsk.h"
#include "utilopts.h"
#include "labelopt.h"

#ifdef __PACIFIC__
# define AV0 "DSKLABEL"
#else
# ifdef HAVE_BASENAME
#  define AV0 (basename(argv[0]))
# else
#  define AV0 argv[0]
# endif
#endif

int do_label(char *outfile, char *outtyp, char *outcomp, int forcehead);

char *new_oemid     = NULL;
char *new_fsid      = NULL;
char *new_bootlabel = NULL;
char *new_dirlabel  = NULL;

int help(int argc, char **argv)
{
	fprintf(stderr, "Syntax: \n"
                "      %s { options } dskimage\n\n"
		"Options are:\n"
                "    -oemid <text>     : Set OEM identifier\n"
                "    -fsid <text>      : Set filesystem ID\n"
                "    -label <text>     : Set disk label in boot sector and root directory\n"
                "    -bootlabel <text> : Set disk label in boot sector only\n"
                "    -dirlabel <text>  : Set disk label in root directory only\n",
			AV0);
	fprintf(stderr,"\nDefault type is autodetect.\n\n");
		
	fprintf(stderr, "eg: %s myfile.DSK\n"
                        "    %s -label 'Boot floppy' myfile.DSK \n", AV0, AV0);
	return 1;
}








int main(int argc, char **argv)
{
	char *outtyp;
	char *outcomp;
	int forcehead;
        int stdret;

        stdret = standard_args(argc, argv); if (!stdret) return 0;
	if (argc < 2) return help(argc, argv);

        ignore_arg("-itype", 2, &argc, argv);
        ignore_arg("-iside", 2, &argc, argv);
        ignore_arg("-icomp", 2, &argc, argv);
        ignore_arg("-otype", 2, &argc, argv);
        ignore_arg("-oside", 2, &argc, argv);
        ignore_arg("-ocomp", 2, &argc, argv);

	outtyp    = check_type  ("-type",   &argc, argv);
	outcomp   = check_type  ("-comp",   &argc, argv);
	forcehead = check_forcehead("-side", &argc, argv);	
        if (find_arg("--help",    argc, argv) > 0) return help(argc, argv);

	check_labelargs (&argc, argv, &new_oemid, &new_fsid, &new_bootlabel,
			&new_dirlabel);

	return do_label(argv[1], outtyp, outcomp, forcehead);
}



int do_label(char *outfile, char *outtyp, char *outcomp, int forcehead)
{
	DSK_PDRIVER outdr = NULL;
	dsk_err_t e;
	DSK_GEOMETRY dg;
	char oemid[9];
	char fsid[9];
	char bootlabel[13];
	char dirlabel[13];

	e = dsk_open(&outdr, outfile, outtyp, outcomp);
	if (!e && forcehead >= 0) e = dsk_set_forcehead(outdr, forcehead);
	if (!e) e = dsk_getgeom(outdr, &dg);
	if (!e)
	{
		if (!new_oemid && !new_fsid && !new_bootlabel && !new_dirlabel)
		{
			e = get_labels(outdr, &dg, oemid, fsid, bootlabel,
					dirlabel);
			if (e >= 0)
			{
                		printf("Filesystem:       %s\n", fsid);
                		printf("OEM ID:           %s\n", oemid);
		                printf("Bootsector label: %s\n", bootlabel);
               			printf("Disk label:       %s\n", dirlabel);
			}
		}
		else	/* Setting new labels */
		{	
			e = set_labels(outdr, &dg, new_oemid, new_fsid,
				new_bootlabel, new_dirlabel);
		}
	}
	if (outdr) 
	{
		if (!e) e = dsk_close(&outdr); else dsk_close(&outdr);
	}
	if (e < DSK_ERR_OK)
	{
		fprintf(stderr, "Failed to get / set drive labels. Possib"
				"ly the disc or file does not contain a\n"
				"filesystem that LibDsk recognises.\n");
		fprintf(stderr, "%s\n", dsk_strerror(e));
		return 1;
	}
	return 0;
}

