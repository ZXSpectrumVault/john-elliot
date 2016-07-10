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
#include <time.h>
#include "config.h"
#include "dskbits.h"
#include "mgtfs.h"


typedef struct mgtfs_impl
{
	DSK_PDRIVER drive;
	DSK_GEOMETRY geom;
	unsigned char dir[256 * 80];
	int capacity;
	int maxtrack;
	dsk_lsect_t next_sec;
	dsk_lsect_t last_sec;
} MGTFS_IMPL;

typedef struct mgtfile_impl
{
	MGTFS_IMPL *fs;
	unsigned char *dirent;
	unsigned long filepos;
	int bufpos;
	dsk_lsect_t cursec;
	unsigned char buf[512];
} MGTFILE_IMPL;



const char *mgtfs_strerror(int err)
{
	switch(err)
	{
		case MGTFS_ERR_DISKFULL: return "Disk full.";
		case MGTFS_ERR_DIRFULL:  return "Directory full.";
		case MGTFS_ERR_EXISTS:   return "File exists.";
	}
	return dsk_strerror(err);
}


void mgtfs_makename(const char *name, char *fcbname)
{
	sprintf(fcbname, "%-10.10s", name);
}

dsk_err_t mgtfs_mkfs(MGTFS *pfs, const char *name, 
		const char *type, const char *compression, int capacity)
{
	MGTFS fs;
	int n;
	dsk_ltrack_t track;
	dsk_lsect_t sector;
	dsk_err_t err;
	char msg[80];

	if (pfs == NULL) return DSK_ERR_BADPTR;

	fs = malloc(sizeof(MGTFS_IMPL));
	if (!fs) return DSK_ERR_NOMEM;
	*pfs = fs;
	memset(fs, 0, sizeof(*fs));

	/* Set up geometry */
	err = dg_stdformat(&fs->geom, FMT_MGT800, NULL, NULL);
	if (err) return err;

	fs->geom.dg_heads = (capacity <= 400) ? 1 : 2;
	fs->geom.dg_cylinders = 80;
	fs->geom.dg_secbase = 1;
	fs->geom.dg_sidedness = SIDES_OUTOUT;
	fs->maxtrack = fs->geom.dg_heads * fs->geom.dg_cylinders;
	fs->next_sec = sizeof(fs->dir) / fs->geom.dg_secsize;
	fs->last_sec = (fs->maxtrack * fs->geom.dg_sectors) - 1;

	/* Create a new DSK file */
	err = dsk_creat(&fs->drive, name, type, compression);
	if (err) return err;

	/* Format all its tracks */
	for (track = 0; track < fs->maxtrack; track++)
	{
		sprintf(msg, "Formatting track %d/%d", track, fs->maxtrack);
		dsk_report(msg);
		if (!err) err = dsk_alform(fs->drive, &fs->geom, track, 0xe5);
	}
	dsk_report("Writing directory");
	mgtfs_sync(fs);
	dsk_report_end();
	return err;

}


dsk_err_t mgtfs_umount(MGTFS fs)
{
	dsk_err_t err;

	if (fs == NULL) return DSK_ERR_BADPTR;
	err = mgtfs_sync(fs);
	if (err) return err;
	err = dsk_close(&fs->drive);
	memset(fs, 0, sizeof(*fs));
	return err;
}

int mgtfs_exists(MGTFS fs, const char *name)
{
	int n;

	for (n = 0; n < sizeof(fs->dir); n += 256)
	{
		if (fs->dir[n] != 0 && 
			!memcmp(name, &fs->dir[n+1], 10)) return 1;
	}
	return 0;
}


/* Find a free directory entry */
unsigned char *mgtfs_new_dirent(MGTFS fs)
{
	unsigned char *dirent;
	int n;

	for (n = 0; n < sizeof(fs->dir); n += 256)
	{
		if (fs->dir[n] == 0) return &fs->dir[n];
	}
	return NULL;
}


dsk_err_t mgtfs_creat(MGTFS fs, MGTFILE *pfp, const char *name)
{
	MGTFILE fp;
	int n;

	if (!pfp) return DSK_ERR_BADPTR;
	*pfp = NULL;

	if (mgtfs_exists(fs, name)) return MGTFS_ERR_EXISTS;
	fp = malloc(sizeof(MGTFILE_IMPL));
	if (!fp) return DSK_ERR_NOMEM;

	memset(fp, 0, sizeof(MGTFILE_IMPL));
	fp->fs = fs;
	fp->dirent = mgtfs_new_dirent(fs);	/* Construct dir entry */
	if (!fp->dirent)
	{
		free(fp);
		return MGTFS_ERR_DIRFULL;
	}
	fp->dirent[0] = 10;			/* opentype */
	memcpy(fp->dirent + 1, name, 10);
	*pfp = fp;
	return DSK_ERR_OK;
}


dsk_err_t mgtfs_setlabel(MGTFS fs, const char *name)
{
	char buf[11];

	sprintf(buf, "%-10.10s", name);
	memcpy(&fs->dir[210], buf, 10);
}


dsk_err_t mgtfs_sync(MGTFS fs)
{
	dsk_lsect_t sector;
	dsk_err_t err = DSK_ERR_OK;
	int n;

	sector = 0;
	for (n = 0; n < sizeof(fs->dir); n += fs->geom.dg_secsize)
	{
		if (!err) err = dsk_lwrite(fs->drive, &fs->geom, fs->dir + n,
			sector);
		++sector;
	}
	return err;	
}


dsk_err_t mgtfs_flush(MGTFILE fp)
{
	dsk_err_t err;

	if (fp->bufpos == 0) return DSK_ERR_OK;

	err = dsk_lwrite(fp->fs->drive, &fp->fs->geom, fp->buf, fp->cursec);
	fp->bufpos = 0;
	return err;	
}

dsk_err_t mgtfs_close(MGTFILE fp)
{
	dsk_err_t err;

	err = mgtfs_flush(fp);

	if (fp->dirent[0] == 8) /* No +3DOS header */
	{
		fp->dirent[210] = (fp->filepos >> 16) & 0xFF;
		fp->dirent[212] = (fp->filepos) & 0xFF;
		fp->dirent[213] = (fp->filepos >> 8) & 0xFF;
	}

	free(fp);
	return err;
}

dsk_err_t mgtfs_new_sector(MGTFILE fp, dsk_lsect_t *sector)
{
	dsk_lsect_t sec;
	unsigned char mask;

	if (fp->fs->next_sec > fp->fs->last_sec) 
		return MGTFS_ERR_DISKFULL;

	*sector = fp->fs->next_sec;
	++fp->fs->next_sec;

	sec = (*sector) - (sizeof(fp->fs->dir) / fp->fs->geom.dg_secsize);
	mask = 1 << (sec & 7);
	fp->dirent[15 + (sec >> 3)] |= mask;
	return DSK_ERR_OK;
}



dsk_err_t mgtfs_putc(unsigned char c, MGTFILE fp)
{
	dsk_pcyl_t cyl;
	dsk_phead_t head;
	dsk_psect_t sec;
	dsk_err_t err;
	dsk_lsect_t sector;

	/* This is the first sector. Allocate it. */
	if (fp->dirent[13] == 0 && fp->dirent[14] == 0)
	{
		err = mgtfs_new_sector(fp, &sector);
		if (err) return err;

		dg_ls2ps(&fp->fs->geom, sector, &cyl, &head, &sec);
		fp->dirent[11] = 0;
		fp->dirent[12] = 1;
		fp->dirent[13] = cyl | (head ? 0x80 : 0);
		fp->dirent[14] = sec;
		fp->cursec = sector;
		memset(fp->buf, 0, sizeof(fp->buf));
		fp->bufpos = 0;
		/* Allocate the new sector in the directory entry */
	}

	/* We have filled up the sector. Go to the next one. */
	if (fp->bufpos >= 510)
	{
		err = mgtfs_new_sector(fp, &sector);
		if (err) return err;

		dg_ls2ps(&fp->fs->geom, sector, &cyl, &head, &sec);
		fp->buf[510] = cyl | (head ? 0x80 : 0);
		fp->buf[511] = sec;

		err = dsk_lwrite(fp->fs->drive, &fp->fs->geom, fp->buf,
				fp->cursec);
		if (err) return err;

		fp->cursec = sector;
		memset(fp->buf, 0, sizeof(fp->buf));
		fp->bufpos = 0;
		++fp->dirent[12];
		if (0 == fp->dirent[12]) ++fp->dirent[11];
	}
	fp->buf[fp->bufpos++] = c;
	++fp->filepos;

	/* Test for a +3DOS header having just been written */
	if (fp->filepos == 128 && !memcmp(fp->buf, "PLUS3DOS\032", 9))
	{
		unsigned char n, cksum;
		for (n = cksum = 0; n < 127; n++) cksum += fp->buf[n];
		if (fp->buf[127] == cksum)
		{
			/* This is a +3DOS file. Convert header and 
			 * rewind file pointer to 0. */
			fp->dirent[0] = fp->buf[15] + 1;	/* type */

			if (fp->buf[15] == 3    && fp->buf[16] == 0 &&
			    fp->buf[17] == 0x1B && fp->buf[18] == 0 &&
			    fp->buf[19] == 0x40) fp->dirent[0] = 7; /* screen */

			fp->dirent[211] = fp->buf[15];	/* type */
			fp->dirent[212] = fp->buf[16];	/* length */
			fp->dirent[213] = fp->buf[17];
			switch (fp->buf[15])
			{
				case 0: fp->dirent[214] = 0xCB; /* load    */
					fp->dirent[215] = 0x5C; /* address */
					fp->dirent[216] = fp->buf[20];
					fp->dirent[217] = fp->buf[21];
					fp->dirent[218] = fp->buf[18];
					fp->dirent[219] = fp->buf[19];
					break;
				case 1:	
				case 2: fp->dirent[216] = fp->buf[18]; break;
				case 3: fp->dirent[214] = fp->buf[18];
					fp->dirent[215] = fp->buf[19]; break;
			}
			memset(fp->buf, 0, sizeof(fp->buf));
			memcpy(fp->buf, fp->dirent + 211, 9);
			fp->filepos = 9;
			fp->bufpos = 9;
		}
	}
	return DSK_ERR_OK;
}





