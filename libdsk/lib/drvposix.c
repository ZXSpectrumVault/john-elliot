/***************************************************************************
 *                                                                         *
 *    LIBDSK: General floppy and diskimage access library                  *
 *    Copyright (C) 2001, 2017  John Elliott <seasip.webmaster@gmail.com>  *
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

/* This driver is the most basic of the drivers, and simply implements 
 * access to a flat file with the tracks laid out in the SIDES_ALT order */

#include <stdio.h>
#include <assert.h>
#include "libdsk.h"
#include "drvi.h"
#include "ldbs.h"
#include "drvposix.h"


/* [LibDsk 1.5.3] Three classes, for three different track orders */

DRV_CLASS dc_posixalt = 
{
	sizeof(POSIX_DSK_DRIVER),
	NULL,		/* superclass */
	"raw\0rawalt\0",
	"Raw file driver (alternate sides) ",
	posix_openalt,	/* open */
	posix_creatalt,	/* create new */
	posix_close,	/* close */
	posix_read,	/* read sector, working from physical address */
	posix_write,	/* write sector, working from physical address */
	posix_format,	/* format track, physical */
	NULL,		/* get geometry */
	NULL,		/* sector ID */
	posix_xseek,	/* seek to track */
	posix_status,	/* drive status */
	NULL, 		/* xread */
	NULL, 		/* xwrite */
	NULL, 		/* tread */
	NULL, 		/* xtread */
	NULL,		/* option_enum */
	NULL,		/* option_set */
	NULL,		/* option_get */
	NULL,		/* trackids */
	NULL,		/* rtread */
	posix_to_ldbs,	/* export as LDBS */
	posix_from_ldbs	/* import as LDBS */
};

DRV_CLASS dc_posixoo = 
{
	sizeof(POSIX_DSK_DRIVER),
	NULL,		/* superclass */
	"rawoo\0",
	"Raw file driver (out and out) ",
	posix_openoo,	/* open */
	posix_creatoo,	/* create new */
	posix_close,	/* close */
	posix_read,	/* read sector, working from physical address */
	posix_write,	/* write sector, working from physical address */
	posix_format,	/* format track, physical */
	NULL,		/* get geometry */
	NULL,		/* sector ID */
	posix_xseek,	/* seek to track */
	posix_status,	/* drive status */
	NULL, 		/* xread */
	NULL, 		/* xwrite */
	NULL, 		/* tread */
	NULL, 		/* xtread */
	NULL,		/* option_enum */
	NULL,		/* option_set */
	NULL,		/* option_get */
	NULL,		/* trackids */
	NULL,		/* rtread */
	posix_to_ldbs,	/* export as LDBS */
	posix_from_ldbs	/* import as LDBS */
};

DRV_CLASS dc_posixob = 
{
	sizeof(POSIX_DSK_DRIVER),
	NULL,		/* superclass */
	"rawob\0",
	"Raw file driver (out and back) ",
	posix_openob,	/* open */
	posix_creatob,	/* create new */
	posix_close,	/* close */
	posix_read,	/* read sector, working from physical address */
	posix_write,	/* write sector, working from physical address */
	posix_format,	/* format track, physical */
	NULL,		/* get geometry */
	NULL,		/* sector ID */
	posix_xseek,	/* seek to track */
	posix_status,	/* drive status */
	NULL, 		/* xread */
	NULL, 		/* xwrite */
	NULL, 		/* tread */
	NULL, 		/* xtread */
	NULL,		/* option_enum */
	NULL,		/* option_set */
	NULL,		/* option_get */
	NULL,		/* trackids */
	NULL,		/* rtread */
	posix_to_ldbs,	/* export as LDBS */
	posix_from_ldbs	/* import as LDBS */
};

#define CHECK_CLASS(s) \
	if (s->dr_class != &dc_posixalt && \
	    s->dr_class != &dc_posixoo  && \
	    s->dr_class != &dc_posixob) return DSK_ERR_BADPTR; \
						\
	pxself = (POSIX_DSK_DRIVER *)s;



dsk_err_t posix_open(DSK_DRIVER *self, const char *filename, dsk_sides_t s)
{
	POSIX_DSK_DRIVER *pxself;
	
	/* Sanity check: Is this meant for our driver? */
	CHECK_CLASS(self);

	pxself->px_sides = s;
	pxself->px_fp = fopen(filename, "r+b");
	if (!pxself->px_fp) 
	{
		pxself->px_readonly = 1;
		pxself->px_fp = fopen(filename, "rb");
	}
	if (!pxself->px_fp) return DSK_ERR_NOTME;
/* v0.9.5: Record exact size, so we can tell if we're writing off the end
 * of the file. Under Windows, writing off the end of the file fills the 
 * gaps with random data, which can cause mess to appear in the directory;
 * and under UNIX, the entire directory is filled with zeroes. */
        if (fseek(pxself->px_fp, 0, SEEK_END)) return DSK_ERR_SYSERR;
        pxself->px_filesize = ftell(pxself->px_fp);

	return DSK_ERR_OK;
}


dsk_err_t posix_openalt(DSK_DRIVER *self, const char *filename)
{
	return posix_open(self, filename, SIDES_ALT);
}

dsk_err_t posix_openoo(DSK_DRIVER *self, const char *filename)
{
	return posix_open(self, filename, SIDES_OUTOUT);
}

dsk_err_t posix_openob(DSK_DRIVER *self, const char *filename)
{
	return posix_open(self, filename, SIDES_OUTBACK);
}

dsk_err_t posix_creat(DSK_DRIVER *self, const char *filename, dsk_sides_t s)
{
	POSIX_DSK_DRIVER *pxself;
	
	/* Sanity check: Is this meant for our driver? */
	CHECK_CLASS(self);

	pxself->px_sides = s;
	pxself->px_fp = fopen(filename, "w+b");
	pxself->px_readonly = 0;
	if (!pxself->px_fp) return DSK_ERR_SYSERR;
	pxself->px_filesize = 0;
	return DSK_ERR_OK;
}

dsk_err_t posix_creatalt(DSK_DRIVER *self, const char *filename)
{
	return posix_creat(self, filename, SIDES_ALT);
}

dsk_err_t posix_creatoo(DSK_DRIVER *self, const char *filename)
{
	return posix_creat(self, filename, SIDES_OUTOUT);
}

dsk_err_t posix_creatob(DSK_DRIVER *self, const char *filename)
{
	return posix_creat(self, filename, SIDES_OUTBACK);
}


dsk_err_t posix_close(DSK_DRIVER *self)
{
	POSIX_DSK_DRIVER *pxself;

	CHECK_CLASS(self);

	if (pxself->px_fp) 
	{
		if (fclose(pxself->px_fp) == EOF) return DSK_ERR_SYSERR;
		pxself->px_fp = NULL;
	}
	return DSK_ERR_OK;	
}


unsigned long posix_offset(POSIX_DSK_DRIVER *pxself, const DSK_GEOMETRY *geom,
			dsk_pcyl_t cylinder, dsk_phead_t head, 
			dsk_psect_t sector)
{
	unsigned long offset = 0;

	/* Work out the offset based on the sidedness of the disk image
	 * (not the sidedness of the geometry) */
	switch (pxself->px_sides)
	{
		case SIDES_EXTSURFACE:
		case SIDES_ALT:
			offset = (cylinder * geom->dg_heads) + head;
			break;
		case SIDES_OUTBACK:
			if (head)
			{
				offset = (2 * geom->dg_cylinders - 1 - cylinder);
			}
			else
			{
				offset = cylinder;
			}
			break;
		case SIDES_OUTOUT:
			offset = (head * geom->dg_cylinders) + cylinder;
			break;
				
	}
	offset *= geom->dg_sectors;
	offset += (sector - geom->dg_secbase);
	offset *=  geom->dg_secsize;
	return offset;
}

dsk_err_t posix_read(DSK_DRIVER *self, const DSK_GEOMETRY *geom,
                             void *buf, dsk_pcyl_t cylinder,
                              dsk_phead_t head, dsk_psect_t sector)
{
	POSIX_DSK_DRIVER *pxself;
	unsigned long offset;

	if (!buf || !self || !geom) return DSK_ERR_BADPTR;
	CHECK_CLASS(self);

	if (!pxself->px_fp) return DSK_ERR_NOTRDY;

	offset = posix_offset(pxself, geom, cylinder, head, sector);

	if (fseek(pxself->px_fp, offset, SEEK_SET)) return DSK_ERR_SYSERR;

	if (fread(buf, 1, geom->dg_secsize, pxself->px_fp) < geom->dg_secsize)
	{
		return DSK_ERR_NOADDR;
	}
	return DSK_ERR_OK;
}


static dsk_err_t seekto(POSIX_DSK_DRIVER *self, unsigned long offset)
{
	/* 0.9.5: Fill any "holes" in the file with 0xE5. Otherwise, UNIX would
	 * fill them with zeroes and Windows would fill them with whatever
	 * happened to be lying around */
	if (self->px_filesize < offset)
	{
		if (fseek(self->px_fp, self->px_filesize, SEEK_SET)) return DSK_ERR_SYSERR;
		while (self->px_filesize < offset)
		{
			if (fputc(0xE5, self->px_fp) == EOF) return DSK_ERR_SYSERR;
			++self->px_filesize;
		}
	}
	if (fseek(self->px_fp, offset, SEEK_SET)) return DSK_ERR_SYSERR;
	return DSK_ERR_OK;
}

dsk_err_t posix_write(DSK_DRIVER *self, const DSK_GEOMETRY *geom,
                             const void *buf, dsk_pcyl_t cylinder,
                              dsk_phead_t head, dsk_psect_t sector)
{
	POSIX_DSK_DRIVER *pxself;
	unsigned long offset;
	dsk_err_t err;

	if (!buf || !self || !geom)
	CHECK_CLASS(self);

	if (!pxself->px_fp) return DSK_ERR_NOTRDY;
	if (pxself->px_readonly) return DSK_ERR_RDONLY;
	if (sector < geom->dg_secbase || sector >= geom->dg_secbase + geom->dg_sectors)
		return DSK_ERR_NOADDR;

	offset = posix_offset(pxself, geom, cylinder, head, sector);

	err = seekto(pxself, offset);
	if (err) return err;

	if (fwrite(buf, 1, geom->dg_secsize, pxself->px_fp) < geom->dg_secsize)
	{
		return DSK_ERR_NOADDR;
	}
	if (pxself->px_filesize < offset + geom->dg_secsize)
		pxself->px_filesize = offset + geom->dg_secsize;
	return DSK_ERR_OK;
}


dsk_err_t posix_format(DSK_DRIVER *self, DSK_GEOMETRY *geom,
                                dsk_pcyl_t cylinder, dsk_phead_t head,
                                const DSK_FORMAT *format, unsigned char filler)
{
/*
 * Note that we completely ignore the "format" parameter, since raw POSIX
 * images don't hold track headers.
 */
	POSIX_DSK_DRIVER *pxself;
	unsigned long offset;
	unsigned long trklen;
	dsk_err_t err;

   (void)format;
	if (!self || !geom) return DSK_ERR_BADPTR;
	CHECK_CLASS(self);

	if (!pxself->px_fp) return DSK_ERR_NOTRDY;
	if (pxself->px_readonly) return DSK_ERR_RDONLY;

	/* Convert from physical to logical sector. However, unlike the dg_* 
	 * functions, this _always_ uses "SIDES_ALT" mapping; this is the 
	 * mapping that both the Linux and NT floppy drivers use to convert 
	 * offsets back into C/H/S. */
	offset = posix_offset(pxself, geom, cylinder, head, 
			geom->dg_secbase);

	trklen = geom->dg_secsize * geom->dg_sectors;
	err = seekto(pxself, offset);
	if (err) return err;
	if (pxself->px_filesize < offset + trklen)
		pxself->px_filesize = offset + trklen;

	for (++trklen; trklen > 1; trklen--)
		if (fputc(filler, pxself->px_fp) == EOF) return DSK_ERR_SYSERR;	

	return DSK_ERR_OK;
}

	

dsk_err_t posix_xseek(DSK_DRIVER *self, const DSK_GEOMETRY *geom,
                      dsk_pcyl_t cylinder, dsk_phead_t head)
{
	POSIX_DSK_DRIVER *pxself;
	long offset;

	if (!self || !geom) return DSK_ERR_BADPTR;
	CHECK_CLASS(self);

	if (!pxself->px_fp) return DSK_ERR_NOTRDY;

	if (cylinder >= geom->dg_cylinders || head >= geom->dg_heads)
		return DSK_ERR_SEEKFAIL;

	/* Convert from physical to logical sector. However, unlike the dg_* 
	 * functions, this _always_ uses "SIDES_ALT" mapping; this is the 
	 * mapping that both the Linux and NT floppy drivers use to convert 
	 * offsets back into C/H/S. */
	offset = (cylinder * geom->dg_heads) + head;	/* Drive track */
	offset *= geom->dg_sectors * geom->dg_secsize;
	
	if (fseek(pxself->px_fp, offset, SEEK_SET)) return DSK_ERR_SEEKFAIL;

	return DSK_ERR_OK;
}

dsk_err_t posix_status(DSK_DRIVER *self, const DSK_GEOMETRY *geom,
                      dsk_phead_t head, unsigned char *result)
{
	POSIX_DSK_DRIVER *pxself;

	if (!self || !geom) return DSK_ERR_BADPTR;
	CHECK_CLASS(self);

	if (!pxself->px_fp) *result &= ~DSK_ST3_READY;
	if (pxself->px_readonly) *result |= DSK_ST3_RO;
	return DSK_ERR_OK;
}


dsk_err_t posix_to_ldbs(DSK_DRIVER *self, struct ldbs **result, DSK_GEOMETRY *geom)
{
	unsigned char bootblock[512];
	DSK_GEOMETRY bootgeom;
	POSIX_DSK_DRIVER *pxself;
	dsk_err_t err;
	dsk_pcyl_t cyl;
	dsk_phead_t head;
	dsk_psect_t sec;
	unsigned char *secbuf;
	LDBS_TRACKHEAD *th;
	int n;

	if (!self || !result) return DSK_ERR_BADPTR;
	CHECK_CLASS(self);

	if (geom == NULL)
	{
		if (fseek(pxself->px_fp, 0, SEEK_SET)) 
			return DSK_ERR_SYSERR;

		if (fread(bootblock, 1, 512, pxself->px_fp) < 512)
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
			err = posix_read(self, geom, secbuf, cyl, head, 
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

static dsk_err_t posix_from_ldbs_callback(PLDBS ldbs, dsk_pcyl_t cyl,
	 dsk_phead_t head, LDBS_TRACKHEAD *th, void *param)
{
	POSIX_DSK_DRIVER *pxself = param;
	dsk_err_t err;
	int n;
	size_t len;
	unsigned char *secbuf;
	long offset;

	if (pxself->px_readonly) return DSK_ERR_RDONLY;

	/* If no geometry is supplied, just dump out all the sectors in
	 * the required size. If one is supplied, constrain all the sectors
	 * to that size. */

	if (pxself->px_export_geom)
	{
		/* Skip cylinders / heads not covered by the geometry */
		if (cyl  >= pxself->px_export_geom->dg_cylinders ||
		    head >= pxself->px_export_geom->dg_heads)
		{
			return DSK_ERR_OK;
		}
		for (n = 0; n < th->count; n++)
		{
			offset = posix_offset(pxself, pxself->px_export_geom, 
					cyl, head, th->sector[n].id_sec);
			len = pxself->px_export_geom->dg_secsize;

			/* Ensure the file is big enough to hold the sector */
			err = seekto(pxself, offset + len);
			if (err) return err;
			/* Then seek to the start of the sector */
			err = seekto(pxself, offset);
			if (err) return err;

			if (th->sector[n].copies)
			{
				err = ldbs_getblock_a(ldbs, 
						th->sector[n].blockid,
						NULL,
						(void **)&secbuf, &len);
				if (err) return err;
				if (len > pxself->px_export_geom->dg_secsize)
				    len = pxself->px_export_geom->dg_secsize;

				if (fwrite(secbuf, 1, len, pxself->px_fp) < len)
				{
					ldbs_free(secbuf);
					return DSK_ERR_SYSERR;
				}
				ldbs_free(secbuf);
			}
			else	/* No copies, write the filler byte */
			{
				while (len > 0)
				{
					if (fputc(th->sector[n].filler,
						pxself->px_fp) == EOF)
					{
						return DSK_ERR_SYSERR;
					}
					--len;
				}	
			}
		}	/* End loop over sectors */
	}	/* End if geometry provided */
	else
	{
		/* Otherwise just regurgitate the whole track in one hit */
		err = ldbs_load_track(ldbs, th, (void *)&secbuf, &len, 0);
		if (err) return err;
		if (fwrite(secbuf, 1, len, pxself->px_fp) < len)
		{
			ldbs_free(secbuf);
			return DSK_ERR_SYSERR;
		}
		ldbs_free(secbuf);
	}
	return DSK_ERR_OK;
}




dsk_err_t posix_from_ldbs(DSK_DRIVER *self, struct ldbs *source, DSK_GEOMETRY *geom)
{
	POSIX_DSK_DRIVER *pxself;
	long pos;

	if (!self || !source) return DSK_ERR_BADPTR;
	CHECK_CLASS(self);

	pxself->px_export_geom = geom;
	/* Erase anything existing in the file */
	if (fseek(pxself->px_fp, 0, SEEK_SET)) return DSK_ERR_SYSERR;

	for (pos = 0; pos < (long)(pxself->px_filesize); pos++)
	{
		if (fputc(0xE5, pxself->px_fp) == EOF) return DSK_ERR_SYSERR;
	}
	if (fseek(pxself->px_fp, 0, SEEK_SET)) return DSK_ERR_SYSERR;

	/* And populate with whatever is in the blockstore */	
	return ldbs_all_tracks(source, posix_from_ldbs_callback,
				pxself->px_sides, pxself);
}


