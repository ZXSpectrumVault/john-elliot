/***************************************************************************
 *                                                                         *
 *    LIBDSK: General floppy and diskimage access library                  *
 *    Copyright (C) 2001,2005  John Elliott <seasip.webmaster@gmail.com>       *
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

/* Access functions for CPCEMU discs */

/* [1.5.4] The Offset-Info extension makes it impractical to rewrite DSK
 *        files in place. Consequently, this driver has been rewritten in 
 *        the same convert-to-LDBS idiom as Teledisk, CopyQM etc.
 */

#include "drvi.h"
#include "drvldbs.h"
#include "drvcpcem.h"


/* This struct contains function pointers to the driver's functions, and the
 * size of its DSK_DRIVER subclass */

/* The CPCEMU drivers for normal and extended modes are in fact the same,
 * except for the "open" and "create" functions; these have been separated
 * simply so EDSKs can be created. */

DRV_CLASS dc_cpcemu = 
{
	sizeof(CPCEMU_DSK_DRIVER),
	&dc_ldbsdisk,		/* superclass */
	"dsk\0",
	"CPCEMU .DSK driver",
	cpcemu_open,	/* open */
	cpcemu_creat,   /* create new */
	cpcemu_close,   /* close */
};

DRV_CLASS dc_cpcext = 
{
	sizeof(CPCEMU_DSK_DRIVER),
	&dc_ldbsdisk,	/* superclass */
	"edsk\0cpcemu\0",
	"Extended .DSK driver",
	cpcext_open,	/* open */
	cpcext_creat,   /* create new */
	cpcemu_close,   /* close */
};			  



static dsk_err_t cpc_open(DSK_DRIVER *self, const char *filename, int ext);
static dsk_err_t cpc_creat(DSK_DRIVER *self, const char *filename, int ext);


dsk_err_t cpcemu_open(DSK_DRIVER *self, const char *filename)
{
	return cpc_open(self, filename, 0);
}

dsk_err_t cpcext_open(DSK_DRIVER *self, const char *filename)
{
	return cpc_open(self, filename, 1);
}

dsk_err_t cpcemu_creat(DSK_DRIVER *self, const char *filename)
{
	return cpc_creat(self, filename, 0);
}

dsk_err_t cpcext_creat(DSK_DRIVER *self, const char *filename)
{
	return cpc_creat(self, filename, 1);
}


#define DC_CHECK(self) if (self->dr_class != &dc_cpcemu && self->dr_class != &dc_cpcext) return DSK_ERR_BADPTR;




/* Migrate a track from CPCEMU .DSK to LDBS format. 
 *
 * For simplicity's sake, read the whole track into memory in one go, 
 * rather than one sector at a time.
 */
static dsk_err_t track_to_ldbs(CPCEMU_DSK_DRIVER *cpc_self, 
	dsk_pcyl_t cyl, dsk_phead_t head, unsigned char *dskhead, 
	unsigned short **poffptr)
{
	dsk_ltrack_t track = (cyl * dskhead[0x31]) + head;
	unsigned char *dsk_track;	/* DSK track record */
	LDBS_TRACKHEAD *ldbs_track;	/* LDBS track header */
	int sector;
	int source;
	size_t dsk_trklen;	/* Length of DSK track record */
	size_t dsk_seclen;	/* Length of sector record */
	size_t dsk_secsize;	/* Theoretical sector size */
	unsigned char same;
	char block_id[4];
	int n;
	dsk_err_t err;

	/* First: work out the length of the track. In a non-extended DSK
	 * this is a constant in the header. */
	if (dskhead[0] != 'E')
	{
		dsk_trklen = ldbs_peek2(dskhead + 0x32);
	}
	else	/* In an extended DSK it is held per track */
	{
		dsk_trklen = dskhead[0x34 + track] * 256;
	}
	/* Allocate and load the track */
	dsk_track = dsk_malloc(dsk_trklen);
	if (!dsk_track) return DSK_ERR_NOMEM;

	memset(dsk_track, 0, dsk_trklen);

	if (fread(dsk_track, 1, dsk_trklen, cpc_self->cpc_fp) < dsk_trklen)
	{
		dsk_free(dsk_track);
		return DSK_ERR_SYSERR;
	}
	/* Check that the track header has the correct magic */
	if (memcmp(dsk_track, "Track-Info\r\n", 12))
	{
		dsk_free(dsk_track);
#ifndef HAVE_WINDOWS_H
		fprintf(stderr, "Track-Info block %d not found\n", track);
#endif
		return DSK_ERR_NOTME;
	}
	/* Create an LDBS track header */
	ldbs_track = ldbs_trackhead_alloc(dsk_track[0x15]);
	if (!ldbs_track)
	{
		dsk_free(dsk_track);
		return DSK_ERR_NOMEM;
	}
	/* Generate the fixed part of the header */
	ldbs_track->count    = dsk_track[0x15];	/* Count of sectors */
	ldbs_track->datarate = dsk_track[0x12];	/* Data rate */
	ldbs_track->recmode  = dsk_track[0x13];	/* Recording mode */
	ldbs_track->gap3     = dsk_track[0x16];	/* GAP#3 length */
	ldbs_track->filler   = dsk_track[0x17];	/* Format filler */

	if (poffptr[0])
	{
		ldbs_track->total_len = poffptr[0][0];
		++poffptr[0];
	}

	source = 256;
	/* In theory a DSK could exist with more than 29 sectors, and 
	 * in that case Track-info would be longer than 256 bytes. */
	if (dsk_track[0x15] > 29)
	{
		source += 8 * (dsk_track[0x15] - 29);
		/* (Rounded up to a multiple of 256 bytes) */
		source = (source + 255) & (~255);
	}
	err = DSK_ERR_OK;
	/* Now migrate each sector, one by one */
	for (sector = 0; sector < dsk_track[0x15]; sector++)
	{
		LDBS_SECTOR_ENTRY *cursec = &ldbs_track->sector[sector];

		cursec->id_cyl  = dsk_track[0x18 + sector*8]; /* ID: Cyl */
		cursec->id_head = dsk_track[0x19 + sector*8]; /* ID: Head */
		cursec->id_sec  = dsk_track[0x1A + sector*8]; /* ID: Sector */
		cursec->id_psh  = dsk_track[0x1B + sector*8]; /* ID: Sector size */
		cursec->st1     = dsk_track[0x1C + sector*8]; /* ST1 flags */
		cursec->st2     = dsk_track[0x1D + sector*8]; /* ST2 flags */
		cursec->copies  = 1;			      /* Copies */
		cursec->filler  = dsk_track[0x17]; 	      /* Filler byte */

		/* Size of theoretical sector record */
		dsk_secsize = (128 << dsk_track[0x1B + sector * 8]);
	
		/* Size of actual on-disk record */
		if (dskhead[0] != 'E') 
		{
			dsk_seclen = dsk_secsize;
		}
		else	
		{
			dsk_seclen = ldbs_peek2(dsk_track + 0x1E + sector*8);
			if (poffptr[0])
			{
				cursec->offset = poffptr[0][0];
				++poffptr[0];
			}
		}

		/* See if sector is all one byte; if so, don't copy it */
		same = dsk_track[source];
		for (n = 1; n < (int)dsk_seclen; n++) 
		{
			if (dsk_track[n+source] != same) break;
		}
		if (n >= (int)dsk_seclen)	/* Sector is all one byte */
		{
			cursec->copies = 0;	/* Copies */
			cursec->filler = same;	/* Filler */
		}
		else
		{
			cursec->trail  = dsk_seclen % dsk_secsize;
			cursec->copies = dsk_seclen / dsk_secsize;
			if (cursec->copies < 1)
				cursec->copies = 1; /* Copies */
		}
	
		/* If sector is blank (no copies) don't need to write it */
		if (!cursec->copies) 
		{
			source += dsk_seclen;
			continue;
		}
		/* Migrate the sector. Record its address. */
		ldbs_encode_secid(block_id, cyl, head, cursec->id_sec);
		cursec->blockid = LDBLOCKID_NULL;


		err = ldbs_putblock(cpc_self->cpc_super.ld_store, 
				&cursec->blockid, block_id,
				dsk_track + source, dsk_seclen);
		if (err)
		{
			ldbs_free(ldbs_track);
			free(dsk_track);
			return err;
		}
		source += dsk_seclen;
	}
	/* Finally write out the track header */
	err = ldbs_put_trackhead(cpc_self->cpc_super.ld_store, ldbs_track, 
				cyl, head);
	ldbs_free(ldbs_track);
	free(dsk_track);
	return err;
}




/* Open DSK image, checking for the magic number */
static dsk_err_t cpc_open(DSK_DRIVER *self, const char *filename, int extended)
{
	CPCEMU_DSK_DRIVER *cpc_self;
	dsk_err_t err;
	unsigned char dskhead[256];
	unsigned char trkhead[256];
	long filepos = 0x100;
	dsk_pcyl_t c;
	dsk_phead_t h;
	dsk_ltrack_t t;
	unsigned char buf[15];
	unsigned offs_count = 0;
	unsigned short *offsets = NULL, *off_ptr = NULL;
	
	/* Sanity check: Is this meant for our driver? */
	DC_CHECK(self)
	cpc_self = (CPCEMU_DSK_DRIVER *)self;

	cpc_self->cpc_fp = fopen(filename, "r+b");
	if (!cpc_self->cpc_fp) 
	{
		cpc_self->cpc_super.ld_readonly = 1;
		cpc_self->cpc_fp = fopen(filename, "rb");
	}
	if (!cpc_self->cpc_fp) return DSK_ERR_NOTME;
	/* Check for CPCEMU signature */
	if (fread(dskhead, 1, 256, cpc_self->cpc_fp) < 256) 
	{
/* 1.1.6 Don't leak file handles */
		fclose(cpc_self->cpc_fp);
		return DSK_ERR_NOTME;
	}

	if (extended)
	{
		if (memcmp("EXTENDED", dskhead, 8)) 
		{
/* 1.1.6 Don't leak file handles */
			fclose(cpc_self->cpc_fp);
			return DSK_ERR_NOTME; 
		}
	}
	else 
	{
		if (memcmp("MV - CPC", dskhead, 8))
		{
/* 1.1.6 Don't leak file handles */
			fclose(cpc_self->cpc_fp);
			return DSK_ERR_NOTME; 
		}
	}
	dsk_report("Parsing CPCEMU-format disk image");
	/* OK, got signature. Now we have to convert to LDBS format.
	 * First, we need to establish if there is an Offset-Info
	 * block. This comes after the last track, so count the
	 * number of tracks and sectors. */

	if (extended)	/* Only extended DSKs have offset info */
	{
		dsk_report("Checking for Offset-Info extension");
		for (t = 0, c = 0; c < dskhead[0x30]; c++)
			for (h = 0; h < dskhead[0x31]; h++)
		{
/* Load each track header in turn... */
			if (fseek(cpc_self->cpc_fp, filepos, SEEK_SET)) 
			{
				fclose(cpc_self->cpc_fp);
				return DSK_ERR_SYSERR;
			}
			if (fread(trkhead, 1, 256, cpc_self->cpc_fp) < 256)
			{
				fclose(cpc_self->cpc_fp);
				return DSK_ERR_CORRUPT;
			}
/* The offset table has one entry for the track, and one for each sector
 * within the track */
			offs_count += (trkhead[0x15] + 1);
	
			filepos += 256L * dskhead[0x34 + t];
			++t;
		}
		offsets = dsk_malloc(offs_count * sizeof(unsigned short));
		if (!offsets)
		{
			fclose(cpc_self->cpc_fp);
			return DSK_ERR_NOMEM;
		}
		/* filepos is now where the Offset-Info extension should be.*/
		if (fseek(cpc_self->cpc_fp, filepos, SEEK_SET)) 
		{
			fclose(cpc_self->cpc_fp);
			dsk_free(offsets);
			return DSK_ERR_SYSERR;
		}
	/* See if there is an Offset-Info signature there */
		if (fread(buf, 1, 15, cpc_self->cpc_fp) == sizeof(buf) &&
	    	    !memcmp(buf, "Offset-Info\r\n", 13))
		{
			t = 0;
			/* Read the offsets */
			while (offs_count)
			{
				if (fread(buf, 1, 2, cpc_self->cpc_fp) < 2) 
				{
					fclose(cpc_self->cpc_fp);
					dsk_free(offsets);
					return DSK_ERR_CORRUPT;
				}
				offsets[t++] = ldbs_peek2(buf);
				--offs_count;
			}		
		}
		else	/* No Offset-Info block */
		{
			dsk_free(offsets);
			offsets = NULL;
		}
		off_ptr = offsets;
	}

	/* Now migrate each track in turn */
	err = ldbs_new(&cpc_self->cpc_super.ld_store, NULL, LDBS_DSK_TYPE);
	if (err)
	{
		if (offsets) dsk_free(offsets);
		fclose(cpc_self->cpc_fp);
		return err;
	}
	filepos = 0x100;

	dsk_report("Loading DSK file  ");
	for (t = 0, c = 0; c < dskhead[0x30]; c++)
		for (h = 0; h < dskhead[0x31]; h++)
	{
		if (fseek(cpc_self->cpc_fp, filepos, SEEK_SET)) 
		{
			ldbs_close(&cpc_self->cpc_super.ld_store);
			if (offsets) dsk_free(offsets);
			fclose(cpc_self->cpc_fp);
			return DSK_ERR_SYSERR;
		}
		err = track_to_ldbs(cpc_self, c, h, dskhead, &off_ptr);
		if (err)
		{
			ldbs_close(&cpc_self->cpc_super.ld_store);
			if (offsets) dsk_free(offsets);
			fclose(cpc_self->cpc_fp);
			return err;
		}	
		if (extended)
		{
			filepos += 256L * dskhead[0x34 + t];
		}
		else
		{
			filepos += ldbs_peek2(dskhead + 0x32);
		}
		++t;
	}
	if (offsets) dsk_free(offsets);
	dsk_report_end();
	cpc_self->cpc_filename = dsk_malloc_string(filename);
	cpc_self->cpc_extended = extended;
	fclose(cpc_self->cpc_fp);
	return ldbsdisk_attach(self);
}

static void init_header(unsigned char *dskhead, int extended)
{
	memset(dskhead, 0, 256);
	
	/* [1.5.4] Let's put the version number in the disk header creator
	 *        field; there's room for it */	
	if (extended) strcpy((char *)dskhead,
		"EXTENDED CPC DSK File\r\nDisk-Info\r\nLIBDSK " LIBDSK_VERSION);
	else strcpy((char *)dskhead,
		"MV - CPCEMU Disk-File\r\nDisk-Info\r\nLIBDSK " LIBDSK_VERSION);
}



/* Create DSK image */
static dsk_err_t cpc_creat(DSK_DRIVER *self, const char *filename, int extended)
{
	CPCEMU_DSK_DRIVER *cpc_self;
	dsk_err_t err;
	unsigned char dskhead[256];
	
	/* Sanity check: Is this meant for our driver? */
	DC_CHECK(self)
	cpc_self = (CPCEMU_DSK_DRIVER *)self;

	cpc_self->cpc_fp = fopen(filename, "w+b");
	cpc_self->cpc_super.ld_readonly = 0;
	if (!cpc_self->cpc_fp) return DSK_ERR_SYSERR;
	
	init_header(dskhead, extended);
	if (fwrite(dskhead, 1 , 256, cpc_self->cpc_fp) < 256) 
		return DSK_ERR_SYSERR;

	cpc_self->cpc_filename = dsk_malloc_string(filename);
	cpc_self->cpc_extended = extended;
	fclose(cpc_self->cpc_fp);
	err = ldbs_new(&cpc_self->cpc_super.ld_store, NULL, LDBS_DSK_TYPE);
	if (err) return err;
	return ldbsdisk_attach(self);
}



typedef struct offset_count
{
	unsigned long sectors;	/* Total number of sectors on disc */
	int uses_offsets;
} OFFSET_COUNT;


dsk_err_t count_sector_callback(PLDBS self, dsk_pcyl_t c, dsk_phead_t h, 
			LDBS_SECTOR_ENTRY *se, LDBS_TRACKHEAD *th, void *param)
{
	OFFSET_COUNT *oc = param;	

	if (th->total_len || se->offset) oc->uses_offsets = 1;
	++oc->sectors;

	return DSK_ERR_OK;
}


/* Migrate a track from LDBS to CPCEMU .DSK */
static dsk_err_t track_from_ldbs(CPCEMU_DSK_DRIVER *cpc_self, PLDBS infile, 
		dsk_pcyl_t cyl, dsk_phead_t head, unsigned char *dskhead,
		unsigned char **poffset_ptr)
{
	dsk_ltrack_t track = (cyl * dskhead[0x31]) + head;
	dsk_err_t err = 0;
	int n;
/* A Track-Info header is normally 256 bytes. However if a track has more 
 * than 29 sectors, Track-Info has to be increased */
	unsigned char trackinfo[2304];
	LDBS_TRACKHEAD *ldbs_track;	/* LDBS track header */
	size_t tracklen, secsize, tilen, m;
	size_t expected = ldbs_peek2(dskhead + 0x32);

/* Initialise the trackinfo block */
	memset(trackinfo, 0, sizeof(trackinfo));
	strcpy((char *)trackinfo, "Track-Info\r\n");
	trackinfo[0x10] = cyl;
	trackinfo[0x11] = head;
	trackinfo[0x17] = 0xE5;

	err = ldbs_get_trackhead(infile, &ldbs_track, cyl, head);
	if (err && err != DSK_ERR_NOADDR) return err;

	if (err == DSK_ERR_NOADDR)
	{
		/* Track not found. Write an empty track-info block */
		if (dskhead[0] == 'E')
		{
			dskhead[0x34 + track] = 1;
		}
		if (fwrite(trackinfo, 1, 256, cpc_self->cpc_fp) < 256) 
		{
			ldbs_free(ldbs_track);
			return DSK_ERR_SYSERR;
		}
		/* If not an EDSK, all tracks are the same length, so write
		 * padding bytes to the right size */
		if (dskhead[0] != 'E')
		{
			for (m = 256; m < expected; m++)
			{
				if (fputc(trackinfo[0x17], cpc_self->cpc_fp) == EOF) 
				{
					ldbs_free(ldbs_track);
					return DSK_ERR_SYSERR;
				}
			}
		}
		ldbs_free(ldbs_track);
		return DSK_ERR_OK;
	}
	/* EDSK can cope with at most 255 sectors / track */
	if (ldbs_track->count > 255) ldbs_track->count = 255;
/* Migrate the track header */
	trackinfo[0x12] = ldbs_track->datarate;
	trackinfo[0x13] = ldbs_track->recmode;
/*	trackinfo[0x14] is sector size: we'll come back to that */
	trackinfo[0x15] = (unsigned char)ldbs_track->count;
	trackinfo[0x16] = ldbs_track->gap3;
	trackinfo[0x17] = ldbs_track->filler;

	if (poffset_ptr[0])
	{
		ldbs_poke2(poffset_ptr[0], ldbs_track->total_len);
		poffset_ptr[0] += 2;
	}

	for (n = 0; n < ldbs_track->count; n++)
	{
		trackinfo[0x14]         = ldbs_track->sector[n].id_psh;
		trackinfo[0x18 + 8 * n] = ldbs_track->sector[n].id_cyl;
		trackinfo[0x19 + 8 * n] = ldbs_track->sector[n].id_head;
		trackinfo[0x1A + 8 * n] = ldbs_track->sector[n].id_sec;
		trackinfo[0x1B + 8 * n] = ldbs_track->sector[n].id_psh;
		trackinfo[0x1C + 8 * n] = ldbs_track->sector[n].st1;
		trackinfo[0x1D + 8 * n] = ldbs_track->sector[n].st2;

		secsize = 128 << ldbs_track->sector[n].id_psh;
		if (ldbs_track->sector[n].copies > 0)
		{
			/* [1.5.4] Provision for trailing bytes */
			secsize += ldbs_track->sector[n].trail;
			secsize *= ldbs_track->sector[n].copies;
		}
		if (dskhead[0] == 'E')
		{
			ldbs_poke2(trackinfo + 0x1E + 8 * n, (unsigned short)secsize);
			if (poffset_ptr[0])
			{
				ldbs_poke2(poffset_ptr[0], ldbs_track->sector[n].offset);
				poffset_ptr[0] += 2;
			}
		}
	}
	tilen = 256;
	if (ldbs_track->count > 29)
	{
		tilen = (ldbs_track->count * 8) + 0x18;
		/* Round up to a multiple of 256 */
		tilen = (tilen + 255) & ~255;
	}
/* Write the Track-Info block */
	if (fwrite(trackinfo, 1, tilen, cpc_self->cpc_fp) < tilen) 
	{
		ldbs_free(ldbs_track);
		return DSK_ERR_SYSERR;
	}
	tracklen = tilen;
/* Migrate the sectors */
	for (n = 0; n < ldbs_track->count; n++)
	{
		secsize = 128 << ldbs_track->sector[n].id_psh;
		if (!ldbs_track->sector[n].copies)
		{
/* Expand blank sector */
			for (m = 0; m < secsize; m++)
			{
				if (fputc(ldbs_track->sector[n].filler, cpc_self->cpc_fp) == EOF)
				{
					ldbs_free(ldbs_track);
					return DSK_ERR_SYSERR;
				}
			}
			tracklen += secsize;
		}
		else
		{
			size_t len = 0;
			char secid[4];
			void *secbuf;

			err = ldbs_getblock_a(infile, 
					ldbs_track->sector[n].blockid, 
					secid, &secbuf, &len);	
			if (err) return err;
			if (fwrite(secbuf, 1, len, cpc_self->cpc_fp) < len)
			{
				ldbs_free(secbuf);
				return DSK_ERR_SYSERR;
			}
			ldbs_free(secbuf);
			tracklen += len;
		}
	}
	/* Pad track to a multiple of 256 bytes (eg, if it has an odd
	 * number of 128-byte sectors) */
	while (tracklen & 0xFF)
	{
		if (fputc(0, cpc_self->cpc_fp) == EOF)
		{
			ldbs_free(ldbs_track);
			return DSK_ERR_SYSERR;
		}
		++tracklen;
	}
	ldbs_free(ldbs_track);
	if (dskhead[0] == 'E')
	{
		dskhead[0x34 + track] = tracklen / 256;
	}
	else
	{
		if (tracklen != expected)
		{
#ifndef HAVE_WINDOWS_H
			fprintf(stderr, "ERROR: Cylinder %d head %d "
					"unexpected track length\n",
					cyl, head);
#endif
		}
	}
	return DSK_ERR_OK;
}





dsk_err_t cpcemu_close(DSK_DRIVER *self)
{
	CPCEMU_DSK_DRIVER *cpc_self;
	LDBS_STATS stats;
	OFFSET_COUNT oc;
	dsk_err_t err;
	dsk_pcyl_t c;
	dsk_phead_t h;
	unsigned char dskhead[256];
	unsigned char *offset_rec = NULL;
	unsigned char *offset_ptr;
	size_t offset_len;

	DC_CHECK(self)
	cpc_self = (CPCEMU_DSK_DRIVER *)self;

	/* Firstly, ensure any pending changes are flushed to the LDBS 
	 * blockstore. Once this has been done we own the blockstore again 
	 * and have to close it after we've finished with it. */
	err = ldbsdisk_detach(self); 
	if (err)
	{
		dsk_free(cpc_self->cpc_filename);
		ldbs_close(&cpc_self->cpc_super.ld_store);
		return err;
	}

	/* If this disc image has not been written to, just close it and 
	 * dispose thereof. */
	if (!self->dr_dirty)
	{
		dsk_free(cpc_self->cpc_filename);
		return ldbs_close(&cpc_self->cpc_super.ld_store);
	}
	/* Trying to save changes but source is read-only */
	if (cpc_self->cpc_super.ld_readonly)
	{
		dsk_free(cpc_self->cpc_filename);
		ldbs_close(&cpc_self->cpc_super.ld_store);
		return DSK_ERR_RDONLY;
	}
	dsk_report(cpc_self->cpc_extended ? "Writing CPCEMU EDSK file" : 
				"Writing CPCEMU DSK file");
	init_header(dskhead, cpc_self->cpc_extended);	

	err = ldbs_get_stats(cpc_self->cpc_super.ld_store, &stats);
	if (err) 
	{
		dsk_free(cpc_self->cpc_filename);
		ldbs_close(&cpc_self->cpc_super.ld_store);
		dsk_report_end();
		return err;
	}
	/* Check for presence of offset info; that forces EDSK */
	memset(&oc, 0, sizeof(oc));
	err = ldbs_all_sectors(cpc_self->cpc_super.ld_store, 
			count_sector_callback, SIDES_ALT, &oc);
	if (err) 
	{
		dsk_free(cpc_self->cpc_filename);
		ldbs_close(&cpc_self->cpc_super.ld_store);
		dsk_report_end();
		return err;
	}

	if (dskhead[0] != 'E' &&
		(stats.max_spt != stats.min_spt ||
		stats.max_sector_size != stats.min_sector_size ||
		stats.max_spt > 29 ||
		oc.uses_offsets))
	{
#ifndef HAVE_WINDOWS_H
		fprintf(stderr, "Input file cannot be converted to DSK -- EDSK format is required.\n");
#endif
		dsk_free(cpc_self->cpc_filename);
		ldbs_close(&cpc_self->cpc_super.ld_store);
		dsk_report_end();
		return DSK_ERR_RDONLY;
	}
	cpc_self->cpc_fp = fopen(cpc_self->cpc_filename, "wb");
	if (!cpc_self->cpc_fp)
	{
		dsk_free(cpc_self->cpc_filename);
		ldbs_close(&cpc_self->cpc_super.ld_store);
		dsk_report_end();
		return DSK_ERR_SYSERR;
	}
	memset(dskhead + 0x30, 0, 0xD0);
		
	if (fseek(cpc_self->cpc_fp, 0, SEEK_SET))
	{
		fclose(cpc_self->cpc_fp);
		dsk_free(cpc_self->cpc_filename);
		ldbs_close(&cpc_self->cpc_super.ld_store);
		dsk_report_end();
		return DSK_ERR_SYSERR;
	}
	/* Get the track directory */
	err = ldbs_max_cyl_head(cpc_self->cpc_super.ld_store, &c, &h);
      	if (err)
	{
		fclose(cpc_self->cpc_fp);
		dsk_free(cpc_self->cpc_filename);
		ldbs_close(&cpc_self->cpc_super.ld_store);
		dsk_report_end();
		return err;
	}
	/* Compute the maximum cylinder and head */
	dskhead[0x30] = c;
	dskhead[0x31] = h;

	if (dskhead[0] != 'E')
	{
		/* Fixed-length tracks: 256 byte header + secsize * spt
		 * sectors */
		ldbs_poke2(dskhead + 0x32,
				(unsigned short)(256 + stats.max_sector_size * stats.max_spt));
	}
	offset_rec = offset_ptr = NULL;
	if (dskhead[0] == 'E' && oc.uses_offsets)
	{
		/* Size of Offset-Info block */
		offset_len = 15 + ((c * h + oc.sectors) * 2);
		offset_rec = ldbs_malloc( offset_len );
		if (offset_rec)
		{
			memcpy(offset_rec, "Offset-Info\r\n", 14);
			offset_rec[14] = 0;
			offset_ptr = offset_rec + 15;
		}
		else	
		{
			fclose(cpc_self->cpc_fp);
			dsk_free(cpc_self->cpc_filename);
			ldbs_close(&cpc_self->cpc_super.ld_store);
			dsk_report_end();
			return DSK_ERR_NOMEM;
		}
	}

	/* Write an (incomplete) DSK / EDSK header */
	if (fwrite(dskhead, 1 , 256, cpc_self->cpc_fp) < 256) 
	{
		fclose(cpc_self->cpc_fp);
		dsk_free(cpc_self->cpc_filename);
		ldbs_close(&cpc_self->cpc_super.ld_store);
		dsk_report_end();
		return DSK_ERR_SYSERR;	
	}
	for (c = 0; c < dskhead[0x30]; c++)
	{
		for (h = 0; h < dskhead[0x31]; h++)
		{
			err = track_from_ldbs(cpc_self, 
				cpc_self->cpc_super.ld_store, c, h, 
				dskhead, &offset_ptr);
			if (err) break;
		}
		if (err) break;
	}
	if (offset_rec && !err)
	{
		if (fwrite(offset_rec, 1, offset_len, cpc_self->cpc_fp) < offset_len)
		{
			err = DSK_ERR_SYSERR;
		}
	}
/* Rewrite the updated header */
	if (!err)
	{
		if (fseek(cpc_self->cpc_fp, 0, SEEK_SET) ||
		    fwrite(dskhead, 1, 256, cpc_self->cpc_fp) < 256) 
		{
			err = DSK_ERR_SYSERR;
		}

	}
	dsk_free(cpc_self->cpc_filename);
	ldbs_close(&cpc_self->cpc_super.ld_store);
	dsk_report_end();
	if (err)
	{
		fclose(cpc_self->cpc_fp);
		return err;
	}
	else
	{
		if (fclose(cpc_self->cpc_fp)) return DSK_ERR_SYSERR;
	}
	return DSK_ERR_OK;
}

	

