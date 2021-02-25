/***************************************************************************
 *                                                                         *
 *    LIBDSK: General floppy and diskimage access library                  *
 *    Copyright (C) 2001-2, 2019  John Elliott <seasip.webmaster@gmail.com>*
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

/* Example formatter */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "config.h"
#ifdef HAVE_LIBGEN_H
# include <libgen.h>
#endif
#ifdef HAVE_TIME_H
# include <time.h>
#endif
#include "config.h"
#include "libdsk.h"
#include "utilopts.h"
#include "formname.h"
#include "apriboot.h"

#ifdef __PACIFIC__
# define AV0 "DSKFORM"
#else
# ifdef HAVE_BASENAME
#  define AV0 (basename(argv[0]))
# else
#  define AV0 argv[0]
# endif
#endif

static int retries = 1;
static int pcdos = 0;
static int apricot = 0;

int do_format(const char *outfile, const char *outtyp, const char *outcomp, 
		int forcehead, dsk_format_t format);

int help(int argc, char **argv)
{
	fprintf(stderr, "Syntax: \n"
                "      %s { options} dskimage \n\n"
		"Options:\n"
                "  -type <type>       Type of disk image file to write.\n"
                "                     '%s -types' lists valid types.\n"
                "  -format <format>   Disk format to use.\n"
                "                     '%s -formats' lists valid formats.\n"
                "  -retry <count>     Set number of retries.\n"
                "  -side <side>       Force format on head 0 or 1.\n"
		"  -pcdos             Create an empty PCDOS filesystem.\n"
		"  -apricot           Create an empty Apricot MSDOS filesystem.\n" 
		, AV0, AV0, AV0);
	fprintf(stderr,"\nDefault type is DSK.\nDefault format is PCW 180k.\n\n");
		
	fprintf(stderr, "eg: %s myfile.DSK\n"
                        "    %s -type floppy -format cpcsys -side 1 /dev/fd0\n", AV0, AV0);

	return 1;
}


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



int main(int argc, char **argv)
{
	const char *outtyp;
	char *outcomp;
	int forcehead;
	int stdret;
	dsk_format_t format;

	if (argc < 2) return help(argc, argv);
        if (find_arg("--help",    argc, argv) > 0) return help(argc, argv);
	stdret = standard_args(argc, argv); if (!stdret) return 0;
        ignore_arg("-itype", 2, &argc, argv);
        ignore_arg("-iside", 2, &argc, argv);
        ignore_arg("-icomp", 2, &argc, argv);
        ignore_arg("-otype", 2, &argc, argv);
        ignore_arg("-oside", 2, &argc, argv);
        ignore_arg("-ocomp", 2, &argc, argv);
        outtyp    = check_type("-type", &argc, argv); 
        outcomp   = check_type("-comp", &argc, argv); 
        forcehead = check_forcehead("-side", &argc, argv);
	format    = check_format("-format", &argc, argv);
        retries   = check_retry("-retry", &argc, argv);
	pcdos = present_arg("-pcdos", &argc, argv);
	apricot = present_arg("-apricot", &argc, argv);

	if (format == -1) format = FMT_180K;
	args_complete(&argc, argv);

	if (!outtyp) outtyp = guess_type(argv[1]);

	return do_format(argv[1], outtyp, outcomp, forcehead, format);
}

static unsigned char spec169 [10] = { 0,    0, 40, 9, 2, 2, 3, 2, 0x2A, 0x52 };
static unsigned char spec180 [10] = { 0,    0, 40, 9, 2, 1, 3, 2, 0x2A, 0x52 };
static unsigned char spec200 [10] = { 0,    0, 40,10, 2, 1, 3, 3, 0x0C, 0x17 };
static unsigned char spec720 [10] = { 3, 0x81, 80, 9, 2, 1, 4, 4, 0x2A, 0x52 };
static unsigned char spec800 [10] = { 3, 0x81, 80,10, 2, 1, 4, 4, 0x0C, 0x17 };

typedef struct bpb
{
	dsk_format_t format;
	unsigned short sector_size;
	unsigned char sectors_per_cluster;
	unsigned short reserved_sectors;
	unsigned char fat_copies;
	unsigned short root_entries;
	unsigned short total_sectors16;
	unsigned char media_descriptor;
	unsigned short sectors_per_fat;
	unsigned short sectors_per_track;
	unsigned short heads;
	unsigned long hidden_sectors;	
	unsigned long total_sectors32;
} BPB;

BPB all_bpbs[] = 
{
	{ FMT_160K,     512, 1, 1, 2,  64,  320, 0xFE, 1,  8, 1, 0, 0 },
	{ FMT_320K,     512, 2, 1, 2, 112,  640, 0xFF, 1,  8, 2, 0, 0 },
	{ FMT_180K,     512, 1, 1, 2,  64,  360, 0xFC, 2,  9, 1, 0, 0 },
	{ FMT_360K,     512, 2, 1, 2, 112,  720, 0xFD, 2,  9, 2, 0, 0 },
	{ FMT_720K,     512, 2, 1, 2, 112, 1440, 0xF9, 3,  9, 2, 0, 0 },
	{ FMT_1200K,    512, 1, 1, 2, 224, 2400, 0xF9, 7, 15, 2, 0, 0 },
	{ FMT_1440K,    512, 1, 1, 2, 224, 2880, 0xF0, 9, 18, 2, 0, 0 },

	{ FMT_ACORN640,	256, 8,16, 1, 112, 2560, 0xFF, 2, 16, 2, 0, 0 },
	{ FMT_ACORN800,1024, 2, 0, 1, 176,  800, 0xFD, 2,  5, 2, 0, 0 },
};

#define BPB_COUNT ((sizeof(all_bpbs) / sizeof(all_bpbs[0])))


static dsk_err_t mkfs_pcdos(const char *filename, DSK_PDRIVER outdr, 
		DSK_GEOMETRY *dg, dsk_format_t format)
{
	unsigned char *secbuf = dsk_malloc(dg->dg_secsize);
	dsk_lsect_t totsectors;
	unsigned clusters;
	unsigned dir_sectors;
	BPB thebpb;
	int n;
	dsk_err_t err = DSK_ERR_OK;
	dsk_lsect_t sec;
	dsk_lsect_t spf;
	time_t tm;

	time (&tm);

	if (!secbuf) return DSK_ERR_NOMEM;

	/* Construct a BPB / boot sector */
	memset(secbuf, 0xE5, dg->dg_secsize);
	memset(&thebpb, 0, sizeof(thebpb));

	totsectors = dg->dg_cylinders;
	totsectors *= dg->dg_heads;
	totsectors *= dg->dg_sectors;

	thebpb.format      = format;
	thebpb.sector_size = dg->dg_secsize;
	/* Sectors per cluster is 1 for single sided or 10+ SPT, else 2 */
	if (dg->dg_heads == 1 || dg->dg_sectors >= 10)
	{
		thebpb.sectors_per_cluster = 1;
	}
	else
	{
		thebpb.sectors_per_cluster = 2;
	}
	thebpb.reserved_sectors = 1;
	thebpb.fat_copies = 2;
	if (dg->dg_heads == 1)
	{
		thebpb.root_entries = 64;
	}
	else if (dg->dg_sectors < 10)
	{
		thebpb.root_entries = 112;
	}
	else
	{
		thebpb.root_entries = 224;
	}
	if (totsectors < 0x10000)
	{
		thebpb.total_sectors16 = (unsigned short)totsectors;
	}
	else
	{
		thebpb.total_sectors32 = totsectors;
	}
	/* Come up with a media byte */
	if (dg->dg_sectors == 8 && dg->dg_heads == 2 && 
		dg->dg_cylinders == 40)
	{
		thebpb.media_descriptor = 0xFF;
	}
	else if (dg->dg_sectors == 8 && dg->dg_heads == 1 && 
		dg->dg_cylinders == 40)
	{
		thebpb.media_descriptor = 0xFE;
	}
	else if (dg->dg_sectors == 9 && dg->dg_heads == 2 && 
		dg->dg_cylinders == 40)
	{
		thebpb.media_descriptor = 0xFD;
	}
	else if (dg->dg_sectors == 9 && dg->dg_heads == 1 && 
		dg->dg_cylinders == 40)
	{
		thebpb.media_descriptor = 0xFC;
	}
	else if (dg->dg_sectors == 8 && dg->dg_heads == 2 && 
		dg->dg_cylinders == 80)
	{
		thebpb.media_descriptor = 0xFB;
	}
	else if (dg->dg_sectors == 8 && dg->dg_heads == 1 && 
		dg->dg_cylinders == 80)
	{
		thebpb.media_descriptor = 0xFA;
	}
	else if (dg->dg_sectors >= 9 && dg->dg_sectors <= 15 &&
		dg->dg_heads == 2 && dg->dg_cylinders == 80)
	{
		thebpb.media_descriptor = 0xF9;
	}
	else if (dg->dg_sectors >= 9 && dg->dg_sectors <= 15 && 
		dg->dg_heads == 1 && dg->dg_cylinders == 80)
	{
		thebpb.media_descriptor = 0xF8;
	}
	else
	{
		thebpb.media_descriptor = 0xF0;
	}
	/* To calculate sectors per FAT, we need to know how many clusters
	 * there are. */
	dir_sectors = ((thebpb.root_entries * 32) + (dg->dg_secsize - 1))  
				/ dg->dg_secsize;
	clusters = (totsectors - thebpb.reserved_sectors - dir_sectors - 2) / 
			thebpb.sectors_per_cluster;

	thebpb.sectors_per_fat = ((((clusters + 2) * 3) / 2) 
				+ (dg->dg_secsize - 1)) / dg->dg_secsize;
	thebpb.sectors_per_track = dg->dg_sectors;
	thebpb.heads             = dg->dg_heads;
/*
printf("\nBPB:\n====\n");
printf("Sector size      =  %x\n", thebpb.sector_size); 
printf("Sectors/cluster  =  %x\n", thebpb.sectors_per_cluster);
printf("Reserved sectors =  %x\n", thebpb.reserved_sectors);
printf("FAT copies       =  %x\n", thebpb.fat_copies);
printf("Root dir entries =  %x\n", thebpb.root_entries);
printf("Total sectors    =  %x\n", thebpb.total_sectors16);
printf("Media descriptor =  %x\n", thebpb.media_descriptor);
printf("Sectors / FAT    =  %x\n", thebpb.sectors_per_fat);
printf("Sectors / track  =  %x\n", thebpb.sectors_per_track);
printf("Heads            =  %x\n", thebpb.heads);
*/
	/* Now we've laboriously constructed a BPB, see if there's a 
	 * predefined one for this format; if so, prefer it. */
	for (n = 0; n < BPB_COUNT; n++)
	{
		if (all_bpbs[n].format == format) 
		{
			thebpb = all_bpbs[n];
			break;
		}
	}

	secbuf[0] = 0xEB;	/* JMP 0x42 */
	secbuf[1] = 0x40;
	secbuf[2] = 0x89;
	/* Some versions of DOS react poorly to OEM labels they don't
	 * recognise. "IBM  3.3" is one of the safer values to use. 
	 *
	 * memcpy(secbuf + 3, "libdsk  ", 8);	*/
	memcpy(secbuf + 3, "IBM  3.3", 8);	
	secbuf[11] = thebpb.sector_size & 0xFF;	/* Sector size */
	secbuf[12] = thebpb.sector_size >> 8;
	secbuf[13] = thebpb.sectors_per_cluster;
	secbuf[14] = thebpb.reserved_sectors & 0xFF;
	secbuf[15] = thebpb.reserved_sectors >> 8;
	secbuf[16] = thebpb.fat_copies;
	secbuf[17] = thebpb.root_entries & 0xFF;
	secbuf[18] = thebpb.root_entries >> 8;
	secbuf[19] = thebpb.total_sectors16 & 0xFF;
	secbuf[20] = thebpb.total_sectors16 >> 8;
	secbuf[21] = thebpb.media_descriptor;
	secbuf[22] = thebpb.sectors_per_fat & 0xFF;
	secbuf[23] = thebpb.sectors_per_fat >> 8;
	secbuf[24] = thebpb.sectors_per_track & 0xFF;
	secbuf[25] = thebpb.sectors_per_track >> 8;
	secbuf[26] = thebpb.heads & 0xFF;
	secbuf[27] = thebpb.heads >> 8;
	secbuf[28] = thebpb.hidden_sectors & 0xFF;
	secbuf[29] = thebpb.hidden_sectors >> 8;
	secbuf[30] = thebpb.hidden_sectors >> 16;
	secbuf[31] = thebpb.hidden_sectors >> 24;
	secbuf[32] = thebpb.total_sectors32 & 0xFF;
	secbuf[33] = thebpb.total_sectors32 >> 8;
	secbuf[34] = thebpb.total_sectors32 >> 16;
	secbuf[35] = thebpb.total_sectors32 >> 24;

	secbuf[38] = 0x29;		/* Extended BPB present */
	memcpy(secbuf + 39, &tm, 4);	/* Volume serial number */
	memcpy(secbuf + 43, "NO NAME    ", 11);
	memcpy(secbuf + 54, "FAT12   ", 8);

	secbuf[0x42] = 0xCD;			/* ROM BASIC */
	secbuf[0x43] = 0x18;	
	secbuf[dg->dg_secsize - 2] = 0x55;
	secbuf[dg->dg_secsize - 1] = 0xAA;	/* Boot signature */

	if (apricot)
	{
		transform(FORCE_APRICOT, secbuf, filename, 0);
	}

	/* Write the boot sector */
	if (thebpb.reserved_sectors)
	{
		err = dsk_lwrite(outdr, dg, secbuf, 0);
	}
	sec = thebpb.reserved_sectors;
	if (!err)
	{
		/* Write the FATs */
		int fat;

		for (fat = 0; fat < thebpb.fat_copies; fat++)
		{
			spf = thebpb.sectors_per_fat - 1;
			memset(secbuf, 0, dg->dg_secsize);
			secbuf[0] = thebpb.media_descriptor;
			secbuf[1] = 0xFF;
			secbuf[2] = 0xFF;
			err = dsk_lwrite(outdr, dg, secbuf, sec++);
			if (err) break;
			memset(secbuf, 0, dg->dg_secsize);
			while (spf)
			{
				err = dsk_lwrite(outdr, dg, secbuf, sec++);
				if (err) break;
				--spf;
			}	
			if (err) break;
		}
	}
	if (!err)
	{
		/* Write the root directory */
		memset(secbuf, 0xE5, dg->dg_secsize);
		memset(secbuf, 0, 0x40);
		memcpy(secbuf, "not named  ", 11);
		secbuf[11] = 8;	/* Volume label */
		err = dsk_lwrite(outdr, dg, secbuf, sec++);
	}
	dsk_free(secbuf);
	return err;
}



int do_format(const char *outfile, const char *outtyp, const char *outcomp, 
		int forcehead, dsk_format_t format)
{
	DSK_PDRIVER outdr = NULL;
	dsk_err_t e;
	dsk_pcyl_t cyl;
	dsk_phead_t head;
	DSK_GEOMETRY dg;
	dsk_cchar_t fdesc;

	dsk_reportfunc_set(report, report_end);
	e = dsk_creat(&outdr, outfile, outtyp, outcomp);
	if (!e) e = dsk_set_retry(outdr, retries);
	if (!e && forcehead >= 0) e = dsk_set_forcehead(outdr, forcehead);
	if (!e) e = dg_stdformat(&dg, format, NULL, &fdesc);
	if (!e)
	{
		printf("Formatting %s as %s\n", outfile, fdesc);
		for (cyl = 0; cyl < dg.dg_cylinders; cyl++)
		{
		    for (head = 0; head < dg.dg_heads; head++)
		    {
			printf("Cyl %02d/%02d Head %d/%d\r", 
				cyl +1, dg.dg_cylinders,
			 	head+1, dg.dg_heads);
			fflush(stdout);

			if (!e) e = dsk_apform(outdr, &dg, cyl, head, 0xE5);
			if (e) break;	
		    }
		}
	
	}
	/* Create a disc spec on the first sector, if the format's recognised */
	if (!e)
	{
		unsigned char bootsec[512];
		unsigned int do_bootsec = 0;

		memset(bootsec, 0xE5, sizeof(bootsec));
		switch(format)
		{
			case FMT_180K:
				memcpy(bootsec, spec180, 10);
				do_bootsec = 1;
				break;
			case FMT_CPCSYS:
				memcpy(bootsec, spec169, 10);
				do_bootsec = 1;
				break;
			case FMT_CPCDATA:
				break;
			case FMT_200K:
				memcpy(bootsec, spec200, 10);
				do_bootsec = 1;
				break;
			case FMT_720K:
				memcpy(bootsec, spec720, 10);
				do_bootsec = 1;
				break;
			case FMT_800K:
				memcpy(bootsec, spec800, 10);
				do_bootsec = 1;
				break;
			default:
				break;
		}
		if (pcdos) 
		{
			printf("Creating PCDOS filesystem\n");
			e = mkfs_pcdos(outfile, outdr, &dg, format);
		}
		else if (apricot) 
		{
			printf("Creating Apricot MS-DOS filesystem\n");
			e = mkfs_pcdos(outfile, outdr, &dg, format);
		}
		else if (do_bootsec) 
		{
			e = dsk_lwrite(outdr, &dg, bootsec, 0);
		}
	}
	printf("\r                                     \r");
	if (outdr) 
	{
		if (!e) e = dsk_close(&outdr); else dsk_close(&outdr);
	}
	if (e)
	{
		fprintf(stderr, "%s\n", dsk_strerror(e));
		return 1;
	}
	return 0;
}


