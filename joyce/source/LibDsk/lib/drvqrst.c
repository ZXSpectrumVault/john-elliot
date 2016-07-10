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

/* QRST is a subclass of LDBS, with the only methods in this file being 
 * QRST-to-LDBS and LDBS-to-QRST */

#include <stdio.h>
#include "libdsk.h"
#include "drvi.h"
#include "drvldbs.h"
#include "drvqrst.h"


/* This struct contains function pointers to the driver's functions, and the
 * size of its DSK_DRIVER subclass */

DRV_CLASS dc_qrst = 
{
	sizeof(QRST_DSK_DRIVER),
	&dc_ldbsdisk,	/* superclass */
	"QRST",
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
			fprintf(stderr, "QRST1: Unexpected EOF\n");
			return DSK_ERR_NOTME;
		}
		--compressed_len; 
		if (!compressed_len) 
		{
			fprintf(stderr, "QRST2: Compression ends mid-block\n");
			return DSK_ERR_NOTME;
		}
		/* Limit to available compressed bytes */
		if (c > compressed_len) 
		{
			fprintf(stderr, "QRST3: Overlong literal run\n");
			c = compressed_len;
		}
		if (c + offset > tracklen) 
		{
			fprintf(stderr, "QRST4: Overlong literal run\n");
			c = tracklen - offset;
		}
		if (fread(trackbuf + offset, 1, c, fp) < c)
		{
			fprintf(stderr, "QRST5: Unexpected EOF\n");
			return DSK_ERR_NOTME;
		}
		compressed_len -= c;
		offset += c;
		if (!compressed_len) return DSK_ERR_OK;
		c = fgetc(fp); 	/* Length of literal data */
		if (c == EOF) 
		{
			fprintf(stderr, "QRST6: Unexpected EOF\n");
			return DSK_ERR_NOTME;
		}
		--compressed_len; 
		if (!compressed_len) return DSK_ERR_NOTME;
		d = fgetc(fp); 	/* Byte to repeat */
		--compressed_len; 
		/* Unexpected EOF */
		if (d == EOF) 
		{
			fprintf(stderr, "QRST7: Unexpected EOF\n");
			return DSK_ERR_NOTME;
		}
		if (c + offset > tracklen) 
		{
			fprintf(stderr, "QRST8: Overlong repeat run\n");
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
	char comment[796];
	dsk_err_t err;
	dsk_pcyl_t cylinder;
	dsk_phead_t head;
	dsk_psect_t sector;
	unsigned char chbuf[3];
	char sectype[4];
	unsigned char *trackbuf, *secbuf;
	size_t tracklen;
	int c;
	LDBS_TRACKHEAD *trackhead;
	
	/* Sanity check: Is this meant for our driver? */
	if (self->dr_class != &dc_qrst) return DSK_ERR_BADPTR;
	qrself = (QRST_DSK_DRIVER *)self;

	/* Save the filename, we'll want it when doing output */
	qrself->qrst_filename = dsk_malloc(1 + strlen(filename));
	if (!qrself->qrst_filename) return DSK_ERR_NOMEM;
	strcpy(qrself->qrst_filename, filename);

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

	if (fread(header, 1, sizeof(header), fp) < sizeof(header) ||
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
	/* Read in the comment. XXX Should transcode CP437 characters! */
	sprintf(comment, "%s\r\n%s", header + 0x0F, header + 0x4B);
	err = ldbs_put_comment(qrself->qrst_super.ld_store, comment);

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
		cylinder = chbuf[0];
		head     = chbuf[1];

		err = DSK_ERR_OK;
		switch (chbuf[2])
		{
			case 0: /* Uncompressed */
				if (fread(trackbuf, 1, tracklen, fp) < tracklen)
					err = DSK_ERR_NOTME;
				break;
			case 1: /* Blank */
				c = fgetc(fp);
				if (c == EOF) err = DSK_ERR_NOTME;
				memset(trackbuf, c, tracklen);
				break;
			case 2: /* Compressed */
				if (fread(chbuf, 1, 2, fp) < 2)
				{
					err = DSK_ERR_NOTME;
				}
				else	err = expand(fp, trackbuf, tracklen, chbuf[0] + 256 * chbuf[1]);
				break;
			default: /* Unsupported compression type */
				err = DSK_ERR_NOTME;
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
				for (c = 1; c < geom.dg_secsize; c++)
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


	return ldbsdisk_attach(self);
}

dsk_err_t qrst_creat(DSK_DRIVER *self, const char *filename)
{
	QRST_DSK_DRIVER *qrself;
	dsk_err_t err;

	/* Sanity check: Is this meant for our driver? */
	if (self->dr_class != &dc_qrst) return DSK_ERR_BADPTR;
	qrself = (QRST_DSK_DRIVER *)self;

	/* Save the filename, we'll want it when doing output */
	qrself->qrst_filename = dsk_malloc(1 + strlen(filename));
	if (!qrself->qrst_filename) return DSK_ERR_NOMEM;
	strcpy(qrself->qrst_filename, filename);

	/* OK, create a new empty blockstore */
	err = ldbs_new(&qrself->qrst_super.ld_store, NULL, LDBS_DSK_TYPE);
	if (err)
	{
		dsk_free(qrself->qrst_filename);
		return err;
	}
	return ldbsdisk_attach(self);
}


dsk_err_t qrst_close(DSK_DRIVER *self)
{
	QRST_DSK_DRIVER *qrself;

	/* Sanity check: Is this meant for our driver? */
	if (self->dr_class != &dc_qrst) return DSK_ERR_BADPTR;

	/* If this disc image has not been written to, just close it and 
	 * dispose thereof. */
	if (!self->dr_dirty)
	{
		dsk_free(qrself->qrst_filename);
		return ldbsdisk_close(self);
	}

	return DSK_ERR_NOTIMPL;
#if 0
	POSIX_DSK_DRIVER *pxself;

	if (self->dr_class != &dc_posix) return DSK_ERR_BADPTR;
	pxself = (POSIX_DSK_DRIVER *)self;

	if (pxself->px_fp) 
	{
		if (fclose(pxself->px_fp) == EOF) return DSK_ERR_SYSERR;
		pxself->px_fp = NULL;
	}
	return DSK_ERR_OK;	
#endif
}

