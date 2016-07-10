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
#include "plus3fs.h"


typedef struct plus3fs_impl
{
	DSK_PDRIVER drive;
	DSK_GEOMETRY geom;
	unsigned char *bootsec;
	unsigned char *cpmdir;
	unsigned char *dosdir;
	unsigned char *dosfat;
	int nextblock;
	int blocksize;
	int maxblock;
	int maxtrack;
	int offset;
	int maxdir;
	int dirblocks;
	int timestamped;
	int clustersize;
	int cluster_offset;
	int exm;
} PLUS3FS_IMPL;

typedef struct plus3file_impl
{
	PLUS3FS_IMPL *fs;
	int bufptr;
	int extent;
	unsigned char *dirent;
	unsigned char *stamp;
	unsigned char buf[1];
} PLUS3FILE_IMPL;



const char *p3fs_strerror(int err)
{
	switch(err)
	{
		case P3FS_ERR_DISKFULL: return "Disk full.";
		case P3FS_ERR_DIRFULL:  return "Directory full.";
		case P3FS_ERR_EXISTS:   return "File exists.";
		case P3FS_ERR_MISALIGN: return "DOS clusters don't align with CP/M blocks.";
	}
	return dsk_strerror(err);
}

void p3fs_83name(const char *name, char *fcbname)
{
	/* Maximum length of plain-text name is 12 characters plus null */
	char namebuf[13];
	int n;
	char *dot;

	sprintf(namebuf, "%-12.12s", name);
	memset(fcbname, ' ', 11);
	fcbname[11] = 0;
	/* Trim trailing spaces from passed name */
	for (n = 11; n > 0; n--)
	{
		if (namebuf[n] == ' ') namebuf[n] = 0;
		else break;
	}
	/* Try to get a +3DOS 8.3 name out of it */
	dot = strchr(namebuf, '.');
	if (dot && strlen(dot) <= 4)
	{
		sprintf(fcbname + 8, "%-3.3s", dot + 1);
		*dot = 0;
		memcpy(fcbname, namebuf, strlen(namebuf));
	}
	else	/* Dot is not in the right place; ignore it */
	{
		sprintf(fcbname, "%-11.11s", namebuf);
	}
	/* Restrict to +3 character set */
	for (n = 0; fcbname[n]; n++)
	{
		fcbname[n] &= 0x7F;
		if (fcbname[n] < '!' || strchr(".:;<>[]", fcbname[n]))
		{
			fcbname[n] = '_';
		}	
		fcbname[n] = toupper(fcbname[n]);
	}
	/* All spaces were changed to underlines. Now change the ones at the
	 * end back. */
	for (n = 10; n >= 8; n--)
	{
		if (fcbname[n] == '_') fcbname[n] = ' ';
		else break;
	}
	/* And the ones in the name proper */
	for (n = 7; n >= 1; n--)
	{
		if (fcbname[n] == '_') fcbname[n] = ' ';
		else break;
	}
}


dsk_err_t p3fs_umount(PLUS3FS fs)
{
	dsk_err_t err;

	if (fs == NULL) return DSK_ERR_BADPTR;
	p3fs_sync(fs);

	err = dsk_close(&fs->drive);
	free(fs->bootsec);
	if (fs->dosdir) free(fs->dosdir);
	memset(fs, 0, sizeof(*fs));
	return err;
}

dsk_err_t p3fs_mkfs(PLUS3FS *pfs, const char *name, const char *type,
	       const char *compression, unsigned char *boot_spec, 
	       int timestamped)
{
	dsk_err_t err;
	dsk_ltrack_t track;
	int heads, dirent;
	char msg[50];
	PLUS3FS fs;

	if (pfs == NULL)
	{
		return DSK_ERR_BADPTR;
	}
	fs = malloc(sizeof(PLUS3FS_IMPL));
	*pfs = fs;
	memset(fs, 0, sizeof(*fs));

	heads = (boot_spec[1] & 3) ? 2 : 1;
	/* Work out how big the buffers for the boot sector and directory
	 * need to be. Start by getting the drive geometry straight. */
	err = dg_pcwgeom(&fs->geom, boot_spec);
	if (err) return err;

	/* Now the CP/M filesystem parameters */
	fs->blocksize       = 128 << (boot_spec[6]);	/* Block size */
	fs->offset          = boot_spec[5];		/* Reserved tracks */
	fs->maxtrack        = heads * boot_spec[2];
	fs->dirblocks       = boot_spec[7];
	fs->nextblock       = fs->dirblocks;		/* First free block */
	fs->maxdir          = (fs->dirblocks * fs->blocksize) / 32;

	/* Max block number is number of usable sectors divided by 
	 * sectors per block */
	fs->maxblock = ((fs->maxtrack - fs->offset) * fs->geom.dg_sectors)
			/ (fs->blocksize / fs->geom.dg_secsize);
	--fs->maxblock;

	if (fs->maxblock < 256) fs->exm = (fs->blocksize / 1024) - 1;
	else			fs->exm = (fs->blocksize / 2048) - 1;

	/* Allocate the buffers */
	fs->bootsec = malloc(fs->geom.dg_secsize + 32 * fs->maxdir);
	if (!fs->bootsec) return DSK_ERR_NOMEM;
	fs->cpmdir = fs->bootsec + fs->geom.dg_secsize;
	memset(fs->cpmdir, 0xE5, 32 * fs->maxdir);

	fs->timestamped = timestamped;
	if (timestamped)
	{
		for (dirent = 3; dirent < fs->maxdir; dirent += 4)
		{
			memset(fs->cpmdir + 32 * dirent, 0, 32);
			fs->cpmdir[32 * dirent] = 0x21;
		}
	}
	memset(fs->bootsec, 0xe5, fs->geom.dg_secsize);
	memcpy(fs->bootsec, boot_spec, 10);

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
	dsk_report("Writing boot sector");
	if (!err) err = dsk_lwrite(fs->drive, &fs->geom, fs->bootsec, 0);
	dsk_report_end();
	return err;
}

unsigned char *p3fs_new_dirent(PLUS3FS fs)
{
	unsigned char *dirent;
	int n;

	for (n = 0; n < fs->maxdir; n++)
	{
		dirent = fs->cpmdir + n * 32;
		if (dirent[0] == 0xE5) return dirent;
	}
	return NULL;
}


unsigned char *p3fs_findextent(PLUS3FS fs, int uid, int extent, char *name)
{
	int n, m, ext;
	unsigned char *dirent;

	for (n = 0; n < fs->maxdir; n++)
	{
		dirent = fs->cpmdir + n * 32;
		if (dirent[0] != uid) continue;
		for (m = 0; m < 11; m++)
		{
			if ((dirent[m + 1] & 0x7F) != (name[m] & 0x7F))
				break;
		}
		if (m == 11)
		{
			ext = dirent[12] + 32 * dirent[14];
			ext /= (fs->exm + 1);
			if (ext == extent) return dirent;
		}
	}
	return NULL;
}

int p3fs_exists(PLUS3FS fs, int uid, char *name)
{
	return (p3fs_findextent(fs, uid, 0, name) != NULL);
}

static int dec2bcd(int v)
{
	return (v % 10) + 16 * (v / 10);
}

static int bcd2dec(int v)
{
	return (v % 10) + 10 * (v / 16);
}

static void stamp2dos(unsigned char *cpmstamp, unsigned char *dosstamp)
{
        time_t secs;
        struct tm *ptime;
        int cpmdays;
	unsigned short dosdate, dostime;

        cpmdays = 256*cpmstamp[1] + cpmstamp[0]; 
        
        secs = (cpmdays + 2921) * 86400; /* Unix day 2921 is CP/M day 0 */
        secs += 3600 * bcd2dec(cpmstamp[2]);
        secs +=   60 * bcd2dec(cpmstamp[3]);

        ptime = gmtime(&secs);

        dosdate =   (ptime->tm_year - 80)         << 9;
        dosdate |= ((ptime->tm_mon + 1) & 0x0F)   << 5;
        dosdate |= (ptime->tm_mday & 0x1F);

        dostime =  (ptime->tm_hour)       << 11;
        dostime |= (ptime->tm_min & 0x3F) <<  5;

	dosstamp[0] = dostime & 0xFF;
	dosstamp[1] = dostime >> 8;
	dosstamp[2] = dosdate & 0xFF;
	dosstamp[3] = dosdate >> 8;
}

static void stamp_now(unsigned char *stamp)
{
	time_t t;
	struct tm *ptm;
	int days, leap;
	memset(stamp, 0, 4);
	time(&t);
	ptm = localtime(&t);
	if (!ptm) return;
	/* CP/M day count */
	if (ptm->tm_year >= 78)
	{
		leap = (ptm->tm_year - 76) / 4;
		leap *= 1461;	/* Days since 1 Jan 1976 */
		/* Days since start of last leap year, +1 for the leap year
		 * itself */
		days = (ptm->tm_year % 4) * 365;
		if (days) ++days;
		days += ptm->tm_yday;
		days += leap;
		days -= 730;	/* Rebase to 1 Jan 1978 */
		stamp[0] = days & 0xFF;
		stamp[1] = days >> 8;
	}
	stamp[2] = dec2bcd(ptm->tm_hour);
	stamp[3] = dec2bcd(ptm->tm_min);
}

dsk_err_t p3fs_creat(PLUS3FS fs, PLUS3FILE *pfp, int uid, char *name)
{
	PLUS3FILE fp;
	int n;

	if (!pfp) return DSK_ERR_BADPTR;
	*pfp = NULL;

	/* Can't overwrite existing, since we have no p3fs_unlink. */
	if (p3fs_exists(fs, uid, name))
	{
		return P3FS_ERR_EXISTS;
	}
	fp = malloc(sizeof(PLUS3FILE_IMPL) + fs->blocksize);
	if (!fp)
	{
		return DSK_ERR_NOMEM;
	}
	fp->fs = fs;
	fp->bufptr = 0;
	fp->extent = 0;
	memset(fp->buf, 0x1A, fs->blocksize);
	fp->dirent = p3fs_new_dirent(fs);
	if (!fp->dirent)
	{
		/* Directory full */
		free(fp);
		return P3FS_ERR_DIRFULL;
	}
	n = (fp->dirent - fp->fs->cpmdir) / 32;
	memset(fp->dirent, 0, 32);
	fp->dirent[0] = uid;
	memcpy(fp->dirent + 1, name, 11);
	*pfp = fp;
		
	if (fs->cpmdir[(n|3) * 32] == 0x21) /* Stamping */
	{	
		fp->stamp = fs->cpmdir + (n|3) * 32 + 1 + ((n & 3) * 10);
		stamp_now(fp->stamp);	/* Create stamp */
		stamp_now(fp->stamp+4);	/* Update stamp */
	}
	else	fp->stamp = NULL;
	return DSK_ERR_OK;	
}


static dsk_lsect_t lookup_block(PLUS3FS fs, int block)
{
	/* Sector of block 0 */
	dsk_lsect_t off = fs->offset * fs->geom.dg_sectors;
	return off + (fs->blocksize / fs->geom.dg_secsize) * block;
}

static dsk_err_t write_block(PLUS3FILE fp, unsigned char *buf, int len)
{
	int n;
	dsk_err_t err;
	dsk_lsect_t blk;

	if (fp->fs->nextblock > fp->fs->maxblock) 
	{
		return P3FS_ERR_DISKFULL;
	}
	blk = lookup_block(fp->fs, fp->fs->nextblock);
	/* Firstly actually write the data */
	for (n = 0; n < fp->fs->blocksize; n += fp->fs->geom.dg_secsize)
	{
		err = dsk_lwrite(fp->fs->drive, &fp->fs->geom, buf + n, blk++);
		if (err) return err;
	}
	/* Now allocate it in the directory entry */
	if (fp->dirent[15] >= 0x80 && 
	    (fp->dirent[12] & fp->fs->exm ) == fp->fs->exm)
	{
		/* Directory entry full. Need a new one. */
		unsigned char *de2 = p3fs_new_dirent(fp->fs);
		if (!de2) return P3FS_ERR_DIRFULL;

		memset(de2, 0, 32);
		memcpy(de2, fp->dirent, 13);
		++de2[12];
		if (de2[12] >= 32)	/* Extent counter overflow */
		{
			++de2[14];
			de2[12] -= 32;
		}
		fp->dirent = de2;
	}
	else if (fp->dirent[15] > 0x80)	/* Next logical extent */
	{
		fp->dirent[15] -= 0x80;
		++fp->dirent[12];
	}
	if (fp->fs->maxblock <= 255)
	{
		for (n = 16; n < 32; n++)
		{
			if (fp->dirent[n] == 0)
			{
				fp->dirent[n] = fp->fs->nextblock;
				break;
			}
		}
	}
	else
	{
		for (n = 16; n < 32; n+= 2)
		{
			if (fp->dirent[n] == 0 && fp->dirent[n+1] == 0)
			{
				fp->dirent[n]   = fp->fs->nextblock & 0xFF;
				fp->dirent[n+1] = fp->fs->nextblock >> 8; 
				break;
			}
		}

	}
	fp->dirent[15] += (len + 127) / 128;
	fp->dirent[13]  = (len & 0x7F);
	++fp->fs->nextblock;
	if (fp->stamp) stamp_now(fp->stamp+4);	/* Update stamp */
	return DSK_ERR_OK;
}


dsk_err_t p3fs_sync(PLUS3FS fs)
{
	int block, n, base;
	dsk_err_t err;

	base = 0;	
	for (block = 0; block < fs->dirblocks; block++)
	{
		dsk_lsect_t blk = lookup_block(fs, block);
		for (n = 0; n < fs->blocksize; n += fs->geom.dg_secsize)
		{
			err = dsk_lwrite(fs->drive, &fs->geom, 
					fs->cpmdir + base, blk++);
			if (err) return err;
			base += fs->geom.dg_secsize;
		}
	}
	return DSK_ERR_OK;
}


dsk_err_t p3fs_flush(PLUS3FILE fp)
{
	int res;

	if (fp->bufptr == 0) return DSK_ERR_OK;
	res = write_block(fp, fp->buf, fp->bufptr);
	fp->bufptr = 0;
	return res;
}

dsk_err_t p3fs_close(PLUS3FILE fp)
{
	int err;

	err = p3fs_flush(fp);
	free(fp);
	return err;
}


dsk_err_t p3fs_putc(unsigned char c, PLUS3FILE fp)
{
	int res;

	if (fp->bufptr >= fp->fs->blocksize)
	{
		res = p3fs_flush(fp);
		if (res) return res;
	}
	fp->buf[fp->bufptr] = c;
	++fp->bufptr;
	return DSK_ERR_OK;
}

unsigned get_alloc(PLUS3FS fs, unsigned char *src)
{
	if (fs->maxblock <= 255) return *src;

	return src[0] + 256 * src[1];
}

static void put_fat(PLUS3FS fs, unsigned cluster, unsigned value)
{
	int offset = (cluster / 2) * 3;

	if (cluster & 1)
	{
		fs->dosfat[offset + 1] &= 0x0F;
		fs->dosfat[offset + 1] |= (value << 4);
		fs->dosfat[offset + 2] = value >> 4;
	}
	else
	{
		fs->dosfat[offset] = value;
		fs->dosfat[offset + 1] &= 0xF0;
		fs->dosfat[offset + 1] |= (value >> 8) & 0x0F;
	}
}


static void label_to_dos(unsigned char *cpm_dirent, unsigned char *dos_dirent)
{
	memcpy(dos_dirent, cpm_dirent + 1, 11);
	dos_dirent[11] = 8;		/* Label */
	stamp2dos(cpm_dirent + 24, dos_dirent + 22);
}

static long p3fs_filesize(PLUS3FS fs, int uid, unsigned char *filename)
{
	int lrbc = 0;
	int extent = 0;
	long size = 0;
	unsigned char *dirent;

	while ((dirent = p3fs_findextent(fs, uid, extent, (char *)filename)))
	{
		if (extent == 0) lrbc = dirent[13] & 0x7F;
		size += (dirent[12] & fs->exm) * 16384L;
		size +=  dirent[15] * 128;
		++extent;
	}
	if (lrbc)
	{
		size -= 0x80;
		size += lrbc;
	}
	return size;
}

/* Given a position in the file, determine what DOS cluster it falls in */
static int find_cluster(PLUS3FS fs, int uid, char *filename, long offset)
{
	int ext, block, multi;
	unsigned char *dirent, *data; 

/*	printf("%s @ %ld: ", filename, offset); */
	ext = (offset / (16384L * (fs->exm + 1)));
/*	printf(" [ex=%d] ", ext); */
	dirent = p3fs_findextent(fs, uid, ext, filename);
	if (!dirent) return 0xFFF;
	offset %= (16384L * (fs->exm + 1));
/*	printf(" [offset=%ld] ", offset); */

	data = dirent + 16;
	while (offset >= fs->blocksize)
	{
		if (fs->maxblock >= 256) data += 2;
		else			 data++;
		offset -= fs->blocksize;
	}
	if (fs->maxblock >= 256) block = data[0] + 256 * data[1];
	else			 block = data[0];

	/* block is now the CP/M block and offset the offset within it. For
	 * CP/M purposes we could stop here. */
/*	printf(" block=%d [%ld] ", block, offset); */
	block -= fs->dirblocks;
	multi = fs->blocksize / fs->clustersize;
	block *= multi;

	while (offset >= fs->clustersize)
	{
		++block;
		offset -= fs->clustersize;
	}
	block += (fs->cluster_offset + 2);
/*	printf("cluster=%d\n", block); */

	return block;
}

static void file_to_dos(PLUS3FS fs, unsigned char *cpm_dirent, unsigned char *dos_dirent)
{
	unsigned char *stamp;
	char filename[12];
	long size, offset;
	int prev, cur;
	int ncpm;

	ncpm = (cpm_dirent - fs->cpmdir) / 32;
	memset(dos_dirent, 0, 32);

	stamp = fs->cpmdir + 32 * (ncpm | 3);
	if (stamp[0] == 0x21)
	{
		stamp += 5;
		stamp += 10 * (ncpm & 3);
		stamp2dos(stamp, dos_dirent + 0x16);
	}
	sprintf(filename, "%-11.11s", cpm_dirent + 1);
	size = p3fs_filesize(fs, cpm_dirent[0], (unsigned char *)filename);
	memcpy(dos_dirent, cpm_dirent + 1, 11);
	dos_dirent[0x1C] = size & 0xFF;
	dos_dirent[0x1D] = (size >> 8) & 0xFF;
	dos_dirent[0x1E] = (size >> 16) & 0xFF;
	dos_dirent[0x1F] = (size >> 24) & 0xFF;

	/* Now allocate the blocks */
	prev = -1;
	offset = 0;
	while (size > 0)
	{
		cur = find_cluster(fs, cpm_dirent[0], filename, offset);
		if (prev == -1)
		{
			dos_dirent[0x1A] = (cur & 0xFF);
			dos_dirent[0x1B] = (cur >> 8) & 0xFF;	
		}
		else
		{
			put_fat(fs, prev, cur);
		}
		prev = cur;
		offset += fs->clustersize;
		size -= fs->clustersize;
	}
	if (prev != -1) put_fat(fs, prev, 0xFFF);
}


dsk_err_t p3fs_dossync(PLUS3FS fs, int format, int dosonly)
{
	dsk_err_t err;
	dsk_lsect_t sec;
	unsigned char *cpm_dirent;
	unsigned char *dos_dirent;
	int ncpm, ndos, dirlen, fatlen, dirsecs;
	int cluster_add;
/* Currently we only support two formats: 180k and 720k. Start by setting
 * up the BPB */
	memset(fs->bootsec + 11, 0, fs->geom.dg_secsize - 11);

	fs->bootsec[11] = fs->geom.dg_secsize & 0xFF;
	fs->bootsec[12] = fs->geom.dg_secsize >> 8;	/* Sector size */
	fs->bootsec[14] = 1;				/* Reserved sectors */
	fs->bootsec[15] = 0;
	fs->bootsec[16] = 2;				/* FAT copies */
	fs->bootsec[18] = 0;
	fs->bootsec[23] = 0;
	fs->bootsec[24] = 9;				/* Sectors / track */
	fs->bootsec[25] = 0;
	fs->bootsec[27] = 0;
	switch(format)
	{
		case 720:	fs->bootsec[13] = 2;	/* Sectors / cluster */
				fs->bootsec[17] = 112;	/* Root dir entries */
				fs->bootsec[19] = 0xA0;
				fs->bootsec[20] = 5;	/* Total sectors */
				fs->bootsec[21] = 0xF9;	/* Media descriptor */
				fs->bootsec[22] = 3;	/* Sectors / FAT */
				fs->bootsec[26] = 2;	/* Heads */
				cluster_add = 10;	/* Skip 10k */
				break;
		case 180:	fs->bootsec[13] = 1;	/* Sectors / cluster */
				fs->bootsec[17] = 64;	/* Root dir entries */
				fs->bootsec[19] = 0x68;
				fs->bootsec[20] = 1;	/* Total sectors */
				fs->bootsec[21] = 0xFC;	/* Media descriptor */
				fs->bootsec[22] = 2;	/* Sectors / FAT */
				fs->bootsec[26] = 1;	/* Heads */
				cluster_add = 4;	/* Skip 2k */
				break;
	}
	fs->clustersize = fs->bootsec[13] * fs->geom.dg_secsize;

	/* DOSONLY: Overwrite the +3DOS drive spec in favour of a full
	 * PCDOS one */
	if (dosonly)
	{
		fs->bootsec[0] = 0xEB;
		fs->bootsec[1] = 0x40;
		fs->bootsec[2] = 0x90;
		memcpy(fs->bootsec + 3, "IBM  3.3", 8);
		fs->bootsec[0x42] = 0xCD;
		fs->bootsec[0x43] = 0x18;
		fs->bootsec[fs->geom.dg_secsize - 2] = 0x55;
		fs->bootsec[fs->geom.dg_secsize - 1] = 0xAA;
	}

	/* Work out size of directory & size of FAT */
	dirlen = fs->bootsec[17] * 32;
	fatlen = fs->bootsec[22] * fs->geom.dg_secsize;
	fs->dosdir = malloc(dirlen + fatlen);
	if (!fs->dosdir) return DSK_ERR_NOMEM;
	memset(fs->dosdir, 0, dirlen + fatlen);
	
	fs->dosfat = fs->dosdir + dirlen;
	fs->dosfat[0] = fs->bootsec[21];
	fs->dosfat[1] = 0xFF;
	fs->dosfat[2] = 0xFF;

	dirsecs = (dirlen + fs->geom.dg_secsize - 1) / fs->geom.dg_secsize;
/*	printf("dirlen=%d dirsecs=%d\n", dirlen, dirsecs); */

	/* Work out what DOS cluster corresponds to the first CP/M block. 
	 * Start with the sector: */
	sec = lookup_block(fs, fs->dirblocks);
/*	printf("Sector of first block=%d\n", sec); */
	/* Now deduct DOS overhead */
	sec -= fs->bootsec[14];	/* Boot sector */
/*	printf("Less boot sector = %d\n", sec); */
	sec -= (fs->bootsec[16] * fs->bootsec[22]);	/* FATs */
/*	printf("Less FATs = %d\n", sec); */
	sec -= dirsecs;
/*	printf("Less dir = %d\n", sec); */

	fs->cluster_offset = sec / fs->bootsec[13];
	if ((fs->cluster_offset * fs->bootsec[13]) != sec)
	{
		return P3FS_ERR_MISALIGN;
	}
	dos_dirent = fs->dosdir;
	ndos = 0;
	for (ncpm = 0; ncpm < fs->maxdir; ncpm++)
	{
		if (ndos >= fs->bootsec[19]) break;	/* DOS dir full */
		cpm_dirent = fs->cpmdir + 32 * ncpm;
		if (cpm_dirent[0] == 0x20)		/* Label */
		{
			label_to_dos(cpm_dirent, dos_dirent);
			++ndos;
			dos_dirent += 32;
			continue;
		}
		if (cpm_dirent[0] < 0x10)		/* File */
		{
			int extent;
		
			extent = cpm_dirent[14] * 32 + cpm_dirent[12];
			extent /= (fs->exm + 1);
			if (!extent) 
			{
				file_to_dos(fs, cpm_dirent, dos_dirent);
				++ndos;
				dos_dirent += 32;
			}
		}
	}
/* Slap 'bad sector' over the CP/M directory. */
	if (!dosonly)
	{
		ncpm = fs->dirblocks * (fs->blocksize / fs->clustersize);
		sec = fs->cluster_offset + 2;	/* First real cluster */
		for (ndos = ncpm - 1; ndos >= 0; ndos--)
		{
			put_fat(fs, --sec, 0xFF7);
		}
	}
/* Write the results */
	sec = 0;
	err = dsk_lwrite(fs->drive, &fs->geom, fs->bootsec, sec++);
	if (!err) for (ndos = 0; ndos < fs->bootsec[16]; ndos++)
	{
		for (ncpm = 0; ncpm < fatlen; ncpm += fs->geom.dg_secsize)
		{
			err = dsk_lwrite(fs->drive, &fs->geom,
					fs->dosfat + ncpm, sec++);
			if (err) break;	
		}
	}
	if (!err) for (ndos = 0; ndos < dirlen; ndos += fs->geom.dg_secsize)
	{
		err = dsk_lwrite(fs->drive, &fs->geom, fs->dosdir + ndos, sec++);
	}
	return err;
}


dsk_err_t p3fs_setlabel(PLUS3FS fs, char *name)
{
	unsigned char *dirent;

	if (!fs->timestamped) return DSK_ERR_OK;

	dirent = p3fs_new_dirent(fs);
	if (!dirent) return P3FS_ERR_DIRFULL;
	memset(dirent, 0, 32);
	dirent[0] = 0x20;
	memcpy(dirent + 1, name, 11);
	dirent[12] = 0x31;	/* Label exists: Create & update stamps */	   
	stamp_now(dirent + 24);
	stamp_now(dirent + 28);
	return DSK_ERR_OK;	
}

