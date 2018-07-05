/***************************************************************************
 *                                                                         *
 *    LIBDSK: General floppy and diskimage access library                  *
 *    Copyright (C) 2001, 2008, 2017  John Elliott <seasip.webmaster@gmail.com>  *
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

/* This driver is for a YAZE YDSK hard drive image, which begins with a
 * 128-byte header describing the geometry. Note that many of the fields 
 * are CP/M filesystem parameters; libdsk cannot set these */

#include <stdio.h>
#include "libdsk.h"
#include "ldbs.h"
#include "drvi.h"
#include "drvydsk.h"


/* This struct contains function pointers to the driver's functions, and the
 * size of its DSK_DRIVER subclass */

DRV_CLASS dc_ydsk = 
{
	sizeof(YDSK_DSK_DRIVER),
	NULL,		/* superclass */
	"ydsk\0YDSK\0",
	"YAZE YDSK driver",
	ydsk_open,	/* open */
	ydsk_creat,	/* create new */
	ydsk_close,	/* close */
	ydsk_read,	/* read sector, working from physical address */
	ydsk_write,	/* write sector, working from physical address */
	ydsk_format,	/* format track, physical */
	ydsk_getgeom,	/* Get geometry */
	NULL,		/* sector ID */
	ydsk_xseek,	/* Seek to track */
	ydsk_status,	/* Get drive status */
	NULL,		/* xread */
	NULL,		/* xwrite */
	NULL,		/* tread */
	NULL,		/* xtread */
	ydsk_option_enum,	/* List options */
	ydsk_option_set,	/* Set option */
	ydsk_option_get,	/* Get option */
	NULL,		/* trackids */
	NULL,		/* rtread */
	ydsk_to_ldbs,	/* export as LDBS */
	ydsk_from_ldbs	/* import as LDBS */
};

static void update_geometry(YDSK_DSK_DRIVER *self, const DSK_GEOMETRY *geom)
{
	unsigned short spt = ldbs_peek2(self->ydsk_header + 32);
	unsigned short psh = self->ydsk_header[47];
	unsigned long secsize = (128L << psh);

	if (geom->dg_sectors != (dsk_psect_t)(spt >> psh) || secsize != geom->dg_secsize)
	{
		spt = geom->dg_sectors << psh;
		self->ydsk_header_dirty = 1;
		self->ydsk_super.dr_dirty = 1;
		self->ydsk_header[47] = dsk_get_psh(geom->dg_secsize);
		ldbs_poke2(self->ydsk_header + 32, spt);

		/* If we have to record a sector size not equal to 128 bytes,
		 * mark this as a YAZE 1.x image */
		if (self->ydsk_header[47] != 0)
		{
			self->ydsk_header[16] = 1;
		}
	}	
}


static void update_geometry_l(YDSK_DSK_DRIVER *self, LDBS_STATS *st)
{
	unsigned short spt = ldbs_peek2(self->ydsk_header + 32);
	unsigned short psh = self->ydsk_header[47];
	unsigned long secsize = (128L << psh);

	if (st->max_spt != (dsk_psect_t)(spt >> psh) || secsize != st->max_sector_size)
	{
		spt = st->max_spt << psh;
		self->ydsk_header_dirty = 1;
		self->ydsk_super.dr_dirty = 1;
		self->ydsk_header[47] = dsk_get_psh(st->max_sector_size);
		ldbs_poke2(self->ydsk_header + 32, spt);

		/* If we have to record a sector size not equal to 128 bytes,
		 * mark this as a YAZE 1.x image */
		if (self->ydsk_header[47] != 0)
		{
			self->ydsk_header[16] = 1;
		}
	}	
}




static dsk_err_t ydsk_seek(YDSK_DSK_DRIVER *self, const DSK_GEOMETRY *geom, 
			dsk_pcyl_t cylinder, dsk_phead_t head, 
			dsk_psect_t sector, int extend)
{
/* Get size of a track */
	unsigned short spt = ldbs_peek2(self->ydsk_header + 32);
	unsigned short psh = self->ydsk_header[47];
	unsigned long secsize = (128L << psh);
	unsigned long tracklen = secsize * (spt >> psh);
/* And convert from that to offset */
	unsigned long offset;
       

	if (geom->dg_heads == 1) offset = cylinder * tracklen;
	else			 offset = (cylinder*2 + head) * tracklen;

	offset += (sector * secsize) + 128;

/* If the file is smaller than required, grow it */
	if (extend && self->ydsk_filesize < offset)
	{
		if (fseek(self->ydsk_fp, self->ydsk_filesize, 
					SEEK_SET)) return DSK_ERR_SYSERR;
		while (self->ydsk_filesize < (offset + secsize))
		{
			if (fputc(0xE5, self->ydsk_fp) == EOF) 
				return DSK_ERR_SYSERR;
			++self->ydsk_filesize;
		}
	}
	if (fseek(self->ydsk_fp, offset, SEEK_SET)) return DSK_ERR_SYSERR;
	return DSK_ERR_OK;
}


dsk_err_t ydsk_open(DSK_DRIVER *self, const char *filename)
{
	YDSK_DSK_DRIVER *ydsk_self;
	
	/* Sanity check: Is this meant for our driver? */
	if (self->dr_class != &dc_ydsk) return DSK_ERR_BADPTR;
	ydsk_self = (YDSK_DSK_DRIVER *)self;

	ydsk_self->ydsk_fp = fopen(filename, "r+b");
	if (!ydsk_self->ydsk_fp) 
	{
		ydsk_self->ydsk_readonly = 1;
		ydsk_self->ydsk_fp = fopen(filename, "rb");
	}
	if (!ydsk_self->ydsk_fp) return DSK_ERR_NOTME;
	if (fread(ydsk_self->ydsk_header, 1, 128, ydsk_self->ydsk_fp) < 128) 
	{
		fclose(ydsk_self->ydsk_fp);
		return DSK_ERR_NOTME;
	}
/* Check magic number */
	if (memcmp(ydsk_self->ydsk_header, "<CPM_Disk>", 10))
	{
		fclose(ydsk_self->ydsk_fp);
		return DSK_ERR_NOTME;
	}
/* Record exact size, so we can tell if we're writing off the end
 * of the file. Under Windows, writing off the end of the file fills the 
 * gaps with random data, which can cause mess to appear in the directory;
 * and under UNIX, the entire directory is filled with zeroes. */
	if (fseek(ydsk_self->ydsk_fp, 0, SEEK_END)) return DSK_ERR_SYSERR;
	ydsk_self->ydsk_filesize = ftell(ydsk_self->ydsk_fp);

	return DSK_ERR_OK;
}


dsk_err_t ydsk_creat(DSK_DRIVER *self, const char *filename)
{
	YDSK_DSK_DRIVER *ydsk_self;
	
	/* Sanity check: Is this meant for our driver? */
	if (self->dr_class != &dc_ydsk) return DSK_ERR_BADPTR;
	ydsk_self = (YDSK_DSK_DRIVER *)self;

	ydsk_self->ydsk_fp = fopen(filename, "w+b");
	ydsk_self->ydsk_readonly = 0;
	if (!ydsk_self->ydsk_fp) return DSK_ERR_SYSERR;

	/* Zap the header */
	memset(ydsk_self->ydsk_header, 0, sizeof(ydsk_self->ydsk_header));
	memcpy(ydsk_self->ydsk_header, "<CPM_Disk>", 10);

	/* Give it 128 sectors/track */
	ydsk_self->ydsk_header[32] = 128;

	if (fwrite(ydsk_self->ydsk_header, 1, 128, ydsk_self->ydsk_fp) < 128)
	{
		fclose(ydsk_self->ydsk_fp);
		return DSK_ERR_SYSERR;
	}
	return DSK_ERR_OK;
}


dsk_err_t ydsk_close(DSK_DRIVER *self)
{
	YDSK_DSK_DRIVER *ydsk_self;

	if (self->dr_class != &dc_ydsk) return DSK_ERR_BADPTR;
	ydsk_self = (YDSK_DSK_DRIVER *)self;

	if (ydsk_self->ydsk_fp) 
	{
		if (ydsk_self->ydsk_header_dirty)
		{
/* Write out the header */
			if (fseek(ydsk_self->ydsk_fp, 0, SEEK_SET)
			||  fwrite(ydsk_self->ydsk_header, 1, 128, 
						ydsk_self->ydsk_fp) < 128)
			{
				fclose(ydsk_self->ydsk_fp);
				return DSK_ERR_SYSERR;
			}
		}
		if (fclose(ydsk_self->ydsk_fp))
			return DSK_ERR_SYSERR;
	}
	return DSK_ERR_OK;
}





dsk_err_t ydsk_read(DSK_DRIVER *self, const DSK_GEOMETRY *geom,
                             void *buf, dsk_pcyl_t cylinder,
                              dsk_phead_t head, dsk_psect_t sector)
{
	YDSK_DSK_DRIVER *ydsk_self;
	unsigned aread;
	dsk_err_t err;

	if (!buf || !self || !geom || self->dr_class != &dc_ydsk) return DSK_ERR_BADPTR;
	ydsk_self = (YDSK_DSK_DRIVER *)self;

	if (!ydsk_self->ydsk_fp) return DSK_ERR_NOTRDY;

	err = ydsk_seek(ydsk_self, geom, cylinder, head, sector - geom->dg_secbase, 0);
	if (err) return err;
	/* Assume unwritten sectors hold 0xE5 */
	aread = fread(buf, 1, geom->dg_secsize, ydsk_self->ydsk_fp);
	while (aread < geom->dg_secsize)
	{
		((unsigned char *)buf)[aread++] = 0xE5;	
	}
	return DSK_ERR_OK;
}



dsk_err_t ydsk_write(DSK_DRIVER *self, const DSK_GEOMETRY *geom,
                             const void *buf, dsk_pcyl_t cylinder,
                              dsk_phead_t head, dsk_psect_t sector)
{
	YDSK_DSK_DRIVER *ydsk_self;
	dsk_err_t err;

	if (!buf || !self || !geom || self->dr_class != &dc_ydsk) 
		return DSK_ERR_BADPTR;
	ydsk_self = (YDSK_DSK_DRIVER *)self;

	if (!ydsk_self->ydsk_fp) return DSK_ERR_NOTRDY;
	if (ydsk_self->ydsk_readonly) return DSK_ERR_RDONLY;

	err = ydsk_seek(ydsk_self, geom, cylinder, head, sector - geom->dg_secbase, 1);
	if (err) return err;

	if (fwrite(buf, 1, geom->dg_secsize, ydsk_self->ydsk_fp) < 
			geom->dg_secsize)
	{
		return DSK_ERR_NOADDR;
	}
	if (fseek(ydsk_self->ydsk_fp, 0, SEEK_END)) return DSK_ERR_SYSERR;
	ydsk_self->ydsk_filesize = ftell(ydsk_self->ydsk_fp);
	return DSK_ERR_OK;
}


dsk_err_t ydsk_format(DSK_DRIVER *self, DSK_GEOMETRY *geom,
                                dsk_pcyl_t cylinder, dsk_phead_t head,
                                const DSK_FORMAT *format, unsigned char filler)
{
/*
 * Note that we completely ignore the "format" parameter, since raw YDSK
 * images don't hold track headers.
 */
	YDSK_DSK_DRIVER *ydsk_self;
	unsigned short spt, psh;
	unsigned long secsize, tracklen;
	dsk_err_t err;

	(void)format;
	if (!self || !geom || self->dr_class != &dc_ydsk) return DSK_ERR_BADPTR;
	ydsk_self = (YDSK_DSK_DRIVER *)self;

	if (!ydsk_self->ydsk_fp) return DSK_ERR_NOTRDY;
	if (ydsk_self->ydsk_readonly) return DSK_ERR_RDONLY;
	spt = ldbs_peek2(ydsk_self->ydsk_header + 32);
	psh = ydsk_self->ydsk_header[47];
	secsize = (128L << psh);
	tracklen = secsize * (spt >> psh);

	/* Update the header if the format geometry doesn't match the 
	 * existing YDSK geometry */
	update_geometry(ydsk_self, geom);

	err = ydsk_seek(ydsk_self, geom, cylinder, head, 0, 1);
	if (err) return err;

	while (tracklen--) 
	{
		if (fputc(filler, ydsk_self->ydsk_fp) == EOF) return DSK_ERR_SYSERR;	
	}
	if (fseek(ydsk_self->ydsk_fp, 0, SEEK_END)) return DSK_ERR_SYSERR;
	ydsk_self->ydsk_filesize = ftell(ydsk_self->ydsk_fp);

	return DSK_ERR_OK;
}


/* We have a CP/M disk parameter block. Convert that to drive geometry */
dsk_err_t ydsk_getgeom(DSK_DRIVER *self, DSK_GEOMETRY *geom)
{
	unsigned short spt, psh, off;
	unsigned long secsize, blocksize, drivesize, dsm, tracklen;
	YDSK_DSK_DRIVER *ydsk_self;

	if (!self || !geom || self->dr_class != &dc_ydsk) return DSK_ERR_BADPTR;
	ydsk_self = (YDSK_DSK_DRIVER *)self;

	spt = ldbs_peek2(ydsk_self->ydsk_header + 32);
	psh = ydsk_self->ydsk_header[47];
	secsize = (128L << psh);
	tracklen = secsize * (spt >> psh);
	/* Work out how many cylinders we have. */
	/* If there is a full DPB, we can use that. */
	dsm = ldbs_peek2(ydsk_self->ydsk_header + 37);
	off = ldbs_peek2(ydsk_self->ydsk_header + 45);
	if (dsm && ydsk_self->ydsk_header[34])
	{
		blocksize = 128L << ydsk_self->ydsk_header[34];
/* Drive size is now size of data area + size of system area */
		drivesize = (blocksize * (1 + dsm)) + tracklen * off;
	}
	else	/* No DPB. Guess. */
	{
		if (fseek(ydsk_self->ydsk_fp, 0, SEEK_END)) return DSK_ERR_SYSERR;
		drivesize = ftell(ydsk_self->ydsk_fp) - 128;
	}

	geom->dg_sidedness = SIDES_ALT;
	geom->dg_cylinders = (drivesize + (tracklen - 1)) / tracklen;
	geom->dg_heads     = 1;
	geom->dg_sectors   = (spt >> psh);
	geom->dg_secbase   = 0;
	geom->dg_secsize   = secsize;
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

dsk_err_t ydsk_xseek(DSK_DRIVER *self, const DSK_GEOMETRY *geom, 
			dsk_pcyl_t cylinder, dsk_phead_t head)
{
	YDSK_DSK_DRIVER *ydsk_self;
	if (!self || !geom || self->dr_class != &dc_ydsk) return DSK_ERR_BADPTR;
	ydsk_self = (YDSK_DSK_DRIVER *)self;

	return ydsk_seek(ydsk_self, geom, cylinder, head, 0, 0);
}


dsk_err_t ydsk_status(DSK_DRIVER *self, const DSK_GEOMETRY *geom,
                      dsk_phead_t head, unsigned char *result)
{
        YDSK_DSK_DRIVER *ydsk_self;

        if (!self || !geom || self->dr_class != &dc_ydsk) return DSK_ERR_BADPTR;
        ydsk_self = (YDSK_DSK_DRIVER *)self;

        if (!ydsk_self->ydsk_fp) *result &= ~DSK_ST3_READY;
        if (ydsk_self->ydsk_readonly) *result |= DSK_ST3_RO;
	return DSK_ERR_OK;
}

/* CP/M-specific filesystem parameters */
static char *option_names[] = 
{
	"FS:CP/M:BSH", "FS:CP/M:BLM", "FS:CP/M:EXM",
	"FS:CP/M:DSM", "FS:CP/M:DRM", "FS:CP/M:AL0", "FS:CP/M:AL1",
	"FS:CP/M:CKS", "FS:CP/M:OFF",
};

#define MAXOPTION (sizeof(option_names) / sizeof(option_names[0]))

dsk_err_t ydsk_option_enum(DSK_DRIVER *self, int idx, char **optname)
{
	if (!self) return DSK_ERR_BADPTR;
	if (self->dr_class != &dc_ydsk) return DSK_ERR_BADPTR;

	if (idx >= 0 && idx < (int)MAXOPTION)
	{
		if (optname) *optname = option_names[idx];
		return DSK_ERR_OK;
	}
	return DSK_ERR_BADOPT;	

}

/* Set a driver-specific option */
dsk_err_t ydsk_option_set(DSK_DRIVER *self, const char *optname, int value)
{
	YDSK_DSK_DRIVER *ydsk_self;
	unsigned idx;

	if (!self || !optname) return DSK_ERR_BADPTR;
	if (self->dr_class != &dc_ydsk) return DSK_ERR_BADPTR;
	ydsk_self = (YDSK_DSK_DRIVER *)self;

	for (idx = 0; idx < MAXOPTION; idx++)
	{
		if (!strcmp(optname, option_names[idx]))
			break;
	}
	if (idx >= MAXOPTION) return DSK_ERR_BADOPT;

	ydsk_self->ydsk_header_dirty = 1;
	self->dr_dirty = 1;
	switch(idx)
	{
		case 0: ydsk_self->ydsk_header[34] = value & 0xFF;	// BSH
			break;
		case 1: ydsk_self->ydsk_header[35] = value & 0xFF;	// BLM
			break;
		case 2: ydsk_self->ydsk_header[36] = value & 0xFF;	// EXM
			break;
		case 3: ydsk_self->ydsk_header[37] = value & 0xFF;	// DSM
			ydsk_self->ydsk_header[38] = (value >> 8) & 0xFF;
			break;
		case 4: ydsk_self->ydsk_header[39] = value & 0xFF;	// DRM
			ydsk_self->ydsk_header[40] = (value >> 8) & 0xFF;
			break;
		case 5: ydsk_self->ydsk_header[41] = value & 0xFF;	// AL0 
			break;
		case 6: ydsk_self->ydsk_header[42] = value & 0xFF;	// AL1
			break;
		case 7: ydsk_self->ydsk_header[43] = value & 0xFF;	// CKS
			ydsk_self->ydsk_header[44] = (value >> 8) & 0xFF;
			break;
		case 8: ydsk_self->ydsk_header[45] = value & 0xFF;	// OFF
			ydsk_self->ydsk_header[46] = (value >> 8) & 0xFF;
			break;
	}
	return DSK_ERR_OK;
}


dsk_err_t ydsk_option_get(DSK_DRIVER *self, const char *optname, int *value)
{
	YDSK_DSK_DRIVER *ydsk_self;
	unsigned idx;
	int v = 0;

	if (!self || !optname) return DSK_ERR_BADPTR;
	if (self->dr_class != &dc_ydsk) return DSK_ERR_BADPTR;
	ydsk_self = (YDSK_DSK_DRIVER *)self;

	for (idx = 0; idx < MAXOPTION; idx++)
	{
		if (!strcmp(optname, option_names[idx]))
			break;
	}
	if (idx >= MAXOPTION) return DSK_ERR_BADOPT;

	switch(idx)
	{
		case 0: v = ydsk_self->ydsk_header[34];	// BSH
			break;
		case 1: v = ydsk_self->ydsk_header[35];	// BLM
			break;
		case 2: v = ydsk_self->ydsk_header[36];	// EXM
			break;
		case 3: v = ldbs_peek2(ydsk_self->ydsk_header + 37); // DSM
			break;
		case 4: v = ldbs_peek2(ydsk_self->ydsk_header + 39); // DRM
			break;
		case 5: v = ydsk_self->ydsk_header[41];	// AL0 
			break;
		case 6: v = ydsk_self->ydsk_header[42];	// AL1
			break;
		case 7: v = ldbs_peek2(ydsk_self->ydsk_header + 43); // CKS
			break;
		case 8: v = ldbs_peek2(ydsk_self->ydsk_header + 45); // OFF
			break;
	}
	if (value) *value = v;
	return DSK_ERR_OK;
}


/* Convert to LDBS format. */
dsk_err_t ydsk_to_ldbs(DSK_DRIVER *self, struct ldbs **result, 
		DSK_GEOMETRY *geom)
{
	dsk_err_t err;
	dsk_pcyl_t cyl;
	dsk_psect_t sec;
	unsigned char *secbuf;
	DSK_GEOMETRY dg;	/* The fixed geometry we'll be using */
	int n;
	LDBS_DPB dpb;
	YDSK_DSK_DRIVER *ydsk_self;

        if (!self || !result || self->dr_class != &dc_ydsk) 
		return DSK_ERR_BADPTR;
	ydsk_self = (YDSK_DSK_DRIVER *)self;
	err = ldbs_new(result, NULL, LDBS_DSK_TYPE);
	if (err) return err;

	err = ydsk_getgeom(self, &dg);
	if (err) return err;

	/* Write the deduced geometry record */
	err = ldbs_put_geometry(*result, &dg);
	if (err) return err;
	/* And the CP/M DPB, if there is one. */
	dpb.spt = ldbs_peek2(ydsk_self->ydsk_header + 32);
	dpb.bsh = ydsk_self->ydsk_header[34];
	dpb.blm = ydsk_self->ydsk_header[35];
	dpb.exm = ydsk_self->ydsk_header[36];
	dpb.dsm = ldbs_peek2(ydsk_self->ydsk_header + 37);
	dpb.drm = ldbs_peek2(ydsk_self->ydsk_header + 39);
	dpb.al[0] = ydsk_self->ydsk_header[41];	
	dpb.al[1] = ydsk_self->ydsk_header[42];	
	dpb.cks = ldbs_peek2(ydsk_self->ydsk_header + 43);
	dpb.off = ldbs_peek2(ydsk_self->ydsk_header + 45);
	dpb.psh = ydsk_self->ydsk_header[47];	
	dpb.phm = ydsk_self->ydsk_header[48];	

	/* Only write a DPB if it looks populated */
	if (!err && dpb.spt && dpb.dsm && dpb.drm) 
	{
		err = ldbs_put_dpb(*result, &dpb);
	}
	if (err)
	{
		ldbs_close(result);
		return err;
	}
	secbuf = dsk_malloc(dg.dg_secsize);
	if (!secbuf)
	{
		ldbs_close(result);
		return DSK_ERR_NOMEM;
	}	
	/* Write out as a single-sided LDBS file */
	for (cyl = 0; cyl < dg.dg_cylinders; cyl++)
	{
		LDBS_TRACKHEAD *trkh = ldbs_trackhead_alloc(dg.dg_sectors);
	
		if (!trkh)
		{
			dsk_free(secbuf);
			ldbs_close(result);
			return DSK_ERR_NOMEM;
		}
		trkh->datarate = 3;
		trkh->recmode = 2;
		trkh->gap3 = 0x52;
		trkh->filler = 0xE5;
		for (sec = 0; sec < dg.dg_sectors; sec++)
		{
/* For each sector in the drive image file, read it in */
			err = ydsk_read(self, &dg, secbuf, cyl, 0, sec);
			if (err)
			{
				dsk_free(secbuf);
				ldbs_free(trkh);
				ldbs_close(result);
				return err;
			}
/* Add it to the track header */
			trkh->sector[sec].id_cyl = cyl;
			trkh->sector[sec].id_head = 0;
			trkh->sector[sec].id_sec = sec + dg.dg_secbase;
			trkh->sector[sec].id_psh = dsk_get_psh(dg.dg_secsize);
			trkh->sector[sec].copies = 0;
			for (n = 1; n < (int)dg.dg_secsize; n++)
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
					dg.dg_secsize);
				if (err)
				{
					ldbs_free(secbuf);
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
			ldbs_free(secbuf);
			ldbs_close(result);
			return err;
		}
	}
	ldbs_free(secbuf);
	return ldbs_sync(*result);
}

static dsk_err_t ydsk_from_ldbs_callback(PLDBS ldbs, dsk_pcyl_t cyl,
	 dsk_phead_t head, LDBS_SECTOR_ENTRY *se, LDBS_TRACKHEAD *th,
	 void *param)
{
	YDSK_DSK_DRIVER *ydsk_self = param;
	dsk_err_t err;
	size_t len;

	len = ydsk_self->ydsk_geom->dg_secsize;
	memset(ydsk_self->ydsk_secbuf, se->filler, len);
	/* Load the sector */
	if (se->copies)
	{
		err = ldbs_getblock(ldbs, se->blockid, NULL, 
				ydsk_self->ydsk_secbuf, &len);
		if (err != DSK_ERR_OK && err != DSK_ERR_OVERRUN)
		{
			return err;
		}
	}
	/* Write the sector buffer */
	return ydsk_write(&ydsk_self->ydsk_super, ydsk_self->ydsk_geom,
			ydsk_self->ydsk_secbuf, cyl, head, se->id_sec);
}


/* Convert from LDBS format. */
dsk_err_t ydsk_from_ldbs(DSK_DRIVER *self, struct ldbs *source, DSK_GEOMETRY *geom)
{
	YDSK_DSK_DRIVER *ydsk_self;
	long pos;
	LDBS_DPB dpb;
	DSK_GEOMETRY lgeom;
	dsk_err_t err;
	int have_dpb = 0;

	if (!self || !source || self->dr_class != &dc_ydsk) 
		return DSK_ERR_BADPTR;
	ydsk_self = (YDSK_DSK_DRIVER *)self;

	if (ydsk_self->ydsk_readonly) return DSK_ERR_RDONLY;

	/* Erase anything existing in the file after the header */
	if (fseek(ydsk_self->ydsk_fp, 128, SEEK_SET)) return DSK_ERR_SYSERR;

	for (pos = 0; pos < (long)ydsk_self->ydsk_filesize; pos++)
	{
		if (fputc(0xE5, ydsk_self->ydsk_fp) == EOF) 
			return DSK_ERR_SYSERR;
	}
	/* If there is a DPB, update the header */
	err = ldbs_get_dpb(source, &dpb);
	if (!err && dpb.spt && dpb.dsm && dpb.drm)
	{
		ldbs_poke2(ydsk_self->ydsk_header + 32, dpb.spt);
		ydsk_self->ydsk_header[34] = dpb.bsh;
		ydsk_self->ydsk_header[35] = dpb.blm;
		ydsk_self->ydsk_header[36] = dpb.exm;
		ldbs_poke2(ydsk_self->ydsk_header + 37, dpb.dsm);
		ldbs_poke2(ydsk_self->ydsk_header + 39, dpb.drm);
		ydsk_self->ydsk_header[41] = dpb.al[0];
		ydsk_self->ydsk_header[42] = dpb.al[1];	
		ldbs_poke2(ydsk_self->ydsk_header + 43, dpb.cks);
		ldbs_poke2(ydsk_self->ydsk_header + 45, dpb.off);
		ydsk_self->ydsk_header[47] = dpb.psh;	
		ydsk_self->ydsk_header[48] = dpb.phm;	
		ydsk_self->ydsk_header_dirty = 1;
		have_dpb = 1;
	}
	/* If the input file doesn't have a DPB but does have a geometry
	 * record, we can at least update the geometry from that */
	if (!have_dpb)
	{
		err = ldbs_get_geometry(source, &lgeom);
		if (!err && lgeom.dg_cylinders && lgeom.dg_heads && 
			lgeom.dg_sectors && lgeom.dg_secsize)
		{
			update_geometry(ydsk_self, &lgeom);
			have_dpb = 1;
		}
	}
	/* If that wasn't an option but a geometry was passed in, use 
	 * that. */
	if (geom && !have_dpb)
	{
		if (geom->dg_cylinders && geom->dg_heads && 
		    geom->dg_sectors && geom->dg_secsize)
		{
			update_geometry(ydsk_self, geom);
			have_dpb = 1;
		}
	}
	/* And if that failed, try guessing from the statistics */
	if (!have_dpb)
	{
		LDBS_STATS stats;

		err = ldbs_get_stats(source, &stats);
		if (!err && !stats.drive_empty)
		{
			update_geometry_l(ydsk_self, &stats);
		}
	}
	/* Allocate a buffer for each sector in turn */
	ydsk_self->ydsk_secbuf = dsk_malloc(128 << ydsk_self->ydsk_header[47]);

	if (geom)
	{
		ydsk_self->ydsk_geom = geom;
	}
	else
	{
		err = ydsk_getgeom(self, &lgeom);
		if (err) return err;
		ydsk_self->ydsk_geom = &lgeom;
	}

	/* Now we have our best shot at a DPB, copy the sectors */
	err = ldbs_all_sectors(source, ydsk_from_ldbs_callback,
				SIDES_ALT, ydsk_self);

	dsk_free(ydsk_self->ydsk_secbuf);
	ydsk_self->ydsk_geom   = NULL;
	ydsk_self->ydsk_secbuf = NULL;
	return err;
}

