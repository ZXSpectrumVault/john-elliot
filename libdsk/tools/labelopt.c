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

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "config.h"
#ifdef HAVE_TIME_H
#include <time.h>
#endif
#include "utilopts.h"
#include "libdsk.h"
#include "labelopt.h"




static char *check_labelarg(int *argc, char **argv, char *arg)
{
	int n = find_arg(arg, *argc, argv);
	char *result;

	if (n < 0) return NULL;	
	/* Remove the option, so n now points at the parameter */
	excise_arg(n, argc, argv);
	if (n >= *argc)
	{
		fprintf(stderr, "Syntax error: '%s' must be followed by a name.\n", arg);
		exit(1);
	}
	result = malloc(1 + strlen(argv[n]));
	strcpy(result, argv[n]);

	excise_arg(n, argc, argv);

	return result;
}

void check_labelargs(int *argc, char **argv,
                char **oemid, char **fsid, char **bootlabel, char **dirlabel)
{
	char *arg;

	arg = check_labelarg(argc, argv, "-oemid"); if (arg) *oemid = arg; 
	arg = check_labelarg(argc, argv, "-fsid"); if (arg) *fsid = arg;
	arg = check_labelarg(argc, argv, "-dirlabel"); if (arg) *dirlabel = arg;
	arg = check_labelarg(argc, argv, "-bootlabel"); if (arg) *bootlabel = arg;
	arg = check_labelarg(argc, argv, "-label"); 
	if (arg) { *dirlabel = arg; *bootlabel = arg; }
}

#define BCD(x)  ((x / 10) << 4) | ((x % 10) & 0x0F)

static void time2cpm(time_t unixtime, unsigned char *cpmtime)
{
        long d  = (unixtime / 86400) - 2921;  /* CP/M day 0 is unix day 2921 */
        long h  = (unixtime % 86400) / 3600;  /* Hour, 0-23 */
        long m  = (unixtime % 3600)  / 60;    /* Minute, 0-59 */

        cpmtime[0] = (unsigned char)(d & 0xFF);
        cpmtime[1] = (unsigned char)((d >> 8) & 0xFF);
        cpmtime[2] = (unsigned char)(BCD(h));
        cpmtime[3] = (unsigned char)(BCD(m));
}



/* Walk a CP/M directory searching for the disk label. 
 *  Operation = 0: Return it in dirlabel
 *  Operation = 1: Update existing dirlabel
 *  Operation = 2: Create new dirlabel
 */ 
static dsk_err_t cpm_walk_dir(DSK_PDRIVER dsk, DSK_GEOMETRY *geom,
			char *dirlabel, int operation)
{
	int dsm, drm, off, al0;
	dsk_err_t err;
	int entry;
	dsk_lsect_t dirsec;
	unsigned char *dirent;
	int match = 0;
	time_t t;	
	unsigned char *secbuf;
	int dirs_per_sec;
	char fcb[12];

	if (operation != 0)
	{	
		char buf[13], *p;

		sprintf(buf, "%-12.12s", dirlabel);
		/* Convert 8.3 filename to FCB */
		p = strchr(buf, '.');
		if (p)
		{
			*p = 0;
			sprintf(fcb,     "%-8.8s", buf);
			sprintf(fcb + 8, "%-3.3s", p + 1);
		}	
		else	sprintf(fcb, "%-11.11s", buf);
	}

	err = dsk_get_option(dsk, "FS:CP/M:DRM", &drm);
	if (err) return err;
	err = dsk_get_option(dsk, "FS:CP/M:DSM", &dsm);
	if (err) return err;
	err = dsk_get_option(dsk, "FS:CP/M:OFF", &off);
	if (err) return err;
	err = dsk_get_option(dsk, "FS:CP/M:AL0", &al0);
	if (err) return err;

	if (!drm || !dsm || !al0) return 1;	/* Not a valid CP/M filesystem */
 	secbuf = dsk_malloc(geom->dg_secsize);
	if (!secbuf) return DSK_ERR_NOMEM;

	/* Find first sector of directory */
	dirsec = (off * geom->dg_sectors);
	dirs_per_sec = geom->dg_secsize / 32;

	for (entry = 0; entry <= drm; entry++)
	{
		if ((entry % dirs_per_sec) == 0)
		{
			err = dsk_lread(dsk, geom, secbuf, 
					(entry / dirs_per_sec) + dirsec);
			if (err) return err;
		}
		dirent = secbuf + 32 * (entry % dirs_per_sec);
		match = 0;
		switch (operation)
		{
			/* Type 0 / 1: Match existing label 
			 * Type 2: Match empty slot */
			case 0: 
			case 1: 
				match = (dirent[0] == 0x20);
				break;
			case 2: match = (dirent[0] == 0xE5);
				break;
		}
		if (!match) continue;
		time (&t);
		switch (operation)
		{
			case 0: /* Type 0: Return disk label */
				sprintf(dirlabel, "%-8.8s.%-3.3s", 
					dirent + 1, dirent + 9);
				dsk_free(secbuf);
				return DSK_ERR_OK;
			case 2: /* Type 2: Create new disk label */
				memset(dirent, 0, 32);
				dirent[0] = 0x20;
				time2cpm(t, &dirent[0x18]);
				/* and fall through */
			case 1:	/* Type 1: Update disk label */
				memcpy(dirent + 1, fcb, 11);
				dirent[12] |= 1;	/* Label exists */
				time2cpm(t, &dirent[0x1C]);
				err = dsk_lwrite(dsk, geom, secbuf, 
					(entry / dirs_per_sec) + dirsec);
				dsk_free(secbuf);
				return err;
		}	/* End switch */
	}
	return 1;	/* No error but label not found */
}




/* Walk a DOS directory searching for the disk label. 
 *  Operation = 0: Return it in dirlabel
 *  Operation = 1: Update existing dirlabel
 *  Operation = 2: Create new dirlabel
 */ 
static dsk_err_t dos_walk_dir(DSK_PDRIVER dsk, DSK_GEOMETRY *geom,
			char *dirlabel, int operation)
{
	int direntries, reserved;
	int fatcopies, secfat;
	dsk_err_t err;
	int entry;
	dsk_lsect_t dirsec;
	unsigned char secbuf[512];
	unsigned char *dirent;
	int match = 0;
	time_t t;	
	struct tm *ptm;

	err = dsk_get_option(dsk, "FS:FAT:FATCOPIES",  &fatcopies);
	if (err) return err;
	err = dsk_get_option(dsk, "FS:FAT:DIRENTRIES", &direntries);
	if (err) return err;
	err = dsk_get_option(dsk, "FS:FAT:SECFAT",     &secfat);
	if (err) return err;
	err = dsk_get_option(dsk, "FS:FAT:RESERVED",   &reserved);
	if (err) return err;

	/* Find first sector of directory */
	dirsec = (fatcopies * secfat) + reserved;
	/* There are 512/32 = 16 directory entries per sector */
	for (entry = 0; entry < direntries; entry++)
	{
		if ((entry % 16) == 0)
		{
			err = dsk_lread(dsk, geom, secbuf, 
					(entry / 16) + dirsec);
			if (err) return err;
		}
		dirent = secbuf + 32 * (entry % 16);
		match = 0;
		/* End of directory */
		if (dirent[0] == 0 && operation != 2) break; 
		switch (operation)
		{
			/* Type 0 / 1: Match existing label 
			 * Type 2: Match empty slot */
			case 0: 
			case 1: 
				match = (dirent[11] & 8) && (!(dirent[11] & 7));
				break;
			case 2: match = (dirent[0] == 0 || dirent[0] == 0xE5);
				break;
		}
		if (!match) continue;
		switch (operation)
		{
			case 0: /* Type 0: Return disk label */
				sprintf(dirlabel, "%-11.11s", dirent);
				return DSK_ERR_OK;
			case 1:	
			case 2: /* Type 1 & 2: Set new disk label */
				memset(dirent, 0, 32);
				sprintf((char *)dirent, "%-11.11s", dirlabel);
				dirent[11] = 8;	/* Volume label */
				time (&t);
				ptm = localtime(&t);
				dirent[22] = (ptm->tm_sec) >> 1 | (ptm->tm_min << 5);
				dirent[23] = (ptm->tm_min >> 3) | (ptm->tm_hour << 3);
				dirent[24] = ptm->tm_mday | ((ptm->tm_mon + 1) << 5);
				dirent[25] = (ptm->tm_mon + 1) >> 3 | (ptm->tm_year - 80) << 1;
				return dsk_lwrite(dsk, geom, secbuf, 
					(entry / 16) + dirsec);
		}	/* End switch */
	}
	return 1;	/* No error but label not found */
}


dsk_err_t get_labels(DSK_PDRIVER dsk, DSK_GEOMETRY *geom, char *oemid, 
		char *fsid, char *bootlabel, char *dirlabel)
{
	dsk_err_t err;
	int fatcopies;
	int direntries;
	int secfat;
	int al0;
	int dsm;
	int reserved;
	DSK_GEOMETRY bootgeom;
	int found = 0;

	if (oemid) oemid[0] = 0;
	if (fsid)  fsid[0] = 0;
	if (bootlabel) bootlabel[0] = 0;
	if (dirlabel) dirlabel[0] = 0;

	/* Get filesystem parameters */
	if (!dsk_get_option(dsk, "FS:FAT:FATCOPIES",  &fatcopies) &&
	    !dsk_get_option(dsk, "FS:FAT:DIRENTRIES", &direntries) &&
            !dsk_get_option(dsk, "FS:FAT:SECFAT",     &secfat) &&
	    !dsk_get_option(dsk, "FS:FAT:RESERVED",   &reserved) &&
	    geom->dg_secsize == 512)
	{
		unsigned char bootsec[512];
		/* It's a FAT disk. */
		err = dsk_lread(dsk, geom, bootsec, 0);
		if (!err && !dg_dosgeom(&bootgeom, bootsec))
		{
			found = 1;
			/* PCDOS FAT */
			if (oemid) sprintf(oemid, "%-8.8s", bootsec + 3);
			if (bootsec[0x26] == 0x29)
			{
				if (bootlabel) sprintf(bootlabel, "%-11.11s", bootsec + 0x2B);
				if (fsid) sprintf(fsid, "%-8.8s", bootsec + 0x36);
			}
		}
		else if (!err && !dg_aprigeom(&bootgeom, bootsec))
		{
			/* Apricot FAT */
			if (oemid) sprintf(oemid, "%-8.8s", " Apricot");
			if (fsid) sprintf(fsid, "%-8.8s", "FAT12");
		}
		if (dirlabel)
		{
			err = dos_walk_dir(dsk, geom, dirlabel, 0);
			if (!err) found = 1;
		}
		if (err) return err;
		if (!found) return 1;
		return DSK_ERR_OK;	
	}
	/* Is there a valid CP/M filesystem? */
	else if (!dsk_get_option(dsk, "FS:CP/M:DRM",  &direntries) &&
	         !dsk_get_option(dsk, "FS:CP/M:OFF",  &reserved) &&
		 !dsk_get_option(dsk, "FS:CP/M:AL0",  &al0) &&
		 !dsk_get_option(dsk, "FS:CP/M:DSM",  &dsm) &&
		 al0 && direntries && dsm)
	{
		if (fsid) sprintf(fsid, "%-8.8s", "CP/M");
		return cpm_walk_dir(dsk, geom, dirlabel, 0);
	}
	else if (!dsk_get_option(dsk, "FS:DFS:SECTORS",  &al0) && al0 >= 2
		&& geom->dg_secsize == 256)
	{
		unsigned char bootsec[512];
		char diskname[13];

		/* It's (probably) a DFS disk. */
		err = dsk_lread(dsk, geom, bootsec, 0);
		if (!err) err = dsk_lread(dsk, geom, bootsec + 256, 1);

		if (!err && !dg_dfsgeom(&bootgeom, bootsec, bootsec+256))
		{ 
			memcpy(diskname, bootsec, 8);
			memcpy(diskname+8, bootsec + 256, 4);
			diskname[12] = 0;

			if (oemid) sprintf(oemid, "%-8.8s", "  Acorn ");
			if (fsid) sprintf(fsid, "%-8.8s", "DFS");
			if (dirlabel) sprintf(dirlabel, "%-12.12s", diskname);
		}
	}
	return DSK_ERR_OK;
}




dsk_err_t set_labels(DSK_PDRIVER dsk, DSK_GEOMETRY *geom, const char *oemid, 
		const char *fsid, const char *bootlabel, const char *dirlabel)
{
	dsk_err_t err;
	int fatcopies;
	int direntries;
	int secfat;
	int al0;
	int dsm;
	int reserved;
	DSK_GEOMETRY bootgeom;
	char buf[13];

	/* Get filesystem parameters */
	if (!dsk_get_option(dsk, "FS:FAT:FATCOPIES",  &fatcopies) &&
	    !dsk_get_option(dsk, "FS:FAT:DIRENTRIES", &direntries) &&
            !dsk_get_option(dsk, "FS:FAT:SECFAT",     &secfat) &&
	    !dsk_get_option(dsk, "FS:FAT:RESERVED",   &reserved) &&
	    geom->dg_secsize == 512)
	{
		unsigned char bootsec[512];
		/* It's a FAT disk. */
		err = dsk_lread(dsk, geom, bootsec, 0);
		if (err) return err;

		if (!dg_dosgeom(&bootgeom, bootsec))
		{
			/* PCDOS FAT */
			if (oemid) 
			{
				sprintf(buf, "%-8.8s", oemid);
				memcpy(bootsec + 3, buf, 8);
			}
			if (bootsec[0x26] == 0x29)
			{
				if (bootlabel) 
				{
					sprintf(buf, "%-11.11s", bootlabel);
					memcpy(bootsec + 0x2B, buf, 11);
				}
				if (fsid) 
				{
					sprintf(buf, "%-8.8s", fsid);
					memcpy(bootsec + 0x36, buf, 8);
				}
			}
			err = dsk_lwrite(dsk, geom, bootsec, 0);
			if (err) return err;
		}
		if (dirlabel)
		{
			sprintf(buf, "%-11.11s", dirlabel);
			err = dos_walk_dir(dsk, geom, buf, 1);
			/* Not found => create new label */
			if (err == 1) err = dos_walk_dir(dsk, geom, buf, 2);

			return err;
		}
		return 1;
	}
	/* Is there a valid CP/M filesystem? */
	else if (!dsk_get_option(dsk, "FS:CP/M:DRM",  &direntries) &&
	         !dsk_get_option(dsk, "FS:CP/M:OFF",  &reserved) &&
		 !dsk_get_option(dsk, "FS:CP/M:AL0",  &al0) &&
		 !dsk_get_option(dsk, "FS:CP/M:DSM",  &dsm) &&
		 al0 && direntries && dsm && dirlabel)
	{
		sprintf(buf, "%-12.12s", dirlabel);
		err = cpm_walk_dir(dsk, geom, buf, 1);
		if (err == 1) err = cpm_walk_dir(dsk, geom, buf, 2);
		return err;
	}
	else if (!dsk_get_option(dsk, "FS:DFS:SECTORS",  &al0) && al0 >= 2
		&& geom->dg_secsize == 256)
	{
		unsigned char bootsec[512];
		char diskname[13];

		/* It's (probably) a DFS disk. */
		err = dsk_lread(dsk, geom, bootsec, 0);
		if (!err) err = dsk_lread(dsk, geom, bootsec + 256, 1);

		if (!err && !dg_dfsgeom(&bootgeom, bootsec, bootsec+256))
		{ 
			sprintf(diskname, "%-12.12s", dirlabel);
			memcpy(bootsec, diskname, 8);
			memcpy(bootsec + 256, diskname + 8, 4);
			err = dsk_lwrite(dsk, geom, bootsec, 0);
			if (!err) err = dsk_lwrite(dsk, geom, bootsec + 256, 1);
			return err;
		}
	}
	return DSK_ERR_OK;
}

