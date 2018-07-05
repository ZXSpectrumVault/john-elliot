/***************************************************************************
 *                                                                         *
 *    LIBDSK: General floppy and diskimage access library                  *
 *    Copyright (C) 2001-2, 2016-7 John Elliott <seasip.webmaster@gmail.com>* 
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

/* This driver works with the ImageDisk "IMD" format. This compresses 
 * individual sectors using RLE, so we have to load the whole image into 
 * memory and work on it as an array of sectors.
 */

#include <stdio.h>
#include "libdsk.h"
#include "drvi.h"
#include "drvldbs.h"
#include "drvimd.h"
#ifdef HAVE_TIME_H
#include <time.h>
#endif

#define ST_NODATA    0	/* ID but no data */
#define ST_NORMAL    1	/* Normal data, uncompressed */
#define ST_CNORMAL   2	/* Normal data, compressed */
#define ST_DELETED   3	/* Deleted data, uncompressed */
#define ST_CDELETED  4	/* Deleted data, compressed */
#define ST_DATAERR   5	/* Normal data, uncompressed, data error on read */
#define ST_CDATAERR  6	/* Normal data, compressed, data error on read */
#define ST_DELERR    7	/* Deleted data, uncompressed, data error on read */
#define ST_CDELERR   8	/* Deleted data, compressed, data error on read */

/* This struct contains function pointers to the driver's functions, and the
 * size of its DSK_DRIVER subclass */

DRV_CLASS dc_imd = 
{
	sizeof(IMD_DSK_DRIVER),
	&dc_ldbsdisk,	/* superclass */
	"imd\0IMD\0",
	"IMD file driver",
	imd_open,	/* open */
	imd_creat,	/* create new */
	imd_close,	/* close */
};



/* For reading strings from the file: Read up to the next occurrence of 
 * either of the specified characters c1 / c2, and return how many bytes 
 * that is (including the terminator).
 *
 * If only interested in one terminating character, pass c2 = c1
 */
static dsk_err_t imd_readto(FILE *fp, char c1, char c2, int *count, int *termch)
{
	int ch;
	long pos = ftell(fp);
	int cnt = 0;

	*termch = EOF;	
	if (pos < 0)
	{
		return DSK_ERR_SYSERR;
	}
	while (1)
	{
		++cnt;
		ch = fgetc(fp);
		if (ch == EOF || ch == c1 || ch == c2) 
		{
			*termch = ch;
			break;
		}	
	}
	if (fseek(fp, pos, SEEK_SET))
	{
		return DSK_ERR_SYSERR;
	}
	*count = cnt;
	return DSK_ERR_OK;
}


dsk_err_t imd_write_header(IMD_DSK_DRIVER *self, FILE *fp, char *comment)
{
	time_t t;
	struct tm *ptm;
	char buf[80];
	char *ucomment = NULL;
	dsk_err_t err = DSK_ERR_OK;

	time (&t);
	ptm = localtime(&t);

	if (comment)
	{
		int len = utf8_to_cp437(comment, NULL, -1);
		ucomment = dsk_malloc(len);
		if (ucomment)
		{
			utf8_to_cp437(comment, ucomment, -1);
		}
	}

/* The spec seems to imply the header must read "IMD 1.18: datestamp" but 
 * TD02IMD writes a completely different header... */
	sprintf(buf, "IMD LibDsk %s: %02d/%02d/%04d %02d:%02d:%02d\r\n",
		LIBDSK_VERSION,
		ptm->tm_mday, ptm->tm_mon + 1, ptm->tm_year + 1900,
		ptm->tm_hour, ptm->tm_min, ptm->tm_sec);

	if (fwrite(buf, 1, strlen(buf), fp) < strlen(buf))
	{
		err = DSK_ERR_SYSERR;
	}
	/* Write comment */
	if (!err && ucomment)
	{
		if (fwrite(ucomment, 1, strlen(ucomment), fp) < strlen(ucomment))
		{
			err = DSK_ERR_SYSERR;
		}
	}
	if (!err && fputc(0x1A, fp) == EOF)
	{
		err = DSK_ERR_SYSERR;
	}
	if (ucomment) dsk_free(ucomment);
	return err;
}


typedef struct
{
	unsigned char   imdt_mode;      /* Recording mode */
	unsigned char   imdt_cylinder;  /* Cylinder */
	unsigned char   imdt_head;      /* Head and flags */
	unsigned char   imdt_sectors;   /* Sector count */
	unsigned short  imdt_seclen;    /* Default sector length */
} IMD_TRACK;


static dsk_err_t imd_load_track(IMD_DSK_DRIVER *self, dsk_ltrack_t count, 
	FILE *fp)
{
	/* Start by loading the track header: Fixed */
	LDBS_TRACKHEAD *trkh;
	IMD_TRACK tmp;
	dsk_err_t err;
	int n, psh, c, status;
	unsigned short datalen[256];

	/* Load the track header: 5 bytes */
	if (fread(&tmp.imdt_mode, 1, 4, fp) < 4)
	{
		return DSK_ERR_OVERRUN;	/* EOF */
	}
	psh = fgetc(fp);
	if (psh == EOF)
	{
		return DSK_ERR_OVERRUN;	/* EOF */
	}
	if (psh == 0xFF) 
	{
		tmp.imdt_seclen = 0xFFFF;
		for (n = 0; n < 256; n++) datalen[n] = 0;
	}
	else		 
	{
		tmp.imdt_seclen = (128 << psh);	
		for (n = 0; n < 256; n++) datalen[n] = tmp.imdt_seclen;
	}

	/* Allocate an LDBS track header for that number of sectors */
	trkh = ldbs_trackhead_alloc(tmp.imdt_sectors);	
	if (!trkh) return DSK_ERR_NOMEM;

	if (trkh->count < 9) 		trkh->gap3 = 0x50;
	else if (trkh->count < 10)	trkh->gap3 = 0x52;
	else				trkh->gap3 = 0x17;

	trkh->filler = 0xE5;
	switch (tmp.imdt_mode)
	{
		case 0: trkh->datarate = 2; trkh->recmode = 1; break;
		case 1: trkh->datarate = 1; trkh->recmode = 1; break;
		case 2: trkh->datarate = 1; trkh->recmode = 1; break;
		case 3: trkh->datarate = 2; trkh->recmode = 2; break;
		case 4: trkh->datarate = 1; trkh->recmode = 2; break;
		case 5: trkh->datarate = 1; trkh->recmode = 2; break;
/* Unofficial LibDsk extension for ED data rate */
		case 6: trkh->datarate = 3; trkh->recmode = 1; break;
		case 9: trkh->datarate = 3; trkh->recmode = 2; break;
	}

/*	printf("Mode %d Cyl %d Head %d Secs %d Size %d\n",
		tmp.imdt_mode, tmp.imdt_cylinder, tmp.imdt_head,
		tmp.imdt_sectors, tmp.imdt_seclen); */

	/* Set cylinder / head IDs to default */
	for (n = 0; n < tmp.imdt_sectors; n++)
	{
		trkh->sector[n].id_cyl  = tmp.imdt_cylinder;
		trkh->sector[n].id_head = tmp.imdt_head & 0x3F;
		trkh->sector[n].id_psh  = psh;
	}
	/* Load sector IDs */	
	for (n = 0; n < tmp.imdt_sectors; n++)
	{
		c = fgetc(fp); 	
		if (c == EOF) 
		{
			ldbs_free(trkh);
			return DSK_ERR_SYSERR;
		}
		trkh->sector[n].id_sec = c;			
	}
	/* Load sector cylinder map (if present) */
	if (tmp.imdt_head & 0x80)
	{
		for (n = 0; n < tmp.imdt_sectors; n++)
		{
			c = fgetc(fp); 	
			if (c == EOF) 
			{
				ldbs_free(trkh);
				return DSK_ERR_SYSERR;
			}
			trkh->sector[n].id_cyl = c;			
		}
	}

	/* Load sector head map (if present) */
	if (tmp.imdt_head & 0x40)
	{
		for (n = 0; n < tmp.imdt_sectors; n++)
		{
			c = fgetc(fp); 	
			if (c == EOF) 
			{
				ldbs_free(trkh);
				return DSK_ERR_SYSERR;
			}
			trkh->sector[n].id_head = c;			
		}
	}
	/* Load sector lengths (if present) */
	if (tmp.imdt_seclen == 0xFFFF)
	{
		int l, h;

		for (n = 0; n < tmp.imdt_sectors; n++)
		{
			l = fgetc(fp); 	
			h = fgetc(fp); 	
			if (l == EOF || h == EOF) 
			{
				ldbs_free(trkh);
				return DSK_ERR_SYSERR;
			}
			datalen[n] = (l & 0xFF) | ((h << 8) & 0xFF00);
			trkh->sector[n].id_psh = dsk_get_psh(datalen[n]);
		}
	}
	/* Sector data records */

	for (n = 0; n < tmp.imdt_sectors; n++)
	{
		unsigned char *buf;
		char secid[4];

		ldbs_encode_secid(secid, tmp.imdt_cylinder,
			tmp.imdt_head & 0x3F, trkh->sector[n].id_sec);
		
		/* Get status for sector */
		c = fgetc(fp); 	
		if (c == EOF) 
		{
			ldbs_free(trkh);
			return DSK_ERR_SYSERR;
		}
		status = c;
		/* Convert status to 8272 register bits */
		switch(status)
		{
			case ST_NODATA:  trkh->sector[n].st1 |= 4;  break;
			case ST_CDELETED:
			case ST_DELETED: trkh->sector[n].st2 |= 0x40; break;
			case ST_CDATAERR:
			case ST_DATAERR: trkh->sector[n].st2 |= 0x20; break;
			case ST_CDELERR:
			case ST_DELERR: trkh->sector[n].st2 |= 0x60; break;
		}
		switch(status)
		{
			default: 
#ifndef WIN16
				fprintf(stderr, "Unsupported IMD status "
					"0x%02x\n", status);
#endif
				ldbs_free(trkh);
				return DSK_ERR_NOTME;
			/* ID, no data */
			case ST_NODATA: 
				trkh->sector[n].blockid = LDBLOCKID_NULL;
				trkh->sector[n].copies = 0;
				trkh->sector[n].filler = 0xF6;
				break;
			/* Compressed */
			case ST_CNORMAL:  
			case ST_CDELETED:
			case ST_CDATAERR:
			case ST_CDELERR: 	
				c = fgetc(fp);
				if (c == EOF) 
				{
					ldbs_free(trkh);
					return DSK_ERR_SYSERR;
				}
				trkh->sector[n].blockid = LDBLOCKID_NULL;
				trkh->sector[n].copies = 0;
				trkh->sector[n].filler = c;
				break;
			/* Uncompressed */
			case ST_NORMAL: 		
			case ST_DELETED:
			case ST_DATAERR:
			case ST_DELERR: 
				trkh->sector[n].copies = 1;
				trkh->sector[n].filler = 0xF6;	
				buf = dsk_malloc(datalen[n]);
				if (!buf)
				{
					ldbs_free(trkh);
					return DSK_ERR_NOMEM;
				}
				if (fread(buf, 1, datalen[n], fp) < datalen[n])
				{
					dsk_free(buf);
					ldbs_free(trkh);
					return DSK_ERR_SYSERR;
				}
				err = ldbs_putblock(self->imd_super.ld_store, 
					&trkh->sector[n].blockid, 
					secid, buf, datalen[n]);
				dsk_free(buf);
				if (err)
				{
					ldbs_free(trkh);
					return err;
				}
				break;
		}	
	}
	/* All sectors read. Write back the track header */
	err = ldbs_put_trackhead(self->imd_super.ld_store, trkh, 
				tmp.imdt_cylinder, tmp.imdt_head & 0x3F);
	ldbs_free(trkh);
	return DSK_ERR_OK;
}



dsk_err_t imd_open(DSK_DRIVER *self, const char *filename)
{
	FILE *fp;
	IMD_DSK_DRIVER *imdself;
	dsk_err_t err;	
	int ccmt;
	dsk_ltrack_t count = 0;
	char *comment, *ucomment;
	int termch;

	/* Sanity check: Is this meant for our driver? */
	if (self->dr_class != &dc_imd) return DSK_ERR_BADPTR;
	imdself = (IMD_DSK_DRIVER *)self;

	fp = fopen(filename, "r+b");
	if (!fp) 
	{
		imdself->imd_super.ld_readonly = 1;
		fp = fopen(filename, "rb");
	}
	if (!fp) return DSK_ERR_NOTME;

	/* Try to check the magic number. Read the first line, which
 	 * may terminate with '\n' if a comment follows, or 0x1A 
	 * otherwise. */
	err = imd_readto(fp, '\n', 0x1A, &ccmt, &termch);
	if (err)
	{
		fclose(fp);
		return DSK_ERR_NOTME;
	}
	comment = dsk_malloc(1 + ccmt);
	if (!comment)
	{
		fclose(fp);
		return DSK_ERR_NOTME;
	}
	if ((int)fread(comment, 1, ccmt, fp) < ccmt)
	{
		fclose(fp);
		return DSK_ERR_NOTME;
	}
	comment[ccmt] = 0;

/*	printf("Header='%s' pos=%ld\n", comment, ftell(fp)); */

	/* IMD signature is 4 bytes magic, then the rest of the line is 
	 * freeform (but probably includes a date stamp) */
	if (memcmp(comment, "IMD ", 4))
	{
		dsk_free(comment);
		fclose(fp);
		return DSK_ERR_NOTME;
	}
	dsk_free(comment);

	err = ldbs_new(&imdself->imd_super.ld_store, NULL, LDBS_DSK_TYPE);
	if (err)
	{
		fclose(fp);
		return err;
	}
	/* If the header wasn't terminated by 0x1A, then a comment
	 * follows. */
	if (termch != 0x1A)
	{
		int n;

		err = imd_readto(fp, 0x1A, 0x1A, &ccmt, &termch);
		if (err)
		{
			fclose(fp);
			return DSK_ERR_NOTME;
		}
		comment = dsk_malloc(1 + ccmt);
		if (!comment)
		{
			fclose(fp);
			return DSK_ERR_NOMEM;
		}
		if ((int)fread(comment, 1, ccmt, fp) < ccmt)
		{
			dsk_free(comment);
			fclose(fp);
			return DSK_ERR_SYSERR;
		}	
		comment[ccmt - 1] = 0;
		n = cp437_to_utf8(comment, NULL, -1);
		ucomment = dsk_malloc(n);
		if (!ucomment)
		{
			dsk_free(comment);
			fclose(fp);
			return DSK_ERR_NOMEM;
		}
		cp437_to_utf8(comment, ucomment, -1);
		err = ldbs_put_comment(imdself->imd_super.ld_store, ucomment);
		dsk_free(ucomment);
		dsk_free(comment);
		if (err)
		{
			fclose(fp);
			return err;
		}
	}
	/* Keep a copy of the filename; when writing back, we will need it */
	imdself->imd_filename = dsk_malloc_string(filename);
	if (!imdself->imd_filename) 
	{
		fclose(fp);
		ldbs_close(&imdself->imd_super.ld_store);
		return DSK_ERR_NOMEM;
	}
	/* And now we're onto the tracks */
	ccmt = 0;
	dsk_report("Loading IMD file into memory");

	err = DSK_ERR_OK;

	while (!feof(fp))
	{
		err = imd_load_track(imdself, count, fp);
		if (err == DSK_ERR_OVERRUN) 	/* EOF */
		{
			dsk_report_end();
			fclose(fp);
			return ldbsdisk_attach(self);
		}
		else if (err) 
		{
			dsk_free(imdself->imd_filename);
			ldbs_close(&imdself->imd_super.ld_store);
			dsk_report_end();
			return err;
		}
		++count;
	} 
	/* Should never get here! */
	dsk_report_end();
	return ldbsdisk_attach(self);
}


dsk_err_t imd_creat(DSK_DRIVER *self, const char *filename)
{
	IMD_DSK_DRIVER *imdself;
	FILE *fp;
	dsk_err_t err;
	
	/* Sanity check: Is this meant for our driver? */
	if (self->dr_class != &dc_imd) return DSK_ERR_BADPTR;
	imdself = (IMD_DSK_DRIVER *)self;

	/* See if the file can be created. But don't hold it open. */
	fp = fopen(filename, "wb");
	if (!fp) return DSK_ERR_SYSERR;
	err = imd_write_header(imdself, fp, NULL);
	fclose(fp);
	if (err) return err;

	err = ldbs_new(&imdself->imd_super.ld_store, NULL, LDBS_DSK_TYPE);
	if (err) return err;

	/* Keep a copy of the filename, for writing back */
	imdself->imd_filename = dsk_malloc_string(filename);
	if (!imdself->imd_filename) return DSK_ERR_NOMEM;
	
	return ldbsdisk_attach(self);
}

static dsk_err_t imd_save_track(PLDBS ldbs, dsk_pcyl_t cyl, dsk_phead_t head,
			LDBS_TRACKHEAD *th, void *param)
{
	IMD_DSK_DRIVER *self = param;
	IMD_TRACK tmp;
	int fm = (th->recmode == 1);	
	unsigned char psh;
	void *buf;
	size_t buflen;
	size_t seclen[256];
	int n, nsec;
	dsk_err_t err;

	for (n = 0; n < 256; n++) seclen[n] = 0;	
	switch (th->datarate)
	{
		default:
/* LDBS does not distinguish between 250kbps and 300kbps data rates, since
 * they're an artefact of how different drives process the same discs. 
 * So when saving, we have to guess which rate was in play at the time. */
		case 1:	if (th->count < 11)
				tmp.imdt_mode = fm ? 2 : 5; 
			else	tmp.imdt_mode = fm ? 1 : 4; 
			break;
		case 2:	tmp.imdt_mode = fm ? 0 : 3; break;
		case 3:	/* Not supported by IMD format, so 
			 * we'll just have to improvise */	
			tmp.imdt_mode = fm ? 6 : 9; break;
		
	}
	tmp.imdt_cylinder = cyl;
	tmp.imdt_head = head;
	tmp.imdt_sectors = (unsigned char)(th->count);

	if (th->count == 0)
	{
		/* Write fixed part of header */
		if (fwrite(&tmp.imdt_mode, 1, 4, self->imd_fp) < 4
		||  fputc(0, self->imd_fp) == EOF)
		{
			return DSK_ERR_SYSERR;
		}
		return DSK_ERR_OK;
	}
	/* OK, we have at least one sector */
	psh = th->sector[0].id_psh;

	/* See if we need to write cylinder / head maps */
	tmp.imdt_head &= 0x3F;

	for (nsec = 0; nsec < th->count; nsec++)
	{
		if (th->sector[nsec].id_cyl != cyl)
			tmp.imdt_head |= 0x80;
		if (th->sector[nsec].id_head != head)
			tmp.imdt_head |= 0x40;

		/* Get actual sector lengths */
		if (th->sector[nsec].blockid == LDBLOCKID_NULL)
		{
			seclen[nsec] = 128 << th->sector[nsec].id_psh;
		}
		else
		{
			err = ldbs_get_blockinfo(ldbs, th->sector[nsec].blockid,
				NULL, &seclen[nsec]);
			if (err) return err;
		}
		/* Do we need to record them? */
		if (psh != 0xFF && seclen[nsec] != (size_t)(128 << psh))
			psh = 0xFF;
	}
	
	/* Write fixed part of header */
	if (fwrite(&tmp.imdt_mode, 1, 4, self->imd_fp) < 4
	||  fputc(psh, self->imd_fp) == EOF)
	{
		return DSK_ERR_SYSERR;
	}
		
	/* Write sector IDs */	
	for (nsec = 0; nsec < th->count; nsec++)
	{
		if (fputc(th->sector[nsec].id_sec, self->imd_fp) == EOF)
			return DSK_ERR_SYSERR;
	}
	/* Write cylinder IDs, if necessary */
	if (tmp.imdt_head & 0x80)
	{
		for (nsec = 0; nsec < th->count; nsec++)
		{
			if (fputc(th->sector[nsec].id_cyl, self->imd_fp) == EOF)
				return DSK_ERR_SYSERR;
		}
	}
	/* Write head IDs, if necessary */
	if (tmp.imdt_head & 0x40)
	{
		for (nsec = 0; nsec < th->count; nsec++)
		{
			if (fputc(th->sector[nsec].id_head, self->imd_fp) == EOF)
				return DSK_ERR_SYSERR;
		}
	}
	/* Write sector sizes, if necessary */
	if (psh == 0xFF)
	{
		for (nsec = 0; nsec < th->count; nsec++)
		{
			if (fputc(seclen[nsec] & 0xFF, self->imd_fp) == EOF)
				return DSK_ERR_SYSERR;
			if (fputc(seclen[nsec] >> 8, self->imd_fp) == EOF)
				return DSK_ERR_SYSERR;
		}
	}

	/* Write sector data */
	for (nsec = 0; nsec < th->count; nsec++)
	{
		char status = ST_NORMAL;
		int compressed = (th->sector[nsec].copies == 0);

		if (th->sector[nsec].st1 & 4)	/* No data */
		{
			status = ST_NODATA;
		}
		else switch(th->sector[nsec].st2 & 0x60)
		{
			case 0x00: status = compressed ?ST_CNORMAL : ST_NORMAL;
				   break;
			case 0x20: status = compressed ?ST_CDATAERR: ST_DATAERR;
				   break;
			case 0x40: status = compressed ?ST_CDELETED: ST_DELETED;
				   break;
			case 0x60: status = compressed ?ST_CDELERR : ST_DELERR;
				   break;
		}	
		if (fputc(status, self->imd_fp) == EOF)
			return DSK_ERR_SYSERR;
		if (status == ST_NODATA) continue;

		if (compressed)
		{
			if (fputc(th->sector[nsec].filler, self->imd_fp) == EOF)
				return DSK_ERR_SYSERR;
		}
		else
		{
			size_t datalen = seclen[nsec];

			err = ldbs_getblock_a(ldbs, th->sector[nsec].blockid,
					NULL, &buf, &buflen);
			if (err) return err;

			/* If sector is short, pad to stated length. Should
			 * never happen because that's the size recorded 
			 * in the blockstore in the first place! */
			if (buflen < datalen)
			{
				if (fwrite(buf, 1, datalen, self->imd_fp) < buflen)
				{
					ldbs_free(buf);
					return DSK_ERR_SYSERR;
				}
				while (buflen < datalen)
				{
					if (fputc(0xCC, self->imd_fp) == EOF)
					{
						ldbs_free(buf);
						return DSK_ERR_SYSERR;
					}
					++buflen;
				}
			}
			else
			{
				if (fwrite(buf, 1, datalen, self->imd_fp) < datalen)
				{
					ldbs_free(buf);
					return DSK_ERR_SYSERR;
				}	
			}
			ldbs_free(buf);
		}
	}
	return DSK_ERR_OK;
}



dsk_err_t imd_close(DSK_DRIVER *self)
{
	IMD_DSK_DRIVER *imdself;
	dsk_err_t err = DSK_ERR_OK;
	char *comment;

	if (self->dr_class != &dc_imd) return DSK_ERR_BADPTR;
	imdself = (IMD_DSK_DRIVER *)self;

	/* Firstly, ensure any pending changes are flushed to the LDBS 
	 * blockstore. Once this has been done we own the blockstore again 
	 * and have to close it after we've finished with it. */
	err = ldbsdisk_detach(self); 
	if (err)
	{
		dsk_free(imdself->imd_filename);
		ldbs_close(&imdself->imd_super.ld_store);
		return err;
	}

	/* If this disc image has not been written to, just close it and 
	 * dispose thereof. */
	if (!self->dr_dirty)
	{
		dsk_free(imdself->imd_filename);
		return ldbs_close(&imdself->imd_super.ld_store);
	}
	/* Trying to save changes but source is read-only */
	if (imdself->imd_super.ld_readonly)
	{
		dsk_free(imdself->imd_filename);
		ldbs_close(&imdself->imd_super.ld_store);
		return DSK_ERR_RDONLY;
	}
	/* OK, we're ready to write back */
	imdself->imd_fp = fopen(imdself->imd_filename, "wb");
	dsk_free(imdself->imd_filename);
	if (!imdself->imd_fp)
	{
		ldbs_close(&imdself->imd_super.ld_store);
		return DSK_ERR_SYSERR;
	}
	dsk_report("Writing IMD file");
	/* Write out the header */
	if (ldbs_get_comment(imdself->imd_super.ld_store, &comment) != DSK_ERR_OK)
	{
		comment = NULL;
	}
	err = imd_write_header(imdself, imdself->imd_fp, comment);
	if (comment) ldbs_free(comment);
	if (err)
	{
		ldbs_close(&imdself->imd_super.ld_store);
		fclose(imdself->imd_fp);
		dsk_report_end();
		return err;
	}
	err = ldbs_all_tracks(imdself->imd_super.ld_store, imd_save_track,
				SIDES_ALT, imdself);
	if (err)
	{
		ldbs_close(&imdself->imd_super.ld_store);
		fclose(imdself->imd_fp);
		dsk_report_end();
		return err;
	}
	if (fclose(imdself->imd_fp))
	{
		ldbs_close(&imdself->imd_super.ld_store);
		dsk_report_end();
		return DSK_ERR_SYSERR;
	}
	dsk_report_end();
	return ldbs_close(&imdself->imd_super.ld_store);
}

