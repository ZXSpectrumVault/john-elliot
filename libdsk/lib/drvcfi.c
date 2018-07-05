/***************************************************************************
 *                                                                         *
 *    LIBDSK: General floppy and diskimage access library                  *
 *    Copyright (C) 2001-2,2017  John Elliott <seasip.webmaster@gmail.com> *
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

/* This driver shows how a disc image file with an awkward compressed format
 * can be handled - by loading it into memory in drv_open() and writing it
 * back (if changed) in drv_close() 
 *
 * Rewritten in 1.5.3 to use LDBS as the storage system, which means all 
 * access can be handed off to dc_ldbsdisk */

#include <stdio.h>
#include "libdsk.h"
#include "drvi.h"
#include "drvldbs.h"
#include "drvcfi.h"


/* This struct contains function pointers to the driver's functions, and the
 * size of its DSK_DRIVER subclass */

DRV_CLASS dc_cfi = 
{
	sizeof(CFI_DSK_DRIVER),
	&dc_ldbsdisk,	/* superclass */
	"cfi\0CFI\0",
	"CFI file driver",
	cfi_open,	/* open */
	cfi_creat,	/* create new */
	cfi_close,	/* close */
};

static dsk_err_t cfi_rdword(FILE *fp, unsigned short *s)
{
	int c = fgetc(fp);
	if (c == EOF) return DSK_ERR_SEEKFAIL;
	*s = (c & 0xFF);
	c = fgetc(fp);
	if (c == EOF) return DSK_ERR_SEEKFAIL;
	*s |= ((c << 8) & 0xFF00);
	return DSK_ERR_OK;
}


/* Given a compressed buffer, either find its uncompressed length
 * or decompress it */
static dsk_err_t cfi_uncompress(unsigned char *cdata, unsigned short clen,
				unsigned char *udata, size_t *ulen)
{
	unsigned char *cdp, *udp;
	unsigned short blklen;

	cdp = cdata;
	udp = (udata ? udata : NULL);	
	*ulen = 0;

	while (clen)
	{
		/* Get block size & compression */
		blklen = ldbs_peek2(cdp);
		cdp += 2;

		if (blklen & 0x8000)	/* Compressed block */
		{
	/* There must be at least 3 bytes for this block! */
			if (clen < 3) return DSK_ERR_NOTME;
			blklen &= 0x7FFF;
			clen -= 3;
			if (udata)
			{
				memset(udp, *cdp, blklen);
				udp += blklen;
			}
			*ulen += blklen;
			cdp++;
		}
		else			/* Uncompressed block */
		{
	/* There must be room for this block in the buffer! */
			if (clen < (2+blklen) || (blklen == 0)) 
			{
				return DSK_ERR_NOTME;
			}
			clen -= (blklen + 2);
			if (udata)
			{
				memcpy(udp, cdp, blklen);
				udp += blklen;
			}
			*ulen += blklen;
			cdp += blklen;
		}
	}
	return DSK_ERR_OK;
}



static dsk_err_t cfi_load_track(CFI_DSK_DRIVER *self, dsk_ltrack_t trk, FILE *fp)
{
	dsk_err_t err;
	unsigned char *cbuf, *ubuf;
	unsigned short clen;
	size_t ulen;
	LDBS_TRACKHEAD *trkh;
	dsk_psect_t sec, sectors;
	dsk_pcyl_t cyl;
	dsk_phead_t head;
	unsigned char *secdata;
	unsigned n, allsame;
	
	/* Load the track (compressed) length. If EOF, then
	 * return DSK_ERR_OVERRUN (EOF, but OK really) */
	err = cfi_rdword(fp, &clen);
	if (err == DSK_ERR_SEEKFAIL) return DSK_ERR_OVERRUN;
	/* Compressed length must be at least 3; a block can't be any 
	 * smaller! */
	if (clen < 3) return DSK_ERR_NOTME;
	cbuf = dsk_malloc(clen);
	if (cbuf == NULL) return DSK_ERR_NOMEM;
	/* Try to load the track buffer. If that fails, bail out */
	if (fread(cbuf, 1, clen, fp) < clen) 
	{
		dsk_free(cbuf);
		return DSK_ERR_NOTME;	
	}

	/* Determine the size of the track */
	err = cfi_uncompress(cbuf, clen, NULL, &ulen);
	if (err)
	{
		dsk_free(cbuf);
		return err;
	}
	/* Allocate space for the uncompressed data */
	ubuf = dsk_malloc(ulen);
	if (!ubuf)
	{
		dsk_free(cbuf);
		return DSK_ERR_NOMEM;
	}
	/* Decompress track */
	err = cfi_uncompress(cbuf, clen, ubuf, &ulen);
	if (err)
	{
		dsk_free(ubuf);
		dsk_free(cbuf);
		return DSK_ERR_NOMEM;
	}
	/* CFI files contain no information about geometry, relying on 
	 * what (we hope) is a DOS boot sector. */
	if (trk == 0)
	{
		err = dg_bootsecgeom(&self->cfi_geom, ubuf);
		if (err != DSK_ERR_OK)
		{
			sectors = ulen / 512;
/* Couldn't determine the format. Let's assume something DOS-ish */
			if   (sectors < 11) 
				dg_stdformat(&self->cfi_geom, FMT_720K, NULL, NULL);
			else if (sectors < 17) 	
				dg_stdformat(&self->cfi_geom, FMT_1200K, NULL, NULL);
			else	dg_stdformat(&self->cfi_geom, FMT_1440K, NULL, NULL);	
			self->cfi_geom.dg_sectors = sectors;
		}
	}
	/* Convert track to cylinder/head using deduced geometry */
	dg_lt2pt(&self->cfi_geom, trk, &cyl, &head);

/* Sectors in this track. Should be the same for all tracks, because 
 * that's about all CFI can cope with. */
	sectors = ulen / self->cfi_geom.dg_secsize;

	/* Now create an LDBS track header */
	trkh = ldbs_trackhead_alloc(self->cfi_geom.dg_sectors);
	trkh->recmode = self->cfi_geom.dg_fm & RECMODE_MASK;	
	trkh->gap3    = self->cfi_geom.dg_fmtgap;
	trkh->filler  = 0xF6;
	for (sec = 0; sec < sectors; sec++)
	{
		secdata = &ubuf[sec * self->cfi_geom.dg_secsize];
		trkh->sector[sec].id_cyl = cyl;
		trkh->sector[sec].id_head = head;
		trkh->sector[sec].id_sec = sec + self->cfi_geom.dg_secbase;
		trkh->sector[sec].id_psh = dsk_get_psh(self->cfi_geom.dg_secsize);
		allsame = 1;
		for (n = 0; n < self->cfi_geom.dg_secsize; n++)
		{
			if (secdata[n] != secdata[0]) { allsame = 0; break; }
		}
		if (allsame)
		{
			trkh->sector[sec].copies = 0;
			trkh->sector[sec].filler = secdata[0];
		}
		else
		{
			char secid[4];

			trkh->sector[sec].copies = 1;
			ldbs_encode_secid(secid, cyl, head, 
					sec + self->cfi_geom.dg_secbase);	
			err = ldbs_putblock(self->cfi_super.ld_store, 
						&trkh->sector[sec].blockid,
						secid, secdata, 
						self->cfi_geom.dg_secsize);
			if (err) break;	
		}
	}
	if (!err) err = ldbs_put_trackhead(self->cfi_super.ld_store, trkh, cyl, head);
	
	dsk_free(trkh);
	dsk_free(ubuf);
	dsk_free(cbuf);

	return err;
}


static int number_same(unsigned char *c, int tlen)
{
	unsigned char m = (*c); 
	int matched = 1;

	while (tlen > 0)
	{
		++c;
		--tlen;
		if (!tlen) break;
		if (*c != m) return matched;
		++matched;	
		if (matched == 0x7FFF) return matched;
	}
	return matched;	
}



static dsk_err_t cfi_save_track(PLDBS ldbs, dsk_pcyl_t cyl, dsk_phead_t head,
			LDBS_TRACKHEAD *th, void *param)
{
	CFI_DSK_DRIVER *self = param;
	dsk_err_t err;
	unsigned blklen;
	unsigned char *ubuf, *cbuf, *rdptr, *rdbase, *wrptr;
	size_t ulen;
	int matched;
	size_t wrlen;

	if (th->count == 0) return DSK_ERR_OK;	/* Vacuous track */

	err = ldbs_load_track(ldbs, th, (void *)&ubuf, &ulen, 0);
	if (err) return err;

	/* Compress the track using RLE. */
	cbuf = dsk_malloc(4 + ulen);
	if (!cbuf) 
	{
		ldbs_free(ubuf);
		return DSK_ERR_NOMEM;
	}
	/* Load the track into the buffer */
	memset(cbuf, th->filler, 4 + ulen);

	rdptr  = ubuf;	/* -> current byte */
	rdbase = ubuf;	/* -> start of current uncompressed block */
	wrptr  = cbuf + 2;
	blklen = 0;
	
	while (ulen)
	{
/* Check for a compressible run */
		matched = number_same(rdptr, ulen);
		if (matched > 5)	/* Start of compressed run */
		{
			if (blklen) 
			{
/* If we're navigating an uncompressed block, append it */
				wrptr[0] =  blklen & 0xFF;
				wrptr[1] = (blklen >> 8) & 0xFF;
				memcpy(wrptr + 2, rdbase, blklen);
				wrptr  += (blklen + 2);
				rdbase += blklen;
				blklen = 0;
			}
/* Append compressed block */
			wrptr[0] = matched & 0xFF;
			wrptr[1] = ((matched >> 8) & 0x7F) | 0x80;
			wrptr[2] = *rdptr;
			rdptr += matched;
			rdbase = rdptr;
			wrptr += 3;
			ulen -= matched;
		}
		else
		{
/* Not RLEable. */
			rdptr++;
			blklen++;
			ulen--;
		}
	}
/* Write the last uncompressed block if there is one */
	if (blklen)
	{
		wrptr[0] =  blklen & 0xFF;
		wrptr[1] = (blklen >> 8) & 0xFF;
		memcpy(wrptr + 2, rdbase, blklen);
		wrptr  += (blklen + 2);
	}
	ldbs_poke2(cbuf, (unsigned short)((wrptr - cbuf) - 2));
	
	wrlen = wrptr - cbuf;
	if (fwrite(cbuf, 1, wrlen, self->cfi_fp) < wrlen)
	{
		dsk_free(cbuf);
		ldbs_free(ubuf);
		return DSK_ERR_SYSERR;
	}
	dsk_free(cbuf);
	ldbs_free(ubuf);
	return DSK_ERR_OK;
}


dsk_err_t cfi_open(DSK_DRIVER *self, const char *filename)
{
	FILE *fp;
	CFI_DSK_DRIVER *cfiself;
	dsk_err_t err;	
	dsk_ltrack_t nt;
	
	/* Sanity check: Is this meant for our driver? */
	if (self->dr_class != &dc_cfi) return DSK_ERR_BADPTR;
	cfiself = (CFI_DSK_DRIVER *)self;

	fp = fopen(filename, "r+b");
	if (!fp) 
	{
		cfiself->cfi_super.ld_readonly = 1;
		fp = fopen(filename, "rb");
	}
	if (!fp) return DSK_ERR_NOTME;

	/* Keep a copy of the filename; when writing back, we will need it */
	cfiself->cfi_filename = dsk_malloc_string(filename);
	if (!cfiself->cfi_filename) 
	{
		fclose(fp);
		return DSK_ERR_NOMEM;
	}
	/* Initialise the blockstore */
	err = ldbs_new(&cfiself->cfi_super.ld_store, NULL, LDBS_DSK_TYPE);
	if (err)
	{
		dsk_free(cfiself->cfi_filename);
		fclose(fp);
		return err;
	}
	/* Now to load the tracks */
	nt = 0;
	dsk_report("Loading CFI file into memory");
	while (!feof(fp))
	{
		err = cfi_load_track(cfiself, nt++, fp);
		/* DSK_ERR_OVERRUN: End of file */
		if (err == DSK_ERR_OVERRUN) break;
		if (err) 
		{
			dsk_free(cfiself->cfi_filename);
			dsk_report_end();
			fclose(fp);
			return err;
		}
	} 
	dsk_report_end();
	fclose(fp);
	return ldbsdisk_attach(self);
}


dsk_err_t cfi_creat(DSK_DRIVER *self, const char *filename)
{
	CFI_DSK_DRIVER *cfiself;
	dsk_err_t err;
	FILE *fp;

	/* Sanity check: Is this meant for our driver? */
	if (self->dr_class != &dc_cfi) return DSK_ERR_BADPTR;
	cfiself = (CFI_DSK_DRIVER *)self;

	/* Save the filename, we'll want it when doing output */
	cfiself->cfi_filename = dsk_malloc_string(filename);
	if (!cfiself->cfi_filename) return DSK_ERR_NOMEM;

	/* Create a 0-byte file, just to be sure we can */
	fp = fopen(filename, "wb");
	if (!fp) return DSK_ERR_SYSERR;
	fclose(fp);

	/* OK, create a new empty blockstore */
	err = ldbs_new(&cfiself->cfi_super.ld_store, NULL, LDBS_DSK_TYPE);
	if (err)
	{
		dsk_free(cfiself->cfi_filename);
		return err;
	}
	/* Finally, hand the blockstore to the superclass so it can provide
	 * all the read/write/format methods */
	return ldbsdisk_attach(self);
}


dsk_err_t cfi_close(DSK_DRIVER *self)
{
	CFI_DSK_DRIVER *cfiself;
	dsk_err_t err = DSK_ERR_OK;

	if (self->dr_class != &dc_cfi) return DSK_ERR_BADPTR;
	cfiself = (CFI_DSK_DRIVER *)self;

	/* Firstly, ensure any pending changes are flushed to the LDBS 
	 * blockstore. Once this has been done we own the blockstore again 
	 * and have to close it after we've finished with it. */
	err = ldbsdisk_detach(self); 
	if (err)
	{
		dsk_free(cfiself->cfi_filename);
		ldbs_close(&cfiself->cfi_super.ld_store);
		return err;
	}

	/* If this disc image has not been written to, just close it and 
	 * dispose thereof. */
	if (!self->dr_dirty)
	{
		dsk_free(cfiself->cfi_filename);
		return ldbs_close(&cfiself->cfi_super.ld_store);
	}
	/* Trying to save changes but source is read-only */
	if (cfiself->cfi_super.ld_readonly)
	{
		dsk_free(cfiself->cfi_filename);
		ldbs_close(&cfiself->cfi_super.ld_store);
		return DSK_ERR_RDONLY;
	}
	/* OK, write out a CFI file */
	cfiself->cfi_fp = fopen(cfiself->cfi_filename, "wb");
	if (!cfiself->cfi_fp) err = DSK_ERR_SYSERR;
	else
	{
		dsk_report("Compressing CFI file");
		err = ldbs_all_tracks(cfiself->cfi_super.ld_store,
					cfi_save_track, SIDES_ALT, cfiself);

		if (fclose(cfiself->cfi_fp)) err = DSK_ERR_SYSERR;
	
		dsk_report_end();
	}
	if (cfiself->cfi_filename) 
	{
		dsk_free(cfiself->cfi_filename);
		cfiself->cfi_filename = NULL;
	}
	if (!err) err = ldbs_close(&cfiself->cfi_super.ld_store);
	else            ldbs_close(&cfiself->cfi_super.ld_store);
	return err;
}


#if 0
static dsk_err_t cfi_find_sector(CFI_DSK_DRIVER *self, const DSK_GEOMETRY *geom,
				dsk_pcyl_t cylinder, dsk_phead_t head, 
				dsk_psect_t sector, void **buf)
{
	dsk_ltrack_t track;
	CFI_TRACK *ltrk;
	unsigned long offset;

	/* Convert from physical to logical sector. However, unlike the dg_* 
	 * functions, this _always_ uses "SIDES_ALT" mapping; this is the 
	 * mapping that both the Linux and NT floppy drivers use to convert 
	 * offsets back into C/H/S. */
	track = (cylinder * geom->dg_heads) + head;	/* Drive track */

	/* Track out of range */
	if (track >= self->cfi_ntracks) return DSK_ERR_NOADDR;
	ltrk = &(self->cfi_tracks[track]);

	/* Track not loaded (ie, not formatted) */
	if (!ltrk->cfit_data) 
		return DSK_ERR_NOADDR;

	offset = (sector - geom->dg_secbase) * geom->dg_secsize;

	/* Sector out of range */
	if (offset + geom->dg_secsize > ltrk->cfit_length) 
		return DSK_ERR_NOADDR;

	*buf = (ltrk->cfit_data + offset);
	return DSK_ERR_OK;
}


dsk_err_t cfi_read(DSK_DRIVER *self, const DSK_GEOMETRY *geom,
                             void *buf, dsk_pcyl_t cylinder,
                              dsk_phead_t head, dsk_psect_t sector)
{
	CFI_DSK_DRIVER *cfiself;
	void *secbuf;
	dsk_err_t err;

	if (!buf || !self || !geom || self->dr_class != &dc_cfi) return DSK_ERR_BADPTR;
	cfiself = (CFI_DSK_DRIVER *)self;

	if (!cfiself->cfi_filename) return DSK_ERR_NOTRDY;

	err = cfi_find_sector(cfiself, geom, cylinder, head, sector, &secbuf);
	if (err) return err;
	memcpy(buf, secbuf, geom->dg_secsize);
	return DSK_ERR_OK;
}



dsk_err_t cfi_write(DSK_DRIVER *self, const DSK_GEOMETRY *geom,
                             const void *buf, dsk_pcyl_t cylinder,
                              dsk_phead_t head, dsk_psect_t sector)
{
	CFI_DSK_DRIVER *cfiself;
	dsk_err_t err;
	void *secbuf;

	if (!buf || !self || !geom || self->dr_class != &dc_cfi) return DSK_ERR_BADPTR;
	cfiself = (CFI_DSK_DRIVER *)self;

	if (!cfiself->cfi_filename) return DSK_ERR_NOTRDY;
	if (cfiself->cfi_readonly) return DSK_ERR_RDONLY;

	err = cfi_find_sector(cfiself, geom, cylinder, head, sector, &secbuf);
	if (err) return err;
	memcpy(secbuf, buf, geom->dg_secsize);
	cfiself->cfi_dirty = 1;
	return DSK_ERR_OK;
}



dsk_err_t cfi_format(DSK_DRIVER *self, DSK_GEOMETRY *geom,
                                dsk_pcyl_t cylinder, dsk_phead_t head,
                                const DSK_FORMAT *format, unsigned char filler)
{
/*
 * Note that we completely ignore the "format" parameter, since CFI
 * images don't hold track headers.
 */
	CFI_DSK_DRIVER *cfiself;
	dsk_ltrack_t track;
	CFI_TRACK *ltrk;
	long trklen;
	dsk_err_t err;

	(void)format;
	if (!self || !geom || self->dr_class != &dc_cfi) return DSK_ERR_BADPTR;
	cfiself = (CFI_DSK_DRIVER *)self;

	if (!cfiself->cfi_filename) return DSK_ERR_NOTRDY;
	if (cfiself->cfi_readonly) return DSK_ERR_RDONLY;

	/* Convert from physical to logical track. However, unlike the dg_* 
	 * functions, this _always_ uses "SIDES_ALT" mapping; this is the 
	 * mapping that both the Linux and NT floppy drivers use to convert 
	 * offsets back into C/H/S. */
	track = (cylinder * geom->dg_heads) + head;	/* Drive track */

	err = cfi_ensure_size(cfiself, track);
	if (err) return err;

	/* Track out of range */
	if (track >= cfiself->cfi_ntracks) return DSK_ERR_NOADDR;
	ltrk = &(cfiself->cfi_tracks[track]);

	/* If existing data, zap them */
	cfi_free_track(ltrk);

	trklen = geom->dg_sectors * geom->dg_secsize;
	ltrk->cfit_data = dsk_malloc((unsigned)trklen);
	if (!ltrk->cfit_data) return DSK_ERR_NOMEM;
	ltrk->cfit_length = (unsigned)trklen;

	memset(ltrk->cfit_data, filler, (unsigned)trklen);	
	cfiself->cfi_dirty = 1;
	return DSK_ERR_OK;
}

	

dsk_err_t cfi_xseek(DSK_DRIVER *self, const DSK_GEOMETRY *geom,
                      dsk_pcyl_t cylinder, dsk_phead_t head)
{
	CFI_DSK_DRIVER *cfiself;
	unsigned long offset;

	if (!self || !geom || self->dr_class != &dc_cfi) return DSK_ERR_BADPTR;
	cfiself = (CFI_DSK_DRIVER *)self;

	if (!cfiself->cfi_filename) return DSK_ERR_NOTRDY;

	if (cylinder >= geom->dg_cylinders || head >= geom->dg_heads)
		return DSK_ERR_SEEKFAIL;

	/* Convert from physical to logical sector. However, unlike the dg_* 
	 * functions, this _always_ uses "SIDES_ALT" mapping; this is the 
	 * mapping that both the Linux and NT floppy drivers use to convert 
	 * offsets back into C/H/S. */
	offset = (cylinder * geom->dg_heads) + head;	/* Drive track */

	if (offset > cfiself->cfi_ntracks) return DSK_ERR_SEEKFAIL;

	return DSK_ERR_OK;
}

dsk_err_t cfi_status(DSK_DRIVER *self, const DSK_GEOMETRY *geom,
                      dsk_phead_t head, unsigned char *result)
{
	CFI_DSK_DRIVER *cfiself;

	if (!self || !geom || self->dr_class != &dc_cfi) return DSK_ERR_BADPTR;
	cfiself = (CFI_DSK_DRIVER *)self;

	if (!cfiself->cfi_filename) *result &= ~DSK_ST3_READY;
	if (cfiself->cfi_readonly) *result |= DSK_ST3_RO;
	return DSK_ERR_OK;
}
#endif
