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


/* Declarations for the JV3 driver */

/* JV3 spec at <http://www.tim-mann.org/trs80/dskspec.html> */

#include "drvi.h"
#include "drvldbs.h"
#include "drvjv3.h"


DRV_CLASS dc_jv3 = 
{
	sizeof(JV3_DSK_DRIVER),
	&dc_ldbsdisk,		/* superclass */
	"jv3\0JV3\0",
	"JV3 file driver",
	jv3_open,
	jv3_creat,
	jv3_close,
};

/* #define MONITOR(x) printf x     */

#define MONITOR(x) 


#define DC_CHECK(s) \
	JV3_DSK_DRIVER *self; \
	if (s->dr_class != &dc_jv3) return DSK_ERR_BADPTR; \
	self = (JV3_DSK_DRIVER *)s;


/* Decode the 2-bit size code as a sector size. Annoyingly the encoding
 * is different for used and free sectors */
static size_t decode_size(int isfree, unsigned char size)
{
	switch (size & JV3_SIZE)
	{
		case 0: return (isfree) ?  512 :  256; 
		case 1: return (isfree) ? 1024 :  128; 	
		case 2: return (isfree) ?  128 : 1024; 	
		case 3: return (isfree) ?  256 :  512; 	
	}
	/* Can't happen */
	return 256;
}


/* Encode the sector size as a 2-bit size code. Annoyingly the encoding
 * is different for used and free sectors */
unsigned char encode_size(int isfree, size_t size)
{
	switch (size)
	{
		case  128: return (isfree) ? 2 : 1;
		default:
		case  256: return (isfree) ? 3 : 0;
		case  512: return (isfree) ? 0 : 3;
		case 1024: return (isfree) ? 1 : 2;
	}
}









dsk_err_t jv3_add_sector(JV3_DSK_DRIVER *self, dsk_pcyl_t cylinder,
				dsk_psect_t sector, unsigned char flags,
				unsigned char *secbuf, size_t seclen)
{
	int allsame = secbuf[0];
	size_t n;
	int nsec;
	LDBLOCKID blkid = LDBLOCKID_NULL;
	char type[4];
	dsk_err_t err;
	LDBS_TRACKHEAD *trkh;

	/* See if all the sector bytes are the same */
	dsk_phead_t head = (flags & JV3_SIDE) ? 1 : 0;
	for (n = 1; n < seclen; n++)
	{
		if (secbuf[n] != secbuf[0]) { allsame = -1; break; }
	}
	if (allsame == -1)	/* Sector must be written */
	{
		ldbs_encode_secid(type, cylinder, head, sector);
		err = ldbs_putblock(self->jv3_super.ld_store, &blkid, type,
			secbuf, seclen);
		if (err) return err;
	}
	/* Sector saved. Now add it to track header */
	err = ldbs_get_trackhead(self->jv3_super.ld_store, &trkh, 
				cylinder, head);

	/* If track header exists add one more sector to it */
	if (trkh)
	{
		nsec = trkh->count;
		trkh = ldbs_trackhead_realloc(trkh, (unsigned short)(1 + trkh->count));
		if (!trkh) return DSK_ERR_NOMEM;
	}
	/* Otherwise create it */
	else
	{
		nsec = 0;
		trkh = ldbs_trackhead_alloc(1);
		if (!trkh) return DSK_ERR_NOMEM;
		trkh->datarate = 1;
		trkh->recmode  = (flags & JV3_DENSITY) ? 2 : 1;
		trkh->filler = 0xE5;
	}
	if (trkh->count < 9) 		trkh->gap3 = 0x50;
	else if (trkh->count < 10)	trkh->gap3 = 0x52;
	else				trkh->gap3 = 0x17;

	trkh->sector[nsec].id_cyl  = cylinder;
	trkh->sector[nsec].id_head = head;
	trkh->sector[nsec].id_sec  = sector;
	trkh->sector[nsec].id_psh  = dsk_get_psh(seclen);
	if (flags & JV3_DAM)   trkh->sector[nsec].st2 |= 0x40;
	if (flags & JV3_ERROR) trkh->sector[nsec].st2 |= 0x20;
	if (allsame == -1)
	{
		trkh->sector[nsec].copies = 1;
		trkh->sector[nsec].filler = 0xE5;
		trkh->sector[nsec].blockid = blkid;	
	}
	else
	{
		trkh->sector[nsec].copies = 0;
		trkh->sector[nsec].filler = allsame;
		trkh->sector[nsec].blockid = LDBLOCKID_NULL;
	}
	err = ldbs_put_trackhead(self->jv3_super.ld_store, trkh, 
				cylinder, head);
	dsk_free(trkh);
	return err;
}


dsk_err_t jv3_open(DSK_DRIVER *s, const char *filename)
{
	FILE *fp;
	dsk_err_t err;
	unsigned n, r;
	unsigned char secbuf[1024];
	DC_CHECK(s);

	fp = fopen(filename, "r+b");
	if (!fp)
	{
		self->jv3_super.ld_readonly = 1;	
		fp = fopen(filename, "rb");

	}
	if (!fp) return DSK_ERR_NOTME;

	if (fread(self->jv3_header, 1, sizeof(self->jv3_header), fp)
		< (int)sizeof(self->jv3_header))
	{
		fclose(fp);
		return DSK_ERR_NOTME;
	}
	/* OK, the header loaded. There's no metadata or magic number
	 * we can check, so just assume this file is in JV3 format and 
	 * load the tracks. */
	err = ldbs_new(&self->jv3_super.ld_store, NULL, LDBS_DSK_TYPE);
	if (err) return err;

	/* Check disc image read-only flag */
	if (self->jv3_header[JV3_HEADER_LEN - 1] == 0)
	{
		self->jv3_super.ld_readonly = 1;	
	}

	do
	{
		for (n = 0; n < JV3_HEADER_COUNT; n++)
		{
			unsigned char cyl   = self->jv3_header[n*3];
			unsigned char sec   = self->jv3_header[n*3+1];
			unsigned char flags = self->jv3_header[n*3+2];
			size_t secsize = 0;
			int free = 0;

			if (cyl == 0xFF && sec == 0xFF && 
				((flags & JV3_FREEF) == JV3_FREEF))
			{
				free = 1;
			}
			secsize = decode_size(free, flags);
			r = fread(secbuf, 1, secsize, fp);
			if (r != 0 && r < secsize)
			{
				ldbs_close(&self->jv3_super.ld_store); 
				return DSK_ERR_SYSERR;
			}
			if (r == 0) break;	/* End of file */
			/* If this is not a free sector, add it */
			if (!free)
			{
				err = jv3_add_sector(self, cyl, sec, flags, secbuf, secsize);
				if (err)
				{
					ldbs_close(&self->jv3_super.ld_store); 
					return err;
				}
			}
		}
		r = fread(self->jv3_header, 1, sizeof(self->jv3_header), fp);
		if (r != 0 && r < (int)sizeof(self->jv3_header))
		{
			ldbs_close(&self->jv3_super.ld_store); 
			return DSK_ERR_SYSERR;
		}
	}
	while (!feof(fp));

	self->jv3_filename = dsk_malloc_string(filename);
	return ldbsdisk_attach(s);
}

dsk_err_t jv3_creat(DSK_DRIVER *s, const char *filename)
{
	FILE *fp;
	dsk_err_t err;
	DC_CHECK(s);

	fp = fopen(filename, "wb");
	if (!fp)
	{
		return DSK_ERR_SYSERR;
	}
	memset(self->jv3_header, 0xFF, JV3_HEADER_LEN);
	if (fwrite(self->jv3_header, 1, JV3_HEADER_LEN, fp) < JV3_HEADER_LEN)
	{
		return DSK_ERR_SYSERR;
	}
	fclose(fp);

	err = ldbs_new(&self->jv3_super.ld_store, NULL, LDBS_DSK_TYPE);
	if (err) return err;

	self->jv3_filename = dsk_malloc_string(filename);
	return DSK_ERR_OK;

}


static dsk_err_t flush_header(JV3_DSK_DRIVER *self, long *newheadpos)
{
	long pos = ftell(self->jv3_fp);

	if (pos < 0) return DSK_ERR_SYSERR;

	if (fseek(self->jv3_fp, self->jv3_headpos, SEEK_SET)) 
		return DSK_ERR_SYSERR;
	if (fwrite(self->jv3_header, 1, JV3_HEADER_LEN, self->jv3_fp) < JV3_HEADER_LEN)
		return DSK_ERR_SYSERR;
	if (fseek(self->jv3_fp, pos, SEEK_SET)) 
		return DSK_ERR_SYSERR;
	memset(self->jv3_header, 0xFF, JV3_HEADER_LEN);
	*newheadpos = pos;

	return DSK_ERR_OK;	
}


static dsk_err_t write_sector(PLDBS ldbs, dsk_pcyl_t cyl, dsk_phead_t head,
	LDBS_SECTOR_ENTRY *se, LDBS_TRACKHEAD *th, void *param)
{
	JV3_DSK_DRIVER *self = param;
	dsk_err_t err;
	unsigned char flags;
	unsigned char secbuf[1024];
	size_t seclen;

	/* Header full? */
	if (self->jv3_sector >= JV3_HEADER_COUNT)
	{
		err = flush_header(self, &self->jv3_headpos);
		if (err) return err;
		/* Write a placeholder header for the next zone */
		if (fwrite(self->jv3_header, 1, JV3_HEADER_LEN, self->jv3_fp) < JV3_HEADER_LEN)
			return DSK_ERR_SYSERR;
		self->jv3_sector = 0;
	}
	flags = encode_size(0, seclen = (128 << se->id_psh));
	if (th->recmode != 1) flags |= JV3_DENSITY;
	if (se->st2 & 0x20) flags |= JV3_ERROR;
	if (se->st2 & 0x40) flags |= JV3_DAM;
	if (head)           flags |= JV3_SIDE;

	self->jv3_header[3 * self->jv3_sector    ] = cyl;
	self->jv3_header[3 * self->jv3_sector + 1] = se->id_sec;
	self->jv3_header[3 * self->jv3_sector + 2] = flags;
	if (se->copies)
	{
		size_t buflen = seclen;

		err = ldbs_getblock(ldbs, se->blockid, NULL, secbuf, &buflen);
		if (err && err != DSK_ERR_OVERRUN) return err;
	}
	else
	{
		memset(secbuf, se->filler, seclen);
	}
	if (fwrite(secbuf, 1, seclen, self->jv3_fp) < seclen)
		return DSK_ERR_SYSERR;
	++self->jv3_sector;
	return DSK_ERR_OK;
}



dsk_err_t jv3_close(DSK_DRIVER *s)
{
	dsk_err_t err;
	DC_CHECK(s);

	/* Detach the blockstore */
	err = ldbsdisk_detach(s);
	if (err)
	{
		dsk_free(self->jv3_filename);
		ldbs_close(&self->jv3_super.ld_store);
		return err;	
	}
	/* If image not touched, close */
	if (!s->dr_dirty)
	{
		dsk_free(self->jv3_filename);
		return ldbs_close(&self->jv3_super.ld_store);
	}
	/* If read-only, abandon */
	if (self->jv3_super.ld_readonly)
	{
		dsk_free(self->jv3_filename);
		ldbs_close(&self->jv3_super.ld_store);
		return DSK_ERR_RDONLY;	
	}
	dsk_report("Writing JV3 file");
	/* Initialise the header */
	memset(self->jv3_header, 0xFF, JV3_HEADER_LEN);
	self->jv3_fp = fopen(self->jv3_filename, "wb");
	dsk_free(self->jv3_filename);
	if (!self->jv3_fp)
	{
		ldbs_close(&self->jv3_super.ld_store);
		dsk_report_end();
		return DSK_ERR_SYSERR;
	}
	if (fwrite(self->jv3_header, 1, JV3_HEADER_LEN, self->jv3_fp) < JV3_HEADER_LEN)
	{
		ldbs_close(&self->jv3_super.ld_store);
		dsk_report_end();
		return DSK_ERR_SYSERR;
	}
	/* Output file opened. */
	self->jv3_sector = 0;
	self->jv3_headpos = 0;

	err = ldbs_all_sectors(self->jv3_super.ld_store, write_sector, 
		SIDES_OUTOUT, self);
	/* Flush the last header */
	if (!err && self->jv3_sector)
	{
		err = flush_header(self, &self->jv3_headpos);
	}
	if (err) fclose(self->jv3_fp);
	else
	{
		if (fclose(self->jv3_fp)) err = DSK_ERR_SYSERR;
	}
	if (err) 
	{
		ldbs_close(&self->jv3_super.ld_store);
		dsk_report_end();
		return err;
	}
	dsk_report_end();
	return ldbs_close(&self->jv3_super.ld_store);
}




