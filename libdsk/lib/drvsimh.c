/***************************************************************************
 *                                                                         *
 *    LIBDSK: General floppy and diskimage access library                  *
 *    Copyright (C) 2001,2008,2017  John Elliott <seasip.webmaster@gmail.com> *
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

#include <stdio.h>
#include "libdsk.h"
#include "ldbs.h"
#include "drvi.h"
#include "drvsimh.h"

/* The SIMH disc image is assumed to be in a single fixed format, like the
 * MYZ80 disc image.
 *
 * Geometry:
 * 	254 tracks (presumed to be 127 cylinders, 2 heads)
 * 	 32 sectors / track
 *      137 bytes/sector: 3 bytes header, 128 bytes data, 4 bytes trailer
 *
 * CP/M filesystem parameters:
 * 	SPT = 0x20
 * 	BSH = 0x04
 * 	BLM = 0x0F
 * 	EXM = 0x00
 * 	DSM = 0x01EF
 * 	DRM = 0x00FF
 * 	AL0 = 0xF0
 * 	AL1 = 0x00
 *	CKS = 0x40
 *	OFF = 0x06
 */
/* This struct contains function pointers to the driver's functions, and the
 * size of its DSK_DRIVER subclass */

DRV_CLASS dc_simh = 
{
	sizeof(SIMH_DSK_DRIVER),
	NULL,		/* superclass */
	"simh\0SIMH\0",
	"SIMH disc image driver",
	simh_open,	/* open */
	simh_creat,	/* create new */
	simh_close,	/* close */
	simh_read,	/* read sector, working from physical address */
	simh_write,	/* write sector, working from physical address */
	simh_format,	/* format track, physical */
	simh_getgeom,	/* Get geometry */
	NULL,		/* sector ID */
	simh_xseek,	/* Seek to track */
	simh_status,	/* Get drive status */
	NULL, 		/* xread */
	NULL, 		/* xwrite */
	NULL, 		/* tread */
	NULL, 		/* xtread */
	NULL,		/* option_enum */
	NULL,		/* option_set */
	NULL,		/* option_get */
	NULL,		/* trackids */
	NULL,		/* rtread */
	simh_to_ldbs,	/* export as LDBS */
	simh_from_ldbs	/* import as LDBS */
};



dsk_err_t simh_open(DSK_DRIVER *self, const char *filename)
{
	SIMH_DSK_DRIVER *simh_self;
	
	/* Sanity check: Is this meant for our driver? */
	if (self->dr_class != &dc_simh) return DSK_ERR_BADPTR;
	simh_self = (SIMH_DSK_DRIVER *)self;

	simh_self->simh_fp = fopen(filename, "r+b");
	if (!simh_self->simh_fp) 
	{
		simh_self->simh_readonly = 1;
		simh_self->simh_fp = fopen(filename, "rb");
	}
	if (!simh_self->simh_fp) return DSK_ERR_NOTME;

	/* This code is left over from MYZ80 and may not be necessary: 
	 * allow shorter images than the standard, and grow them when
	 * necessary. */
	if (fseek(simh_self->simh_fp, 0, SEEK_END)) return DSK_ERR_SYSERR;
	simh_self->simh_filesize = ftell(simh_self->simh_fp);

	/* Set up CP/M filesystem parameters */
	dsk_isetoption(self, "FS:CP/M:BSH", 4, 1);
	dsk_isetoption(self, "FS:CP/M:BLM", 15, 1);
	dsk_isetoption(self, "FS:CP/M:EXM", 0, 1); 
	dsk_isetoption(self, "FS:CP/M:DSM", 0x1ef, 1);
	dsk_isetoption(self, "FS:CP/M:DRM", 0x0ff, 1);
	dsk_isetoption(self, "FS:CP/M:AL0", 0xF0, 1);
	dsk_isetoption(self, "FS:CP/M:AL1", 0, 1);
	dsk_isetoption(self, "FS:CP/M:CKS", 0x40, 1);
	dsk_isetoption(self, "FS:CP/M:OFF", 6, 1);

	return DSK_ERR_OK;
}


dsk_err_t simh_creat(DSK_DRIVER *self, const char *filename)
{
	SIMH_DSK_DRIVER *simh_self;
	
	/* Sanity check: Is this meant for our driver? */
	if (self->dr_class != &dc_simh) return DSK_ERR_BADPTR;
	simh_self = (SIMH_DSK_DRIVER *)self;

	simh_self->simh_fp = fopen(filename, "w+b");
	simh_self->simh_readonly = 0;
	if (!simh_self->simh_fp) return DSK_ERR_SYSERR;
	dsk_isetoption(self, "FS:CP/M:BSH", 4, 1);
	dsk_isetoption(self, "FS:CP/M:BLM", 15, 1);
	dsk_isetoption(self, "FS:CP/M:EXM", 0, 1); 
	dsk_isetoption(self, "FS:CP/M:DSM", 0x1ef, 1);
	dsk_isetoption(self, "FS:CP/M:DRM", 0x0ff, 1);
	dsk_isetoption(self, "FS:CP/M:AL0", 0xF0, 1);
	dsk_isetoption(self, "FS:CP/M:AL1", 0, 1);
	dsk_isetoption(self, "FS:CP/M:CKS", 0x40, 1);
	dsk_isetoption(self, "FS:CP/M:OFF", 6, 1);

	return DSK_ERR_OK;
}


dsk_err_t simh_close(DSK_DRIVER *self)
{
	SIMH_DSK_DRIVER *simh_self;

	if (self->dr_class != &dc_simh) return DSK_ERR_BADPTR;
	simh_self = (SIMH_DSK_DRIVER *)self;

	if (simh_self->simh_fp) 
	{
		if (fclose(simh_self->simh_fp) == EOF) return DSK_ERR_SYSERR;
		simh_self->simh_fp = NULL;
	}
	return DSK_ERR_OK;	
}


dsk_err_t simh_read(DSK_DRIVER *self, const DSK_GEOMETRY *geom,
                             void *buf, dsk_pcyl_t cylinder,
                              dsk_phead_t head, dsk_psect_t sector)
{
	SIMH_DSK_DRIVER *simh_self;
	long offset;
	unsigned aread;

	if (!buf || !self || !geom || self->dr_class != &dc_simh) return DSK_ERR_BADPTR;
	simh_self = (SIMH_DSK_DRIVER *)self;

	if (!simh_self->simh_fp) return DSK_ERR_NOTRDY;

	/* Convert from physical to logical sector, using the fixed geometry. */
	offset = (4384L * (2*cylinder+head)) + (137L * sector) + 3;

	if (fseek(simh_self->simh_fp, offset, SEEK_SET)) return DSK_ERR_SYSERR;

	/* Fill missing data with 0xE5 */
	aread = fread(buf, 1, geom->dg_secsize, simh_self->simh_fp);
	while (aread < geom->dg_secsize)
	{
		((unsigned char *)buf)[aread++] = 0xE5;	
	}
	return DSK_ERR_OK;
}

static unsigned char trailer[4] = { 0xE5, 0xE5, 0xE5, 0xE5 };

dsk_err_t simh_write(DSK_DRIVER *self, const DSK_GEOMETRY *geom,
                             const void *buf, dsk_pcyl_t cylinder,
                              dsk_phead_t head, dsk_psect_t sector)
{
	SIMH_DSK_DRIVER *simh_self;
	unsigned long offset;

	if (!buf || !self || !geom || self->dr_class != &dc_simh) return DSK_ERR_BADPTR;
	simh_self = (SIMH_DSK_DRIVER *)self;

	if (!simh_self->simh_fp) return DSK_ERR_NOTRDY;
	if (simh_self->simh_readonly) return DSK_ERR_RDONLY;

	/* Convert from physical to logical sector, using the fixed geometry. */
	offset = (4384L * (2*cylinder+head)) + (137L * sector) + 3;

	if (simh_self->simh_filesize < offset)
	{
		if (fseek(simh_self->simh_fp, simh_self->simh_filesize, SEEK_SET)) return DSK_ERR_SYSERR;
		while (simh_self->simh_filesize < (offset + geom->dg_secsize))
		{
			if (fputc(0xE5, simh_self->simh_fp) == EOF) return DSK_ERR_SYSERR;
			++simh_self->simh_filesize;
		}
	}	
	if (fseek(simh_self->simh_fp, offset, SEEK_SET)) return DSK_ERR_SYSERR;

	if (fwrite(buf, 1, geom->dg_secsize, simh_self->simh_fp) < geom->dg_secsize)
	{
		return DSK_ERR_NOADDR;
	}
	/* After the sector, write 4 bytes trailer */
	if (fwrite(trailer, 1, sizeof(trailer), simh_self->simh_fp) < 
			(int)sizeof(trailer))
	{
		return DSK_ERR_NOADDR;
	}

	if (fseek(simh_self->simh_fp, 0, SEEK_END)) return DSK_ERR_SYSERR;
	simh_self->simh_filesize = ftell(simh_self->simh_fp);
	return DSK_ERR_OK;
}


dsk_err_t simh_format(DSK_DRIVER *self, DSK_GEOMETRY *geom,
                                dsk_pcyl_t cylinder, dsk_phead_t head,
                                const DSK_FORMAT *format, unsigned char filler)
{
/*
 * Note that we completely ignore the "format" parameter, since raw SIMH
 * images don't hold track headers.
 */
	SIMH_DSK_DRIVER *simh_self;
	unsigned long offset;
	long trklen;

	(void)format;
	if (!self || !geom || self->dr_class != &dc_simh) return DSK_ERR_BADPTR;
	simh_self = (SIMH_DSK_DRIVER *)self;

	if (!simh_self->simh_fp) return DSK_ERR_NOTRDY;
	if (simh_self->simh_readonly) return DSK_ERR_RDONLY;

	/* Convert from physical to logical sector. However, unlike the dg_* 
	 * functions, this _always_ uses "SIDES_ALT" mapping; this is the 
	 * mapping that both the Linux and NT floppy drivers use to convert 
	 * offsets back into C/H/S. */
	/* Convert from physical to logical sector, using the fixed geometry. */
	offset = (4384L * (2*cylinder+head)) + 3;
	trklen = 4348L;

	if (simh_self->simh_filesize < offset)
	{
		if (fseek(simh_self->simh_fp, simh_self->simh_filesize, SEEK_SET)) return DSK_ERR_SYSERR;
		while (simh_self->simh_filesize < offset + trklen)
		{
			if (fputc(0xE5, simh_self->simh_fp) == EOF) return DSK_ERR_SYSERR;
			++simh_self->simh_filesize;
		}
	}	
	if (fseek(simh_self->simh_fp, offset, SEEK_SET)) return DSK_ERR_SYSERR;

	while (trklen--) 
		if (fputc(filler, simh_self->simh_fp) == EOF) return DSK_ERR_SYSERR;	
	if (fseek(simh_self->simh_fp, 0, SEEK_END)) return DSK_ERR_SYSERR;
	simh_self->simh_filesize = ftell(simh_self->simh_fp);

	return DSK_ERR_OK;
}

	
dsk_err_t simh_getgeom(DSK_DRIVER *self, DSK_GEOMETRY *geom)
{
	if (!geom || !self || self->dr_class != &dc_simh) return DSK_ERR_BADPTR;
	geom->dg_sidedness = SIDES_ALT;
	geom->dg_cylinders = 127;
	geom->dg_heads     = 2;
	geom->dg_sectors   = 32;
	geom->dg_secbase   = 0;
	geom->dg_secsize   = 128;
	geom->dg_datarate  = RATE_DD;	/* From here on, values are dummy 
					 * values. I don't know what the
					 * correct entries are for an 8" 
					 * floppy, which is what I presume
					 * this image relates to. */
	geom->dg_rwgap     = 0x2A;
	geom->dg_fmtgap    = 0x52;
	geom->dg_fm        = RECMODE_MFM;
	geom->dg_nomulti   = 0;
	return DSK_ERR_OK;
}

dsk_err_t simh_xseek(DSK_DRIVER *self, const DSK_GEOMETRY *geom, 
			dsk_pcyl_t cylinder, dsk_phead_t head)
{
	if (cylinder >= 128) return DSK_ERR_SEEKFAIL;
	return DSK_ERR_OK;
}


dsk_err_t simh_status(DSK_DRIVER *self, const DSK_GEOMETRY *geom,
                      dsk_phead_t head, unsigned char *result)
{
        SIMH_DSK_DRIVER *simh_self;

        if (!self || !geom || self->dr_class != &dc_simh) return DSK_ERR_BADPTR;
        simh_self = (SIMH_DSK_DRIVER *)self;

        if (!simh_self->simh_fp) *result &= ~DSK_ST3_READY;
        if (simh_self->simh_readonly) *result |= DSK_ST3_RO;
	return DSK_ERR_OK;
}


static const LDBS_DPB simh_dpb =
{
	32,	/* SPT */
	4, 	/* BSH (2k blocks) */
	15, 	/* BLM */
	0, 	/* EXM */
	0x1EF,	/* DSM (496 * 2k) */
	0xFF,	/* DRM */
	{ 0xF0, 0, }, 	/* AL0 / AL1 */
	0x040,  /* CKS */
	6, 	/* OFF */
	0, 	/* PSH */
	0 	/* PHM */
};


/* Convert to LDBS format. */
dsk_err_t simh_to_ldbs(DSK_DRIVER *self, struct ldbs **result, 
		DSK_GEOMETRY *geom)
{
	dsk_err_t err;
	dsk_pcyl_t cyl;
	dsk_phead_t head;
	dsk_psect_t sec;
	unsigned char secbuf[128];
	DSK_GEOMETRY dg;	/* The fixed geometry we'll be using */
	int n;

        if (!self || !result || self->dr_class != &dc_simh) 
		return DSK_ERR_BADPTR;
	err = ldbs_new(result, NULL, LDBS_DSK_TYPE);
	if (err) return err;

	err = simh_getgeom(self, &dg);
	/* Write a fixed geometry record */
	if (!err) err = ldbs_put_geometry(*result, &dg);
	/* And a fixed DPB record */
	if (!err) err = ldbs_put_dpb(*result, &simh_dpb);
	if (err)
	{
		ldbs_close(result);
		return err;
	}	

	/* Format 128 tracks each with 32 sectors */
	for (cyl = 0; cyl < 127; cyl++)
	{
		for (head = 0; head < 2; head++)
		{
			LDBS_TRACKHEAD *trkh = ldbs_trackhead_alloc(32);
	
			if (!trkh)
			{
				ldbs_close(result);
				return DSK_ERR_NOMEM;
			}
			/* To fit 32 * 128 byte sectors on a track, I think
			 * you need either MFM @ 250kbps or FM @ 500kbps. 	
			 * Let's go with MFM. */
			trkh->datarate = 1;
			trkh->recmode = 2;
			trkh->gap3 = 0x52;
			trkh->filler = 0xE5;
			for (sec = 0; sec < 32; sec++)
			{
/* For each sector in the SIMH drive image file, read it in */
				err = simh_read(self, &dg, secbuf, cyl, head, sec);
				if (err)
				{
					ldbs_free(trkh);
					ldbs_close(result);
					return err;
				}
/* Add it to the track header */
				trkh->sector[sec].id_cyl = cyl;
				trkh->sector[sec].id_head = head;
				trkh->sector[sec].id_sec = sec;
				trkh->sector[sec].id_psh = 0;
				trkh->sector[sec].copies = 0;
				for (n = 1; n < 128; n++)
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
							id, secbuf, 128);
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

static dsk_err_t simh_from_ldbs_callback(PLDBS ldbs, dsk_pcyl_t cyl,
	 dsk_phead_t head, LDBS_SECTOR_ENTRY *se, LDBS_TRACKHEAD *th,
	 void *param)
{
	SIMH_DSK_DRIVER *simh_self = param;
	unsigned char secbuf[128];
	dsk_err_t err;
	long offset = (4384L * (2 * cyl + head)) + (137 * se->id_sec) + 3;
	size_t len;

	/* Ignore cylinders/heads/sectors that are out of range */
	if (cyl > 126 || head > 1 || se->id_sec > 31)
	{
		return DSK_ERR_OK;
	}

	memset(secbuf, se->filler, 128);
	/* Load the sector */
	if (se->copies)
	{
		len = 128;
		err = ldbs_getblock(ldbs, se->blockid, NULL, secbuf, &len);
		if (err != DSK_ERR_OK && err != DSK_ERR_OVERRUN)
		{
			return err;
		}
	}
	/* Pad the file out to the right size */
	if ((long)simh_self->simh_filesize < offset)
	{
		if (fseek(simh_self->simh_fp, simh_self->simh_filesize, SEEK_SET)) 
			return DSK_ERR_SYSERR;
		while ((long)simh_self->simh_filesize < offset)
		{
			if (fputc(0xE5, simh_self->simh_fp) == EOF) 
				return DSK_ERR_SYSERR;
			++simh_self->simh_filesize;
		}
	}
	/* Write the sector */
	if (fseek(simh_self->simh_fp, offset, SEEK_SET) ||
	    fwrite(secbuf, 1, 128, simh_self->simh_fp) < 128) 
		return DSK_ERR_SYSERR;

	/* After the sector, write 4 bytes trailer */
	if (fwrite(trailer, 1, sizeof(trailer), simh_self->simh_fp) < 
			(int)sizeof(trailer))
	{
		return DSK_ERR_NOADDR;
	}

	if (offset + 132 > (long)simh_self->simh_filesize)
	{
		simh_self->simh_filesize = offset + 132;
	}	
	return DSK_ERR_OK;
}


/* Convert from LDBS format. */
dsk_err_t simh_from_ldbs(DSK_DRIVER *self, struct ldbs *source, DSK_GEOMETRY *geom)
{
	SIMH_DSK_DRIVER *simh_self;
	long pos;

	if (!self || !source || self->dr_class != &dc_simh) 
		return DSK_ERR_BADPTR;
	simh_self = (SIMH_DSK_DRIVER *)self;
	if (simh_self->simh_readonly) return DSK_ERR_RDONLY;

	/* Erase anything existing in the file */
	if (fseek(simh_self->simh_fp, 0, SEEK_SET)) return DSK_ERR_SYSERR;

	for (pos = 0; pos < (long)simh_self->simh_filesize; pos++)
	{
		if (fputc(0xE5, simh_self->simh_fp) == EOF) return DSK_ERR_SYSERR;
	}

	/* And populate with whatever is in the blockstore */	
	return ldbs_all_sectors(source, simh_from_ldbs_callback,
				SIDES_ALT, simh_self);
}


