/***************************************************************************
 *                                                                         *
 *    LIBDSK: General floppy and diskimage access library                  *
 *    Copyright (C) 2001-2017  John Elliott <seasip.webmaster@gmail.com>   *
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
#include "drvi.h"
#include "drvldbs.h"


/* Access functions for LDBS discs */

DRV_CLASS dc_ldbsdisk = 
{
	sizeof(LDBSDISK_DSK_DRIVER),
	NULL,
	"ldbs\0LDBS\0",
	"LibDsk block store",
	ldbsdisk_open,	/* open */
	ldbsdisk_creat,   /* create new */
	ldbsdisk_close,   /* close */
	ldbsdisk_read,	/* read sector, working from physical address */
	ldbsdisk_write,   /* write sector, working from physical address */
	ldbsdisk_format,  /* format track, physical */
	ldbsdisk_getgeom, /* get geometry */
	ldbsdisk_secid,   /* logical sector ID */
	ldbsdisk_xseek,   /* seek to track */
	ldbsdisk_status,  /* get drive status */
	ldbsdisk_xread,   /* read sector */
	ldbsdisk_xwrite,  /* write sector */ 
	NULL,		/* Read a track (8272 READ TRACK command) */
	NULL,		/* Read a track: Version where the sector ID doesn't necessarily match */
	ldbsdisk_option_enum,	/* List driver-specific options */
	ldbsdisk_option_set,	/* Set a driver-specific option */
	ldbsdisk_option_get,	/* Get a driver-specific option */
	ldbsdisk_trackids,	/* Read headers for an entire track at once */
	NULL,			/* Read raw track, including sector headers */
	ldbsdisk_to_ldbs,	/* Convert to LDBS format (trivially easy) */
	ldbsdisk_from_ldbs,	/* Convert from LDBS format (ditto) */
};




#define DC_CHECK(self) if (!drv_instanceof(self, &dc_ldbsdisk)) return DSK_ERR_BADPTR;




static dsk_err_t ldbsdisk_flush_cur_track(LDBSDISK_DSK_DRIVER *self)
{
	dsk_err_t err = DSK_ERR_OK;

	if (self->ld_cur_track)
	{
		if (self->ld_cur_track->dirty && !self->ld_readonly)
		{
			err = ldbs_put_trackhead(self->ld_store, self->ld_cur_track,
						self->ld_cur_cyl, self->ld_cur_head);
		}

		dsk_free(self->ld_cur_track);
		self->ld_cur_track = NULL;
		self->ld_cur_cyl = -1;
		self->ld_cur_head = -1;
	}
	return err;
}


static dsk_err_t ldbsdisk_select_track(LDBSDISK_DSK_DRIVER *self,
					dsk_pcyl_t cyl, dsk_phead_t head)
{
	dsk_err_t err = DSK_ERR_OK;
	LDBS_TRACKHEAD *t;

	if (self->ld_cur_cyl == cyl && self->ld_cur_head == head)
	{
		return DSK_ERR_OK;	/* Correct track already loaded */
	}
	err = ldbsdisk_flush_cur_track(self);
	if (err) return err;

	err = ldbs_get_trackhead(self->ld_store, &t, cyl, head);
	if (err) return err;

	self->ld_cur_track = t;
	self->ld_cur_cyl = cyl;
	self->ld_cur_head = head;	
	return DSK_ERR_OK;		
}


/* See if the currently-loaded track has the same data rate and recording
 * mode as the caller wants. If not return DSK_ERR_NOADDR */
static dsk_err_t check_density(LDBSDISK_DSK_DRIVER *self, 
				const DSK_GEOMETRY *geom)
{
	unsigned sector_size;
	unsigned char rate, recording;

	/* No track loaded, test is vacuous */
	if (!self->ld_cur_track || 0 == self->ld_cur_track->count) return DSK_ERR_OK;

	/* We have a track loaded... */
	
	/* Check if the track density and recording mode match the density
	 * and recording mode in the geometry. */
	sector_size = 128 << self->ld_cur_track->sector[0].id_psh;

	rate	  = self->ld_cur_track->datarate;
	recording = self->ld_cur_track->recmode;

	/* Guess the data rate used. We assume Double Density, and then
	 * look at the number of sectors in the track to see if the
	 * format looks like a High Density one. */
	if (rate == 0)
	{
		if (sector_size == 1024 && self->ld_cur_track->count >= 7)
		{
			rate = 2; /* ADFS F */
		}
		else if (sector_size == 512 && self->ld_cur_track->count >= 15)
		{
			rate = 2; /* IBM PC 1.2M or 1.4M */
		}
		else rate = 1;
	}
	/* Similarly for recording mode. Note that I check for exactly
	 * 10 sectors, because the MD3 copy protection scheme uses 9 
	 * 256-byte sectors and they're recorded using MFM. */
	if (recording == 0)
	{
		if (sector_size == 256 && self->ld_cur_track->count == 10)
		{
			recording = 1;  /* BBC Micro DFS */
		}
		else recording = 2;
	}
	switch(rate)
	{
		/* 1: Single / Double Density */
		case 1: if (geom->dg_datarate != RATE_SD && 
			    geom->dg_datarate != RATE_DD) return DSK_ERR_NOADDR;
			break;
		/* 2: High density */
		case 2: if (geom->dg_datarate != RATE_HD) return DSK_ERR_NOADDR;
			break;
		/* 3: Extra High Density */
		case 3: if (geom->dg_datarate != RATE_ED) return DSK_ERR_NOADDR;
			break;
		/* Unknown density */
		default:
			return DSK_ERR_NOADDR;
	}
	/* Check data rate */
	switch(recording)
	{
		/* Recording mode: 1 for FM */
		case 1: if ((geom->dg_fm & RECMODE_MASK) != RECMODE_FM) 
				return DSK_ERR_NOADDR;
			break;
		/* Recording mode: 2 for MFM */
		case 2: if ((geom->dg_fm & RECMODE_MASK) != RECMODE_MFM) 
				return DSK_ERR_NOADDR;
			break;
		default:	/* GCR??? */
			return DSK_ERR_NOADDR;
	}
	return DSK_ERR_OK;
}



/* Open DSK image, checking for the magic number */
dsk_err_t ldbsdisk_open(DSK_DRIVER *pdriver, const char *filename)
{
	LDBSDISK_DSK_DRIVER *self;
	dsk_err_t err;
	char type[4];
	
	/* Sanity check: Is this meant for our driver? */
	DC_CHECK(pdriver)
	self = (LDBSDISK_DSK_DRIVER *)pdriver;

	self->ld_readonly = 0;
	err = ldbs_open(&self->ld_store, filename, type, &self->ld_readonly);
	if (err) return err;
	if (memcmp(type, LDBS_DSK_TYPE, 4) &&
	    memcmp(type, LDBS_DSK_TYPE_V1, 4))
	{
		ldbs_close(&self->ld_store);
		return DSK_ERR_NOTME;
	}

	return ldbsdisk_attach(pdriver);
}


dsk_err_t ldbsdisk_attach(DSK_DRIVER *pdriver)
{
	LDBSDISK_DSK_DRIVER *self;
	char *comment;
	dsk_err_t err;

	DC_CHECK(pdriver)
	self = (LDBSDISK_DSK_DRIVER *)pdriver;

	err = ldbs_get_dpb(self->ld_store, &self->ld_dpb);
	if (err) return err;

	err = ldbs_get_comment(self->ld_store, &comment);
	if (err) return err;

	if (comment)
	{
		/* dsk_set_comment() sets the dirty flag, so preserve it */
		int dirty = pdriver->dr_dirty;
;
		dsk_set_comment(pdriver, comment);
		ldbs_free(comment);

		pdriver->dr_dirty = dirty;
	}
	self->ld_cur_cyl = -1;
	self->ld_cur_head = -1;
	self->ld_cur_track = NULL;

	return DSK_ERR_OK;
}






/* Create DSK image */
dsk_err_t ldbsdisk_creat(DSK_DRIVER *pdriver, const char *filename)
{
	LDBSDISK_DSK_DRIVER *self;
	dsk_err_t err;
	
	/* Sanity check: Is this meant for our driver? */
	DC_CHECK(pdriver)
	self = (LDBSDISK_DSK_DRIVER *)pdriver;

	err = ldbs_new(&self->ld_store, filename, LDBS_DSK_TYPE);
	if (err) return err;

	err = ldbs_put_creator (self->ld_store, "LIBDSK " LIBDSK_VERSION);
	if (err) return err;

	/* File created. Set up our internal structures */
	self->ld_readonly = 0;
	self->ld_sector = 0;
	memset(&self->ld_dpb, 0, sizeof(self->ld_dpb));

	self->ld_cur_track = NULL;
	self->ld_cur_cyl = -1;
	self->ld_cur_head = -1;
	return DSK_ERR_OK;
}


dsk_err_t ldbsdisk_close(DSK_DRIVER *pdriver)
{
	LDBSDISK_DSK_DRIVER *self;
	dsk_err_t err;

	DC_CHECK(pdriver)
	self = (LDBSDISK_DSK_DRIVER *)pdriver;

	err = ldbsdisk_detach(pdriver);
	if (err) return err;

	if (!self->ld_store)
	{
		return DSK_ERR_OK;
	}
	return ldbs_close(&self->ld_store);
}



dsk_err_t ldbsdisk_detach(DSK_DRIVER *pdriver)
{
	LDBSDISK_DSK_DRIVER *self;
	char *comment = NULL;

	DC_CHECK(pdriver)
	self = (LDBSDISK_DSK_DRIVER *)pdriver;

	ldbsdisk_flush_cur_track(self);

	/* If the DPB has been populated, record it. A valid CP/M DPB
	 * must have at least SPT, DSM, DRM and AL0 populated */
	if (self->ld_dpb.spt && self->ld_dpb.dsm && self->ld_dpb.drm && 
	    self->ld_dpb.al[0] && !self->ld_readonly)
	{
		ldbs_put_dpb(self->ld_store, &self->ld_dpb);
	}

        dsk_get_comment(pdriver, &comment);
	ldbs_put_comment(self->ld_store, comment);

	ldbs_sync(self->ld_store);	

	return DSK_ERR_OK;
}




/* Read a sector ID from a given track */
dsk_err_t ldbsdisk_secid(DSK_DRIVER *pdriver, const DSK_GEOMETRY *geom,
			dsk_pcyl_t cyl, dsk_phead_t head, DSK_FORMAT *result)
{
	dsk_err_t err;
	LDBSDISK_DSK_DRIVER *self;
	int n;

	DC_CHECK(pdriver)
	self = (LDBSDISK_DSK_DRIVER *)pdriver;

	err = ldbsdisk_select_track(self, cyl, head);
	if (err) return err;

	if (!self->ld_cur_track)	/* Track not found */
	{
		self->ld_sector = 0;
		return DSK_ERR_NOADDR;
	}
	/* Check the track was recorded with the requested density and
	 * recording mode */
	err = check_density(self, geom);
	if (err) return err;

	n = self->ld_sector % self->ld_cur_track->count;

	if (result)
	{
		result->fmt_cylinder = self->ld_cur_track->sector[n].id_cyl;
		result->fmt_head     = self->ld_cur_track->sector[n].id_head;
		result->fmt_sector   = self->ld_cur_track->sector[n].id_sec;
		result->fmt_secsize  = 128 << self->ld_cur_track->sector[n].id_psh;
	}	

	++self->ld_sector;
	return DSK_ERR_OK;
}






static dsk_err_t lookup_sector(LDBSDISK_DSK_DRIVER *self, dsk_pcyl_t cyl_expect,
			dsk_phead_t head_expect, dsk_psect_t sector,
			size_t size_expected, size_t *size_actual,
			LDBS_SECTOR_ENTRY **result)
{
	int n;

	*result = NULL;
	/* Look for the sector we want */ 
	for (n = 0; n < self->ld_cur_track->count; n++)
	{
		if (self->ld_cur_track->sector[n].id_cyl  == cyl_expect &&
		    self->ld_cur_track->sector[n].id_head == head_expect &&
		    self->ld_cur_track->sector[n].id_sec  == sector)
		{
			*result = &self->ld_cur_track->sector[n];
			break;	
		}
	}
	if (!*result) return DSK_ERR_NOADDR; /* No matching sector found */

	/* Found and it's the right size */	
	*size_actual = 128 << result[0]->id_psh;
	if (*size_actual > size_expected)
	{
		*size_actual = size_expected;
		return DSK_ERR_DATAERR;
	}
	if (*size_actual < size_expected)
	{
		return DSK_ERR_DATAERR;
	}
	return DSK_ERR_OK;
}


/* Read a sector */
dsk_err_t ldbsdisk_read(DSK_DRIVER *self, const DSK_GEOMETRY *geom,
		      void *buf, dsk_pcyl_t cylinder,
		      dsk_phead_t head, dsk_psect_t sector)
{
	return ldbsdisk_xread(self, geom, buf, cylinder, head, cylinder,
				dg_x_head(geom, head), 
				dg_x_sector(geom, head, sector), 
				geom->dg_secsize, 0);
}




dsk_err_t ldbsdisk_xread(DSK_DRIVER *pdriver, const DSK_GEOMETRY *geom, void *buf, 
		       dsk_pcyl_t cylinder,   dsk_phead_t head, 
		       dsk_pcyl_t cyl_expect, dsk_phead_t head_expect,
		       dsk_psect_t sector, size_t size_expect, int *deleted)
{
	dsk_err_t err;
	LDBSDISK_DSK_DRIVER *self;
	int n;
	int rdeleted = 0;
	size_t size_actual = size_expect;
	int try_again = 0;
	LDBS_SECTOR_ENTRY *cursec = NULL;
	unsigned char *result = (unsigned char *)buf;
	
	if (!buf || !geom || !pdriver) return DSK_ERR_BADPTR;
	DC_CHECK(pdriver)
	self = (LDBSDISK_DSK_DRIVER *)pdriver;

	/* See if the requested cylinder exists */
	err = ldbsdisk_select_track(self, cylinder, head);
	if (err) return err;	

	if (!self->ld_cur_track) return DSK_ERR_NOADDR;

	/* Check the track was recorded with the requested density and
	 * recording mode */
	err = check_density(self, geom);
	if (err) return err;

	/* Check if we're supposed to be reading deleted sectors */
	if (deleted && *deleted) rdeleted = 0x40;

	do
	{
		err = lookup_sector(self, cyl_expect, head_expect, sector,
					size_expect, &size_actual, &cursec);

/* Are we retrying because we are looking for deleted data and found 
 * nondeleted or vice versa?
 *
 * If so, and we have run out of sectors in this track, AND we are on head 0,
 * AND the disc has 2 heads, AND we are in multitrack mode, then look on head 1
 * as well. Amazing.
 * */
                if (try_again == 1 && !cursec)
                {
                        err = DSK_ERR_NODATA;
                        if ((!geom->dg_nomulti) && head == 0)
                        {
/* OK, we're in multitrack mode and on head 0. Try to load headers for 
 * head 1 */
                                head++;
				err = ldbsdisk_select_track(self, cylinder, head);
				if (err) return err;	
				err = check_density(self, geom);
				if (err) return err;
				if (!self->ld_cur_track) return DSK_ERR_NOADDR;
                                sector = geom->dg_secbase;
                                continue;
                        }
                }
		try_again = 0;
		if (err == DSK_ERR_NOADDR) self->ld_sector = 0;

		/* If we couldn't find the sector at all, don't try to
		 * read it */
		if (err != DSK_ERR_DATAERR && err != DSK_ERR_OK)
			return err;

		if (deleted) *deleted = 0;

		if (rdeleted != (cursec->st1 & 0x40))
		{
			/* We want non-deleted and got deleted, or vice versa */
			if (geom->dg_noskip)
			{
				if (deleted) *deleted = 0;
			}
			else
			{
				try_again = 1;
				++sector;
				continue;
			}
		}

		/* OK. cursec does appear to be pointing at an 
		 * actual sector. Is the sector blank? */
		if (cursec->copies == 0 || cursec->blockid == LDBLOCKID_NULL)
		{
			for (n = 0; n < (int)size_actual; n++)
			{
				result[n] = cursec->filler;
			}	
		}
		else
		{
			/* Sector really exists. Load it. */
			unsigned char *secbuf;
			size_t sblen;
			size_t offset = 0;
			char sbtype[4];
			dsk_err_t err2;		
	
			err2 = ldbs_getblock_a(self->ld_store, cursec->blockid,
					sbtype, (void **)&secbuf, &sblen);
			if (err2) return err2;

			/* Size on disk is smaller than expected size? */
			if (size_actual > sblen)
			{
				size_actual = sblen;
				if (!err) err = DSK_ERR_DATAERR;
			}
			/* Offset for multiple copies */
			if (cursec->copies > 1)
			{
				offset = (rand() % cursec->copies) * 
					(128 << cursec->id_psh);
			}
			/* Should never happen! */
			if (offset + size_actual > sblen)
			{
				offset = 0;
			}	
		
			memcpy(result, secbuf + offset, size_actual);
			ldbs_free(secbuf);	
		}

		/* LDBS disks, like CPCEMU disks, can record errors made at
		 * the time of imaging */
		if (err == DSK_ERR_OK && 
			((cursec->st1 & 0x01) || (cursec->st2 & 0x01)))
		{
			err = DSK_ERR_NOADDR;
		}
		if (err == DSK_ERR_OK && (cursec->st1 & 0x04))
		{
			err = DSK_ERR_NODATA;
		}
		if (err == DSK_ERR_OK && 
			((cursec->st1 & 0x20) || (cursec->st2 & 0x20)))
		{
			err = DSK_ERR_DATAERR;
		}

	}
	while (try_again);
	return err;

}


/* Write a sector */
dsk_err_t ldbsdisk_write(DSK_DRIVER *self, const DSK_GEOMETRY *geom,
			const void *buf, dsk_pcyl_t cylinder,
			dsk_phead_t head, dsk_psect_t sector)
{
	return ldbsdisk_xwrite(self, geom, buf, cylinder, head, cylinder,
				dg_x_head(geom, head),
				dg_x_sector(geom, head, sector), 
				geom->dg_secsize, 0);
}

dsk_err_t ldbsdisk_xwrite(DSK_DRIVER *pdriver, const DSK_GEOMETRY *geom, 
			  const void *buf, 
			  dsk_pcyl_t cylinder,   dsk_phead_t head, 
			  dsk_pcyl_t cyl_expect, dsk_phead_t head_expect,
			  dsk_psect_t sector, size_t size_expect,
			  int deleted)
{
	dsk_err_t err;
	LDBSDISK_DSK_DRIVER *self;
	int n;
	size_t size_actual = size_expect;
	int allsame;
	LDBS_SECTOR_ENTRY *cursec;
	unsigned char *data = (unsigned char *)buf;
	
	if (!buf || !geom || !pdriver) return DSK_ERR_BADPTR;
	DC_CHECK(pdriver)
	self = (LDBSDISK_DSK_DRIVER *)pdriver;
	
	if (self->ld_readonly) return DSK_ERR_RDONLY;

	/* See if the requested sector contains all the same values */
	allsame = 1;
	for (n = 1; n < (int)size_expect; n++)
	{
		if (data[n] != data[0]) 
		{
			allsame = 0;
			break;
		}
	}

	/* See if the requested cylinder exists */
	err = ldbsdisk_select_track(self, cylinder, head);
	if (err) return err;	

	if (!self->ld_cur_track) return DSK_ERR_NOADDR;

	/* Check the track was recorded with the requested density and
	 * recording mode */
	err = check_density(self, geom);
	if (err) return err;

	err = lookup_sector(self, cyl_expect, head_expect, sector,
					size_expect, &size_actual, &cursec);

	if (err != DSK_ERR_DATAERR && err != DSK_ERR_OK) return err;

	/* Sector found! */
	if (allsame)
	{
		/* This is a blank sector. Record it as blank in the header,
		 * and wipe it from the disk file */
		cursec->filler = data[0];
		cursec->copies = 0;
		if (cursec->blockid != LDBLOCKID_NULL)
		{
			err = ldbs_delblock(self->ld_store, cursec->blockid);
			cursec->blockid = LDBLOCKID_NULL;
		}
	}
	else
	{
		/* This is a non-blank sector. Write it to the file. */
		char type[4];

		type[0] = 'S';
		type[1] = cylinder;
		type[2] = head;
		type[3] = sector;

		cursec->copies = 1;	/* We don't support multiple copies */
		cursec->trail  = 0;	/* We don't support trailing bytes */
		err = ldbs_putblock(self->ld_store, &cursec->blockid, type,
				buf, size_expect);
	}

	self->ld_cur_track->dirty = 1;
	return err;
}


dsk_err_t ldbsdisk_wipe_track(LDBSDISK_DSK_DRIVER *self)
{
	int sector;
	LDBLOCKID blkid = LDBLOCKID_NULL;
	dsk_err_t err;

	if (self->ld_cur_track == NULL) return DSK_ERR_OK;

	/* Blow away all the track's sectors. Write back the header,
	 * now empty */
	for (sector = 0; sector < self->ld_cur_track->count; sector++)
	{
		blkid = self->ld_cur_track->sector[sector].blockid;
		if (blkid != LDBLOCKID_NULL)
		{
			err = ldbs_delblock(self->ld_store, blkid);
			if (err) return err;
			self->ld_cur_track->sector[sector].copies = 0;
			self->ld_cur_track->sector[sector].trail = 0;
			self->ld_cur_track->sector[sector].blockid = LDBLOCKID_NULL;
			self->ld_cur_track->dirty = 1;
		}	
	}
	return ldbsdisk_flush_cur_track(self);
}



/* Format a track on a DSK. Can grow the DSK file. */
dsk_err_t ldbsdisk_format(DSK_DRIVER *pdriver, DSK_GEOMETRY *geom,
			dsk_pcyl_t cylinder, dsk_phead_t head,
			const DSK_FORMAT *format, unsigned char filler)
{
	dsk_err_t err;
	LDBSDISK_DSK_DRIVER *self;
	dsk_psect_t sector;
	
	if (!format || !geom || !pdriver) return DSK_ERR_BADPTR;
	DC_CHECK(pdriver)
	self = (LDBSDISK_DSK_DRIVER *)pdriver;

	if (self->ld_readonly) return DSK_ERR_RDONLY;

	err = ldbsdisk_select_track(self, cylinder, head);
	if (err) return err;

	err = ldbsdisk_wipe_track(self);
	if (err) return err;

	/* The track's sectors have been removed. All that remains is 
	 * its header (empty). Create a new header that will replace it. */
	self->ld_cur_track = ldbs_trackhead_alloc(geom->dg_sectors);
	if (!self->ld_cur_track)
	{
		return DSK_ERR_NOMEM;
	}
	self->ld_cur_cyl = cylinder;
	self->ld_cur_head = head;

	/* Populate the track header */
	switch (geom->dg_datarate)
	{
		case RATE_SD: self->ld_cur_track->datarate = 1; break;
		case RATE_DD: self->ld_cur_track->datarate = 1; break;
		case RATE_HD: self->ld_cur_track->datarate = 2; break;
		case RATE_ED: self->ld_cur_track->datarate = 3; break;
	}
	switch (geom->dg_fm & RECMODE_MASK)
	{
		case RECMODE_FM:  self->ld_cur_track->recmode = 1; break;
		case RECMODE_MFM: self->ld_cur_track->recmode = 2; break;
	}
	self->ld_cur_track->gap3 = geom->dg_fmtgap;
	self->ld_cur_track->filler = filler;
	self->ld_cur_track->dirty = 1;

	/* Write the sectors. Since they are all blank we don't actually
	 * need to send them to the file */
	for (sector = 0; sector < geom->dg_sectors; sector++)
	{
		LDBS_SECTOR_ENTRY *secent = &self->ld_cur_track->sector[sector];

		secent->id_cyl  = format[sector].fmt_cylinder;
		secent->id_head = format[sector].fmt_head;
		secent->id_sec  = format[sector].fmt_sector;
		secent->id_psh  = dsk_get_psh(format[sector].fmt_secsize);
		secent->st1 = 0;
		secent->st2 = 0;
		secent->copies = 0;
		secent->trail = 0;
		secent->offset = 0;	/* XXX Can probably calculate a */
		secent->filler = filler;/* default value for 'offset' */
	}
	/* Write the track header */
	err = ldbsdisk_flush_cur_track(self);
	if (err) return err;

	/* Populate bits of the DPB that the standard geometry probe
	 * does not */
	self->ld_dpb.spt = geom->dg_sectors * (geom->dg_secsize / 128);
	self->ld_dpb.psh = dsk_get_psh(geom->dg_secsize);
	self->ld_dpb.phm = (1 << self->ld_dpb.psh) - 1;

	err = ldbs_put_geometry(self->ld_store, geom);
	return err;
}

typedef struct
{
	DSK_GEOMETRY dg;
	int minsec[2], maxsec[2];
	int extsurface;
	int secsizes[8];
} GETGEOM_STATS;


static dsk_err_t getgeom_callback(PLDBS ldbs, dsk_pcyl_t cyl, dsk_phead_t head,
				LDBS_SECTOR_ENTRY *se, LDBS_TRACKHEAD *th,
				void *param)
{
	GETGEOM_STATS *stats = param;
	int h;

	/* Calculate cylinder and head count */
	if (cyl + 1 > stats->dg.dg_cylinders)
		stats->dg.dg_cylinders = cyl + 1;	
	if (head + 1 > stats->dg.dg_heads)
		stats->dg.dg_heads = cyl + 1;	
	if (th->count > stats->dg.dg_sectors)
		stats->dg.dg_sectors = th->count;
	
	if (se->id_psh < 8)
	{
		++stats->secsizes[se->id_psh];
	}

	h = head ? 1 : 0;
	if (se->id_psh < stats->minsec[h]) stats->minsec[h] = se->id_psh;	
	if (se->id_psh > stats->maxsec[h]) stats->maxsec[h] = se->id_psh;	

	/* Possibly 'extended surface' geometry if sectors on head 1 
	 * have head IDs of 0 */
	if (h && se->id_head == 0) stats->extsurface = 1;

	switch (th->datarate)
	{
		case 1: stats->dg.dg_datarate = RATE_SD; break;
		case 2: stats->dg.dg_datarate = RATE_HD; break;
		case 3: stats->dg.dg_datarate = RATE_ED; break;
	}
	switch (th->recmode)
	{
		case 1: stats->dg.dg_fm = RECMODE_FM; break;
		case 2: stats->dg.dg_fm = RECMODE_MFM; break;
	}

	return DSK_ERR_OK;	
}


/* Get drive geometry */
dsk_err_t ldbsdisk_getgeom(DSK_DRIVER *pdriver, DSK_GEOMETRY *geom)
{
	dsk_err_t err, err2;
	LDBSDISK_DSK_DRIVER *self;
	GETGEOM_STATS stats;
	int n, count = -1;	

	DC_CHECK(pdriver)
	self = (LDBSDISK_DSK_DRIVER *)pdriver;

	/* Firstly try the default LibDsk geometry probe */
	err = dsk_defgetgeom(pdriver, geom);
	if (!err) return DSK_ERR_OK;

	/* If that fails, see if the LDBS file describes its own geometry */
	err2 = ldbs_get_geometry(self->ld_store, geom);
	if (err2) return err2;

	/* If it populated 'geom', it does. Otherwise no dice. */
	if (geom->dg_cylinders && geom->dg_heads && 
	    geom->dg_sectors && geom->dg_secsize)
	{
		return DSK_ERR_OK;
	}
	/* This was in imd_geom() in previous versions. Basically, a 
	 * fully-populated LDBS file contains enough information to 
	 * give us a decent chance at determining the drive geometry 
	 * ourselves. */
	memset(&stats, 0, sizeof(stats));
	dg_stdformat(&stats.dg, FMT_180K, NULL, NULL);
	stats.minsec[0] = stats.minsec[1] = 256;
	stats.maxsec[0] = stats.maxsec[1] = 0;

	err = ldbs_all_sectors(self->ld_store, getgeom_callback,
				SIDES_ALT, &stats);
	if (err) return DSK_ERR_BADFMT;

	/* Report the most frequent sector size */
	for (n = 0; n < 8; n++)
	{
		if (stats.secsizes[n] > count)
		{
			count = stats.secsizes[n];
			stats.dg.dg_secsize = 128 << n;
		}
	}

	stats.dg.dg_secbase = stats.minsec[0];
	/* Check for an 'extended surface' format: 2 heads, the 
	 * sector ranges on each side are disjoint, and sectors on head 1
	 * are labelled as head 0 */
	if (stats.dg.dg_heads == 2 && (stats.maxsec[0] + 1 == 
			stats.minsec[1]) && stats.extsurface)
	{
		stats.dg.dg_sidedness = SIDES_EXTSURFACE;
	}
	if (stats.dg.dg_cylinders == 0 || stats.dg.dg_sectors == 0)
	{
		return DSK_ERR_BADFMT;
	}
	memcpy(geom, &stats.dg, sizeof(*geom));
	return DSK_ERR_OK;
}


/* Seek to a cylinder. */
dsk_err_t ldbsdisk_xseek(DSK_DRIVER *pdriver, const DSK_GEOMETRY *geom,
					dsk_pcyl_t cyl, dsk_phead_t head)
{
	LDBSDISK_DSK_DRIVER *self;

	DC_CHECK(pdriver)
	self = (LDBSDISK_DSK_DRIVER *)pdriver;
	return ldbsdisk_select_track(self, cyl, head);
}


dsk_err_t ldbsdisk_trackids(DSK_DRIVER *pdriver, const DSK_GEOMETRY *geom,
			  dsk_pcyl_t cyl, dsk_phead_t head,
			  dsk_psect_t *count, DSK_FORMAT **result)
{
	LDBSDISK_DSK_DRIVER *self;
	dsk_err_t e;
	int n;


	if (!pdriver || !geom || !result)
		return DSK_ERR_BADPTR;
	DC_CHECK(pdriver)
	self = (LDBSDISK_DSK_DRIVER *)pdriver;

	e = ldbsdisk_select_track(self, cyl, head);
	if (e) return e;

	/* Is there a track there? */
	if (!self->ld_cur_track) return DSK_ERR_NOADDR;
	//
	*result = dsk_malloc( self->ld_cur_track->count * sizeof(DSK_FORMAT) );
	if (!(*result)) return DSK_ERR_NOMEM;

	*count = self->ld_cur_track->count;
	for (n = 0; n < (int)(*count); n++)
	{
		LDBS_SECTOR_ENTRY *cursec = &self->ld_cur_track->sector[n];

		(*result)[n].fmt_cylinder = cursec->id_cyl;
		(*result)[n].fmt_head     = cursec->id_head;
		(*result)[n].fmt_sector   = cursec->id_sec;
		(*result)[n].fmt_secsize  = (128 << cursec->id_psh);
	}

	return DSK_ERR_OK;
}

dsk_err_t ldbsdisk_status(DSK_DRIVER *pdriver, const DSK_GEOMETRY *geom,
					  dsk_phead_t head, unsigned char *result)
{
	LDBSDISK_DSK_DRIVER *self;

	if (!pdriver || !geom)
	{
		return DSK_ERR_BADPTR;
	}
	DC_CHECK(pdriver)
	self = (LDBSDISK_DSK_DRIVER *)pdriver;
	if (self->ld_readonly) *result |= DSK_ST3_RO;
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

dsk_err_t ldbsdisk_option_enum(DSK_DRIVER *self, int idx, char **optname)
{
	if (!self) return DSK_ERR_BADPTR;
	if (self->dr_class != &dc_ldbsdisk) return DSK_ERR_BADPTR;

	if (idx >= 0 && idx < (int)MAXOPTION)
	{
		if (optname) *optname = option_names[idx];
		return DSK_ERR_OK;
	}
	return DSK_ERR_BADOPT;	

}

/* Set a driver-specific option */
dsk_err_t ldbsdisk_option_set(DSK_DRIVER *self, const char *optname, int value)
{
	LDBSDISK_DSK_DRIVER *ldbs_self;
	unsigned idx;

	if (!self || !optname) return DSK_ERR_BADPTR;
	if (self->dr_class != &dc_ldbsdisk) return DSK_ERR_BADPTR;
	ldbs_self = (LDBSDISK_DSK_DRIVER *)self;

	for (idx = 0; idx < MAXOPTION; idx++)
	{
		if (!strcmp(optname, option_names[idx]))
			break;
	}
	if (idx >= MAXOPTION) return DSK_ERR_BADOPT;

	switch(idx)
	{
		case 0: ldbs_self->ld_dpb.bsh = value;	// BSH
			break;
		case 1: ldbs_self->ld_dpb.blm = value;	// BLM
			break;
		case 2: ldbs_self->ld_dpb.exm = value;	// EXM
			break;
		case 3: ldbs_self->ld_dpb.dsm = value;	// DSM
			break;
		case 4: ldbs_self->ld_dpb.drm = value;	// DRM
			break;
		case 5: ldbs_self->ld_dpb.al[0] = value;// AL0 
			break;
		case 6: ldbs_self->ld_dpb.al[1] = value;// AL1 
			break;
		case 7: ldbs_self->ld_dpb.cks = value;	// CKS
			break;
		case 8: ldbs_self->ld_dpb.off = value;	// OFF
			break;
	}
	return DSK_ERR_OK;
}


dsk_err_t ldbsdisk_option_get(DSK_DRIVER *self, const char *optname, int *value)
{
	LDBSDISK_DSK_DRIVER *ldbs_self;
	unsigned idx;
	int v = 0;

	if (!self || !optname) return DSK_ERR_BADPTR;
	if (self->dr_class != &dc_ldbsdisk) return DSK_ERR_BADPTR;
	ldbs_self = (LDBSDISK_DSK_DRIVER *)self;

	for (idx = 0; idx < MAXOPTION; idx++)
	{
		if (!strcmp(optname, option_names[idx]))
			break;
	}
	if (idx >= MAXOPTION) return DSK_ERR_BADOPT;

	switch(idx)
	{
		case 0: v = ldbs_self->ld_dpb.bsh;	// BSH
			break;
		case 1: v = ldbs_self->ld_dpb.blm;	// BLM
			break;
		case 2: v = ldbs_self->ld_dpb.exm;	// EXM
			break;
		case 3: v = ldbs_self->ld_dpb.dsm;	// DSM
			break;
		case 4: v = ldbs_self->ld_dpb.drm;	// DRM
			break;
		case 5: v = ldbs_self->ld_dpb.al[0];	// AL0 
			break;
		case 6: v = ldbs_self->ld_dpb.al[1];	// AL1 
			break;
		case 7: v = ldbs_self->ld_dpb.cks;	// AL1 
			break;
		case 8: v = ldbs_self->ld_dpb.off;	// OFF
			break;
	}
	if (value) *value = v;
	return DSK_ERR_OK;
}


/* Convert to LDBS format. Basically: detach from our blockstore, 
 * flush it, clone it, and reattach. */
dsk_err_t ldbsdisk_to_ldbs(DSK_DRIVER *self, struct ldbs **result, 						DSK_GEOMETRY *geom)
{
	dsk_err_t err;
	LDBSDISK_DSK_DRIVER *ldbs_self;

	if (!self || !result) return DSK_ERR_BADPTR;
	DC_CHECK(self)
	ldbs_self = (LDBSDISK_DSK_DRIVER *)self;

	err = ldbs_new(result, NULL, LDBS_DSK_TYPE);
	if (err) return err;

	/* Ensure our blockstore is up to date */
	err = ldbsdisk_detach(self);
	if (err)
	{
		ldbs_close(result);
		return err;
	}
	err = ldbs_clone(ldbs_self->ld_store, *result);
	if (err)
	{
		ldbsdisk_attach(self);
		return err;
	}
	return ldbsdisk_attach(self);
}

/* Import from LDBS format. */
dsk_err_t ldbsdisk_from_ldbs(DSK_DRIVER *self, struct ldbs *source, 
				DSK_GEOMETRY *geom)
{
	dsk_err_t err;
	LDBSDISK_DSK_DRIVER *ldbs_self;

	if (!self || !source) return DSK_ERR_BADPTR;
	DC_CHECK(self)
	ldbs_self = (LDBSDISK_DSK_DRIVER *)self;

	if (ldbs_self->ld_readonly) return DSK_ERR_RDONLY;

	/* Detach from our current blockstore */	
	err = ldbsdisk_detach(self);
	if (err) return err;

	/* Copy from the source file into it */
	err = ldbs_clone(source, ldbs_self->ld_store);
	if (err) return err;

	/* And reattach to it */
	self->dr_dirty = 1;
	return ldbsdisk_attach(self);
}

