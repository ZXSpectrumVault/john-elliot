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

/* Support for the Compaq Quick Release Sector Transfer format. This is 
 * implemented as a subclass of LDBS, with the only code in this file 
 * being the QRST-to-LDBS and LDBS-to-QRST conversions */

#include <stdio.h>
#include "libdsk.h"
#include "drvi.h"
#include "drvldbs.h"
#include "drvqrst.h"

static LDBS_STATS known_formats[] = 
{
	{ 0, 39, 0, 1, 0, 9, 9, 512, 512, 9, 1 }, /* 360k */
	{ 0, 79, 0, 1, 0,15,15, 512, 512,15, 1 }, /* 1.2M */
	{ 0, 79, 0, 1, 0, 9, 9, 512, 512, 9, 1 }, /* 720k */
	{ 0, 79, 0, 1, 0,18,18, 512, 512,18, 1 }, /* 1440k */
	{ 0, 39, 0, 0, 0, 8, 8, 512, 512, 8, 1 }, /* 160k */
	{ 0, 39, 0, 0, 0, 9, 9, 512, 512, 9, 1 }, /* 180k */
	{ 0, 39, 0, 1, 0, 8, 8, 512, 512, 8, 1 }, /* 320k */
};

/* Signature for an LDBS block containing QRST data */
#define QRST_USER_BLOCK "uqrs"


/* This struct contains function pointers to the driver's functions, and the
 * size of its DSK_DRIVER subclass */

DRV_CLASS dc_qrst = 
{
	sizeof(QRST_DSK_DRIVER),
	&dc_ldbsdisk,	/* superclass */
	"qrst\0QRST\0",
	"Quick Release Sector Transfer",
	qrst_open,	/* open */
	qrst_creat,	/* create new */
	qrst_close,	/* close */
};



/* Expand a compressed QRST track: */

static dsk_err_t expand(FILE *fp, unsigned char *trackbuf, size_t tracklen,
			size_t compressed_len)
{
	int c, d;
	size_t offset = 0;

	while (compressed_len)
	{
		/* Compressed blocks consist of alternating sequences of 
		 * literal runs <length><bytes>  and compressed runs 
		 * <length><repeated_byte> */
		c = fgetc(fp); 	/* Length of literal data */
		/* Unexpected EOF */
		if (c == EOF) 
		{    
#ifndef WIN16
			fprintf(stderr, "QRST1: Unexpected EOF\n");
#endif
			return DSK_ERR_NOTME;
		}
		--compressed_len; 
		if (!compressed_len) 
		{
#ifndef WIN16
			fprintf(stderr, "QRST2: Compression ends mid-block\n");
#endif
			return DSK_ERR_NOTME;
		}
		/* Limit to available compressed bytes */
		if ((unsigned)c > compressed_len) 
		{
#ifndef WIN16
			fprintf(stderr, "QRST3: Overlong literal run\n"); 
#endif
			c = compressed_len;
		}
		if (c + offset > tracklen) 
		{
#ifndef WIN16
			fprintf(stderr, "QRST4: Overlong literal run\n");
#endif
			c = tracklen - offset;
		}
		if (fread(trackbuf + offset, 1, c, fp) < (size_t)c)
		{
#ifndef WIN16
			fprintf(stderr, "QRST5: Unexpected EOF\n");
#endif
			return DSK_ERR_NOTME;
		}
		compressed_len -= c;
		offset += c;
		if (!compressed_len) return DSK_ERR_OK;
		c = fgetc(fp); 	/* Length of literal data */
		if (c == EOF) 
		{
#ifndef WIN16
			fprintf(stderr, "QRST6: Unexpected EOF\n");
#endif
			return DSK_ERR_NOTME;
		}
		--compressed_len; 
		if (!compressed_len) return DSK_ERR_NOTME;
		d = fgetc(fp); 	/* Byte to repeat */
		--compressed_len; 
		/* Unexpected EOF */
		if (d == EOF) 
		{
#ifndef WIN16
			fprintf(stderr, "QRST7: Unexpected EOF\n");
#endif
			return DSK_ERR_NOTME;
		}
		if (c + offset > tracklen) 
		{
#ifndef WIN16
			fprintf(stderr, "QRST8: Overlong repeat run\n");
#endif
			c = tracklen - offset;
		}
		memset(trackbuf + offset, d, c);	
		offset += c;
	}
	return DSK_ERR_OK;
}


dsk_err_t qrst_open(DSK_DRIVER *self, const char *filename)
{
	DSK_GEOMETRY geom;
	QRST_DSK_DRIVER *qrself;
	FILE *fp;
	unsigned char header[796];
	char *comment = NULL;
	dsk_err_t err;
	dsk_pcyl_t cylinder;
	dsk_phead_t head;
	dsk_psect_t sector;
	unsigned char chbuf[3];
	char sectype[4];
	unsigned char *trackbuf, *secbuf;
	size_t tracklen;
	int c, l1, l2;
	LDBS_TRACKHEAD *trackhead = NULL;
	
	/* Sanity check: Is this meant for our driver? */
	if (self->dr_class != &dc_qrst) return DSK_ERR_BADPTR;
	qrself = (QRST_DSK_DRIVER *)self;

	/* Save the filename, we'll want it when doing output */
	qrself->qrst_filename = dsk_malloc_string(filename);
	if (!qrself->qrst_filename) return DSK_ERR_NOMEM;

	fp = fopen(filename, "r+b");
	if (!fp)
	{
		qrself->qrst_super.ld_readonly = 1;
		fp = fopen(filename, "rb");
	}
	if (!fp) 
	{
		dsk_free(qrself->qrst_filename);
		return DSK_ERR_NOTME;
	}
/* QRST supports a number of fixed formats; I'm unsure whether the boot
 * sector on track 0 takes priority over these formats. For now let the
 * boot sector be the final arbiter. */

	if (fread(header, 1, sizeof(header), fp) < (int)sizeof(header) ||
	    memcmp(header, "QRST", 4) ||
	    header[12] < 1 || header[12] > 7)
	{
		dsk_free(qrself->qrst_filename);
		fclose(fp);
		return DSK_ERR_NOTME;	
	}
	switch (header[12])
	{
		case 1: dg_stdformat(&geom, FMT_360K, NULL, NULL); break;
		case 2: dg_stdformat(&geom, FMT_1200K, NULL, NULL); break;
		case 3: dg_stdformat(&geom, FMT_720K, NULL, NULL); break;
		case 4: dg_stdformat(&geom, FMT_1440K, NULL, NULL); break;
		case 5: dg_stdformat(&geom, FMT_160K, NULL, NULL); break;
		case 6: dg_stdformat(&geom, FMT_180K, NULL, NULL); break;
		case 7: dg_stdformat(&geom, FMT_320K, NULL, NULL); break;
	}
	tracklen = geom.dg_sectors * geom.dg_secsize;

	/* OK, we're ready to load the file. */
	err = ldbs_new(&qrself->qrst_super.ld_store, NULL, LDBS_DSK_TYPE);
	if (err)
	{
		dsk_free(qrself->qrst_filename);
		fclose(fp);
		return err;
	}
	/* Add in the comment, transcoding from codepage 437 to UTF8 */
	l1 = cp437_to_utf8((char *)(header + 0x0F), NULL, -1);
	l2 = cp437_to_utf8((char *)(header + 0x4B), NULL, -1);

	comment = dsk_malloc(5 + l1 + l2);
	if (!comment)
	{
		dsk_free(qrself->qrst_filename);
		ldbs_close(&qrself->qrst_super.ld_store);
		fclose(fp);
		return DSK_ERR_NOMEM;
	}
	cp437_to_utf8((char *)(header + 0x0F), comment, -1);
	strcat(comment, "\r\n");
	cp437_to_utf8((char *)(header + 0x4B), comment + strlen(comment), -1);

	err = ldbs_put_comment(qrself->qrst_super.ld_store, comment);
	dsk_free(comment);
	/* Save the volume counts */
	if (!err)
	{
		err = ldbs_putblock_d(qrself->qrst_super.ld_store, 
				QRST_USER_BLOCK, header + 0x0D, 2);
	}
	if (err)
	{
		dsk_free(qrself->qrst_filename);
		ldbs_close(&qrself->qrst_super.ld_store);
		fclose(fp);
		return err;
	}
	/* Now do the tracks */
	trackbuf = dsk_malloc(tracklen);
	if (!trackbuf)
	{
		dsk_free(qrself->qrst_filename);
		ldbs_close(&qrself->qrst_super.ld_store);
		fclose(fp);
		return DSK_ERR_NOMEM;
	}
	/* Read in each track */
	while (fread(chbuf, 1, sizeof(chbuf), fp) == sizeof(chbuf))
	{
		long pos;

		cylinder = chbuf[0];
		head     = chbuf[1];
		pos = ftell(fp);

		err = DSK_ERR_OK;
		switch (chbuf[2])
		{
			case 0: /* Uncompressed */
				if (fread(trackbuf, 1, tracklen, fp) < tracklen)
				{   
#ifndef WIN16
					fprintf(stderr, "QRST: Short type 0 block @ 0x%lx\n", pos);
#endif
					err = DSK_ERR_NOTME;
				}
				break;
			case 1: /* Blank */
				c = fgetc(fp);
				if (c == EOF) 
				{
					err = DSK_ERR_NOTME;
#ifndef WIN16
					fprintf(stderr, "QRST: Short type 1 block @ 0x%lx\n", pos);
#endif
				}
				memset(trackbuf, c, tracklen);
				break;
			case 2: /* Compressed */
				if (fread(chbuf, 1, 2, fp) < 2)
				{
					err = DSK_ERR_NOTME;
#ifndef WIN16
					fprintf(stderr, "QRST: Short type 2 block @ 0x%lx\n", pos);
#endif
				}
				else	err = expand(fp, trackbuf, tracklen, chbuf[0] + 256 * chbuf[1]);
				break;
			default: /* Unsupported compression type */
				err = DSK_ERR_NOTME;
#ifndef WIN16
				fprintf(stderr, "QRST: Unknown compression type %02x @ 0x%lx\n", chbuf[2], pos);
#endif
				break;
		}
/* Track is loaded. Now to populate the equivalent track in the blockstore */
		if (!err)
		{
			trackhead = ldbs_trackhead_alloc(geom.dg_sectors);
			if (!trackhead) err = DSK_ERR_NOMEM;
		}
		if (!err)
		{
			trackhead->datarate = (geom.dg_sectors >= 15) ?  2 : 1;
			trackhead->recmode = 2;
			trackhead->gap3 = geom.dg_fmtgap;
			trackhead->filler = 0xF6;

			for (sector = 0; sector < geom.dg_sectors; sector++)
			{
				LDBS_SECTOR_ENTRY *secent = &trackhead->sector[sector];
				secbuf = trackbuf + sector * geom.dg_secsize;

				ldbs_encode_secid(sectype, cylinder, head,
					sector + geom.dg_secbase);

				secent->id_cyl = cylinder;
				secent->id_head = head;
				secent->id_sec = sector + geom.dg_secbase;
				secent->id_psh = dsk_get_psh(geom.dg_secsize);
				secent->copies = 0;
				/* See if the sector is blank */
				for (c = 1; c < (int)geom.dg_secsize; c++)
				{
					if (secbuf[c] != secbuf[0])
					{
						secent->copies = 1;
						break;
					}
				}
				if (secent->copies == 0) 
				{
					secent->filler = secbuf[0];
				}
				else	
				{
					secent->filler = trackhead->filler;
					err = ldbs_putblock
						(qrself->qrst_super.ld_store,
						&secent->blockid,
						sectype,
						secbuf,
						geom.dg_secsize);
				}
			}
			if (err) break;
		}
		if (!err)
		{
			err = ldbs_put_trackhead(qrself->qrst_super.ld_store,
					trackhead, cylinder, head);
		}
		if (trackhead)
		{
			dsk_free(trackhead);
			trackhead = NULL;
		}
		if (err) break;
	}
	if (trackbuf)
	{
		dsk_free(trackbuf);
		trackbuf = NULL;
	}

	if (!err) err = ldbs_put_geometry(qrself->qrst_super.ld_store, &geom);
	if (err)
	{
		dsk_free(qrself->qrst_filename);
		ldbs_close(&qrself->qrst_super.ld_store);
		fclose(fp);
		return err;
	}

	/* Finally, hand the blockstore to the superclass so it can provide
	 * all the read/write/format methods */
	return ldbsdisk_attach(self);
}

dsk_err_t qrst_creat(DSK_DRIVER *self, const char *filename)
{
	QRST_DSK_DRIVER *qrself;
	dsk_err_t err;
	FILE *fp;

	/* Sanity check: Is this meant for our driver? */
	if (self->dr_class != &dc_qrst) return DSK_ERR_BADPTR;
	qrself = (QRST_DSK_DRIVER *)self;

	/* Save the filename, we'll want it when doing output */
	qrself->qrst_filename = dsk_malloc_string(filename);
	if (!qrself->qrst_filename) return DSK_ERR_NOMEM;

	/* Create a 0-byte file, just to be sure we can */
	fp = fopen(filename, "wb");
	if (!fp) return DSK_ERR_SYSERR;
	fclose(fp);

	/* OK, create a new empty blockstore */
	err = ldbs_new(&qrself->qrst_super.ld_store, NULL, LDBS_DSK_TYPE);
	if (err)
	{
		dsk_free(qrself->qrst_filename);
		return err;
	}
	/* Finally, hand the blockstore to the superclass so it can provide
	 * all the read/write/format methods */
	return ldbsdisk_attach(self);
}


/* Compress a QRST track. Returns compressed size */
static size_t compress(QRST_DSK_DRIVER *qrself, 
		unsigned char *src, size_t buflen, unsigned char *dest)
{
	size_t dptr = 0;
	size_t pos  = 0;	
	size_t n;
	
	/* QRST compression: Alternate literal runs and compressed runs. We
	 * will compress any run of three or more identical bytes. */
	while (pos < buflen)
	{
		/* First do a literal run */
		for (n = pos; (n - pos) < 255 && n < buflen; n++)
		{
			/* Three successive bytes the same -- switch to
			 * compressed mode */
			if (n < (buflen - 2) && 
				src[n] == src[n+1] &&
				src[n] == src[n+2])   break;
		}
		/* Write the literal run */
		if (dest)
		{
			dest[dptr] = n - pos;
			memcpy(dest + dptr + 1, src + pos, n - pos);
		}
		dptr += (n - pos + 1);
		pos = n;
		/* Now a compressed run */
		for (n = pos; (n - pos) < 255 && n < buflen; n++)
		{
			/* If next byte differs from this, break */
			if (src[n] != src[pos]) break;
		}
		if (dest)
		{
			dest[dptr]   = (n - pos);
			dest[dptr+1] = src[pos]; 
		}
		dptr += 2;
		pos = n;
	}
	return dptr;
}



/* Called once for each track. Checksum the track */
static dsk_err_t checksum_track(PLDBS self, dsk_pcyl_t cyl, dsk_phead_t head,
			LDBS_TRACKHEAD *th, void *param)
{
	QRST_DSK_DRIVER *qrself = (QRST_DSK_DRIVER *)param;
	size_t n;
	dsk_err_t err;
	unsigned char *buf;
	size_t buflen;

	if (th->count == 0)	/* No sectors; track is blank */
	{
		for (n = 0; n < qrself->qrst_tracklen; n++)
		{
			++qrself->qrst_bias;
			qrself->qrst_checksum += qrself->qrst_bias * th->filler;
		}
		return DSK_ERR_OK;	
	}
	else
	{
		err = ldbs_load_track(self, th, (void **)&buf, &buflen, 0);
		if (err) return err;

	}
	/* Now calculate checksum */
	for (n = 0; n < buflen; n++)
	{
		++qrself->qrst_bias;
		qrself->qrst_checksum += qrself->qrst_bias * buf[n];
	}
	ldbs_free(buf);
	return DSK_ERR_OK;
}



/* Called once for each track. Write track data out to disk. */
static dsk_err_t save_callback(PLDBS self, dsk_pcyl_t cyl, dsk_phead_t head,
			LDBS_TRACKHEAD *th, void *param)
{
	QRST_DSK_DRIVER *qrself = (QRST_DSK_DRIVER *)param;
	dsk_psect_t sector;
	int allblank = 1;
	int minsecid = -1;
	unsigned char trkh[5];	/* QRST track header */
	dsk_err_t err;
	size_t clen;
	unsigned char *buffer;
	size_t buflen;

	trkh[0] = cyl;		/* Set up as for a blank track */
	trkh[1] = head;
	trkh[2] = 1;
	trkh[3] = th->filler;

	if (th->count == 0)	/* No sectors. Write a blank track. */
	{
		if (fwrite(trkh, 1, 4, qrself->qrst_fp) < 4) 
			return DSK_ERR_SYSERR;
		else	return DSK_ERR_OK;
	}
	trkh[3] = th->sector[0].filler;
	
	for (sector = 0; sector < th->count; sector++)
	{
		if (th->sector[sector].copies > 0 ||
		    th->sector[sector].filler != th->sector[0].filler)
		{
			allblank = 0;
		}
		if (minsecid == -1 || minsecid > th->sector[sector].id_sec)
		{
			minsecid = th->sector[sector].id_sec;
		}
	}	
	/* All sectors are blank and have the same filler. Write them. */
	if (allblank)
	{
		if (fwrite(trkh, 1, 4, qrself->qrst_fp) < 4) 
			return DSK_ERR_SYSERR;
		else	return DSK_ERR_OK;
	}
	/* OK, there's actually data here. Load the sectors */
	err = ldbs_load_track(self, th, (void **)&buffer, &buflen, 0);
	if (err) return err;
	
	/* Attempt compression */
	clen = compress(qrself, buffer, buflen, NULL);
	if (clen < buflen && clen < 0x10000)
	{
		unsigned char *cbuf = dsk_malloc(clen);

		if (!cbuf) 
		{
			ldbs_free(buffer);
			return DSK_ERR_NOMEM;
		}
		compress(qrself, buffer, buflen, cbuf);
		trkh[2] = 2;
		trkh[3] = (clen & 0xFF);
		trkh[4] = (clen >> 8);
		if (fwrite(trkh, 1, 5, qrself->qrst_fp) < 5) 
		{
			dsk_free(cbuf);
			ldbs_free(buffer);
			return DSK_ERR_SYSERR;
		}
		if (fwrite(cbuf, 1, clen, qrself->qrst_fp) < clen)
		{
			dsk_free(cbuf);
			ldbs_free(buffer);
			return DSK_ERR_SYSERR;
		}
		dsk_free(cbuf);
	}
	else
	{
		/* Write uncompressed */	
		trkh[2] = 0;
		if (fwrite(trkh, 1, 3, qrself->qrst_fp) < 3) 
		{
			ldbs_free(buffer);
			return DSK_ERR_SYSERR;
		}
		if (fwrite(buffer, 1, buflen, qrself->qrst_fp) < buflen)
		{
			ldbs_free(buffer);
			return DSK_ERR_SYSERR;
		}
	}
	return DSK_ERR_OK;
}

dsk_err_t qrst_close(DSK_DRIVER *self)
{
	QRST_DSK_DRIVER *qrself;
	LDBS_STATS stats;
	dsk_err_t err;
	unsigned char header[796];
	char *comment, *desc;
	int n;
	size_t len;
	unsigned char volinfo[2];

	/* Sanity check: Is this meant for our driver? */
	if (self->dr_class != &dc_qrst) return DSK_ERR_BADPTR;
	qrself = (QRST_DSK_DRIVER *)self;

	/* Firstly, ensure any pending changes are flushed to the LDBS 
	 * blockstore. Once this has been done we own the blockstore again 
	 * and have to close it after we've finished with it. */
	err = ldbsdisk_detach(self); 
	if (err)
	{
		dsk_free(qrself->qrst_filename);
		ldbs_close(&qrself->qrst_super.ld_store);
		return err;
	}

	/* If this disc image has not been written to, just close it and 
	 * dispose thereof. */
	if (!self->dr_dirty)
	{
		dsk_free(qrself->qrst_filename);
		return ldbs_close(&qrself->qrst_super.ld_store);
	}
	/* Trying to save changes but source is read-only */
	if (qrself->qrst_super.ld_readonly)
	{
		dsk_free(qrself->qrst_filename);
		ldbs_close(&qrself->qrst_super.ld_store);
		return DSK_ERR_RDONLY;
	}
	dsk_report("Writing QRST file");

 	/* Convert the blockstore to a Compaq QRST file. 
	 * Start by generating the header */
	memset(header, 0, sizeof(header));
	memcpy(header, "QRST", 5);
	/* I don't know what these are (they may be part of the magic number)
	 * but they're present in all the QRST images I've looked at */
	header[6] = 0x80;
	header[7] = 0x3F;
	
	header[13] = 1;	/* Volume 1 */
	header[14] = 1;	/* ... of 1 */

	/* Retrieve the uqrs block (if present) and extract the saved volume
	 * details */
	len = 2;
	err = ldbs_getblock_d(qrself->qrst_super.ld_store, QRST_USER_BLOCK,
		volinfo, &len);
	if ((err == DSK_ERR_OK || err == DSK_ERR_OVERRUN) && len >= 2)
	{
		header[13] = volinfo[0];
		header[14] = volinfo[1];
	}

	if (dsk_get_comment(self, &comment) == DSK_ERR_OK && NULL != comment)
	{
		desc = strchr(comment, '\r'); 
		if (desc)
		{
			*desc = 0;
			utf8_to_cp437(comment, (char *)(header + 0x0F), 96);
			*desc = '\r';
			while (*desc == '\n' || *desc == '\r') ++desc;
			utf8_to_cp437(desc, (char *)(header + 0x4B), 721);
		}
		else
		{
			utf8_to_cp437(comment, (char *)(header + 0x0F), 96);
		}
	}
	/* That's all the header except byte 0x0c (capacity) and the dword at
	 * offset 0x08 (checksum). The checksum has to come last; let's do
	 * capacity */

	err = ldbs_get_stats(qrself->qrst_super.ld_store, &stats);
	if (err) 
	{
		ldbs_close(&qrself->qrst_super.ld_store);
		dsk_report_end();
		return err;
	}
	/* QRST works on the assumption that the format is reasonably 
	 * regular: All tracks have the same number of sectors, all 
	 * sectors are the same size, and all sectors are sequentially
	 * numbered. If this is not the case, bail. */
	if (stats.max_sector_size != stats.min_sector_size ||
	    stats.max_spt != stats.min_spt ||
	    (stats.max_secid + 1 - stats.min_secid != (int)stats.max_spt))
	{                     
#ifndef WIN16
		fprintf(stderr, "%s cannot be accurately represented "
				"in QRST format. Abandoning.", 
				qrself->qrst_filename);
#endif
		dsk_free(qrself->qrst_filename);
		ldbs_close(&qrself->qrst_super.ld_store);
		dsk_report_end();
		return DSK_ERR_RDONLY;
	}

	stats.drive_empty = 0;	/* We don't care about this one */
	for (n = 0; n < (int)(sizeof(known_formats) / sizeof(known_formats[0])); n++)
	{
		if (!memcmp(&stats, &known_formats[n], sizeof(stats)))
		{
			header[0x0C] = 1 + n;
			break;
		}
	}
	/* Allocate a track buffer. We have established that all
	 * tracks have the same sector size and sector count, so
	 * this buffer size will do for all. 
	 *
	 * Allocate double the size we need; the second half will be used
	 * when compressing tracks. 
	 */
	qrself->qrst_tracklen = stats.max_sector_size * stats.max_spt;

	/* Initialise the checksummer */
	qrself->qrst_bias = 0;
	qrself->qrst_checksum = 0;

	/* Calculate the checksum */
	ldbs_all_tracks(qrself->qrst_super.ld_store, checksum_track, 
		SIDES_ALT, qrself);

	/* Store the checksum */
	ldbs_poke4(header+8, qrself->qrst_checksum);

	/* Write out the header */
	qrself->qrst_fp = fopen(qrself->qrst_filename, "wb");
	if (!qrself->qrst_fp || 
	    fwrite(header, 1, sizeof(header), qrself->qrst_fp) < (int)sizeof(header))
	{
		if (qrself->qrst_fp) 
		{
			fclose(qrself->qrst_fp);
			remove(qrself->qrst_filename);
		}
		dsk_free(qrself->qrst_filename);
		ldbs_close(&qrself->qrst_super.ld_store);
		dsk_report_end();
		return DSK_ERR_SYSERR;
	}

	/* Write out the tracks */
	ldbs_all_tracks(qrself->qrst_super.ld_store, save_callback, 
		SIDES_ALT, qrself);

	dsk_report_end();
	dsk_free(qrself->qrst_filename);
	if (fclose(qrself->qrst_fp)) return DSK_ERR_SYSERR;
	return ldbs_close(&qrself->qrst_super.ld_store);
}


