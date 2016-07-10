/***************************************************************************
 *                                                                         *
 *    LIBDSK: General floppy and diskimage access library                  *
 *    Copyright (C) 2001-2016  John Elliott <seasip.webmaster@gmail.com>   *
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
#include "ldbsdisk.h"

#define LDBSDISK_MAGIC "DSK\1"
#define LDBSDIR_MAGIC  "DIR\1"

/* Access functions for LDBS discs */

DRV_CLASS dc_ldbsdisk = 
{
	sizeof(LDBSDISK_DSK_DRIVER),
	"ldbs",
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
	NULL			/* Read raw track, including sector headers */
};




#define DC_CHECK(self) if (self->dr_class != &dc_ldbsdisk) return DSK_ERR_BADPTR;




static dsk_err_t ldbsdisk_flush_cur_track(LDBSDISK_DSK_DRIVER *self)
{
	dsk_err_t err = DSK_ERR_OK;

	if (self->cur_track)
	{
		if (self->cur_track->dirty && !self->readonly)
		{
			err = ldbs_put_trackhead(self->store, self->cur_track,
						self->cur_cyl, self->cur_head);
		}

		dsk_free(self->cur_track);
		self->cur_track = NULL;
		self->cur_cyl = -1;
		self->cur_head = -1;
	}
	return err;
}


static dsk_err_t ldbsdisk_select_track(LDBSDISK_DSK_DRIVER *self,
					dsk_pcyl_t cyl, dsk_phead_t head)
{
	dsk_err_t err = DSK_ERR_OK;

	if (self->cur_cyl == cyl && self->cur_head == head)
	{
		return DSK_ERR_OK;	/* Correct track already loaded */
	}
	err = ldbsdisk_flush_cur_track(self);
	if (err) return err;

	LDBS_TRACKHEAD *t;

	err = ldbs_get_trackhead(self->store, &t, cyl, head);
	if (err) return err;

	self->cur_track = t;
	self->cur_cyl = cyl;
	self->cur_head = head;	
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
	if (!self->cur_track || 0 == self->cur_track->count) return DSK_ERR_OK;

	/* We have a track loaded... */
	
	/* Check if the track density and recording mode match the density
	 * and recording mode in the geometry. */
	sector_size = 128 << self->cur_track->sector[0].id_psh;

	rate	  = self->cur_track->datarate;
	recording = self->cur_track->recmode;

	/* Guess the data rate used. We assume Double Density, and then
	 * look at the number of sectors in the track to see if the
	 * format looks like a High Density one. */
	if (rate == 0)
	{
		if (sector_size == 1024 && self->cur_track->count >= 7)
		{
			rate = 2; /* ADFS F */
		}
		else if (sector_size == 512 && self->cur_track->count >= 15)
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
		if (sector_size == 256 && self->cur_track->count == 10)
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
	char type[4];
	dsk_err_t err;
	
	/* Sanity check: Is this meant for our driver? */
	DC_CHECK(pdriver)
	self = (LDBSDISK_DSK_DRIVER *)pdriver;

	self->readonly = 0;
	err = ldbs_open(&self->store, filename, type, &self->readonly);
	if (err) return err;
	if (memcmp(type, LDBS_DSK_TYPE, 4))
	{
		ldbs_close(&self->store);
		return DSK_ERR_NOTME;
	}
	err = ldbs_get_dpb(self->store, &self->dpb);
	if (err) return err;

	self->cur_cyl = -1;
	self->cur_head = -1;
	self->cur_track = NULL;

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

	err = ldbs_new(&self->store, filename, LDBSDISK_MAGIC);
	if (err) return err;

	err = ldbs_put_creator (self->store, "LIBDSK " LIBDSK_VERSION);
	if (err) return err;

	/* File created. Set up our internal structures */
	self->readonly = 0;
	self->sector = 0;
	memset(&self->dpb, 0, sizeof(self->dpb));

	self->cur_track = NULL;
	self->cur_cyl = -1;
	self->cur_head = -1;
	return DSK_ERR_OK;
}


dsk_err_t ldbsdisk_close(DSK_DRIVER *pdriver)
{
	LDBSDISK_DSK_DRIVER *self;

	DC_CHECK(pdriver)
	self = (LDBSDISK_DSK_DRIVER *)pdriver;

	ldbsdisk_flush_cur_track(self);

	/* If the DPB has been populated, record it. A valid CP/M DPB
	 * must have at least SPT, DSM, DRM and AL0 populated */
	if (self->dpb.spt && self->dpb.dsm && self->dpb.drm && 
	    self->dpb.al[0] && !self->readonly)
	{
		ldbs_put_dpb(self->store, &self->dpb);
	}

	if (!self->store) 
	{
		return DSK_ERR_OK;
	}
	return ldbs_close(&self->store);
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

	if (!self->cur_track)	/* Track not found */
	{
		self->sector = 0;
		return DSK_ERR_NOADDR;
	}
	/* Check the track was recorded with the requested density and
	 * recording mode */
	err = check_density(self, geom);
	if (err) return err;

	n = self->sector % self->cur_track->count;

	if (result)
	{
		result->fmt_cylinder = self->cur_track->sector[n].id_cyl;
		result->fmt_head     = self->cur_track->sector[n].id_head;
		result->fmt_sector   = self->cur_track->sector[n].id_sec;
		result->fmt_secsize  = 128 << self->cur_track->sector[n].id_psh;
	}	

	++self->sector;
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
	for (n = 0; n < self->cur_track->count; n++)
	{
		if (self->cur_track->sector[n].id_cyl  == cyl_expect &&
		    self->cur_track->sector[n].id_head == head_expect &&
		    self->cur_track->sector[n].id_sec  == sector)
		{
			*result = &self->cur_track->sector[n];
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
	LDBS_SECTOR_ENTRY *cursec;
//	LDBLOCKID sector_id = LDBLOCKID_NULL;
	unsigned char *result = (unsigned char *)buf;
	
	if (!buf || !geom || !pdriver) return DSK_ERR_BADPTR;
	DC_CHECK(pdriver)
	self = (LDBSDISK_DSK_DRIVER *)pdriver;

	/* See if the requested cylinder exists */
	err = ldbsdisk_select_track(self, cylinder, head);
	if (err) return err;	

	if (!self->cur_track) return DSK_ERR_NOADDR;

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
				if (!self->cur_track) return DSK_ERR_NOADDR;
                                sector = geom->dg_secbase;
                                continue;
                        }
                }
		try_again = 0;
		if (err == DSK_ERR_NOADDR) self->sector = 0;

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
			for (n = 0; n < size_actual; n++)
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
	
			err2 = ldbs_getblock_a(self->store, cursec->blockid,
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
	
	if (self->readonly) return DSK_ERR_RDONLY;

	/* See if the requested sector contains all the same values */
	allsame = 1;
	for (n = 1; n < size_expect; n++)
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

	if (!self->cur_track) return DSK_ERR_NOADDR;

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
			err = ldbs_delblock(self->store, cursec->blockid);
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
		err = ldbs_putblock(self->store, &cursec->blockid, type,
				buf, size_expect);
	}

	self->cur_track->dirty = 1;
	return err;
}


dsk_err_t ldbsdisk_wipe_track(LDBSDISK_DSK_DRIVER *self)
{
	int sector;
	LDBLOCKID blkid = LDBLOCKID_NULL;
	dsk_err_t err;

	if (self->cur_track == NULL) return DSK_ERR_OK;

	/* Blow away all the track's sectors. Write back the header,
	 * now empty */
	for (sector = 0; sector < self->cur_track->count; sector++)
	{
		blkid = self->cur_track->sector[sector].blockid;
		if (blkid != LDBLOCKID_NULL)
		{
			err = ldbs_delblock(self->store, blkid);
			if (err) return err;
			self->cur_track->sector[sector].copies = 0;
			self->cur_track->sector[sector].blockid = LDBLOCKID_NULL;
			self->cur_track->dirty = 1;
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

	if (self->readonly) return DSK_ERR_RDONLY;

	err = ldbsdisk_select_track(self, cylinder, head);
	if (err) return err;

	err = ldbsdisk_wipe_track(self);
	if (err) return err;

	/* The track's sectors have been removed. All that remains is 
	 * its header (empty). Create a new header that will replace it. */
	self->cur_track = ldbs_trackhead_alloc(geom->dg_sectors);
	if (!self->cur_track)
	{
		return DSK_ERR_NOMEM;
	}
	self->cur_cyl = cylinder;
	self->cur_head = head;

	/* Populate the track header */
	switch (geom->dg_datarate)
	{
		case RATE_SD: self->cur_track->datarate = 1; break;
		case RATE_DD: self->cur_track->datarate = 1; break;
		case RATE_HD: self->cur_track->datarate = 2; break;
		case RATE_ED: self->cur_track->datarate = 3; break;
	}
	switch (geom->dg_fm & RECMODE_MASK)
	{
		case RECMODE_FM:  self->cur_track->recmode = 1; break;
		case RECMODE_MFM: self->cur_track->recmode = 2; break;
	}
	self->cur_track->gap3 = geom->dg_fmtgap;
	self->cur_track->filler = filler;
	self->cur_track->dirty = 1;

	/* Write the sectors. Since they are all blank we don't actually
	 * need to send them to the file */
	for (sector = 0; sector < geom->dg_sectors; sector++)
	{
		LDBS_SECTOR_ENTRY *secent = &self->cur_track->sector[sector];

		secent->id_cyl  = format[sector].fmt_cylinder;
		secent->id_head = format[sector].fmt_head;
		secent->id_sec  = format[sector].fmt_sector;
		secent->id_psh  = dsk_get_psh(format[sector].fmt_secsize);
		secent->st1 = 0;
		secent->st2 = 0;
		secent->copies = 0;
		secent->filler = filler;
	}
	/* Write the track header */
	err = ldbsdisk_flush_cur_track(self);
	if (err) return err;

	/* Populate bits of the DPB that the standard geometry probe
	 * does not */
	self->dpb.spt = geom->dg_sectors * (geom->dg_secsize / 128);
	self->dpb.psh = dsk_get_psh(geom->dg_secsize);
	self->dpb.phm = (1 << self->dpb.psh) - 1;

	err = ldbs_put_geometry(self->store, geom);
	return err;
}

/* Get drive geometry */
dsk_err_t ldbsdisk_getgeom(DSK_DRIVER *pdriver, DSK_GEOMETRY *geom)
{
	dsk_err_t err, err2;
	LDBSDISK_DSK_DRIVER *self;

	DC_CHECK(pdriver)
	self = (LDBSDISK_DSK_DRIVER *)pdriver;

	/* Firstly try the default LibDsk geometry probe */
	err = dsk_defgetgeom(pdriver, geom);
	if (!err) return DSK_ERR_OK;

	/* If that fails, see if the LDBS file describes its own geometry */
	err2 = ldbs_get_geometry(self->store, geom);
	if (err2) return err2;

	/* If it populated 'geom', it does. Otherwise no dice. */
	if (geom->dg_cylinders && geom->dg_heads && 
	    geom->dg_sectors && geom->dg_secsize)
	{
		return DSK_ERR_OK;
	}
	return err;
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
	if (!self->cur_track) return DSK_ERR_NOADDR;
	//
	*result = dsk_malloc( self->cur_track->count * sizeof(DSK_FORMAT) );
	if (!(*result)) return DSK_ERR_NOMEM;

	*count = self->cur_track->count;
	for (n = 0; n < *count; n++)
	{
		LDBS_SECTOR_ENTRY *cursec = &self->cur_track->sector[n];

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
	if (self->readonly) *result |= DSK_ST3_RO;
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
		case 0: ldbs_self->dpb.bsh = value;	// BSH
			break;
		case 1: ldbs_self->dpb.blm = value;	// BLM
			break;
		case 2: ldbs_self->dpb.exm = value;	// EXM
			break;
		case 3: ldbs_self->dpb.dsm = value;	// DSM
			break;
		case 4: ldbs_self->dpb.drm = value;	// DRM
			break;
		case 5: ldbs_self->dpb.al[0] = value;	// AL0 
			break;
		case 6: ldbs_self->dpb.al[1] = value;	// AL1 
			break;
		case 7: ldbs_self->dpb.cks = value;	// CKS
			break;
		case 8: ldbs_self->dpb.off = value;	// OFF
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
		case 0: v = ldbs_self->dpb.bsh;		// BSH
			break;
		case 1: v = ldbs_self->dpb.blm;		// BLM
			break;
		case 2: v = ldbs_self->dpb.exm;		// EXM
			break;
		case 3: v = ldbs_self->dpb.dsm;		// DSM
			break;
		case 4: v = ldbs_self->dpb.drm;		// DRM
			break;
		case 5: v = ldbs_self->dpb.al[0];	// AL0 
			break;
		case 6: v = ldbs_self->dpb.al[1];	// AL1 
			break;
		case 7: v = ldbs_self->dpb.cks;		// AL1 
			break;
		case 8: v = ldbs_self->dpb.off;		// OFF
			break;
	}
	if (value) *value = v;
	return DSK_ERR_OK;
}


