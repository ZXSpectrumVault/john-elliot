/***************************************************************************
 *                                                                         *
 *    LIBDSK: General floppy and diskimage access library                  *
 *    Copyright (C) 2001,2007  John Elliott <seasip.webmaster@gmail.com>       *
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

/* This driver implements access to a flat file, like drvposix, but with the
 * sides laid out in the order specified by the disk geometry. What you end
 * up with is a logical filesystem image, hence the name. */

#include <stdio.h>
#include "libdsk.h"
#include "ldbs.h"
#include "drvi.h"
#include "drvlogi.h"


/* This struct contains function pointers to the driver's functions, and the
 * size of its DSK_DRIVER subclass */

DRV_CLASS dc_logical = 
{
	sizeof(LOGICAL_DSK_DRIVER),
	NULL,		/* superclass */
	"logical\0",
	"Raw file logical sector order",
	logical_open,	/* open */
	logical_creat,	/* create new */
	logical_close,	/* close */
	logical_read,	/* read sector, working from physical address */
	logical_write,	/* write sector, working from physical address */
	logical_format,	/* format track, physical */
	NULL,		/* get geometry */
	NULL,		/* sector ID */
	logical_xseek,	/* seek to track */
	logical_status,	/* drive status */
	NULL, 		/* xread */
	NULL, 		/* xwrite */
	NULL, 		/* tread */
	NULL, 		/* xtread */
	NULL,		/* option_enum */
	NULL,		/* option_set */
	NULL,		/* option_get */
	NULL,		/* trackids */
	NULL,		/* rtread */
	logical_to_ldbs,	/* export as LDBS */
	logical_from_ldbs	/* import as LDBS */
};

dsk_err_t logical_open(DSK_DRIVER *self, const char *filename)
{
	LOGICAL_DSK_DRIVER *lpxself;
	
	/* Sanity check: Is this meant for our driver? */
	if (self->dr_class != &dc_logical) return DSK_ERR_BADPTR;
	lpxself = (LOGICAL_DSK_DRIVER *)self;

	lpxself->lpx_fp = fopen(filename, "r+b");
	if (!lpxself->lpx_fp) 
	{
		lpxself->lpx_readonly = 1;
		lpxself->lpx_fp = fopen(filename, "rb");
	}
	if (!lpxself->lpx_fp) return DSK_ERR_NOTME;
/* v0.9.5: Record exact size, so we can tell if we're writing off the end
 * of the file. Under Windows, writing off the end of the file fills the 
 * gaps with random data, which can cause mess to appear in the directory;
 * and under UNIX, the entire directory is filled with zeroes. */
        if (fseek(lpxself->lpx_fp, 0, SEEK_END)) return DSK_ERR_SYSERR;
        lpxself->lpx_filesize = ftell(lpxself->lpx_fp);

	return DSK_ERR_OK;
}


dsk_err_t logical_creat(DSK_DRIVER *self, const char *filename)
{
	LOGICAL_DSK_DRIVER *lpxself;
	
	/* Sanity check: Is this meant for our driver? */
	if (self->dr_class != &dc_logical) return DSK_ERR_BADPTR;
	lpxself = (LOGICAL_DSK_DRIVER *)self;

	lpxself->lpx_fp = fopen(filename, "w+b");
	lpxself->lpx_readonly = 0;
	if (!lpxself->lpx_fp) return DSK_ERR_SYSERR;
	lpxself->lpx_filesize = 0;
	return DSK_ERR_OK;
}


dsk_err_t logical_close(DSK_DRIVER *self)
{
	LOGICAL_DSK_DRIVER *lpxself;

	if (self->dr_class != &dc_logical) return DSK_ERR_BADPTR;
	lpxself = (LOGICAL_DSK_DRIVER *)self;

	if (lpxself->lpx_fp) 
	{
		if (fclose(lpxself->lpx_fp) == EOF) return DSK_ERR_SYSERR;
		lpxself->lpx_fp = NULL;
	}
	return DSK_ERR_OK;	
}


dsk_err_t logical_read(DSK_DRIVER *self, const DSK_GEOMETRY *geom,
                             void *buf, dsk_pcyl_t cylinder,
                              dsk_phead_t head, dsk_psect_t sector)
{
	LOGICAL_DSK_DRIVER *lpxself;
	dsk_lsect_t offset;
	dsk_err_t err;

	if (!buf || !self || !geom || self->dr_class != &dc_logical) return DSK_ERR_BADPTR;
	lpxself = (LOGICAL_DSK_DRIVER *)self;

	if (!lpxself->lpx_fp) return DSK_ERR_NOTRDY;

	err = dg_ps2ls(geom, cylinder, head, sector, &offset);
	if (err) return err;
	offset *=  geom->dg_secsize;

	if (fseek(lpxself->lpx_fp, offset, SEEK_SET)) return DSK_ERR_SYSERR;

	if (fread(buf, 1, geom->dg_secsize, lpxself->lpx_fp) < geom->dg_secsize)
	{
		return DSK_ERR_NOADDR;
	}
	return DSK_ERR_OK;
}


static dsk_err_t seekto(LOGICAL_DSK_DRIVER *self, unsigned long offset)
{
	/* 0.9.5: Fill any "holes" in the file with 0xE5. Otherwise, UNIX would
	 * fill them with zeroes and Windows would fill them with whatever
	 * happened to be lying around */
	if (self->lpx_filesize < offset)
	{
		if (fseek(self->lpx_fp, self->lpx_filesize, SEEK_SET)) return DSK_ERR_SYSERR;
		while (self->lpx_filesize < offset)
		{
			if (fputc(0xE5, self->lpx_fp) == EOF) return DSK_ERR_SYSERR;
			++self->lpx_filesize;
		}
	}
	if (fseek(self->lpx_fp, offset, SEEK_SET)) return DSK_ERR_SYSERR;
	return DSK_ERR_OK;
}

dsk_err_t logical_write(DSK_DRIVER *self, const DSK_GEOMETRY *geom,
                             const void *buf, dsk_pcyl_t cylinder,
                              dsk_phead_t head, dsk_psect_t sector)
{
	LOGICAL_DSK_DRIVER *lpxself;
	dsk_lsect_t offset;
	dsk_err_t err;

	if (!buf || !self || !geom || self->dr_class != &dc_logical) return DSK_ERR_BADPTR;
	lpxself = (LOGICAL_DSK_DRIVER *)self;

	if (!lpxself->lpx_fp) return DSK_ERR_NOTRDY;
	if (lpxself->lpx_readonly) return DSK_ERR_RDONLY;

	err = dg_ps2ls(geom, cylinder, head, sector, &offset);
	if (err) return err;
	offset *=  geom->dg_secsize;

	err = seekto(lpxself, offset);
	if (err) return err;

	if (fwrite(buf, 1, geom->dg_secsize, lpxself->lpx_fp) < geom->dg_secsize)
	{
		return DSK_ERR_NOADDR;
	}
	if (lpxself->lpx_filesize < offset + geom->dg_secsize)
		lpxself->lpx_filesize = offset + geom->dg_secsize;
	return DSK_ERR_OK;
}


dsk_err_t logical_format(DSK_DRIVER *self, DSK_GEOMETRY *geom,
                                dsk_pcyl_t cylinder, dsk_phead_t head,
                                const DSK_FORMAT *format, unsigned char filler)
{
/*
 * Note that we completely ignore the "format" parameter, since raw LOGICAL
 * images don't hold track headers.
 */
	LOGICAL_DSK_DRIVER *lpxself;
	dsk_lsect_t offset;
	unsigned long trklen;
	dsk_err_t err;

   (void)format;
	if (!self || !geom || self->dr_class != &dc_logical) return DSK_ERR_BADPTR;
	lpxself = (LOGICAL_DSK_DRIVER *)self;

	if (!lpxself->lpx_fp) return DSK_ERR_NOTRDY;
	if (lpxself->lpx_readonly) return DSK_ERR_RDONLY;

	trklen = geom->dg_sectors * geom->dg_secsize;

	err = dg_ps2ls(geom, cylinder, head, geom->dg_secbase, &offset);
	if (err) return err;
	offset *= geom->dg_secsize;

	err = seekto(lpxself, offset);
	if (err) return err;
	if (lpxself->lpx_filesize < offset + trklen)
		lpxself->lpx_filesize = offset + trklen;

	while (trklen--) 
		if (fputc(filler, lpxself->lpx_fp) == EOF) return DSK_ERR_SYSERR;	

	return DSK_ERR_OK;
}

	

dsk_err_t logical_xseek(DSK_DRIVER *self, const DSK_GEOMETRY *geom,
                      dsk_pcyl_t cylinder, dsk_phead_t head)
{
	LOGICAL_DSK_DRIVER *lpxself;
	dsk_err_t err;
	dsk_lsect_t offset;

	if (!self || !geom || self->dr_class != &dc_logical) return DSK_ERR_BADPTR;
	lpxself = (LOGICAL_DSK_DRIVER *)self;

	if (!lpxself->lpx_fp) return DSK_ERR_NOTRDY;

	if (cylinder >= geom->dg_cylinders || head >= geom->dg_heads)
		return DSK_ERR_SEEKFAIL;

	err = dg_ps2ls(geom, cylinder, head, geom->dg_secbase, &offset);
	if (err) return err;
	offset *= geom->dg_secsize;
	
	if (fseek(lpxself->lpx_fp, offset, SEEK_SET)) return DSK_ERR_SEEKFAIL;

	return DSK_ERR_OK;
}

dsk_err_t logical_status(DSK_DRIVER *self, const DSK_GEOMETRY *geom,
                      dsk_phead_t head, unsigned char *result)
{
	LOGICAL_DSK_DRIVER *lpxself;

	if (!self || !geom || self->dr_class != &dc_logical) return DSK_ERR_BADPTR;
	lpxself = (LOGICAL_DSK_DRIVER *)self;

	if (!lpxself->lpx_fp) *result &= ~DSK_ST3_READY;
	if (lpxself->lpx_readonly) *result |= DSK_ST3_RO;
	return DSK_ERR_OK;
}


dsk_err_t logical_to_ldbs(DSK_DRIVER *self, struct ldbs **result, DSK_GEOMETRY *geom)
{
	unsigned char bootblock[512];
	DSK_GEOMETRY bootgeom;
	LOGICAL_DSK_DRIVER *lpxself;
	dsk_err_t err;
	dsk_pcyl_t cyl;
	dsk_phead_t head;
	dsk_psect_t sec;
	unsigned char *secbuf;
	LDBS_TRACKHEAD *th;
	int n;

	if (!self || !result || self->dr_class != &dc_logical) return DSK_ERR_BADPTR;
	lpxself = (LOGICAL_DSK_DRIVER *)self;

	if (lpxself->lpx_readonly) return DSK_ERR_RDONLY;
	if (geom == NULL)
	{
		if (fseek(lpxself->lpx_fp, 0, SEEK_SET)) 
			return DSK_ERR_SYSERR;

		if (fread(bootblock, 1, 512, lpxself->lpx_fp) < 512)
		{
			return DSK_ERR_BADFMT;
		}
		err = dg_bootsecgeom(&bootgeom, bootblock);
		if (err) return err;

		geom = &bootgeom;	
	}
	secbuf = dsk_malloc(geom->dg_secsize);
	if (!secbuf) return DSK_ERR_NOMEM;

	err = ldbs_new(result, NULL, LDBS_DSK_TYPE);
	if (err)
	{
		dsk_free(secbuf);
		return err;
	}
	/* If a geometry was provided, save it in the file */
	if (geom != &bootgeom)
	{
		err = ldbs_put_geometry(*result, geom);
		if (err)
		{
			ldbs_close(result);
			dsk_free(secbuf);
			return err;
		}
	}
	for (cyl = 0; cyl < geom->dg_cylinders; cyl++)
	    for (head = 0; head < geom->dg_heads; head++)
	{
		th = ldbs_trackhead_alloc(geom->dg_sectors);
		if (!th)
		{
			dsk_free(secbuf);
			ldbs_close(result);
			return DSK_ERR_NOMEM;
		}
		for (sec = 0; sec < geom->dg_sectors; sec++)
		{
			err = logical_read(self, geom, secbuf, cyl, head, 
					sec + geom->dg_secbase);
			if (err)
			{
				ldbs_free(th);
				dsk_free(secbuf);
				ldbs_close(result);
				return err;
			}
			th->sector[sec].id_cyl  = cyl;
			th->sector[sec].id_head = head;
			th->sector[sec].id_sec  = sec + geom->dg_secbase;
			th->sector[sec].id_psh  = dsk_get_psh(geom->dg_secsize);
			th->sector[sec].copies = 0;
			for (n = 1; n < (int)(geom->dg_secsize); n++)
			{
				if (secbuf[n] != secbuf[0])
				{
					th->sector[sec].copies = 1;
					break;
				}
			}
			if (!th->sector[sec].copies)
			{
				th->sector[sec].filler = secbuf[0];
			}
			else
			{
				char secid[4];
			
				ldbs_encode_secid(secid, cyl, head, 	
						sec + geom->dg_secbase);
				err = ldbs_putblock(*result, 
						&th->sector[sec].blockid,
							secid, secbuf,
							geom->dg_secsize);
				if (err)
				{
					ldbs_free(th);
					dsk_free(secbuf);
					ldbs_close(result);
					return err;
				}
			}	
		}	/* End of loop over sectors */
		err = ldbs_put_trackhead(*result, th, cyl, head);
		ldbs_free(th);
		if (err)
		{
			dsk_free(secbuf);
			ldbs_close(result);
			return err;
		}
	}	/* End of loop over cyls / heads */
	dsk_free(secbuf);	
	return ldbs_sync(*result);
}

static dsk_err_t logical_from_ldbs_callback(PLDBS ldbs, dsk_pcyl_t cyl,
	 dsk_phead_t head, LDBS_SECTOR_ENTRY *se, LDBS_TRACKHEAD *th, 	
	void *param)
{
	LOGICAL_DSK_DRIVER *lpxself = param;
	dsk_err_t err;
	size_t len;

	/* Skip cylinders / heads not covered by the geometry */
	if (cyl  >= lpxself->lpx_export_geom->dg_cylinders ||
	    head >= lpxself->lpx_export_geom->dg_heads)
	{
		return DSK_ERR_OK;
	}

	len = lpxself->lpx_export_geom->dg_secsize;
	memset(lpxself->lpx_secbuf, se->filler, len);
	if (se->copies)
	{
		err = ldbs_getblock(ldbs, se->blockid, NULL, 
				lpxself->lpx_secbuf, &len);
		if (err != DSK_ERR_OK && err != DSK_ERR_OVERRUN)
			return err;
	}
	return logical_write(&lpxself->lpx_super, lpxself->lpx_export_geom,
			lpxself->lpx_secbuf, cyl, head, se->id_sec);
}




dsk_err_t logical_from_ldbs(DSK_DRIVER *self, struct ldbs *source, DSK_GEOMETRY *geom)
{
	LOGICAL_DSK_DRIVER *lpxself;
	long pos;
	dsk_err_t err;

	if (!self || !source || self->dr_class != &dc_logical) return DSK_ERR_BADPTR;

	lpxself = (LOGICAL_DSK_DRIVER *)self;

	if (!geom)
	{
		/* XXX Probe geometry from boot sector */
		return DSK_ERR_BADFMT;
	}

	lpxself->lpx_export_geom = geom;
	/* Erase anything existing in the file */
	if (fseek(lpxself->lpx_fp, 0, SEEK_SET)) return DSK_ERR_SYSERR;

	for (pos = 0; pos < (long)(lpxself->lpx_filesize); pos++)
	{
		if (fputc(0xE5, lpxself->lpx_fp) == EOF) return DSK_ERR_SYSERR;
	}
	if (fseek(lpxself->lpx_fp, 0, SEEK_SET)) return DSK_ERR_SYSERR;

	lpxself->lpx_secbuf = dsk_malloc(geom->dg_secsize);
	if (!lpxself->lpx_secbuf) return DSK_ERR_NOMEM;

	/* And populate with whatever is in the blockstore */	
	err =  ldbs_all_sectors(source, logical_from_ldbs_callback,
				geom->dg_sidedness, lpxself);
	dsk_free(lpxself->lpx_secbuf);
	return err;
}



