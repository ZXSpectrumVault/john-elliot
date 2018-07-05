/***************************************************************************
 *                                                                         *
 *    LIBDSK: General floppy and diskimage access library                  *
 *    Copyright (C) 2001,2017  John Elliott <seasip.webmaster@gmail.com>   *
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

/* This driver is used to access Microbee files written by the NanoWasp 
 * emulator. This is similar to the POSIX driver, but sectors are stored in 
 * the file in SIDES_OUTOUT rather than SIDES_ALT order (though the MicroBee
 * actually writes them in SIDES_ALT order). Additionally, I'm reversing the 
 * sector skew so that sectors come out in the logical order, though this is
 * actually an abuse of libdsk (skewing should be done at the cpmtools level). 
 * However, cpmtools doesn't support the type of skewing done by the microbee
 * (sector 1 doesn't map to sector 1). */

#include <stdio.h>
#include "libdsk.h"
#include "ldbs.h"
#include "drvi.h"
#include "drvnwasp.h"


/* This struct contains function pointers to the driver's functions, and the
 * size of its DSK_DRIVER subclass */

DRV_CLASS dc_nwasp = 
{
	sizeof(NWASP_DSK_DRIVER),
	NULL,		/* superclass */
	"nanowasp\0nwasp\0",
	"NanoWasp image file driver",
	nwasp_open,	/* open */
	nwasp_creat,	/* create new */
	nwasp_close,	/* close */
	nwasp_read,	/* read sector, working from physical address */
	nwasp_write,	/* write sector, working from physical address */
	nwasp_format,	/* format track, physical */
	nwasp_getgeom,	/* get geometry */
	NULL,		/* sector ID */
	nwasp_xseek,	/* seek to track */
	nwasp_status,	/* drive status */
	NULL, 		/* xread */
	NULL, 		/* xwrite */
	NULL, 		/* tread */
	NULL, 		/* xtread */
	NULL,		/* option_enum */
	NULL,		/* option_set */
	NULL,		/* option_get */
	NULL,		/* trackids */
	NULL,		/* rtread */
	nwasp_to_ldbs,	/* export as LDBS */
	nwasp_from_ldbs	/* import as LDBS */
};

dsk_err_t nwasp_open(DSK_DRIVER *self, const char *filename)
{
	NWASP_DSK_DRIVER *nwself;
	
	/* Sanity check: Is this meant for our driver? */
	if (self->dr_class != &dc_nwasp) return DSK_ERR_BADPTR;
	nwself = (NWASP_DSK_DRIVER *)self;

	nwself->nw_fp = fopen(filename, "r+b");
	if (!nwself->nw_fp) 
	{
		nwself->nw_readonly = 1;
		nwself->nw_fp = fopen(filename, "rb");
	}
	if (!nwself->nw_fp) return DSK_ERR_NOTME;
/* v0.9.5: Record exact size, so we can tell if we're writing off the end
 * of the file. Under Windows, writing off the end of the file fills the 
 * gaps with random data, which can cause mess to appear in the directory;
 * and under UNIX, the entire directory is filled with zeroes. */
        if (fseek(nwself->nw_fp, 0, SEEK_END)) return DSK_ERR_SYSERR;
        nwself->nw_filesize = ftell(nwself->nw_fp);

	return DSK_ERR_OK;
}


dsk_err_t nwasp_creat(DSK_DRIVER *self, const char *filename)
{
	NWASP_DSK_DRIVER *nwself;
	
	/* Sanity check: Is this meant for our driver? */
	if (self->dr_class != &dc_nwasp) return DSK_ERR_BADPTR;
	nwself = (NWASP_DSK_DRIVER *)self;

	nwself->nw_fp = fopen(filename, "w+b");
	nwself->nw_readonly = 0;
	if (!nwself->nw_fp) return DSK_ERR_SYSERR;
	nwself->nw_filesize = 0;
	return DSK_ERR_OK;
}


dsk_err_t nwasp_close(DSK_DRIVER *self)
{
	NWASP_DSK_DRIVER *nwself;

	if (self->dr_class != &dc_nwasp) return DSK_ERR_BADPTR;
	nwself = (NWASP_DSK_DRIVER *)self;

	if (nwself->nw_fp) 
	{
		if (fclose(nwself->nw_fp) == EOF) return DSK_ERR_SYSERR;
		nwself->nw_fp = NULL;
	}
	return DSK_ERR_OK;	
}

static const int skew[10] = { 1,4,7,0,3,6,9,2,5,8 };


dsk_err_t nwasp_read(DSK_DRIVER *self, const DSK_GEOMETRY *geom,
                             void *buf, dsk_pcyl_t cylinder,
                              dsk_phead_t head, dsk_psect_t sector)
{
	NWASP_DSK_DRIVER *nwself;
	long offset;

	if (!buf || !self || !geom || self->dr_class != &dc_nwasp) return DSK_ERR_BADPTR;
	nwself = (NWASP_DSK_DRIVER *)self;

	if (!nwself->nw_fp) return DSK_ERR_NOTRDY;

	/* Sectors are always numbered in the 1-10 range */
	if (sector < 1 || sector > 10) return DSK_ERR_NOADDR;

	/* Convert from physical to logical sector. However, unlike the dg_* 
	 * functions, this _always_ uses "SIDES_OUTOUT" mapping */
	offset = 204800L * head + 5120L * cylinder + 512 * skew[sector-1];

	if (fseek(nwself->nw_fp, offset, SEEK_SET)) return DSK_ERR_SYSERR;

	if (fread(buf, 1, geom->dg_secsize, nwself->nw_fp) < geom->dg_secsize)
	{
		return DSK_ERR_NOADDR;
	}
	return DSK_ERR_OK;
}


static dsk_err_t seekto(NWASP_DSK_DRIVER *self, unsigned long offset)
{
	/* 0.9.5: Fill any "holes" in the file with 0xE5. Otherwise, UNIX would
	 * fill them with zeroes and Windows would fill them with whatever
	 * happened to be lying around */
	if (self->nw_filesize < offset)
	{
		if (fseek(self->nw_fp, self->nw_filesize, SEEK_SET)) return DSK_ERR_SYSERR;
		while (self->nw_filesize < offset)
		{
			if (fputc(0xE5, self->nw_fp) == EOF) return DSK_ERR_SYSERR;
			++self->nw_filesize;
		}
	}
	if (fseek(self->nw_fp, offset, SEEK_SET)) return DSK_ERR_SYSERR;
	return DSK_ERR_OK;
}

dsk_err_t nwasp_write(DSK_DRIVER *self, const DSK_GEOMETRY *geom,
                             const void *buf, dsk_pcyl_t cylinder,
                              dsk_phead_t head, dsk_psect_t sector)
{
	NWASP_DSK_DRIVER *nwself;
	unsigned long offset;
	dsk_err_t err;

	if (!buf || !self || !geom || self->dr_class != &dc_nwasp) return DSK_ERR_BADPTR;
	nwself = (NWASP_DSK_DRIVER *)self;

	if (!nwself->nw_fp) return DSK_ERR_NOTRDY;
	if (nwself->nw_readonly) return DSK_ERR_RDONLY;
	/* Sectors are always numbered in the 1-10 range */
	if (sector < 1 || sector > 10) return DSK_ERR_NOADDR;

	/* Convert from physical to logical sector. However, unlike the dg_* 
	 * functions, this _always_ uses "SIDES_OUTOUT" mapping */
	offset = 204800L * head + 5120L * cylinder + 512 * skew[sector-1];

	err = seekto(nwself, offset);
	if (err) return err;

	if (fwrite(buf, 1, geom->dg_secsize, nwself->nw_fp) < geom->dg_secsize)
	{
		return DSK_ERR_NOADDR;
	}
	if (nwself->nw_filesize < offset + geom->dg_secsize)
		nwself->nw_filesize = offset + geom->dg_secsize;
	return DSK_ERR_OK;
}


dsk_err_t nwasp_format(DSK_DRIVER *self, DSK_GEOMETRY *geom,
                                dsk_pcyl_t cylinder, dsk_phead_t head,
                                const DSK_FORMAT *format, unsigned char filler)
{
/*
 * Note that we completely ignore the "format" parameter, since raw NWASP
 * images don't hold track headers.
 */
	NWASP_DSK_DRIVER *nwself;
	unsigned long offset;
	unsigned long trklen;
	dsk_err_t err;

   (void)format;
	if (!self || !geom || self->dr_class != &dc_nwasp) return DSK_ERR_BADPTR;
	nwself = (NWASP_DSK_DRIVER *)self;

	if (!nwself->nw_fp) return DSK_ERR_NOTRDY;
	if (nwself->nw_readonly) return DSK_ERR_RDONLY;

	/* Convert from physical to logical sector. However, unlike the dg_* 
	 * functions, this _always_ uses "SIDES_OUTOUT" mapping */
	offset = 204800L * head + 5120L * cylinder;/* + 512 * skew[sector-1];*/
	trklen = 5120L;

	err = seekto(nwself, offset);
	if (err) return err;
	if (nwself->nw_filesize < offset + trklen)
		nwself->nw_filesize = offset + trklen;

	while (trklen--) 
		if (fputc(filler, nwself->nw_fp) == EOF) return DSK_ERR_SYSERR;	

	return DSK_ERR_OK;
}

	

dsk_err_t nwasp_xseek(DSK_DRIVER *self, const DSK_GEOMETRY *geom,
                      dsk_pcyl_t cylinder, dsk_phead_t head)
{
	NWASP_DSK_DRIVER *nwself;
	long offset;

	if (!self || !geom || self->dr_class != &dc_nwasp) return DSK_ERR_BADPTR;
	nwself = (NWASP_DSK_DRIVER *)self;

	if (!nwself->nw_fp) return DSK_ERR_NOTRDY;

	if (cylinder >= geom->dg_cylinders || head >= geom->dg_heads)
		return DSK_ERR_SEEKFAIL;

	/* Convert from physical to logical sector. However, unlike the dg_* 
	 * functions, this _always_ uses "SIDES_OUTOUT" mapping */
	offset = 204800L * head + 5120L * cylinder;/* + 512 * skew[sector-1];*/
	
	if (fseek(nwself->nw_fp, offset, SEEK_SET)) return DSK_ERR_SEEKFAIL;

	return DSK_ERR_OK;
}

dsk_err_t nwasp_status(DSK_DRIVER *self, const DSK_GEOMETRY *geom,
                      dsk_phead_t head, unsigned char *result)
{
	NWASP_DSK_DRIVER *nwself;

	if (!self || !geom || self->dr_class != &dc_nwasp) return DSK_ERR_BADPTR;
	nwself = (NWASP_DSK_DRIVER *)self;

	if (!nwself->nw_fp) *result &= ~DSK_ST3_READY;
	if (nwself->nw_readonly) *result |= DSK_ST3_RO;
	return DSK_ERR_OK;
}


dsk_err_t nwasp_getgeom(DSK_DRIVER *self, DSK_GEOMETRY *geom)
{
        if (!geom || !self || self->dr_class != &dc_nwasp) return DSK_ERR_BADPTR;
        geom->dg_sidedness = SIDES_ALT;
        geom->dg_cylinders = 40;
        geom->dg_heads     = 2;
        geom->dg_sectors   = 10;
        geom->dg_secbase   = 1;
        geom->dg_secsize   = 512;
        geom->dg_datarate  = RATE_DD;   /* These values (for accessing  
                                         * mbee floppies) are untested. */
        geom->dg_rwgap     = 0x0C;
        geom->dg_fmtgap    = 0x17;
        geom->dg_fm        = RECMODE_MFM;
        geom->dg_nomulti   = 0;
        return DSK_ERR_OK;
}

static LDBS_DPB nwasp_dpb = 
{
	0x28,	/* SPT */
	0x04,	/* BSH */
	0x0F,	/* BLM */
	0x01,	/* EXM */
	0xC2,	/* DSM */
	0x7F,	/* DRM */
	{ 0xC0, 0x00 },	/* AL0 / AL1 */
	0x20,	/* CKS */
	0x02,	/* OFF */
	0x02,	/* PSH */
	0x03	/* PHM */	
};

dsk_err_t nwasp_to_ldbs(DSK_DRIVER *self, struct ldbs **result, DSK_GEOMETRY *geom)
{
	dsk_err_t err;
	dsk_pcyl_t cyl;
	dsk_phead_t head;
	dsk_psect_t sec;
	unsigned char secbuf[512];
	DSK_GEOMETRY dg;	/* The fixed geometry we'll be using */
	int n;

        if (!self || !result || self->dr_class != &dc_nwasp) 
		return DSK_ERR_BADPTR;
	err = ldbs_new(result, NULL, LDBS_DSK_TYPE);
	if (err) return err;

	err = nwasp_getgeom(self, &dg);
	/* Write a fixed geometry record */
	if (!err) err = ldbs_put_geometry(*result, &dg);
	/* And a fixed DPB record. */
	if (!err) err = ldbs_put_dpb(*result, &nwasp_dpb);

	if (err)
	{
		ldbs_close(result);
		return err;
	}	

	/* Format 40 cylinders, 2 heads, 10 sectors */
	for (cyl = 0; cyl < 40; cyl++)
	{
		for (head = 0; head < 2; head++)
		{
			LDBS_TRACKHEAD *trkh = ldbs_trackhead_alloc(10);
	
			if (!trkh)
			{
				ldbs_close(result);
				return DSK_ERR_NOMEM;
			}
			trkh->datarate = 1;
			trkh->recmode = 2;
			trkh->gap3 = 0x17;
			trkh->filler = 0xE5;
			for (sec = 0; sec < 10; sec++)
			{
/* For each sector in the Nanowasp drive image file, read it in */
				err = nwasp_read(self, &dg, secbuf, cyl, head, sec + 1);
				if (err)
				{
					ldbs_free(trkh);
					ldbs_close(result);
					return err;
				}
/* Add it to the track header */
				trkh->sector[sec].id_cyl = cyl;
				trkh->sector[sec].id_head = head;
				trkh->sector[sec].id_sec = sec + 1;
				trkh->sector[sec].id_psh = 2;
				trkh->sector[sec].copies = 0;
				for (n = 1; n < 512; n++)
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
					ldbs_encode_secid(id, cyl, head, sec);
					err = ldbs_putblock(*result, 
						&trkh->sector[sec].blockid, 
							id, secbuf, 512);
					if (err)
					{
						ldbs_free(trkh);
						ldbs_close(result);
						return err;
					}
				}
			}
			/* All sectors transferred */
			err = ldbs_put_trackhead(*result, trkh, cyl, head);
			ldbs_free(trkh);
			if (err)
			{
				ldbs_close(result);
				return err;
			}
		}
	}
	return ldbs_sync(*result);
}



static dsk_err_t nwasp_from_ldbs_callback(PLDBS ldbs, dsk_pcyl_t cyl,
	 dsk_phead_t head, LDBS_SECTOR_ENTRY *se, LDBS_TRACKHEAD *th,
	 void *param)
{
	NWASP_DSK_DRIVER *nwasp_self = param;
	unsigned char secbuf[512];
	dsk_err_t err;
	long offset;
	size_t len;

	/* Ignore cylinders/heads/sectors that are out of range */
	if (cyl > 39 || head > 1 || se->id_sec < 1 || se->id_sec > 10)
	{
		return DSK_ERR_OK;
	}

	offset = 204800L * head + 5120L * cyl + 512 * skew[se->id_sec - 1];
	memset(secbuf, se->filler, 512);
	/* Load the sector */
	if (se->copies)
	{
		len = 512;
		err = ldbs_getblock(ldbs, se->blockid, NULL, secbuf, &len);
		if (err != DSK_ERR_OK && err != DSK_ERR_OVERRUN)
		{
			return err;
		}
	}
	err = seekto(nwasp_self, offset);

	/* Write the sector */
	if (fwrite(secbuf, 1, 512, nwasp_self->nw_fp) < 512) 
		return DSK_ERR_SYSERR;

	if (offset + 512 > (long)(nwasp_self->nw_filesize))
	{
		nwasp_self->nw_filesize = offset + 512;
	}	
	return DSK_ERR_OK;
}


/* Convert from LDBS format. */
dsk_err_t nwasp_from_ldbs(DSK_DRIVER *self, struct ldbs *source, DSK_GEOMETRY *geom)
{
	NWASP_DSK_DRIVER *nwasp_self;
	long pos;

	if (!self || !source || self->dr_class != &dc_nwasp) 
		return DSK_ERR_BADPTR;

	nwasp_self = (NWASP_DSK_DRIVER *)self;
	if (nwasp_self->nw_readonly) return DSK_ERR_RDONLY;

	/* Erase anything existing in the file */
	if (fseek(nwasp_self->nw_fp, 0, SEEK_SET)) return DSK_ERR_SYSERR;

	for (pos = 0; pos < (long)(nwasp_self->nw_filesize); pos++)
	{
		if (fputc(0xE5, nwasp_self->nw_fp) == EOF) return DSK_ERR_SYSERR;
	}

	/* And populate with whatever is in the blockstore */	
	return ldbs_all_sectors(source, nwasp_from_ldbs_callback,
				SIDES_ALT, nwasp_self);
}


