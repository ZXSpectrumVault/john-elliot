/***************************************************************************
 *                                                                         *
 *    LIBDSK: General floppy and diskimage access library                  *
 *    Copyright (C) 2001-2,2007,2017  John Elliott <seasip.webmaster@gmail.com>*
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

/* This driver provides limited read-only support for Teledisk files. It is
 * based on the file format documentation at
 * <http://www.fpns.net/willy/wteledsk.htm>
 * <ftp://bitsavers.informatik.uni-stuttgart.de/pdf/sydex/Teledisk_1.05_Sep88.pdf>
 * <http://www.classiccmp.org/dunfield/img54306/td0notes.txt>
 *
 * No code from wteledsk has been used in this file, since it is under GPL 
 * and LibDsk is under LGPL.
 *
 * (GPL code from wteledsk was incorporated into comptlzh.c / comptlzh.h;
 *  in that case, it was relicensed as LGPL with permission).
 *
 * Current bugs / limitations:
 *
 * - No support for images split into multiple files (.TD0, .TD1, .TD2...)
 *   
 * - Advanced compression is read-only. Rather than having it built-in to 
 *   this driver, we use a compression module to convert advanced-compression 
 *   TD0 files to normal, using code from wteledsk. But this doesn't go the
 *   other way.
 *
 *   There's also an "old advanced" compression, which uses 12-bit LZ 
 *   compression in 6k blocks, so another 'comp' driver would be needed for 
 *   that.
 *
 */


#include "drvi.h"
#include "drvldbs.h"
#include "drvtele.h"
#ifdef HAVE_TIME_H
#include <time.h>
#endif

/* Signature for an LDBS block containing Teledisk-specific data */
#define TELEDISK_USER_BLOCK "utd0"

extern unsigned short teledisk_crc(unsigned char *buf, unsigned short len);

DRV_CLASS dc_tele = 
{
	sizeof(TELE_DSK_DRIVER),
	&dc_ldbsdisk,		/* superclass */
	"tele\0teledisk\0TD0\0td0\0",
	"TeleDisk file driver",
	tele_open,
	tele_creat,
	tele_close,
};

/* #define MONITOR(x) printf x */    

#define MONITOR(x) 


#define DC_CHECK(s) \
	TELE_DSK_DRIVER *self; \
	if (s->dr_class != &dc_tele) return DSK_ERR_BADPTR; \
	self = (TELE_DSK_DRIVER *)s;


/* Read a number of bytes from the file we're processing, and return a 
 * dsk_err_t in case of error or EOF. The original intention was that 
 * it could be expanded to handle compressed TD0 files, but instead a 
 * separate compression driver was written.  */
static dsk_err_t tele_fread(TELE_DSK_DRIVER *self, tele_byte *buf, int count)
{
	if (!buf) 
	{
		if (fseek(self->tele_fp, count, SEEK_CUR))
		{
			return DSK_ERR_SYSERR;
		}
	}
	else
	{
		if (fread(buf, 1, count, self->tele_fp) < (size_t)count)
		{
			return DSK_ERR_SYSERR;
		}
	}
	return DSK_ERR_OK;
}


/* Expand a compressed Teledisk sector to an LDBS sector, updating its
 * entry in the track header trkh. */
static dsk_err_t convert_sector(TELE_DSK_DRIVER *self, LDBS_TRACKHEAD *trkh,
				dsk_pcyl_t cyl, dsk_phead_t head, unsigned nsec)
{
	dsk_err_t err;
	tele_byte buf[6];
	tele_byte syndrome;
	size_t ulen, pos, wleft, n;
	tele_byte encoding;
	tele_byte pattern[257];
	tele_byte *secbuf;
	int allsame; 
	char secid[4];

	MONITOR (("Loading sector cyl=%d head=%d nsec=%d\n", cyl, head, nsec));
	err = tele_fread(self, buf, 6);
	if (err) return err;
	trkh->sector[nsec].id_cyl  = buf[0];
	trkh->sector[nsec].id_head = buf[1];
	trkh->sector[nsec].id_sec  = buf[2];
	trkh->sector[nsec].id_psh  = buf[3];
	syndrome = buf[4];
	/* buf[5] is the CRC, ignored on load */

	MONITOR(("ID=(%d,%d,%d) size=%d syndrome=0x%02x\n",
			buf[0], buf[1], buf[2], 128 << buf[3], syndrome));
	/* Convert syndrome byte to error flags */
	if (syndrome & 2) trkh->sector[nsec].st2 |= 0x20; /* CRC error */
	if (syndrome & 4) trkh->sector[nsec].st2 |= 0x40; /* Control mark */
	if (syndrome & 0x20) trkh->sector[nsec].st1 |= 0x04; /* No data */
	if (syndrome & 0x40) trkh->sector[nsec].st1 |= 0x01; /* Data, no ID */

	/* Syndromes 0x10 and 0x20 omit sector data */
	if ((syndrome & 0x30) != 0)
	{
		trkh->sector[nsec].copies = 0;	
		trkh->sector[nsec].filler = 0xF6;
		trkh->sector[nsec].blockid = LDBLOCKID_NULL;
		return DSK_ERR_OK;
	}
	/* Otherwise sector data follow */
	err = tele_fread(self, buf, 3);
	if (err) return err;

	encoding   = buf[2];

	ulen = 128 << trkh->sector[nsec].id_psh;
	secbuf = dsk_malloc(ulen + 2);	/* In case of overflow */
	if (!secbuf) return DSK_ERR_NOMEM;

	MONITOR(("Compressed len = %d encoding=0x%02x ulen=%ld\n", 
		ldbs_peek2(buf), encoding, ulen));

	switch(encoding)
	{
		case 0:	/* Uncompressed */
			err = tele_fread(self, secbuf, ulen);
			if (err) { dsk_free(secbuf); return err; }
			break;
		case 1:	/* One pattern */
			err = tele_fread(self, pattern, 4);
			if (err) { dsk_free(secbuf); return err; }

			for (n = 0; n < ldbs_peek2(pattern) && n*2 < ulen; n++)
			{
				secbuf[n*2]   = pattern[2];
				secbuf[n*2+1] = pattern[3];
			}
			break;
	
		case 2:	/* More than one pattern */
			pos = 0;
			while (pos < ulen)
			{
				tele_byte ptype, plen;

				wleft = ulen - pos;
				err = tele_fread(self, pattern, 2);
				if (err) { dsk_free(secbuf); return err; }
				/* Load pattern type & length */
				ptype = pattern[0];
				plen = pattern[1];
				/* Uncompressed run? */
				if (ptype == 0)
				{
					err = tele_fread(self, pattern, plen);
					if (err) { dsk_free(secbuf); return err; }
					if (plen > wleft) plen = wleft;
					memcpy(secbuf + pos, pattern, plen);
					pos += plen;
					continue;
				}
				/*  Compressed run */
				err = tele_fread(self, pattern, (1 << ptype));
				if (err) { dsk_free(secbuf); return err; }
				for (n = 0; n < plen; n++)
				{
/* Ensure the amount of data written does not exceed len */
					if ((unsigned int)(1 << ptype) > wleft)
						memcpy(secbuf + pos, pattern,wleft);	
					else	memcpy(secbuf + pos, pattern, 1 << ptype);
					pos += (1 << ptype);	
					wleft -= (1 << ptype);
				}
			}
			break;
		default:	
#ifndef WIN16
			fprintf(stderr, "Teledisk: Unsupported sector compression method %d!\n", encoding);
#endif
			dsk_free(secbuf);
			return DSK_ERR_NOTME;
	}
	/* Sector is now loaded. See if it's all one byte. */
	allsame = 1;
	for (n = 1; n < ulen; n++)
	{
		if (secbuf[n] != secbuf[0]) { allsame = 0; break; }
	}
	if (allsame)
	{
		trkh->sector[nsec].copies = 0;	
		trkh->sector[nsec].filler = secbuf[0];
		trkh->sector[nsec].blockid = LDBLOCKID_NULL;
		dsk_free(secbuf);
		return DSK_ERR_OK;
	}
	/* If not, it must be stored in the blockstore. */
	ldbs_encode_secid(secid, cyl, head, trkh->sector[nsec].id_sec);
	err = ldbs_putblock(self->tele_super.ld_store, 
				&trkh->sector[nsec].blockid, secid,
				secbuf, ulen);
	trkh->sector[nsec].copies = 1;
	trkh->sector[nsec].filler = 0xF6;
	dsk_free(secbuf);
	return err;
}





/* Open a Teledisk file and load it into the blockstore */
dsk_err_t tele_open(DSK_DRIVER *s, const char *filename)
{
	dsk_err_t err;
	unsigned short crc;
	unsigned char header[12];
	DC_CHECK(s);

	self->tele_fp = fopen(filename, "r+b");
	if (!self->tele_fp) 
	{
		self->tele_super.ld_readonly = 1;
		self->tele_fp = fopen(filename, "rb");
		if (!self->tele_fp)
		{
			return DSK_ERR_NOTME;
		}
	}
	/* Loading the header uses fread() rather than tele_read(), because
	 * advanced compression doesn't touch the header. */
	if (fread(header, 1, sizeof(header), self->tele_fp) < (int)sizeof(header) ||
	    (memcmp(header, "TD", 2) && memcmp(header, "td", 2)))
	{
		fclose(self->tele_fp);
		return DSK_ERR_NOTME;
	}
	/* Parse the header */
	memset(&self->tele_head, 0, sizeof(self->tele_head));
	memcpy(self->tele_head.magic, header, 2);
	self->tele_head.magic[2] = 0;
	self->tele_head.volume_seq  = header[2];
	self->tele_head.volume_id   = header[3];
	self->tele_head.ver         = header[4];
	self->tele_head.datarate    = header[5];
	self->tele_head.drivetype   = header[6];
	self->tele_head.doubletrack = header[7];
	self->tele_head.dosmode     = header[8];
	self->tele_head.sides       = header[9];
	crc       = ((tele_word)header[11]) << 8 | header[10];

	/* Check header CRC; if it's wrong, this probably isn't a 
 	 * Teledisk file at all.. */
	if (teledisk_crc(header, 10) != crc)
	{
		fclose(self->tele_fp);
		return DSK_ERR_NOTME;
	}

	/* Advanced compression not handled here -- see comptlzh.c */
	if (!strcmp((char *)header, "td"))
	{
#ifndef WIN16
		fprintf(stderr, "LibDsk TD0 driver: Advanced compression not supported\n");
#endif
		fclose(self->tele_fp);
		return DSK_ERR_NOTIMPL;
	}
	/* Create an LDBS instance */
	err = ldbs_new(&self->tele_super.ld_store, NULL, LDBS_DSK_TYPE);
	if (err)
	{
		fclose(self->tele_fp);
		return err;
	}

	/* Read the comment if there is one (converting it to UTF-8) */
	if (self->tele_head.doubletrack & 0x80)
	{
		size_t comment_len, n, ulen;
		char *comment_data;
		char *ucomment;

		/* Load comment header */
		if (tele_fread(self, header, 10))
		{
			fclose(self->tele_fp);
			return DSK_ERR_SYSERR;
		}
		comment_len  = ldbs_peek2(header + 2);
		comment_data = dsk_malloc(comment_len + 1);
		if (!comment_data)
		{
			ldbs_close(&self->tele_super.ld_store);
			fclose(self->tele_fp);
			return DSK_ERR_NOMEM;
		}
		if (tele_fread(self, (tele_byte *)comment_data, comment_len))
		{
			dsk_free(comment_data);
			ldbs_close(&self->tele_super.ld_store);
			fclose(self->tele_fp);
			return DSK_ERR_SYSERR;
		}
		/* 0-terminate the loaded comment */
		comment_data[comment_len] = 0;
		/* Replace double-zeroes with CR/LF pairs */
		for (n = 0; n < comment_len; n++)
		{
			if (comment_data[n] == 0 &&
			    comment_data[n+1] == 0)
			{
				comment_data[n] = '\r';
				comment_data[n+1] = '\n';
			}
		}
		/* Convert from codepage 437 to UTF8 */
		ulen = cp437_to_utf8(comment_data, NULL, -1);
		ucomment = dsk_malloc(ulen + 22);
		if (!ucomment)
		{
			dsk_free(comment_data);
			ldbs_close(&self->tele_super.ld_store);
			fclose(self->tele_fp);
			return DSK_ERR_NOMEM;
		}
		/* In a Teledisk file, the comment record has a timestamp.
		 * Convert this to a text timestamp at the start of the 
		 * comment string */
		sprintf(ucomment, "[%04d-%02d-%02dT%02d:%02d:%02d] ",
				1900 + header[4], 1 + (header[5] % 12), 
					header[6] % 100, header[7] % 100,
					header[8] % 100, header[9] % 100);
	
		cp437_to_utf8(comment_data, ucomment + 22, -1);	
		err = ldbs_put_comment(self->tele_super.ld_store, ucomment);
		dsk_free(ucomment);
		dsk_free(comment_data);	
		if (err)
		{
			ldbs_close(&self->tele_super.ld_store);
			fclose(self->tele_fp);
			return err;
		}
	}
	/* Save the information from the header */
	err = ldbs_putblock_d(self->tele_super.ld_store, TELEDISK_USER_BLOCK,
				&self->tele_head, sizeof(self->tele_head));
	if (err)
	{
		ldbs_close(&self->tele_super.ld_store);
		fclose(self->tele_fp);
		return err;
	}

	/* Now to parse the tracks. 
 	 * TODO: When we reach EOF, if this is a multi-file set, close 
	 * the TD0 file and look for TD1, TD2 etc. */
	while (!feof(self->tele_fp))
	{
		LDBS_TRACKHEAD *trkh;
		dsk_pcyl_t cyl;
		dsk_phead_t head;
		unsigned n;

		/* Try to read the track header. If it isn't present because
		 * of EOF, fine. */
		if (tele_fread(self, header, 4))
		{
			if (feof(self->tele_fp)) break;

			ldbs_close(&self->tele_super.ld_store);
			fclose(self->tele_fp);
			return err;
		}
		/* header[0] = sectors / track, if 0xFF then break */
		if (header[0] == 0xFF) break;
		cyl = header[1];
		head = header[2] & 0x7F;
		trkh = ldbs_trackhead_alloc(header[0]);
		/* Populate data rate & recording mode. LDBS does not 
		 * distinguish between the 250Kbps data rate (360k disc in
		 * 360k drive) and 300Kbs data rate (360k disc in 1.2M drive)
		 * since it considers data rate a property of the disc, not
		 * the drive. */
		switch (self->tele_head.datarate & 0x7F)
		{
			case 0: case 1:  trkh->datarate = 1; break;
			case 2: 	 trkh->datarate = 2; break;
			case 3: 	 trkh->datarate = 3; break;
		}
		/* Recording mode may be indicated in disk or track
		 * header. */
		if ((self->tele_head.datarate & 0x80) || header[2] & 0x80)
			trkh->recmode = 1;
		else	trkh->recmode = 2;
		trkh->filler = 0xF6;
/* TD0 files don't hold GAP3 -- take a wild guess. */
		if (trkh->count < 9)            trkh->gap3 = 0x50;
		else if (trkh->count < 10)      trkh->gap3 = 0x52;
		else                            trkh->gap3 = 0x17;

		for (n = 0; n < header[0]; n++)
		{
			err = convert_sector(self, trkh, cyl, head, n);
			if (err)
			{
				ldbs_free(trkh);
				ldbs_close(&self->tele_super.ld_store);
				fclose(self->tele_fp);
				return err;
			}
		}
		err = ldbs_put_trackhead(self->tele_super.ld_store, trkh,
					cyl, head);	
		ldbs_free(trkh);
		if (err)
		{
			ldbs_close(&self->tele_super.ld_store);
			fclose(self->tele_fp);
			return err;
		}
	}
	fclose(self->tele_fp);
	self->tele_fp = NULL;
	self->tele_filename = dsk_malloc_string(filename);
	if (!self->tele_filename)
	{
		ldbs_close(&self->tele_super.ld_store);
		return DSK_ERR_NOMEM;
	}
	return ldbsdisk_attach(s);
}

/* Set a Teledisk header to default values */
static void default_header(unsigned char *header)
{
	unsigned short crc;

	memset(header, 0, 12);
	header[0] = 'T';
	header[1] = 'D';
	header[2] = 0;			/* Volume sequence */
	header[3] = rand() & 0xFF;	/* Volume ID */
	header[4] = 0x15;		/* File format version */
	header[5] = 0;			/* Source density */
	header[6] = 3;			/* Drive type */
	header[7] = 0;			/* Track density matches */
	header[8] = 0;			/* All sectors */
	header[9] = 1;			/* 1 head */
	crc = teledisk_crc(header, 10);
	ldbs_poke2(header + 10, crc);
}


/* Create a blank TD0 file */
dsk_err_t tele_creat(DSK_DRIVER *s, const char *filename)
{
	dsk_err_t err;
	unsigned char header[12];
	DC_CHECK(s);

	self->tele_fp = fopen(filename, "wb");
	if (!self->tele_fp) 
	{
		return DSK_ERR_SYSERR;
	}
	/* Create a dummy Teledisk header */
	default_header(header);

	if (fwrite(header, 1, sizeof(header), self->tele_fp) < (int)sizeof(header))
	{
		fclose(self->tele_fp);
		return DSK_ERR_SYSERR;
	}
	/* Create an LDBS store */
	err = ldbs_new(&self->tele_super.ld_store, NULL, LDBS_DSK_TYPE);
	if (err)
	{
		fclose(self->tele_fp);
		return err;
	}
	fclose(self->tele_fp);
	self->tele_fp = NULL;
	self->tele_filename = dsk_malloc_string(filename);
	if (!self->tele_filename)
	{
		ldbs_close(&self->tele_super.ld_store);
		return DSK_ERR_NOMEM;
	}
	return ldbsdisk_attach(s);
}


/* Callback for gathering statistics on what data rate is used */
static dsk_err_t tele_datarate(PLDBS ldbs, dsk_pcyl_t cyl, dsk_phead_t head,
				LDBS_TRACKHEAD *th, void *param)
{
	TELE_DSK_DRIVER *self = param;
	size_t track_size = 0;
	int n;

	/* Calculate an approximate track size -- if data rate isn't
	 * specified we guess it based on how many bytes the track holds */
	for (n = 0; n < th->count; n++)
	{
		track_size += (128 << th->sector[n].id_psh);
	}

	if (th->recmode == 1) 	/* If FM, track size will be about half of */
	{			/* what it would be with MFM, so double it */
		++self->tele_fm;
		track_size *= 2;
	}
	else
	{
	      ++self->tele_mfm;
	}
	/* If rate is known, tally it */
	if (th->datarate > 0 && th->datarate < 4) 
	{
		++self->tele_rate[th->datarate - 1];
	}
	else	/* Guess data rate from number of bytes in track */
	{
		if (track_size <= 5632)       ++self->tele_rate[0];
		else if (track_size <= 11264) ++self->tele_rate[1];
		else			      ++self->tele_rate[2];
	}	
	return DSK_ERR_OK;
}

/* Compression code.
 *
 * This doesn't seem to produce TD0 files that are byte-for-byte identical
 * with those generated by the original Teledisk. But they pass validation
 * with TDCHECK and this driver handles them fine.
 * 
 * This checks if a repeating pattern of length 'patlen' begins here, and
 * returns the number of repetitions, 1-255.
 */
static int type2_repeats(tele_byte *src, size_t remaining, size_t patlen)
{
	int count = 0;
	int pos = patlen;	/* Start looking patlen bytes in */

	for (pos = patlen; pos + patlen <= remaining; pos += patlen)
	{
		if (memcmp(src, src + pos, patlen)) break;
		count++;
		if (count == 0xFE) break;	/* Can't have > 255 repeats */
	}
	return count + 1;	
}


/* Attempt to compress a sector using type 2 compression. Returns the 
 * compressed size (may well be bigger if compression overhead exceeds
 * bytes saved) */
static size_t type2_compress(tele_byte *buffer, tele_byte *dest, size_t len)
{
	tele_byte *src;	/* Next byte to inspect */
	int remaining;	/* Number of bytes left to inspect */
	int rep;	/* Number of repetitions starting at current byte */
	size_t dptr = 0;/* Write offset in destination buffer */
	int patlen;	/* Length of current compressed run */
	int repeats;	/* Indicator that a repeating pattern was found */
	unsigned char pl;/* log2(patlen) */
	int llen = 0;	/* Length of current literal block, 0 if none */
	int lptr = -1;	/* Start of current literal block, -1 if none */

	remaining = len;
	src = buffer;		

	while (remaining > 0)
	{
		repeats = 0;
		/* Limit compression to 2-byte patterns for now, for 
		 * compatibility with Teledisk 2.12. 2.16 images seem
		 * to be rare. */
		for (pl = 1, patlen = 2; patlen <= 2/*(len/2)*/; patlen *= 2, pl++)
		{
			/* Does a pattern start here and repeat at least 3 
			 * times? */
			rep = type2_repeats(src, remaining, patlen);
			if (rep > 3)
			{
				/* If so, write the pattern to the output */
				if (dest)
				{
					dest[dptr  ] = pl;
					dest[dptr+1] = rep;
					memcpy(dest + dptr + 2, src, patlen);
				}
				/* Skip over the number of bytes saved */
				src += rep * patlen;
				remaining -= rep * patlen;
				/* Indicate there is no current literal block */
				llen = 0;
				lptr = -1;
				/* Increase output pointer */
				dptr += 2 + patlen;	
				/* And indicate a repeating block was found */
				repeats = 1;
				break;
			}
		}
		/* No repeating block. Append a literal byte. */
		if (!repeats)
		{
			/* If there's no current literal block, start one */
			if (lptr < 0)
			{
				llen = 0;
				if (dest) dest[dptr] = 0;	/* Literal */
				++dptr;
				lptr = dptr;		/* Length of literal */
				++dptr;
				if (dest) dest[lptr] = 0;
			}
			/* Append literal byte to current block */
			if (dest) 
			{
				dest[lptr]++;
				dest[dptr] = *src;
			}
			++llen;
			/* Do not allow literal block to exceed 255 bytes */
			if (llen == 255) { llen = 0; lptr = -1; }
			++dptr;
			++src;
			--remaining;
		}
	}

	return dptr;	
}


/* Write an LDBS track out as a Teledisk track */
static dsk_err_t tele_write_track(PLDBS ldbs, dsk_pcyl_t cyl, dsk_phead_t head,
				LDBS_TRACKHEAD *th, void *param)
{
	TELE_DSK_DRIVER *self = param;
	unsigned char thead[4];
	unsigned char *secdata;
	dsk_err_t err;
	unsigned sec, crc;
	size_t buflen, complen;
	int n;

	/* Create the 4-byte track header */
	thead[0] = (unsigned char)(th->count);
	thead[1] = (unsigned char)(cyl);
	thead[2] = (unsigned char)(head);

	if (th->recmode == 1) thead[2] |= 0x80;	/* FM indicator */
	thead[3] = (unsigned char)(teledisk_crc(thead, 3));

	/* Write track header */
	if (fwrite(thead, 1, 4, self->tele_fp) < 4) return DSK_ERR_SYSERR;

	/* For each sector... */
	for (sec = 0; sec < th->count; sec++)
	{
		size_t seclen = 128 << th->sector[sec].id_psh;

		/* Allocate space for the sector (2 copies: compressed 
		 * and uncompressed) plus a 9-byte header */
		secdata = dsk_malloc(2*seclen + 9);	
		if (!secdata) return DSK_ERR_NOMEM;

		/* Blank the buffer with the sector's filler byte */
		memset(secdata, th->sector[sec].filler, 2 * seclen + 9);
		buflen = seclen; 
		/* Load the sector if it's present */
		if (th->sector[sec].blockid)
		{
			err = ldbs_getblock(ldbs, th->sector[sec].blockid, 
				NULL, secdata + 9, &buflen);
			if (err) return err;
		}
		/* Populate the sector header */
		secdata[0] = th->sector[sec].id_cyl;
		secdata[1] = th->sector[sec].id_head;
		secdata[2] = th->sector[sec].id_sec;
		secdata[3] = th->sector[sec].id_psh;
		secdata[4] = 0;
		secdata[5] = 0;
		/* Convert error indicators to syndrome byte */
		if (th->sector[sec].st2 & 0x20) secdata[4] |= 2; /* CRC error */
		if (th->sector[sec].st2 & 0x40) secdata[4] |= 4; /* Control mark */
		if (th->sector[sec].st1 & 0x04) secdata[4] |= 0x20; /* No data */
		if (th->sector[sec].st1 & 0x01) secdata[4] |= 0x40; /* Data, no ID */
		if (secdata[4] & 0x30)	/* Sector header only, no data */
		{
			secdata[5] = (unsigned char)(teledisk_crc(secdata, 5));
			if (fwrite(secdata, 1, 6, self->tele_fp) < 6) 
			{
				dsk_free(secdata);
				return DSK_ERR_SYSERR;
			}
			dsk_free(secdata);
			continue;
		}
		/* Need to write the full sector. */

		/* See if it can be stored as type 1 RLE */
		secdata[8] = 1;
		for (n = 2; n < (int)seclen; n += 2)
		{
			if (secdata[ 9 + n] != secdata[ 9] ||
		 	    secdata[10 + n] != secdata[10])
			{
				secdata[8] = 0;
				break;
			}
		}
		/* <http://www.classiccmp.org/dunfield/img54306/td0notes.txt>
		 * says that the CRC covers headers and data. But in my 
		 * tests it seems to cover just the sector body. */
		crc = teledisk_crc(secdata + 9, (unsigned short)seclen);

		/* Were we able to do a type 1 RLE? */
		if (secdata[8] == 1)
		{
			ldbs_poke2(secdata + 6, 5);
			secdata[8] = 1;	/* Single RLE */ 
			ldbs_poke2(secdata + 9, (unsigned short)(seclen / 2));
			/* The pattern is already present in secdata[11-12] */

			secdata[5] = crc;
			if (fwrite(secdata, 1, 13, self->tele_fp) < 13) 
			{
				dsk_free(secdata);
				return DSK_ERR_SYSERR;
			}
			dsk_free(secdata);
			continue;
		}
		/* Type 1 wasn't possible. See if type 2 compression will			 * have any effect. */
		complen = type2_compress(secdata + 9, NULL, seclen);	

		if (complen < seclen)	/* It will! */
		{
			/* Compress sector for real */
			type2_compress(secdata + 9, secdata + seclen + 9,
					seclen);
			/* Save compressed length in header (+1 for compression
			 * type) */
			ldbs_poke2(secdata + 6, (unsigned short)(complen + 1));
			secdata[8] = 2; /* Fully compressed */
			secdata[5] = crc;
			/* Copy compressed buffer to after header */
			memcpy(secdata + 9, secdata + seclen + 9, complen);
			if (fwrite(secdata, 1, complen + 9, self->tele_fp) < complen + 9)
			{
				dsk_free(secdata);
				return DSK_ERR_SYSERR;
			}	
		}
		else	/* Can't compress; save uncompressed */
		{	
			/* Sector size (+1 for compression type) */
			ldbs_poke2(secdata + 6, (unsigned short)(seclen + 1));
			secdata[8] = 0; /* Uncompressed */

			secdata[5] = crc;
			if (fwrite(secdata, 1, seclen + 9, self->tele_fp) < seclen + 9)
			{
				dsk_free(secdata);
				return DSK_ERR_SYSERR;
			}	
		}
		dsk_free(secdata);
	}
	return DSK_ERR_OK;
}



/* Convert LDBS blockstore to TD0 file */
dsk_err_t tele_close(DSK_DRIVER *s)
{
	dsk_err_t err;
	size_t len;
	unsigned char header[12];
	LDBS_STATS st;
	int n, rate, have_header;
	char *comment;
	unsigned crc;

	DC_CHECK(s);
	/* Firstly, ensure any pending changes are flushed to the LDBS 
	 * blockstore. Once this has been done we own the blockstore again 
	 * and have to close it after we've finished with it. */
	err = ldbsdisk_detach(s); 
	if (err)
	{
		dsk_free(self->tele_filename);
		ldbs_close(&self->tele_super.ld_store);
		return err;
	}

	/* If this disc image has not been written to, just close it and 
	 * dispose thereof. */
	if (!s->dr_dirty)
	{
		dsk_free(self->tele_filename);
		return ldbs_close(&self->tele_super.ld_store);
	}
	/* Trying to save changes but source is read-only */
	if (self->tele_super.ld_readonly)
	{
		dsk_free(self->tele_filename);
		ldbs_close(&self->tele_super.ld_store);
		return DSK_ERR_RDONLY;
	}
	dsk_report("Writing Teledisk file");
	/* OK, we've got a disk image we need to save. Did it have a 
	 * Teledisk header? */
	default_header(header);	
	have_header = 0;

	/* If the blockstore contains a Teledisk header, default the new 
	 * header to match that one. */
	memset(&self->tele_head, 0, sizeof(self->tele_head));
	len = sizeof(self->tele_head);
	err = ldbs_getblock_d(self->tele_super.ld_store, TELEDISK_USER_BLOCK,
		&self->tele_head, &len);
	/* There may be an overrun if the structure on-disk was padded out
	 * to larger than the structure in-memory */
	if ((err == DSK_ERR_OK || err == DSK_ERR_OVERRUN) && len != 0)
	{
		header[2] = self->tele_head.volume_seq;
		header[3] = self->tele_head.volume_id;
		header[4] = self->tele_head.ver;
		header[5] = self->tele_head.datarate;
		header[6] = self->tele_head.drivetype;
		header[7] = self->tele_head.doubletrack;
		header[8] = self->tele_head.dosmode;
		header[9] = self->tele_head.sides;
		have_header = 1;
	}

	err = ldbs_get_stats(self->tele_super.ld_store, &st);
	if (err)
	{
		dsk_free(self->tele_filename);
		ldbs_close(&self->tele_super.ld_store);
		dsk_report_end();
		return err;
	}

	header[4] = 21;	/* Always report Teledisk version as 21 (2.1x). This
			 * seems to be the most common version in the 
			 * TD0 files available to me. */

	/* LDBS stores data rate by track. TD0 stores it globally, so we
	 * have to analyse the blockstore, determine which data rate is most
	 * common, and store that. 
	 * We also analyse recording mode, but that can be stored by track,
	 * so only set it here if all tracks are FM */
	self->tele_fm  = 0;
	self->tele_mfm = 0;
	self->tele_rate[0] = 0;
	self->tele_rate[1] = 0;
	self->tele_rate[2] = 0;
	err = ldbs_all_tracks(self->tele_super.ld_store, tele_datarate, 
				SIDES_ALT, self);
	if (err)
	{
		dsk_free(self->tele_filename);
		ldbs_close(&self->tele_super.ld_store);
		dsk_report_end();
		return err;
	}
	/* Let rate be the most common data rate */
	rate = 0;
	for (n = 1; n < 3; n++)
	{
		if (self->tele_rate[n] > self->tele_rate[rate])
		{	
			rate = n;
		}
	}
	switch (rate)
	{
/* If the header says 300Kbps and the blockstore says 250Kbps, or vice 
 * versa, no need to change it */
		case 0: if (header[5] >= 2) header[5] = 0; break; /* SD / DD */
		case 1: header[5] = 2; break;	/* HD */
		case 2: header[5] = 3; break;	/* ED */
	}
/* If all tracks are FM and none MFM, set the global FM flag */
	if (self->tele_fm > 0 && self->tele_mfm == 0)
	{
		header[5] |= 0x80;
	}
	/* [1.5.6] Attempt to guess a reasonable value for header[6] (drive 
	 * type) based on the stats. I can't at the moment think of a useful
	 * way to populate header[7] (stepping) so leaver that as it is */
	if (!have_header)
	{
		/* Estimate track capacity, bytes */
		unsigned long max_tracksize = st.max_spt * st.max_sector_size;

		/* If FM recording, double it to get MFM capacity */
		if (header[5] & 0x80) max_tracksize *= 2;

		/* Up to 43 tracks, up to 5120 bytes / track. Assume 360k */
		if (st.max_cylinder < 44 && max_tracksize <= 5120)
		{
			header[6] = 1;	/* 5.25" 48tpi */
		}
		/* Otherwise if up to 5120 bytes assume 720k */
		else if (max_tracksize <= 5120)
		{
			header[6] = 3;	/* 3.5" 720k */
		}
		/* Otherwise if up to 8k assume 1.2M */
		else if (max_tracksize <= 8192)
		{
			header[6] = 2;	/* 5.25" 96tpi */
		}
		else	/* Everything else assumes 1.4M */
		{ 
			header[6] = 4;	/* 3.5" 1440k */
		}
		/* Other possible values: 
		 * 0 => 96tpi disk in 48tpi drive,
		 * 5 => 8" drive
		 * 6 => 3.5" drive (?ED drive)
		 */
	}

	err = ldbs_get_comment(self->tele_super.ld_store, &comment);	
	if (err)
	{
		dsk_free(self->tele_filename);
		ldbs_close(&self->tele_super.ld_store);
		dsk_report_end();
		return err;
	}
	if (comment) header[7] |= 0x80; else header[7] &= 0x7F;
	header[8] = 0;	/* All sectors are always included */

	/* Count of heads */
	if (st.max_head == st.min_head) header[9] = 1; else header[9] = 2;

	crc = teledisk_crc(header, 10);
	ldbs_poke2(header + 10, (unsigned short)crc);

	/* Write the header */
	self->tele_fp = fopen(self->tele_filename, "wb");
	if (!self->tele_fp)
	{
		ldbs_free(comment);
		dsk_free(self->tele_filename);
		ldbs_close(&self->tele_super.ld_store);
		dsk_report_end();
		return DSK_ERR_SYSERR;
	}
	if (fwrite(header, 1, 12, self->tele_fp) < 12)
	{
		fclose(self->tele_fp);
		ldbs_free(comment);
		dsk_free(self->tele_filename);
		ldbs_close(&self->tele_super.ld_store);
		dsk_report_end();
		return DSK_ERR_SYSERR;
	}
	/* Write the comment. When loading we included the comment timestamp
	 * in the textual comment, so reverse that procedure here. */
	if (comment)
	{
		time_t t;		/* To get current time */
		struct tm *ptm;		/* Ditto */
		size_t len;
		char *ucmt = comment;	/* UTF-8 comment */
		char *ccmt;		/* CP437 comment */
		int stamp[6];		/* Parsed timestamp */

		/* Initialise stamp[] */
		time (&t);
		ptm = localtime(&t);
		if (ptm)
		{
			stamp[0] = ptm->tm_year;
			stamp[1] = ptm->tm_mon;
			stamp[2] = ptm->tm_mday;
			stamp[3] = ptm->tm_hour;
			stamp[4] = ptm->tm_min;
			stamp[5] = ptm->tm_sec;
		}
		else
		{
			memset(stamp, 0, sizeof(stamp));
		}

		/* If comment begins with a timestamp, parse it into stamp,
		 * overwriting current date. */
		if (sscanf(ucmt, "[%d-%d-%dT%d:%d:%d]", &stamp[0], &stamp[1],
			&stamp[2], &stamp[3], &stamp[4], &stamp[5]) == 6)
		{
			stamp[0] -= 1900;	/* Year relative to 1900 */
			--stamp[1];		/* Month 0-based */

			/* And point ucmt at just after the timestamp */
			ucmt = strchr(ucmt, ']');
			if (ucmt)
			{
				++ucmt;
				while (*ucmt == ' ') ++ucmt;
			}
			else ucmt = comment;
		}
		/* Convert to CP437: Get space required */ 
 		len = utf8_to_cp437(ucmt, NULL, -1);
 		ccmt = dsk_malloc(len + 10);

		/* malloc space for CP437 comment */	
		if (!ccmt)
		{
			fclose(self->tele_fp);
			ldbs_free(comment);
			dsk_free(self->tele_filename);
			ldbs_close(&self->tele_super.ld_store);
			dsk_report_end();
			return DSK_ERR_NOMEM;
		}
		/* Convert comment */
		utf8_to_cp437(ucmt, ccmt + 10, -1);
		ldbs_free(comment);
		/* Convert CR/LF into zeroes */
		for (n = 0; n < (int)len; n++)
		{
			if (ccmt[n+10] == '\n' || ccmt[n+10] == '\r') 
				ccmt[n+10] = 0;
		}
		/* Store comment length & timestamp */
		ldbs_poke2((tele_byte *)ccmt + 2, (unsigned short)(len - 1));
		for (n = 0; n < 6; n++) ccmt[n+4] = stamp[n];	
		
		/* Checksum 8 bytes of header, and len bytes of data 
		 * less one terminating nul */
		crc = teledisk_crc((tele_byte *)ccmt + 2, (unsigned short)(len + 7));
		ldbs_poke2((tele_byte *)ccmt, (unsigned short)crc);

		/* Write the comment */	
		if (fwrite(ccmt, 1, len + 9, self->tele_fp) < len + 9)
		{
			fclose(self->tele_fp);
			dsk_free(ccmt);
			dsk_free(self->tele_filename);
			ldbs_close(&self->tele_super.ld_store);
			dsk_report_end();
			return DSK_ERR_SYSERR;
		}
		dsk_free(ccmt);
	}
	/* Now ready to write out the tracks */

	err = ldbs_all_tracks(self->tele_super.ld_store, tele_write_track, 
				SIDES_ALT, self);

	/* Write the last track header [EOF] */
	header[0] = header[1] = header[2] = 0xFF;
	header[3] = (unsigned char)(teledisk_crc(header, 3));
	if (!err && fwrite(header, 1, 4, self->tele_fp) < 4)
	{
		err = DSK_ERR_SYSERR;
	}
	if (!err)
	{
		if (fclose(self->tele_fp)) err = DSK_ERR_SYSERR;
	}
	else	fclose(self->tele_fp);
	if (self->tele_filename)
	{
		dsk_free(self->tele_filename);
		self->tele_filename = NULL;
	}	
	dsk_report_end();
	return ldbs_close(&self->tele_super.ld_store);
}



