/***************************************************************************
 *                                                                         *
 *    LIBDSK: General floppy and diskimage access library                  *
 *    Copyright (C) 2001-2, 2017  John Elliott <seasip.webmaster@gmail.com>*
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

/* This driver works with the compressed "apridisk" format. The compression
 * works by doing RLE on individual sectors. We use the same method as for
 * CFI - decompressing the whole file into memory (or, since 1.5.3, into 
 * an LDBS blockstore), and writing back in drv_close(). 
 */

#include <stdio.h>
#include "libdsk.h"
#include "drvi.h"
#include "drvldbs.h"
#include "drvadisk.h"

#define APRIDISK_DELETED      0xE31D0000UL
#define APRIDISK_MAGIC        0xE31D0001UL
#define APRIDISK_COMMENT      0xE31D0002UL
#define APRIDISK_CREATOR      0xE31D0003UL
#define APRIDISK_UNCOMPRESSED 0x9E90
#define APRIDISK_COMPRESSED   0x3E5A

/* This struct contains function pointers to the driver's functions, and the
 * size of its DSK_DRIVER subclass */

DRV_CLASS dc_adisk = 
{
	sizeof(ADISK_DSK_DRIVER),
	&dc_ldbsdisk,	/* superclass */
	"apridisk\0",
	"APRIDISK file driver",
	adisk_open,	/* open */
	adisk_creat,	/* create new */
	adisk_close,	/* close */
};

static char adisk_wmagic[128] =
{
	'A','C','T',' ','A','p','r','i','c','o','t',' ','d','i','s','k',
	' ','i','m','a','g','e',26,4 
};

/* 16-byte header of an Apridisk record */
typedef struct adisk_recheader
{
	unsigned long item_type;
	unsigned short compression;
	unsigned short header_size;
	unsigned long data_size;
	unsigned char head;
	unsigned char sector;
	unsigned short cylinder;	
} ADISK_RECHEADER;

static dsk_err_t adisk_readheader(FILE *fp, ADISK_RECHEADER *result)
{
	unsigned char buf[16];
	int len;

	len = fread(buf, 1, sizeof(buf), fp);
	if (len < (int)sizeof(buf)) return DSK_ERR_SEEKFAIL;

	result->item_type   = ldbs_peek4(buf);
	result->compression = ldbs_peek2(buf+4);
	result->header_size = ldbs_peek2(buf+6);
	result->data_size   = ldbs_peek4(buf+8);
	result->head        = buf[12];
	result->sector      = buf[13];
	result->cylinder    = ldbs_peek2(buf+14);

	len = result->header_size - 16;
	while (len)
	{
		if (fgetc(fp) == EOF) return DSK_ERR_SEEKFAIL;
	}
	
	return DSK_ERR_OK;
}


/* Given a compressed buffer, either find its uncompressed length
 * or decompress it */
static size_t adisk_decompress(unsigned char *compressed, size_t c_len,
				  unsigned char *result)
{
	unsigned char *readp, *writep;
	unsigned short blklen;
	size_t u_len = 0;

	readp  = compressed;
	writep = NULL;	
	if (result) writep = result;
	u_len = 0;

	while (c_len >= 3)
	{
		blklen = ldbs_peek2(readp); 
		readp += 2;

		if (writep)
		{
			memset(writep, readp[0], blklen);
			writep += blklen;
		}
		u_len += blklen;
		readp++;
		c_len -= 3;
	}
	return u_len;
}



/* Read the next block from the Apridisk file and add it to the 
 * blockstore */
static dsk_err_t adisk_add_block(ADISK_DSK_DRIVER *self, FILE *fp)
{
	dsk_err_t err;
	unsigned char *buf, *buf2;
	char secid[4];
	int n, nsec, allsame;
	LDBLOCKID blkid;

	/* Load the track header. */
	ADISK_RECHEADER rh;
	LDBS_TRACKHEAD *trkh;

	err = adisk_readheader(fp, &rh); if (err) return DSK_ERR_OVERRUN;
/*
	printf("rh: item_type=%08x\n", rh.item_type);
	printf("rh: compression=%04x\n", rh.compression);
	printf("rh: header size=%04x\n", rh.header_size);
	printf("rh: data size=%08x\n",   rh.data_size);
	printf("rh: head=%02x\n", rh.head);
	printf("rh: sector=%02x\n", rh.sector);
	printf("rh: sector=%04x\n", rh.cylinder);
*/
	if (rh.item_type != APRIDISK_MAGIC 
	&&  rh.item_type != APRIDISK_COMMENT 
	&&  rh.item_type != APRIDISK_DELETED
	&&  rh.item_type != APRIDISK_CREATOR)
	{
		return DSK_ERR_NOTME;
	}
	if (rh.compression != APRIDISK_COMPRESSED 
	&&  rh.compression != APRIDISK_UNCOMPRESSED) 
	{
		return DSK_ERR_NOTME;
	}
	/* Compressed length must be at least 3; a block can't be any 
	 * smaller! */
	if (rh.data_size < 3 && rh.compression == APRIDISK_COMPRESSED) 
	{
		return DSK_ERR_NOTME;
	}
	/* Skip over deleted data blocks */
	if (rh.item_type == APRIDISK_DELETED)
	{	
		if (fseek(fp, rh.data_size, SEEK_CUR)) return DSK_ERR_SYSERR;
		return DSK_ERR_OK;
	}
	buf = dsk_malloc(1 + rh.data_size);
	buf[rh.data_size] = 0;
	if (buf == NULL) return DSK_ERR_NOMEM;

	/* Try to load the payload. If that fails, bail out */
	if (fread(buf, 1, rh.data_size, fp) < rh.data_size)
	{
		dsk_free(buf);
		return DSK_ERR_NOTME;	
	}
	/* If block is compressed, decompress it */
	if (rh.compression == APRIDISK_COMPRESSED)
	{
		size_t u_len = adisk_decompress(buf, rh.data_size, NULL);

		buf2 = dsk_malloc(u_len + 1);
		if (!buf2)
		{
			dsk_free(buf);
			return DSK_ERR_NOMEM;
		}
		adisk_decompress(buf, rh.data_size, buf2);
		buf2[u_len] = 0;
		dsk_free(buf);
		buf = buf2;
		rh.data_size = u_len;
	}
	switch (rh.item_type)
	{
		char *c;

		case APRIDISK_COMMENT:

		/* APRIDISK comments for some reason are terminated by \r */	
			for (n = 0; buf[n]; n++) 
			{
				if (buf[n] == '\r' && buf[n+1] != '\n') 
					buf[n] = '\n';
			}
		/* Assume the original comment was in codepage 437 */
			n = cp437_to_utf8((char *)buf, NULL, -1);
			c = dsk_malloc(n + 1);
			err = DSK_ERR_NOMEM;
			if (c)
			{
				cp437_to_utf8((char *)buf, c, -1);
				err = ldbs_put_comment
					(self->adisk_super.ld_store, c);
				dsk_free(c);
			}
			dsk_free(buf);
			return err;

		case APRIDISK_CREATOR:
			n = cp437_to_utf8((char *)buf, NULL, -1);
			c = dsk_malloc(n + 1);
			err = DSK_ERR_NOMEM;
			if (c)
			{
				cp437_to_utf8((char *)buf, c, -1);
				err = ldbs_put_creator
					(self->adisk_super.ld_store, c);
				dsk_free(c);
			}
			dsk_free(buf);
			return err;
	}
	/* Right, it's a sector. Store it. Unless, that is, it's all the
	 * same byte. */
	allsame = buf[0];
	for (n = 1; n < (int)(rh.data_size); n++)
	{
		if (buf[n] != buf[0]) 
		{
			allsame = -1;
			break;
		}
	}
	if (allsame != -1)
	{
		dsk_free(buf);
	}
	else
	{
		ldbs_encode_secid(secid, rh.cylinder, rh.head, rh.sector);
		blkid = LDBLOCKID_NULL;
		err = ldbs_putblock(self->adisk_super.ld_store, &blkid, secid, 
				buf, rh.data_size);
		dsk_free(buf);
		if (err) return err;
	}
	/* Now it needs to be recorded in the track header */
	err = ldbs_get_trackhead(self->adisk_super.ld_store, &trkh, 
				rh.cylinder, rh.head);
	if (err) { dsk_free(buf); return err; }

	if (trkh)
	{
		nsec = trkh->count;
		trkh = ldbs_trackhead_realloc(trkh, (unsigned short)(1 + trkh->count));
		if (!trkh) { dsk_free(buf); return DSK_ERR_NOMEM; }
	}
	else
	{
		nsec = 0;
		trkh = ldbs_trackhead_alloc(1);
		if (!trkh) { dsk_free(buf); return DSK_ERR_NOMEM; }

		trkh->datarate = 1;
		trkh->recmode = 2;
		trkh->filler = 0xf6;
	}
	if (trkh->count < 9) 		trkh->gap3 = 0x50;
	else if (trkh->count < 10)	trkh->gap3 = 0x52;
	else				trkh->gap3 = 0x17;

	trkh->sector[nsec].id_cyl  = (unsigned char)(rh.cylinder);
	trkh->sector[nsec].id_head = rh.head;
	trkh->sector[nsec].id_sec  = rh.sector;
	trkh->sector[nsec].id_psh  = dsk_get_psh(rh.data_size);
	trkh->sector[nsec].st1     = 0;
	trkh->sector[nsec].st2     = 0;
	if (allsame == -1)
	{
		trkh->sector[nsec].copies  = 1;
		trkh->sector[nsec].filler  = 0xF6;
		trkh->sector[nsec].blockid = blkid;
	}
	else
	{
		trkh->sector[nsec].copies  = 0;
		trkh->sector[nsec].filler  = allsame;
		trkh->sector[nsec].blockid = LDBLOCKID_NULL;
	}
	err = ldbs_put_trackhead(self->adisk_super.ld_store, trkh, 
				rh.cylinder, rh.head);
	dsk_free(trkh);
	return err;
}



/* Open an Apridisk drive image and convert to LDBS */
dsk_err_t adisk_open(DSK_DRIVER *self, const char *filename)
{
	FILE *fp;
	ADISK_DSK_DRIVER *adiskself;
	dsk_err_t err;	
	unsigned long magic;
	unsigned char header[128];
	unsigned char magbuf[4];
	
	/* Sanity check: Is this meant for our driver? */
	if (self->dr_class != &dc_adisk) return DSK_ERR_BADPTR;
	adiskself = (ADISK_DSK_DRIVER *)self;

	fp = fopen(filename, "r+b");
	if (!fp) 
	{
		adiskself->adisk_super.ld_readonly = 1;
		fp = fopen(filename, "rb");
	}
	if (!fp) return DSK_ERR_NOTME;

	/* Try to check the magic number at 0x80 */
	if (fread(header, 1, sizeof(header), fp) < (int)sizeof(header) 
	|| (memcmp(header, adisk_wmagic, sizeof(adisk_wmagic)))
	|| (fread(magbuf, 1, sizeof(magbuf), fp) < (int)sizeof(magbuf))
	|| ((magic = ldbs_peek4(magbuf)) != APRIDISK_MAGIC && 
	    magic != APRIDISK_CREATOR &&
	    magic != APRIDISK_COMMENT &&
	    magic != APRIDISK_DELETED))
	{
		fclose(fp);
		return DSK_ERR_NOTME;
	}
	/* Seek to end of header */
	fseek(fp, 0x80, SEEK_SET);	
	/* Keep a copy of the filename; when writing back, we will need it */
	adiskself->adisk_filename = dsk_malloc_string(filename);
	if (!adiskself->adisk_filename) return DSK_ERR_NOMEM;

	/* Initialise a new blockstore */
	err = ldbs_new(&adiskself->adisk_super.ld_store, NULL, LDBS_DSK_TYPE);
	if (err)
	{
		dsk_free(adiskself->adisk_filename);
		fclose(fp);
		return err;
	}

	dsk_report("Loading APRIDISK file into memory");
	while (!feof(fp))	
	{
		err = adisk_add_block(adiskself, fp);
		/* DSK_ERR_OVERRUN: End of file */
		if (err == DSK_ERR_OVERRUN) 
		{
			break;
		}
		if (err) 
		{
			dsk_free(adiskself->adisk_filename);
			dsk_report_end();
			return err;
		}
	} 
	dsk_report_end();
	err = ldbsdisk_attach(self);
	return err;
}


dsk_err_t adisk_creat(DSK_DRIVER *self, const char *filename)
{
	FILE *fp;
	ADISK_DSK_DRIVER *adiskself;
	dsk_err_t err;	
	
	/* Sanity check: Is this meant for our driver? */
	if (self->dr_class != &dc_adisk) return DSK_ERR_BADPTR;
	adiskself = (ADISK_DSK_DRIVER *)self;

	/* Create a 0-byte file, just to be sure we can */
	fp = fopen(filename, "wb");
	if (!fp) return DSK_ERR_SYSERR;
	if (fwrite(adisk_wmagic, 1, 128, adiskself->adisk_fp) < 128)
	{
		fclose(fp);
		return DSK_ERR_SYSERR;
	}
	fclose(fp);

	adiskself->adisk_filename = dsk_malloc_string(filename);
	if (!adiskself->adisk_filename) return DSK_ERR_NOMEM;

	/* Initialise a new blockstore */ 
	err = ldbs_new(&adiskself->adisk_super.ld_store, NULL, LDBS_DSK_TYPE);
	if (err)
	{
		dsk_free(adiskself->adisk_filename);
		fclose(fp);
		return err;
	}
	return ldbsdisk_attach(self);
}



static dsk_err_t adisk_save_creator(ADISK_DSK_DRIVER *self, FILE *fp)
{
	char *cmt = NULL;
	unsigned char *buf;
	unsigned long slen;
	int n;

	cmt = "LIBDSK v" LIBDSK_VERSION;

	/* No need to do utf8_to_cp437, because cmt is ASCII */
	
	buf = dsk_malloc(slen = 17 + strlen(cmt));
	if (!buf) return DSK_ERR_OK;
	memset(buf, 0, slen);
	strcpy((char *)buf + 16, cmt);
	/* Convert lone newlines to lone CRs */
	for (n = 17; buf[n]; n++)	 
	{
		if (buf[n] == '\n' && buf[n-1] != '\r') buf[n] = '\r';
	}
	ldbs_poke4(buf,     APRIDISK_CREATOR);
	ldbs_poke2(buf + 4, (short)APRIDISK_UNCOMPRESSED);
	ldbs_poke2(buf + 6, 0x10);
	ldbs_poke4(buf + 8, slen - 16);

	if (fwrite(buf, 1, slen, fp) < slen)
	{
		dsk_free(buf);
		return DSK_ERR_SYSERR;
	}
	dsk_free(buf);
	return DSK_ERR_OK;
}


static dsk_err_t adisk_save_comment(ADISK_DSK_DRIVER *self, FILE *fp)
{
	char *cmt = NULL;
	unsigned char *buf;
	unsigned long slen;
	int n;
	dsk_err_t err;

	err = ldbs_get_comment(self->adisk_super.ld_store, &cmt); 
	if (err || !cmt) return DSK_ERR_OK;	/* No comment */

	n = utf8_to_cp437(cmt, NULL, -1);
	buf = dsk_malloc(slen = 16 + n);
	if (!buf) return DSK_ERR_OK;
	memset(buf, 0, slen);
	utf8_to_cp437(cmt, (char *)(buf + 16), slen - 16);

	/* Convert lone newlines to lone CRs */
	for (n = 17; buf[n]; n++)	 
	{
		if (buf[n] == '\n' && buf[n-1] != '\r') buf[n] = '\r';
	}	
	ldbs_poke4(buf,     APRIDISK_COMMENT);
	ldbs_poke2(buf + 4, (short)APRIDISK_UNCOMPRESSED);
	ldbs_poke2(buf + 6, 0x10);
	ldbs_poke4(buf + 8, slen - 16);

	if (fwrite(buf, 1, slen, fp) < slen)
	{
		dsk_free(buf);
		return DSK_ERR_SYSERR;
	}
	dsk_free(buf);
	return DSK_ERR_OK;
}




static dsk_err_t sector_callback(PLDBS store, dsk_pcyl_t cyl, dsk_phead_t head,
			LDBS_SECTOR_ENTRY *se, LDBS_TRACKHEAD *th, void *param)
{
	ADISK_DSK_DRIVER *adiskself = param;
	unsigned long slen;
	int compress = 1;
	unsigned char *buf;
	char type[4];
	size_t secsize = (128 << se->id_psh);
	dsk_err_t err;

	/* If the sector is blank, save as compressed; otherwise 
	 * uncompressed. */
	compress = (se->copies == 0);

	if (compress) slen = 16 + 3;
	else	      slen = 16 + secsize;

	buf = dsk_malloc(slen); if (!buf) return DSK_ERR_NOMEM;

	if (compress)
	{
		ldbs_poke2(buf + 4, (unsigned short)APRIDISK_COMPRESSED);
		ldbs_poke4(buf + 8, 3);
		ldbs_poke2(buf + 16, (unsigned short)secsize);
		buf[18] =  se->filler;
	}
	else
	{
		ldbs_poke2(buf + 4, (unsigned short)APRIDISK_UNCOMPRESSED);
		ldbs_poke4(buf + 8, 128 << se->id_psh);
		err = ldbs_getblock(adiskself->adisk_super.ld_store, 
			se->blockid, type, buf + 16, &secsize);
		if (err && err != DSK_ERR_OVERRUN)
		{	
			dsk_free(buf);
			return err;
		}
	}
	ldbs_poke4(buf,     APRIDISK_MAGIC);
	ldbs_poke2(buf + 6, 0x10);
	buf[12] =  head;
	buf[13] =  se->id_sec;
	ldbs_poke2(buf + 14, (unsigned short)cyl);

	if (fwrite(buf, 1, slen, adiskself->adisk_fp) < slen)
	{
		dsk_free(buf);
		return DSK_ERR_SYSERR;
	}
	dsk_free(buf);
	return DSK_ERR_OK;

}



dsk_err_t adisk_close(DSK_DRIVER *self)
{
	ADISK_DSK_DRIVER *adiskself;
	dsk_err_t err;

	if (self->dr_class != &dc_adisk) return DSK_ERR_BADPTR;
	adiskself = (ADISK_DSK_DRIVER *)self;

	/* Firstly, ensure any pending changes are flushed to the LDBS 
	 * blockstore. Once this has been done we own the blockstore again 
	 * and have to close it after we've finished with it. */
	err = ldbsdisk_detach(self); 
	if (err)
	{
		dsk_free(adiskself->adisk_filename);
		ldbs_close(&adiskself->adisk_super.ld_store);
		return err;
	}

	/* If this disc image has not been written to, just close it and 
	 * dispose thereof. */
	if (!self->dr_dirty)
	{
		dsk_free(adiskself->adisk_filename);
		return ldbs_close(&adiskself->adisk_super.ld_store);
	}
	/* Trying to save changes but source is read-only */
	if (adiskself->adisk_super.ld_readonly)
	{
		dsk_free(adiskself->adisk_filename);
		ldbs_close(&adiskself->adisk_super.ld_store);
		return DSK_ERR_RDONLY;
	}
	adiskself->adisk_fp = fopen(adiskself->adisk_filename, "wb");
	if (!adiskself->adisk_fp)
	{
		dsk_free(adiskself->adisk_filename);
		ldbs_close(&adiskself->adisk_super.ld_store);
		return DSK_ERR_SYSERR;	
	}
	dsk_report("Compressing APRIDISK file");
	if (fwrite(adisk_wmagic, 1, 128, adiskself->adisk_fp) < 128)
	{
		err = DSK_ERR_SYSERR;
	}
	/* Write creator. Which is LibDsk, not the original utility */
	if (!err)
	{
		err = adisk_save_creator(adiskself, adiskself->adisk_fp);
	}
	/* Dump the sectors */	
	if (!err)
	{
		err = ldbs_all_sectors(adiskself->adisk_super.ld_store, 
				sector_callback, SIDES_ALT, adiskself);
	}
	/* Write the comment */
	if (!err)
	{
		err = adisk_save_comment(adiskself, adiskself->adisk_fp);
	}
	if (!err)
	{
		if (fclose(adiskself->adisk_fp))
		{
			err = DSK_ERR_SYSERR;
		}
	}
	else
	{
		fclose(adiskself->adisk_fp);
	}
	dsk_report_end();
	dsk_free(adiskself->adisk_filename);
	ldbs_close(&adiskself->adisk_super.ld_store);

	return err;
}






