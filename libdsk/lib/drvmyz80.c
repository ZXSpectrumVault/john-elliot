/***************************************************************************
 *                                                                         *
 *    LIBDSK: General floppy and diskimage access library                  *
 *    Copyright (C) 2001  John Elliott <seasip.webmaster@gmail.com>            *
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

/* This driver is for a MYZ80 hard drive image, which has a fixed geometry:
 * 128 sectors, 1 head, 64 cylinders, 1k/sector, with 256 bytes of 0xE5 
 * before the first sector. */

#include <stdio.h>
#include "libdsk.h"
#include "ldbs.h"
#include "drvi.h"
#include "drvmyz80.h"

/* This struct contains function pointers to the driver's functions, and the
 * size of its DSK_DRIVER subclass */

DRV_CLASS dc_myz80 = 
{
	sizeof(MYZ80_DSK_DRIVER),
	NULL,		/* superclass */
	"myz80\0MYZ80\0",
	"MYZ80 hard drive driver",
	myz80_open,	/* open */
	myz80_creat,	/* create new */
	myz80_close,	/* close */
	myz80_read,	/* read sector, working from physical address */
	myz80_write,	/* write sector, working from physical address */
	myz80_format,	/* format track, physical */
	myz80_getgeom,	/* Get geometry */
	NULL,		/* sector ID */
	myz80_xseek,	/* Seek to track */
	myz80_status,	/* Get drive status */
	NULL, 		/* xread */
	NULL, 		/* xwrite */
	NULL, 		/* tread */
	NULL, 		/* xtread */
	NULL,		/* option_enum */
	NULL,		/* option_set */
	NULL,		/* option_get */
	NULL,		/* trackids */
	NULL,		/* rtread */
	myz80_to_ldbs,	/* export as LDBS */
	myz80_from_ldbs	/* import as LDBS */
};

dsk_err_t myz80_open(DSK_DRIVER *self, const char *filename)
{
	MYZ80_DSK_DRIVER *mzself;
	unsigned char header[256];
	int n;
	
	/* Sanity check: Is this meant for our driver? */
	if (self->dr_class != &dc_myz80) return DSK_ERR_BADPTR;
	mzself = (MYZ80_DSK_DRIVER *)self;

	mzself->mz_fp = fopen(filename, "r+b");
	if (!mzself->mz_fp) 
	{
		mzself->mz_readonly = 1;
		mzself->mz_fp = fopen(filename, "rb");
	}
	if (!mzself->mz_fp) return DSK_ERR_NOTME;
	if (fread(header, 1, 256, mzself->mz_fp) < 256) 
	{
		fclose(mzself->mz_fp);
		return DSK_ERR_NOTME;
	}
	for (n = 0; n < 256; n++) if (header[n] != 0xE5) 
	{
		fclose(mzself->mz_fp);
		return DSK_ERR_NOTME;
	}
/* v0.9.5: Record exact size, so we can tell if we're writing off the end
 * of the file. Under Windows, writing off the end of the file fills the 
 * gaps with random data, which can cause mess to appear in the directory;
 * and under UNIX, the entire directory is filled with zeroes. */
	if (fseek(mzself->mz_fp, 0, SEEK_END)) return DSK_ERR_SYSERR;
	mzself->mz_filesize = ftell(mzself->mz_fp);

	/* Set up CP/M filesystem parameters */
	dsk_isetoption(self, "FS:CP/M:BSH", 5, 1);
	dsk_isetoption(self, "FS:CP/M:BLM", 31, 1);
	dsk_isetoption(self, "FS:CP/M:EXM", 1, 1); 
	dsk_isetoption(self, "FS:CP/M:DSM", 0x7ff, 1);
	dsk_isetoption(self, "FS:CP/M:DRM", 0x3ff, 1);
	dsk_isetoption(self, "FS:CP/M:AL0", 0xFF, 1);
	dsk_isetoption(self, "FS:CP/M:AL1", 0, 1);
	dsk_isetoption(self, "FS:CP/M:CKS", (int)0x8000, 1);
	dsk_isetoption(self, "FS:CP/M:OFF", 0, 1);

	return DSK_ERR_OK;
}


dsk_err_t myz80_creat(DSK_DRIVER *self, const char *filename)
{
	MYZ80_DSK_DRIVER *mzself;
	int n;
	
	/* Sanity check: Is this meant for our driver? */
	if (self->dr_class != &dc_myz80) return DSK_ERR_BADPTR;
	mzself = (MYZ80_DSK_DRIVER *)self;

	mzself->mz_fp = fopen(filename, "w+b");
	mzself->mz_readonly = 0;
	if (!mzself->mz_fp) return DSK_ERR_SYSERR;
	for (n = 0; n < 256; n++) 
	{
		if (fputc(0xE5, mzself->mz_fp) == EOF)
		{
			fclose(mzself->mz_fp);
			return DSK_ERR_SYSERR;
		}

	}
	dsk_isetoption(self, "FS:CP/M:BSH", 5, 1);
	dsk_isetoption(self, "FS:CP/M:BLM", 31, 1);
	dsk_isetoption(self, "FS:CP/M:EXM", 1, 1); 
	dsk_isetoption(self, "FS:CP/M:DSM", 0x7ff, 1);
	dsk_isetoption(self, "FS:CP/M:DRM", 0x3ff, 1);
	dsk_isetoption(self, "FS:CP/M:AL0", 0xFF, 1);
	dsk_isetoption(self, "FS:CP/M:AL1", 0, 1);
	dsk_isetoption(self, "FS:CP/M:CKS", (int)0x8000, 1);
	dsk_isetoption(self, "FS:CP/M:OFF", 0, 1);
	return DSK_ERR_OK;
}


dsk_err_t myz80_close(DSK_DRIVER *self)
{
	MYZ80_DSK_DRIVER *mzself;

	if (self->dr_class != &dc_myz80) return DSK_ERR_BADPTR;
	mzself = (MYZ80_DSK_DRIVER *)self;

	if (mzself->mz_fp) 
	{
		if (fclose(mzself->mz_fp) == EOF) return DSK_ERR_SYSERR;
		mzself->mz_fp = NULL;
	}
	return DSK_ERR_OK;	
}


dsk_err_t myz80_read(DSK_DRIVER *self, const DSK_GEOMETRY *geom,
                             void *buf, dsk_pcyl_t cylinder,
                              dsk_phead_t head, dsk_psect_t sector)
{
	MYZ80_DSK_DRIVER *mzself;
	long offset;
	unsigned aread;

	if (!buf || !self || !geom || self->dr_class != &dc_myz80) return DSK_ERR_BADPTR;
	mzself = (MYZ80_DSK_DRIVER *)self;

	if (!mzself->mz_fp) return DSK_ERR_NOTRDY;

	/* Convert from physical to logical sector, using the fixed geometry. */
	offset = (131072L * cylinder) + (1024L * sector) + 256;

	if (fseek(mzself->mz_fp, offset, SEEK_SET)) return DSK_ERR_SYSERR;

	/* v0.4.3: MYZ80 disc files can be shorter than the full 8Mb. If so,
	 *        the missing sectors are all assumed to be full of 0xE5s.
	 *        Unlike in "raw" files, it is not an error to try to read 
	 *        a missing sector. */
	aread = fread(buf, 1, geom->dg_secsize, mzself->mz_fp);
	while (aread < geom->dg_secsize)
	{
		((unsigned char *)buf)[aread++] = 0xE5;	
	}
	return DSK_ERR_OK;
}



dsk_err_t myz80_write(DSK_DRIVER *self, const DSK_GEOMETRY *geom,
                             const void *buf, dsk_pcyl_t cylinder,
                              dsk_phead_t head, dsk_psect_t sector)
{
	MYZ80_DSK_DRIVER *mzself;
	unsigned long offset;

	if (!buf || !self || !geom || self->dr_class != &dc_myz80) return DSK_ERR_BADPTR;
	mzself = (MYZ80_DSK_DRIVER *)self;

	if (!mzself->mz_fp) return DSK_ERR_NOTRDY;
	if (mzself->mz_readonly) return DSK_ERR_RDONLY;

	/* Convert from physical to logical sector, using the fixed geometry. */
	offset = (131072L * cylinder) + (1024L * sector) + 256;

	/* v0.9.5: If the file is extended, it must be with zeroes. Otherwise
	 * the intervening blocks get filled with zeroes (Unix) / 
	 * rubbish (Windows). Not a problem for data blocks, but dangerous
	 * for the directory. */
	if (mzself->mz_filesize < offset)
	{
		if (fseek(mzself->mz_fp, mzself->mz_filesize, SEEK_SET)) return DSK_ERR_SYSERR;
		while (mzself->mz_filesize < (offset + geom->dg_secsize))
		{
			if (fputc(0xE5, mzself->mz_fp) == EOF) return DSK_ERR_SYSERR;
			++mzself->mz_filesize;
		}
	}	
	if (fseek(mzself->mz_fp, offset, SEEK_SET)) return DSK_ERR_SYSERR;

	if (fwrite(buf, 1, geom->dg_secsize, mzself->mz_fp) < geom->dg_secsize)
	{
		return DSK_ERR_NOADDR;
	}
	if (fseek(mzself->mz_fp, 0, SEEK_END)) return DSK_ERR_SYSERR;
	mzself->mz_filesize = ftell(mzself->mz_fp);
	return DSK_ERR_OK;
}


dsk_err_t myz80_format(DSK_DRIVER *self, DSK_GEOMETRY *geom,
                                dsk_pcyl_t cylinder, dsk_phead_t head,
                                const DSK_FORMAT *format, unsigned char filler)
{
/*
 * Note that we completely ignore the "format" parameter, since raw MYZ80
 * images don't hold track headers.
 */
	MYZ80_DSK_DRIVER *mzself;
	unsigned long offset;
	long trklen;

	(void)format;
	if (!self || !geom || self->dr_class != &dc_myz80) return DSK_ERR_BADPTR;
	mzself = (MYZ80_DSK_DRIVER *)self;

	if (!mzself->mz_fp) return DSK_ERR_NOTRDY;
	if (mzself->mz_readonly) return DSK_ERR_RDONLY;

	/* Convert from physical to logical sector. However, unlike the dg_* 
	 * functions, this _always_ uses "SIDES_ALT" mapping; this is the 
	 * mapping that both the Linux and NT floppy drivers use to convert 
	 * offsets back into C/H/S. */
	/* Convert from physical to logical sector, using the fixed geometry. */
	offset = (131072L * cylinder) + 256;
	trklen = 131072L;

	/* v0.9.5: If the file is extended, it must be with zeroes. Otherwise
	 * the intervening blocks get filled with zeroes (Unix) / 
	 * rubbish (Windows). Not a problem for data blocks, but dangerous
	 * for the directory. */
	if (mzself->mz_filesize < offset)
	{
		if (fseek(mzself->mz_fp, mzself->mz_filesize, SEEK_SET)) return DSK_ERR_SYSERR;
		while (mzself->mz_filesize < offset + trklen)
		{
			if (fputc(0xE5, mzself->mz_fp) == EOF) return DSK_ERR_SYSERR;
			++mzself->mz_filesize;
		}
	}	
	if (fseek(mzself->mz_fp, offset, SEEK_SET)) return DSK_ERR_SYSERR;

	while (trklen--) 
		if (fputc(filler, mzself->mz_fp) == EOF) return DSK_ERR_SYSERR;	
	if (fseek(mzself->mz_fp, 0, SEEK_END)) return DSK_ERR_SYSERR;
	mzself->mz_filesize = ftell(mzself->mz_fp);

	return DSK_ERR_OK;
}

	
dsk_err_t myz80_getgeom(DSK_DRIVER *self, DSK_GEOMETRY *geom)
{
	if (!geom || !self || self->dr_class != &dc_myz80) return DSK_ERR_BADPTR;
	geom->dg_sidedness = SIDES_ALT;
	geom->dg_cylinders = 64;
	geom->dg_heads     = 1;
	geom->dg_sectors   = 128;
	geom->dg_secbase   = 0;
	geom->dg_secsize   = 1024;
	geom->dg_datarate  = RATE_ED;	/* From here on, values are dummy 
					 * values. It's highly unlikely 
					 * anyone will be able to format a
					 * floppy to this format! */
	geom->dg_rwgap     = 0x2A;
	geom->dg_fmtgap    = 0x52;
	geom->dg_fm        = RECMODE_MFM;
	geom->dg_nomulti   = 0;
	return DSK_ERR_OK;
}

dsk_err_t myz80_xseek(DSK_DRIVER *self, const DSK_GEOMETRY *geom, 
			dsk_pcyl_t cylinder, dsk_phead_t head)
{
	if (cylinder >= 64) return DSK_ERR_SEEKFAIL;
	return DSK_ERR_OK;
}


dsk_err_t myz80_status(DSK_DRIVER *self, const DSK_GEOMETRY *geom,
                      dsk_phead_t head, unsigned char *result)
{
        MYZ80_DSK_DRIVER *mzself;

        if (!self || !geom || self->dr_class != &dc_myz80) return DSK_ERR_BADPTR;
        mzself = (MYZ80_DSK_DRIVER *)self;

        if (!mzself->mz_fp) *result &= ~DSK_ST3_READY;
        if (mzself->mz_readonly) *result |= DSK_ST3_RO;
	return DSK_ERR_OK;
}

static const LDBS_DPB myz80_dpb =
{
	128,	/* SPT */
	5, 	/* BSH (4k blocks) */
	31, 	/* BLM */
	1, 	/* EXM */
	0x7FF,	/* DSM (2048 * 4k) */
	0x3FF,	/* DRM */
	{ 0xFF, 0, }, 	/* AL0 / AL1 */
	0x8000, /* CKS */
	0, 	/* OFF */
	3, 	/* PSH */
	7 	/* PHM */
};


/* Convert to LDBS format. */
dsk_err_t myz80_to_ldbs(DSK_DRIVER *self, struct ldbs **result, 
		DSK_GEOMETRY *geom)
{
	dsk_err_t err;
	dsk_pcyl_t cyl;
	dsk_psect_t sec;
	unsigned char secbuf[1024];
	DSK_GEOMETRY dg;	/* The fixed geometry we'll be using */
	int n;

        if (!self || !result || self->dr_class != &dc_myz80) 
		return DSK_ERR_BADPTR;
	err = ldbs_new(result, NULL, LDBS_DSK_TYPE);
	if (err) return err;

	err = myz80_getgeom(self, &dg);
	/* Write a fixed geometry record */
	if (!err) err = ldbs_put_geometry(*result, &dg);
	/* And a fixed DPB record */
	if (!err) err = ldbs_put_dpb(*result, &myz80_dpb);
	if (err)
	{
		ldbs_close(result);
		return err;
	}	

	/* Format 64 tracks each with 128 sectors */
	for (cyl = 0; cyl < 64; cyl++)
	{
		LDBS_TRACKHEAD *trkh = ldbs_trackhead_alloc(128);
	
		if (!trkh)
		{
			ldbs_close(result);
			return DSK_ERR_NOMEM;
		}
		trkh->datarate = 3;
		trkh->recmode = 2;
		trkh->gap3 = 0x52;
		trkh->filler = 0xE5;
		for (sec = 0; sec < 128; sec++)
		{
/* For each sector in the MYZ80 drive image file, read it in */
			err = myz80_read(self, &dg, secbuf, cyl, 0, sec);
			if (err)
			{
				ldbs_free(trkh);
				ldbs_close(result);
				return err;
			}
/* Add it to the track header */
			trkh->sector[sec].id_cyl = cyl;
			trkh->sector[sec].id_head = 0;
			trkh->sector[sec].id_sec = sec;
			trkh->sector[sec].id_psh = 3;
			trkh->sector[sec].copies = 0;
			for (n = 1; n < 1024; n++)
				if (secbuf[n] != secbuf[0])
			{
				trkh->sector[sec].copies = 1;
				break;
			}
/* And write it out (if it's not blank) */
			if (!trkh->sector[sec].copies)
			{
				trkh->sector[sec].filler = secbuf[0];
			}
			else
			{
				char id[4];
				ldbs_encode_secid(id, cyl, 0, sec);
				err = ldbs_putblock(*result, 
					&trkh->sector[sec].blockid, id, secbuf,
					1024);
				if (err)
				{
					ldbs_free(trkh);
					ldbs_close(result);
					return err;
				}
			}
		}
		/* All sectors transferred */
		err = ldbs_put_trackhead(*result, trkh, cyl, 0);
		ldbs_free(trkh);
		if (err)
		{
			ldbs_close(result);
			return err;
		}
	}
	return ldbs_sync(*result);
}

static dsk_err_t myz80_from_ldbs_callback(PLDBS ldbs, dsk_pcyl_t cyl,
	 dsk_phead_t head, LDBS_SECTOR_ENTRY *se, LDBS_TRACKHEAD *th,
	 void *param)
{
	MYZ80_DSK_DRIVER *mzself = param;
	unsigned char secbuf[1024];
	dsk_err_t err;
	int n, blank;
	long offset = (131072L * cyl) + (1024L * se->id_sec) + 256;
	size_t len;

	if (mzself->mz_readonly) return DSK_ERR_RDONLY;

	/* Ignore cylinders / heads / sectors that are out of range */
	if (cyl > 63 || head > 0 || se->id_sec > 127) return DSK_ERR_OK;

	memset(secbuf, se->filler, 1024);
	/* Load the sector */
	if (se->copies)
	{
		len = 1024;
		err = ldbs_getblock(ldbs, se->blockid, NULL, secbuf, &len);
		if (err != DSK_ERR_OK && err != DSK_ERR_OVERRUN)
		{
			return err;
		}
	}
	blank = 1;	
	for (n = 0; n < 1024; n++)
	{
		if (secbuf[n] != 0xE5) blank = 0;
	}
	/* Don't write blank sectors past the end of the file */
	if (offset >= (long)(mzself->mz_filesize) && blank)
	{
		return DSK_ERR_OK;
	}
	/* Pad the file out to the right size */
	if ((long)(mzself->mz_filesize) < offset)
	{
		if (fseek(mzself->mz_fp, mzself->mz_filesize, SEEK_SET)) 
			return DSK_ERR_SYSERR;
		while ((long)(mzself->mz_filesize) < offset)
		{
			if (fputc(0xE5, mzself->mz_fp) == EOF) 
				return DSK_ERR_SYSERR;
			++mzself->mz_filesize;
		}
	}
	/* Write the sector */
	if (fseek(mzself->mz_fp, offset, SEEK_SET) ||
	    fwrite(secbuf, 1, 1024, mzself->mz_fp) < 1024) 
		return DSK_ERR_SYSERR;

	if ((unsigned long)(offset + 1024) > mzself->mz_filesize)
	{
		mzself->mz_filesize = offset + 1024;
	}	
	return DSK_ERR_OK;
}


/* Convert from LDBS format. */
dsk_err_t myz80_from_ldbs(DSK_DRIVER *self, struct ldbs *source, DSK_GEOMETRY *geom)
{
	MYZ80_DSK_DRIVER *mzself;
	long pos;

	if (!self || !source || self->dr_class != &dc_myz80) 
		return DSK_ERR_BADPTR;
	mzself = (MYZ80_DSK_DRIVER *)self;

	/* Erase anything existing in the file */
	if (fseek(mzself->mz_fp, 0, SEEK_SET)) return DSK_ERR_SYSERR;

	for (pos = 0; pos < (long)(mzself->mz_filesize); pos++)
	{
		if (fputc(0xE5, mzself->mz_fp) == EOF) return DSK_ERR_SYSERR;
	}

	/* And populate with whatever is in the blockstore */	
	return ldbs_all_sectors(source, myz80_from_ldbs_callback,
				SIDES_ALT, mzself);
}

