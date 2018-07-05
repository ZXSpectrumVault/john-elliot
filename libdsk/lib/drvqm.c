/***************************************************************************
 *                                                                         *
 *    LIBDSK: General floppy and diskimage access library                  *
 *    Copyright (C) 2001-2,2005,2017  John Elliott <seasip.webmaster@gmail.com>*
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
/*
 *  This driver is for CopyQM images
 *  Written by Per Ola Ingvarsson.
 *  Thanks to Roger Plant for the expandqm program which has helped a lot.
 *  Currently it is read only
 */
/*
 *  Release 2011-04-17
 *  This is an rewritten enhanced QM driver with write-back capabilty.
 *  The image is loaded into memory in drv_open() and written
 *  back (if changed) in drv_close().
 *  Written by Ralf-Peter Nerlich <early8bitz@arcor.de>.
 *  From an idea by Matt Knoth <www.knothusa.net>.
 *  Please read the CHANGELOG.DRVQM for information.
 *  Warning! Code is highly experimental and not tested by any
 *  other person.
 *  !! Use at your own risk !!
 *  !! Don't use in production environment !!
 * 
 *  [JCE 21-2-2017] Rewritten to load the image into an LDBS object rather 
 *  than memory. All caveats about untestedness now apply with even more force.
 */
#include <stdio.h>
#include <string.h>
#include "libdsk.h"
#include "drvi.h"
#include "drvldbs.h"
#include "drvqm.h"
#include "crctable.h"
#ifdef HAVE_TIME_H
#include <time.h>
#endif

/* #define DRV_QM_DEBUG */
#undef DRV_QM_DEBUG 

/* User block recording the original header */
#define QM_USER_BLOCK "ucqm"

#define MAKE_CHECK_SELF QM_DSK_DRIVER* qm_self;\
	                    if ( self->dr_class != &dc_qm ) return DSK_ERR_BADPTR;\
	                    qm_self = (QM_DSK_DRIVER*)self;

dsk_err_t drv_qm_open(DSK_DRIVER * self, const char *filename);
dsk_err_t drv_qm_create(DSK_DRIVER * self, const char *filename);
dsk_err_t drv_qm_close(DSK_DRIVER * self);
dsk_err_t drv_qm_read(DSK_DRIVER * self, const DSK_GEOMETRY * geom,
		      void *buf, dsk_pcyl_t cylinder, dsk_phead_t head, dsk_psect_t sector);
dsk_err_t drv_qm_write(DSK_DRIVER * self, const DSK_GEOMETRY * geom,
		       const void *buf, dsk_pcyl_t cylinder, dsk_phead_t head, dsk_psect_t sector);
dsk_err_t drv_qm_format(DSK_DRIVER * self, DSK_GEOMETRY * geom,
			dsk_pcyl_t cylinder, dsk_phead_t head, const DSK_FORMAT * format,
			unsigned char filler);
dsk_err_t drv_qm_xseek(DSK_DRIVER * self, const DSK_GEOMETRY * geom, dsk_pcyl_t cylinder,
		       dsk_phead_t head);
dsk_err_t drv_qm_status(DSK_DRIVER * self, const DSK_GEOMETRY * geom, dsk_phead_t head,
			unsigned char *result);
dsk_err_t drv_qm_getgeom(DSK_DRIVER * self, DSK_GEOMETRY * geom);

dsk_err_t drv_qm_secid(DSK_DRIVER * self, const DSK_GEOMETRY * geom,
		       dsk_pcyl_t cylinder, dsk_phead_t head, DSK_FORMAT * result);

/* This struct contains function pointers to the driver's functions, and the
 * size of its DSK_DRIVER subclass
 */
DRV_CLASS dc_qm = {
	sizeof(QM_DSK_DRIVER),
	&dc_ldbsdisk,		/* superclass */
	"copyqm\0qm\0",
	"CopyQM file driver",
	drv_qm_open,					   /* open */
	drv_qm_create,					   /* create new */
	drv_qm_close,					   /* close */
};

/************************************************
 * misc. internal functions                     *
 ************************************************/
static int get_i16(unsigned char *buf, int pos)
{
	unsigned char low_byte;
	unsigned char high_byte;
	int outInt;

	low_byte = buf[pos];
	high_byte = buf[++pos];

	/* Signed, eh. Lets see. */
	outInt = 0;
	if(high_byte & 0x80)				   /* then negative */
	/* Set all to ones except for the lower 16 */
	/* Should work if sizeof(int) >= 16 */
	outInt = (-1) << 16;
	return (outInt |= (high_byte << 8) | low_byte);
}

static unsigned int get_u16(unsigned char *buf, int pos)
{
	return ((unsigned int) get_i16(buf, pos)) & 0xffff;
}

static void put_u16(unsigned char *buf, int pos, unsigned int val)
{
	buf[pos] = (unsigned char) val;
	buf[++pos] = (unsigned char) (val >> 8);
}

static unsigned long get_u32(unsigned char *buf, int pos)
{
	int i;
	unsigned long ret_val = 0;
	for(i = 3; i >= 0; --i)
	{
	ret_val <<= 8;
	ret_val |= ((unsigned long) buf[pos + i] & 0xff);
	}
	return ret_val;
}

static void drv_qm_update_crc(unsigned long *crc, unsigned char byte)
{
	/* Note that there is a bug in the CopyQM CRC calculation  */
	/* When indexing in this table, they shift the crc ^ data  */
	/* 2 bits up to address longwords, but they do that in an  */
	/* eight bit register, so that the top 2 bits are lost,    */
	/* thus the anding with 0x3f                               */
	*crc = crc32r_table[(byte ^ (unsigned char) *crc) & 0x3f] ^ (*crc >> 8);
}


/************************************************
 * read the QM header                           *
 * used by drv_qm_open                          *
 ************************************************/
static dsk_err_t drv_qm_load_header(QM_DSK_DRIVER * qm_self, unsigned char *header)
{
	int i;

	/* Check the header checksum */
	unsigned char chksum = 0;
	for(i = QM_H_BASE; i < QM_HEADER_SIZE; i++)
	chksum += header[i];

	if(chksum)						   /* should be 0x00 */
	{
#ifdef DRV_QM_DEBUG
		fprintf(stderr, "qm: header checksum error\n");
#endif
	return DSK_ERR_NOTME;
	} 
	else
	{
#ifdef DRV_QM_DEBUG
		fprintf(stderr, "qm: header checksum is ok!\n");
#endif
	}
	if(header[QM_H_BASE] != 'C' || header[QM_H_BASE + 1] != 'Q')
	{
#ifdef DRV_QM_DEBUG
		fprintf(stderr, "qm: First two chars are %c%c\n", header[QM_H_BASE], header[QM_H_BASE + 1]);
#endif
		return DSK_ERR_NOTME;
	}
	/* Parse the BPB at offset 3 */
	qm_self->qm_h_bpb_sector_size = get_u16(header, QM_H_SECSIZE);
	qm_self->qm_h_bpb_secclus     = header[QM_H_SECCLUS];
	qm_self->qm_h_bpb_reserved    = get_u16(header, QM_H_RESERVED);
	qm_self->qm_h_bpb_fats        = get_u16(header, QM_H_FATS);
	qm_self->qm_h_bpb_rootentries = get_u16(header, QM_H_ROOTENTRIES);
	qm_self->qm_h_bpb_total_sectors = get_u16(header, QM_H_SECTOTL);
	qm_self->qm_h_bpb_media_id    = header[QM_H_MEDIA_ID];
	qm_self->qm_h_bpb_secfat      = get_u16(header, QM_H_SECFAT);
	qm_self->qm_h_bpb_sectrack    = get_u16(header, QM_H_SECPTRK);
	qm_self->qm_h_bpb_heads       = get_u16(header, QM_H_HEADS);
	qm_self->qm_h_bpb_hidden      = get_u16(header, QM_H_HIDDEN);

	/* Blind or not */
	qm_self->qm_h_blind = header[QM_H_BLIND];
	/* Density - 0 is DD, 1 means HD */
	qm_self->qm_h_density = header[QM_H_DENS];
	/* Number of used tracks */
	qm_self->qm_h_used_cyls = header[QM_H_USED_CYL];
	/* Number of total tracks */
	qm_self->qm_h_total_cyls = header[QM_H_TOTL_CYL];
	/* CRC 0x5c - 0x5f */
	qm_self->qm_h_crc = get_u32(header, QM_H_DATA_CRC);
	/* Length of comment */
	qm_self->qm_h_comment_len = get_u16(header, QM_H_CMT_SIZE);
	/* 0x71 is first sector number - 1 */
	qm_self->qm_h_secbase = header[QM_H_SECBASE];
	/* 0x74 is interleave, I think. Normally 1, but 0 for old copyqm */
	qm_self->qm_h_interleave = header[QM_H_INTLV];
	/* 0x75 is skew. Normally 0. Negative number for alternating sides */
	qm_self->qm_h_skew = header[QM_H_SKEW];
	/* 0x76 is the source drive type */
	qm_self->qm_h_drive = header[QM_H_DRIVE];

#ifdef DRV_QM_DEBUG
	fprintf(stderr, "qm: sector size is %ld\n", qm_self->qm_h_bpb_sector_size);
	fprintf(stderr, "qm: sectors / cluster %d\n", qm_self->qm_h_bpb_secclus);
	fprintf(stderr, "qm: reserved sectors %d\n", qm_self->qm_h_bpb_reserved);
	fprintf(stderr, "qm: FAT copies %d\n", qm_self->qm_h_bpb_fats);
	fprintf(stderr, "qm: root dir entries %d\n", qm_self->qm_h_bpb_rootentries);
	fprintf(stderr, "qm: total sectors %lu\n", qm_self->qm_h_bpb_total_sectors);
	fprintf(stderr, "qm: media ID 0x%02x\n", qm_self->qm_h_bpb_media_id);
	fprintf(stderr, "qm: sectors / FAT %d\n", qm_self->qm_h_bpb_secfat);
	fprintf(stderr, "qm: nbr sectors / track %u\n", qm_self->qm_h_bpb_sectrack);
	fprintf(stderr, "qm: nbr heads %d\n", qm_self->qm_h_bpb_heads);
	fprintf(stderr, "qm: hidden sectors %d\n", qm_self->qm_h_bpb_hidden);
	fprintf(stderr, "qm: secbase %u (%u in image)\n", (unsigned char)(qm_self->qm_h_secbase) + 1,
	    qm_self->qm_h_secbase);
	fprintf(stderr, "qm: density %d\n", qm_self->qm_h_density);
	fprintf(stderr, "qm: used cylinders %d\n", qm_self->qm_h_used_cyls);
	fprintf(stderr, "qm: total cylinders %d\n", qm_self->qm_h_total_cyls);
	fprintf(stderr, "qm: CRC 0x%08lx\n", qm_self->qm_h_crc);
	fprintf(stderr, "qm: interleave %d\n", qm_self->qm_h_interleave);
	fprintf(stderr, "qm: skew %d\n", qm_self->qm_h_skew);
	fprintf(stderr, "qm: drive type %d\n", qm_self->qm_h_drive);
	fprintf(stderr, "qm: description \"%s\"\n", header + QM_H_DESCR);
#endif

	/* Fix the interleave value for old versions of CopyQM */
	if(qm_self->qm_h_interleave == 0) 
		qm_self->qm_h_interleave = 1;

	return DSK_ERR_OK;
}

typedef struct rlestate
{
	FILE *fp;
	signed short len;
	unsigned char rep;	
} RLESTATE;

static dsk_err_t drv_qm_next_byte(unsigned char *c, RLESTATE *rs)
{
	int nextc = 0;
	unsigned char buf[2];

	/* Start of next block */
	if (rs->len == 0)
	{
		if (fread(buf, 1, 2, rs->fp) < 2) return DSK_ERR_NOTME;
		rs->len = get_i16(buf, 0);
		if (rs->len < 0)	/* Repeated run */
		{
			nextc = fgetc(rs->fp);
			if (nextc == EOF) return DSK_ERR_NOTME;
		}
		rs->rep = nextc;
	}
	/* Literal run */
	if (rs->len > 0)
	{
		nextc = fgetc(rs->fp);
		if (nextc == EOF) return DSK_ERR_NOTME;
		*c = nextc;
		--rs->len;
		return DSK_ERR_OK;	
	}
	/* Repeating run */
	if (rs->len < 0)
	{
		*c = rs->rep;
		++rs->len;
		return DSK_ERR_OK;
	}
	/* Should never get here */
	*c = 0xF6;
	return DSK_ERR_OK;
}

/************************************************
 * read run length coded data                   *
 * used by drv_qm_open                          *
 ************************************************/
static dsk_err_t drv_qm_load_image(QM_DSK_DRIVER * qm_self, FILE * fp)
{
	int n;
	dsk_err_t errcond = DSK_ERR_OK;
	dsk_pcyl_t cyl;
	dsk_phead_t head;
	dsk_psect_t sec;
	size_t seclen;
	unsigned char *secbuf;
	LDBS_TRACKHEAD *trkh;
	RLESTATE rle_state;

	/* Set the position after the header and comment */
	if(fseek(fp, QM_HEADER_SIZE + qm_self->qm_h_comment_len, SEEK_SET))
		return DSK_ERR_NOTME;

	/* Allocate a buffer for the current sector */
	seclen = qm_self->qm_h_bpb_sector_size;
	secbuf = dsk_malloc(seclen);
	if (!secbuf) return DSK_ERR_NOMEM;

	/* Create a LibDsk blockstore */
	errcond = ldbs_new(&qm_self->qm_super.ld_store, NULL, LDBS_DSK_TYPE);
	if (errcond) 
	{
		dsk_free(secbuf);
		return errcond;
	}
	rle_state.fp = fp;
	rle_state.len = 0;
	rle_state.rep = 0;

	/* XXX This stores sectors in the order read. The CopyQM format 
	 * is aware of interleave so we should use that to order the 
	 * sectors */

	/* Start reading. CopyQM, unlike Teledisk, doesn't allow tracks to
	 * have a different geometry from the disk header, so we know
	 * all sectors will be the same size */
	for (cyl = 0; cyl < qm_self->qm_h_total_cyls; cyl++)
	{
		for (head = 0; head < qm_self->qm_h_bpb_heads; head++)
		{
			trkh = ldbs_trackhead_alloc
					(qm_self->qm_h_bpb_sectrack);
			if (!trkh)
			{
				ldbs_close(&qm_self->qm_super.ld_store);
				dsk_free(secbuf);
				return DSK_ERR_NOMEM;
			}
			switch (qm_self->qm_h_density)
			{
				case QM_DENS_DD: trkh->datarate = 1; break;
				case QM_DENS_HD: trkh->datarate = 2; break;
				case QM_DENS_ED: trkh->datarate = 3; break;
			}
			trkh->recmode = 2;	/* CopyQM doesn't do FM */
			trkh->filler = 0xF6;
/* CopyQM files don't hold GAP3 -- take a wild guess. */
			if (trkh->count < 9)            trkh->gap3 = 0x50;
			else if (trkh->count < 10)      trkh->gap3 = 0x52;
			else                            trkh->gap3 = 0x17;

			for (sec = 0; sec < trkh->count; sec++)
			{
				trkh->sector[sec].id_cyl  = cyl;
				trkh->sector[sec].id_head = head;
				trkh->sector[sec].id_sec  = sec + 1 + 
					qm_self->qm_h_secbase;
				trkh->sector[sec].id_psh = dsk_get_psh(seclen);
				/* Load sector data */
				for (n = 0; n < (int)seclen; n++)
				{
					errcond = drv_qm_next_byte(&secbuf[n], 
							&rle_state);
					if (errcond)
					{
						ldbs_free(trkh);
						ldbs_close(&qm_self->qm_super.ld_store);
						dsk_free(secbuf);
						return errcond;
					}
				}
/* Check for all bytes being the same */
				trkh->sector[sec].copies = 0;
				for (n = 1; n < (int)seclen; n++)
				{
					if (secbuf[n] != secbuf[0]) 
					{
						trkh->sector[sec].copies = 1;
						break;	
					}
				}
/* Update CRC */
				for (n = 0; n < (int)seclen; n++)
				{
					drv_qm_update_crc(&qm_self->qm_calc_crc,
						secbuf[n]);
				}
				if (trkh->sector[sec].copies)
				{
					char sector_id[4];
					ldbs_encode_secid(sector_id, 
						cyl, head, 
						trkh->sector[sec].id_sec);
					trkh->sector[sec].filler = 0xF6;
					errcond = ldbs_putblock(
						qm_self->qm_super.ld_store,
						&trkh->sector[sec].blockid,
						sector_id, secbuf, seclen);		
					if (errcond)
					{
						ldbs_free(trkh);
						ldbs_close(&qm_self->qm_super.ld_store);
						dsk_free(secbuf);
						return errcond;
					}
				}
				else
				{
					trkh->sector[sec].filler = secbuf[0];
				}
			}
			/* All sectors written */
			errcond = ldbs_put_trackhead(
					qm_self->qm_super.ld_store, trkh,
					cyl, head);
			ldbs_free(trkh);
			if (errcond)
			{
				ldbs_close(&qm_self->qm_super.ld_store);
				dsk_free(secbuf);
				return errcond;
			}
		}
	}
	dsk_free(secbuf);	
#ifdef DRV_QM_DEBUG
	fprintf(stderr, "drv_qm_load_image - crc from header = 0x%08lx, "
	    "calc = 0x%08lx\n", qm_self->qm_h_crc, qm_self->qm_calc_crc);
#endif
	/* Compare the CRCs */
	/* The CRC is zero on old images so it cannot be checked then */
	if(qm_self->qm_h_crc)
	{
		if(qm_self->qm_h_crc != qm_self->qm_calc_crc)
		{
			ldbs_close(&qm_self->qm_super.ld_store);
			return DSK_ERR_CORRUPT;
		}
	}
	return errcond;
}

/************************************************
 * write run length coded data                  *
 * used by drv_qm_close                         *
 ************************************************/
static int drv_qm_write_rl(int rl, FILE * fp)
{
	unsigned char rlbuf[2];

	put_u16(rlbuf, 0, (unsigned int) rl);
	return (1 == fwrite(rlbuf, 2, 1, fp));		   /* TRUE if succesful */
}

/* Write RL coded data block */
static dsk_err_t drv_qm_dump_compressed(FILE * fp, unsigned long *pcrc, 
					unsigned char *rd_ptr, size_t size)
{
	unsigned char *p;
	unsigned char a;
	int i, l, len;

	for(i = 0; i < (int)size; i++)
	{
		drv_qm_update_crc(pcrc, rd_ptr[i]);   /* warming up cache */
	}
	for(p = rd_ptr, i = 0, l = 0, len = size - 4; l < len;)
	{
		a = p[i];   /* equals break even after 3, minimum 4 required */
			    /* [JCE] CopyQM actually uses minimum 5, because */
			    /* of the 2-byte overhead of starting a new block */
		if((a == p[i + 1]) && (a == p[i + 2]) && (a == p[i + 3] &&
		    a == p[i + 4]))
		{
			if(i)	   /* flush out previous non-equals */
	    		{
				if(!drv_qm_write_rl(i, fp)) /* positive length */
				{
					return DSK_ERR_SYSERR;
				}
				if(1 != fwrite(p, i, 1, fp))   /* runlen unencoded data */
				{
					return DSK_ERR_SYSERR;
				}
				p += i;
	    		}
			for(a = *p, i = 0; (l < (int)size) && (a == *p);)   /* find true length of equals */
	    		{
				i++;
				l++;
				p++;
			}
			if(!drv_qm_write_rl(-i, fp))	   /* negative length */
			{
				return DSK_ERR_SYSERR;
			}
			if(1 != fwrite(&a, 1, 1, fp))   /* runlen data */
			{
				return DSK_ERR_SYSERR;
			}
			i = 0;
		}
		else
		{
			i++;
			l++;
		}
	}
	if(i || (l < (int)size))   /* dump remaining buffer after end of block */
	{
		i += size - l;
		if(!drv_qm_write_rl(i, fp))	   /* unencoded rest of block */
		{
			return DSK_ERR_NOTME;
		}
		if(1 != fwrite(p, i, 1, fp))	   /* runlen data */
		{
			return DSK_ERR_NOTME;
		}
	}
	return DSK_ERR_OK;
}

/************************************************
 * public functions                             *
 ************************************************/
/************************************************
 * open                                         *
 ************************************************/
dsk_err_t drv_qm_open(DSK_DRIVER * self, const char *filename)
{
	FILE *fp;
	unsigned char header[QM_HEADER_SIZE];
	char *comment_buf = NULL;
	dsk_err_t errcond = DSK_ERR_OK;

	/* Create self pointer or return if wrong type */
	MAKE_CHECK_SELF;

	/* Zero some stuff */
	qm_self->qm_filename = NULL;

	/* Open file. Read only for now */
	fp = fopen(filename, "r+b");
	if (!fp)
	{
		qm_self->qm_super.ld_readonly = 1;
		fp = fopen(filename, "rb");
	}
	if (!fp) return DSK_ERR_NOTME;

	/* Keep a copy of the filename for future writing use */
	qm_self->qm_filename = dsk_malloc_string(filename);
	if(qm_self->qm_filename == NULL)
	{
		fclose(fp);
		return DSK_ERR_NOMEM;
	}
	strcpy(qm_self->qm_filename, filename);

	/* Load the header */
	if (1 != fread(header, QM_HEADER_SIZE, 1, fp))
	{
		fclose(fp);
		dsk_free(qm_self->qm_filename);
		return DSK_ERR_NOTME;
	}
	/* Check magic before we even try to load the header */
	if (header[0] != 'C' || header[1] != 'Q')
	{
		fclose(fp);
		dsk_free(qm_self->qm_filename);
		return DSK_ERR_NOTME;
	}

	/* Load the header */
	errcond = drv_qm_load_header(qm_self, header);
	if (errcond)
	{
		fclose(fp);
		dsk_free(qm_self->qm_filename);
		return errcond;
	}

	/* If there's a comment, allocate a temporary buffer for it and load it. */
	if(errcond == DSK_ERR_OK && qm_self->qm_h_comment_len)
	{
		comment_buf = dsk_malloc(1 + qm_self->qm_h_comment_len);
		/* If malloc fails, ignore it - comments aren't essential */
		if(comment_buf)
		{
			if (fseek(fp, QM_HEADER_SIZE, SEEK_SET)) 
			{
				errcond = DSK_ERR_NOTME;
			}
	    		else
	    		{
				if(1 != fread(comment_buf, qm_self->qm_h_comment_len, 1, fp))
				{
					errcond = DSK_ERR_NOTME;
				}
				else
				{
				    comment_buf[qm_self->qm_h_comment_len] = '\0';
				}
			}
		}
	}
	/* Load the rest */
	if(errcond == DSK_ERR_OK)
	{
		errcond = drv_qm_load_image(qm_self, fp);
		if(errcond != DSK_ERR_OK)
		{
#ifdef DRV_QM_DEBUG
	    fprintf(stderr, "drv_qm_load_image returned %d\n", (int) errcond);
#endif
		}
	}
	if (errcond == DSK_ERR_OK && comment_buf != NULL)
	{
		size_t u8size;
		char *u8cmt;
		unsigned short dosdate, dostime;

		dosdate = ldbs_peek2(header + QM_H_DATE);
		dostime = ldbs_peek2(header + QM_H_TIME);
#ifdef DRV_QM_DEBUG
		fprintf(stderr, "dosdate=%04x dostime=%04x\n", 
				dosdate, dostime);
#endif
		u8size = cp437_to_utf8(comment_buf, NULL, -1);
		u8cmt = dsk_malloc(u8size + 22);
		if (u8cmt)
		{
			/* Include date stamp in the comment */
			sprintf(u8cmt, "[%04d-%02d-%02dT%02d:%02d:%02d] ",
				((dosdate >> 9) & 0x7f) + 1980, 
				(dosdate >> 5) & 0x0F,
				dosdate & 0x1F,
				(dostime >> 11) & 0x1F,
				(dostime >> 5) & 0x3F,
				(dostime << 1) & 0x3F);

			cp437_to_utf8(comment_buf, u8cmt + 22, -1);
			errcond = ldbs_put_comment(qm_self->qm_super.ld_store,
					u8cmt);
			dsk_free(u8cmt);
		}
	}
	if (comment_buf != NULL) dsk_free(comment_buf);
	/* Close the file */
	if(fp) fclose(fp);

	if (!errcond)
	{
		errcond = ldbs_putblock_d(qm_self->qm_super.ld_store, 
			QM_USER_BLOCK, header, QM_HEADER_SIZE);
	}
	if (errcond) 
	{
		ldbs_close(&qm_self->qm_super.ld_store);
		return errcond;
	}
	return ldbsdisk_attach(self);
}

/************************************************
 * create                                       *
 ************************************************/
dsk_err_t drv_qm_create(DSK_DRIVER * self, const char *filename)
{
	FILE *fp;
	dsk_err_t errcond = DSK_ERR_OK;
	unsigned char header[QM_HEADER_SIZE];
	int n, sum;
#ifdef HAVE_TIME_H
	time_t mod;
	struct tm *lz;
#endif

	MAKE_CHECK_SELF;

	/* Create file */
	fp = fopen(filename, "wb");
	if (!fp)
	{
		return DSK_ERR_SYSERR;
	}

	/* Keep a copy of the filename for future writing use */
	qm_self->qm_filename = dsk_malloc_string(filename);
	if(qm_self->qm_filename == NULL)
	{
		fclose(fp);
		return DSK_ERR_NOMEM;
	}
	strcpy(qm_self->qm_filename, filename);

	/* Create a skeleton header and write it. Hopefully qm_close()
	 * will overwrite the whole file anyway */
	memset(header, 0, sizeof(header));

	sprintf((char *) &header[QM_H_BASE], "CQ\x14");	   /* Signature */
	strcpy((char *) &header[QM_H_DESCR], "0K CQM floppy image");
	memset(&header[QM_H_LABEL], ' ', QM_H_LBL_SIZE);   
	header[QM_H_BLIND] = QM_BLIND_BLN;   /* always blind in this version */

	/* Processing date and time if available, else leave unchanged */
#ifdef HAVE_TIME_H
	mod = time(NULL);				   /* Modificaten time */
	lz = localtime(&mod);
	put_u16(header, QM_H_TIME, (unsigned int)
		(lz->tm_hour & 0x1f) << 11 | (lz->tm_min & 0x3f) << 5 | ((lz->tm_sec / 2) & 0x1f));
	put_u16(header, QM_H_DATE, (unsigned int)
		((lz->tm_year - 80) & 0x7f) << 9 | (++(lz->tm_mon) & 0x0f) << 5
		| (lz->tm_mday & 0x1f));
#endif

	for (sum = 0, n = QM_H_BASE; n < QM_HEADER_SIZE - 1; n++)
	    sum += header[n];
	header[QM_H_HEAD_CRC] = (unsigned char) (-sum);

	/* Save the header */
	if (1 != fwrite(header, QM_HEADER_SIZE, 1, fp))
	{
		fclose(fp);
		dsk_free(qm_self->qm_filename);
		return DSK_ERR_SYSERR;
	}
	fclose(fp);
	errcond = ldbs_new(&qm_self->qm_super.ld_store, NULL, LDBS_DSK_TYPE);
	if (errcond) return errcond;
	return ldbsdisk_attach(self);


}


static dsk_err_t qm_get_density(PLDBS ldbs, dsk_pcyl_t cyl, dsk_phead_t head,
				LDBS_TRACKHEAD *th, void *param)
{
	QM_DSK_DRIVER *qm_self = param;
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
		track_size *= 2;
	}
	/* If rate is known, tally it */
	if (th->datarate > 0 && th->datarate < 4) 
	{
		++qm_self->qm_density[th->datarate - 1];
	}
	else	/* Guess data rate from number of bytes in track */
	{
		if (track_size <= 5632)       ++qm_self->qm_density[QM_DENS_DD];
		else if (track_size <= 11264) ++qm_self->qm_density[QM_DENS_HD];
		else			      ++qm_self->qm_density[QM_DENS_ED];
	}	
	return DSK_ERR_OK;

}



/* We have a possible DOS BPB. Does it match the known facts of the 
 * drive geometry? */
static dsk_err_t try_dos_bpb(QM_DSK_DRIVER *self, unsigned char *header, 
			unsigned char *dosbpb, size_t len)
{
	unsigned short secsize  = get_u16(dosbpb, 0);
	unsigned short reserved = get_u16(dosbpb, 3);
	unsigned short fats     = dosbpb[5];
	unsigned short dirents  = get_u16(dosbpb, 6);
	unsigned short secfat   = get_u16(dosbpb, 11);
	unsigned short sectrack = get_u16(dosbpb, 13);
	unsigned short heads    = get_u16(dosbpb, 15);
	LDBS_TRACKHEAD *th = NULL;
	unsigned char *dirbuf, *dirpos;
	int n;
	dsk_lsect_t lsect;
	dsk_pcyl_t cyl;
	dsk_phead_t head;
	dsk_psect_t sec;
	dsk_err_t err;
	size_t remaining;

	if (secsize  && secsize  == get_u16(header, QM_H_SECSIZE) &&
	    sectrack && sectrack == get_u16(header, QM_H_SECPTRK) &&
	    heads    && heads    == get_u16(header, QM_H_HEADS))
	{
		memcpy(header + QM_H_SECSIZE, dosbpb, len);
		header[QM_H_BLIND] = QM_BLIND_DOS;

		/* Locate the root directory */
		lsect = ((long)fats * (long)secfat) + reserved;

		sec  = (lsect % sectrack);
		head = (lsect / sectrack) % heads;
		cyl  = (lsect / sectrack) / heads;

		dirbuf = dsk_malloc(32 * dirents);
		if (!dirbuf) return DSK_ERR_NOMEM;
		dirpos = dirbuf;
#ifdef DRV_QM_DEBUG 
		fprintf(stderr, "Loading root directory length %d at cyl %d, head %d, sec %d\n", dirents, cyl, head, sec);
#endif
		while (dirents)
		{
			remaining = dirents * 32;
			if (remaining > secsize) remaining = secsize;
			if (th == NULL)
			{
				err = ldbs_get_trackhead
					(self->qm_super.ld_store, &th, 
					cyl, head);
				if (err || !th) 
				{
					ldbs_free(th);
					return err;
				}
			}
#ifdef DRV_QM_DEBUG
		fprintf(stderr, "Track header loaded for cyl %d head %d\n",
				cyl, head);
#endif

			for (n = 0; n < th->count; n++)
			{
				/* Add 1 because FAT discs always use 1-based 
				 * sector numbering (well, except for Master
				 * 512 discs, but they don't have boot sectors
				 * so this code won't be called for them) */
				if (th->sector[n].id_sec == sec + 1)
				{
					if (th->sector[n].copies)
					{
						size_t len = remaining;
						err = ldbs_getblock
						   (self->qm_super.ld_store, 
						    th->sector[n].blockid,
						    NULL, dirpos, &len);
						if (err != DSK_ERR_OK &&
						    err != DSK_ERR_OVERRUN)
						{
							ldbs_free(th);
							return err;
						}
					}
					else 
					{
						memset(dirpos, th->sector[n].filler, remaining);
					}
				}
			}	
			dirents -= (remaining / 32);
			dirpos += remaining;
			++sec;
			/* Move to next track */
			if (sec >= sectrack)
			{
				if (th) ldbs_free(th);
				th = NULL;
				sec = 0;
				++head;
				if (head >= heads)
				{
					head = 0;
					++cyl;
				}
			}
		}
		if (th) 
		{
			ldbs_free(th);
		}
/* And after all that, look for a label entry -- that is, one with 'Label' 
 * set and 'System' and 'Hidden' not set (ones with those attributes set are
 * LFNs) */
		dirents  = get_u16(dosbpb, 6);
		for (n = 0; n < dirents; n++)
		{
			if (dirbuf[n * 32] == 0) break;	/* End of directory */
			if ((dirbuf[n * 32 + 11] & 0x0E) == 8)
			{
				memcpy(header + QM_H_LABEL, dirbuf + 32 * n,
					QM_H_LBL_SIZE);
				break;
			}
		}
		dsk_free(dirbuf);
		return DSK_ERR_OK;
	}
	return DSK_ERR_NOTME;	/* DOS BPB does not match drive geometry */
}



/************************************************
 * close                                        *
 ************************************************/
dsk_err_t drv_qm_close(DSK_DRIVER * self)
{
	FILE *fp;
	unsigned char header[QM_HEADER_SIZE];
	char *ucmt = NULL, *ccmt = NULL, *ccmt_orig = NULL;
	unsigned char *trk_buf = NULL;
	dsk_err_t errcond = DSK_ERR_OK;
	size_t len;
	unsigned long crc;
	int tmp, n;
	dsk_pcyl_t wr_cyl;
	dsk_phead_t wr_hd;
	size_t trk_size;
	time_t mod;
	struct tm *lz;
	LDBS_STATS stats;

	QM_DSK_DRIVER *qm_self;
#ifdef DRV_QM_DEBUG
	fprintf(stderr, "drv_qm_close\n");
#endif
	if(self->dr_class != &dc_qm) return DSK_ERR_BADPTR;
	qm_self = (QM_DSK_DRIVER *) self;
	if(!qm_self->qm_filename) return DSK_ERR_NOTRDY;

	/* Firstly, ensure any pending changes are flushed to the LDBS 
	 * blockstore. Once this has been done we own the blockstore again 
	 * and have to close it after we've finished with it. */
	errcond = ldbsdisk_detach(self); 
	if (errcond)
	{
		dsk_free(qm_self->qm_filename);
		ldbs_close(&qm_self->qm_super.ld_store);
		return errcond;
	}

	/* If this disc image has not been written to, just close it and 
	 * dispose thereof. */
	if (!self->dr_dirty)
	{
		dsk_free(qm_self->qm_filename);
		return ldbs_close(&qm_self->qm_super.ld_store);
	}
	/* Trying to save changes but source is read-only */
	if (qm_self->qm_super.ld_readonly)
	{
		dsk_free(qm_self->qm_filename);
		ldbs_close(&qm_self->qm_super.ld_store);
		return DSK_ERR_RDONLY;
	}
	dsk_report("Analysing disc image");
	/* Get the stats of what we're about to save */
	memset(qm_self->qm_density, 0, sizeof(qm_self->qm_density));
	errcond = ldbs_get_stats(qm_self->qm_super.ld_store, &stats);
	if (!errcond) 
	{
		errcond = ldbs_all_tracks(qm_self->qm_super.ld_store,
				qm_get_density, SIDES_ALT, self);
	}
	if (errcond)
	{
		dsk_free(qm_self->qm_filename);
		ldbs_close(&qm_self->qm_super.ld_store);
		dsk_report_end();
		return errcond;
	}
	dsk_report("Compressing CopyQM file");

	fp = fopen(qm_self->qm_filename, "wb");
	if(!fp)
	{
		dsk_report_end();
		return DSK_ERR_SYSERR;
	}
	dsk_free(qm_self->qm_filename);
	/* Did we have a saved copy of the header? */
	len = QM_HEADER_SIZE;
	errcond = ldbs_getblock_d(qm_self->qm_super.ld_store, QM_USER_BLOCK,
			header, &len);
	if ((errcond != DSK_ERR_OK && errcond != DSK_ERR_OVERRUN) || 
		len < QM_HEADER_SIZE)
	{
		/* If not, initialise it to defaults */
		memset(header, 0x00, QM_HEADER_SIZE);

		/* Image size = cylinders * heads * sectors * secsize / 1024 */

		tmp = (int)((((long)stats.max_cylinder + 1) 
			   * ((long)stats.max_head + 1)
			   * ((long)stats.max_spt)
			   * ((long)stats.max_sector_size)) >> 10);

		sprintf((char *) &header[QM_H_DESCR], "%dK %s-Sided", tmp,
				stats.max_head == 0 ? "Single" : "Double");
		memset(&header[QM_H_LABEL], ' ', QM_H_LBL_SIZE);   /* Empty volume label */
		/* alternatively, you can set a 11byte label */
		sprintf((char *) &header[QM_H_LABEL], "%.*s", QM_H_LBL_SIZE, "** NONE ** ");

	}
	sprintf((char *) &header[QM_H_BASE], "CQ\x14");	   /* Signature */

	put_u16(header, QM_H_SECSIZE, stats.max_sector_size);

	tmp = (stats.max_cylinder + 1) * (stats.max_head + 1) * (stats.max_spt);
	put_u16(header, QM_H_SECTOTL, tmp);
	put_u16(header, QM_H_SECPTRK, stats.max_spt);
	put_u16(header, QM_H_HEADS, stats.max_head + 1);
	header[QM_H_BLIND] = QM_BLIND_BLN;   /* always blind in this version */

	/* See which is the most common density and pick that */
	tmp = 0;
	for (n = QM_DENS_DD; n < QM_DENS_ED; n++)
	{
		if ((int)qm_self->qm_density[n] > tmp)
		{
			tmp = qm_self->qm_density[n];
			header[QM_H_DENS] = n;
		}
	}
	header[QM_H_USED_CYL] = (unsigned char) (stats.max_cylinder + 1);
	header[QM_H_TOTL_CYL] = (unsigned char) (stats.max_cylinder + 1);

	/* Processing date and time if available, else leave unchanged */
#ifdef HAVE_TIME_H
	mod = time(NULL);				   /* Modificaten time */
	lz = localtime(&mod);
	put_u16(header, QM_H_TIME, (unsigned int)
		(lz->tm_hour & 0x1f) << 11 | (lz->tm_min & 0x3f) << 5 | ((lz->tm_sec / 2) & 0x1f));
	put_u16(header, QM_H_DATE, (unsigned int)
		((lz->tm_year - 80) & 0x7f) << 9 | (++(lz->tm_mon) & 0x0f) << 5
		| (lz->tm_mday & 0x1f));
#endif

	header[QM_H_SECBASE] = (unsigned char)(stats.min_secid - 1);
/* XXX Calculate interleave 
	header[QM_H_INTLV] = (unsigned char) qm_self->qm_h_interleave;
	header[QM_H_SKEW] = (unsigned char) qm_self->qm_h_skew;
	header[QM_H_DRIVE] = (unsigned char) qm_self->qm_h_drive;
*/

	/* Default to no comment */
	put_u16(header, QM_H_CMT_SIZE, 0);	
	/* Now the comment. As with teledisk, the datestamp has been 
	 * prepended to it. */	

	errcond = ldbs_get_comment(qm_self->qm_super.ld_store, &ucmt);
	if (!errcond && ucmt != NULL)
	{
		len = utf8_to_cp437(ucmt, NULL, -1);
		ccmt_orig = ccmt = dsk_malloc(len);
		if (!ccmt) errcond = DSK_ERR_NOMEM;
		else
		{
			int stamp[6];

			len = utf8_to_cp437(ucmt, ccmt, -1);
			/* Does comment start with a timestamp? */
			if (sscanf(ccmt, "[%d-%d-%dT%d:%d:%d]", &stamp[0],
				&stamp[1], &stamp[2], &stamp[3], &stamp[4],
				&stamp[5]) == 6)
			{
			/* Update timestamp in header */
				put_u16(header, QM_H_TIME, (unsigned int)
		(stamp[3] & 0x1f) << 11 | (stamp[4] & 0x3f) << 5 | 
		((stamp[5] / 2) & 0x1f));
				put_u16(header, QM_H_DATE, (unsigned int)
		((stamp[0] - 1980) & 0x7f) << 9 | (stamp[1] & 0x0f) << 5 | 
		(stamp[2] & 0x1f));
			/* And skip over comment */	
				while (*ccmt != ']') ++ccmt;		
				++ccmt;
				while (*ccmt == ' ') ++ccmt;		
			}
			/* Don't store the terminating nul */
			qm_self->qm_h_comment_len = strlen(ccmt);
#ifdef DRV_QM_DEBUG
	fprintf(stderr, "Comment size = %u\n", qm_self->qm_h_comment_len);
#endif
			put_u16(header, QM_H_CMT_SIZE, 
				(unsigned int) qm_self->qm_h_comment_len);

		}
		if (ucmt) { ldbs_free(ucmt); ucmt = NULL; }
	}
	if (errcond)
	{
		if (ccmt_orig) dsk_free(ccmt_orig);
		if (ucmt) { ldbs_free(ucmt); ucmt = NULL; }
		ldbs_close(&qm_self->qm_super.ld_store);
		fclose(fp);
		dsk_report_end();
		return errcond;
	}	
	dsk_report("Writing CopyQM file");

	/* write header preliminary */
	if(1 != fwrite(header, QM_HEADER_SIZE, 1, fp))
	{
		if (ccmt_orig) dsk_free(ccmt_orig);
		if (ucmt) { ldbs_free(ucmt); ucmt = NULL; }
		ldbs_close(&qm_self->qm_super.ld_store);
		fclose(fp);
		dsk_report_end();
		return DSK_ERR_SYSERR;
	}
#ifdef DRV_QM_DEBUG
	fprintf(stderr, "header written: file pointer=%08lx\n", ftell(fp));
#endif
	/* Write the comment if exist */
	if (ccmt)
	{
	    if(1 != fwrite(ccmt, strlen(ccmt), 1, fp))
	    {
		if (ccmt_orig) dsk_free(ccmt_orig);
		if (ucmt) { ldbs_free(ucmt); ucmt = NULL; }
		fclose(fp);
		ldbs_close(&qm_self->qm_super.ld_store);
		dsk_report_end();
		return DSK_ERR_SYSERR;
	    }
	}
	if (ccmt_orig) dsk_free(ccmt_orig);
	if (ucmt) { ldbs_free(ucmt); ucmt = NULL; }
#ifdef DRV_QM_DEBUG
	fprintf(stderr, "comment written: file pointer=%08lx\n", ftell(fp));
#endif
	/* Write the image  RL coded, one run per track */
	crc = 0l;

	trk_size = (size_t) (stats.max_spt * stats.max_sector_size);
	trk_buf = dsk_malloc(trk_size);
	if (!trk_buf)
	{
		fclose(fp);
		ldbs_close(&qm_self->qm_super.ld_store);
		return DSK_ERR_NOMEM;
	}
	for (wr_cyl = 0; wr_cyl <= stats.max_cylinder; wr_cyl++)
	{
		for(wr_hd = 0; wr_hd <= stats.max_head; wr_hd++)
		{
			LDBS_TRACKHEAD *trkh;

			memset(trk_buf, 0xF6, trk_size);
			errcond = ldbs_get_trackhead(qm_self->qm_super.ld_store,
					&trkh, wr_cyl, wr_hd);
			if (errcond)
			{
				if (trk_buf) dsk_free(trk_buf);
				if (ucmt) { ldbs_free(ucmt); ucmt = NULL; }
				if (trkh) ldbs_free(trkh);
				fclose(fp);
				ldbs_close(&qm_self->qm_super.ld_store);
				return errcond;
			}
			if (trkh)	/* Track exists? */
			{
				errcond = ldbs_load_track(qm_self->qm_super.ld_store, trkh, (void **)&ucmt, &len, stats.max_sector_size);
				if (errcond)
				{
					if (trk_buf) dsk_free(trk_buf);
					if (ucmt) { ldbs_free(ucmt); ucmt = NULL; }
					if (trkh) ldbs_free(trkh);
					fclose(fp);
					ldbs_close(&qm_self->qm_super.ld_store);
					return errcond;
				}
				if (len)
				{
					if (len > trk_size) len = trk_size;
					memcpy(trk_buf, ucmt, len);
				}
				if (ucmt) { ldbs_free(ucmt); ucmt = NULL; }
			}	
			if (trkh) ldbs_free(trkh);
/* If this looks like a DOS BPB, parse it and save the relevant bits to 
 * our header. 
 *
 * XXX Ideally, we should parse the root directory and extract the label 
 * too! */
			if (wr_cyl == 0 && wr_hd == 0)
			{
				DSK_GEOMETRY dg;
				if (!dg_dosgeom(&dg, trk_buf))
				{
					try_dos_bpb(qm_self, header, trk_buf + 11, 0x15);
				}
				else if (!dg_aprigeom(&dg, trk_buf))
				{
					try_dos_bpb(qm_self, header, trk_buf + 80, 0x12);
				}
			}
			/* Track loaded into trk_buf */
			errcond = drv_qm_dump_compressed(fp, &crc, 
							trk_buf, trk_size);
			if (errcond)
			{
				if (trk_buf) dsk_free(trk_buf);
				fclose(fp);
				ldbs_close(&qm_self->qm_super.ld_store);
				return errcond;
	    		}
		}
	}
	dsk_report("Finalizing");
	if (trk_buf) dsk_free(trk_buf);
	ldbs_close(&qm_self->qm_super.ld_store);
#ifdef DRV_QM_DEBUG
	fprintf(stderr, "qm: CRC 0x%08lx\n", crc);
#endif
	/* Write data CRC */
	put_u16(header, QM_H_DATA_CRC, (unsigned int) crc);
	put_u16(header, QM_H_DATA_CRC + 2, (unsigned int) (crc >> 16));

	crc = 0L;		   /* Calculate header checksum */
	for(tmp = QM_H_BASE; tmp < QM_HEADER_SIZE - 1; tmp++)
	    crc += header[tmp];
	header[QM_H_HEAD_CRC] = (unsigned char)(-(signed long)crc);
	if(fseek(fp, 0l, SEEK_SET))
	{
		fclose(fp);
		return DSK_ERR_SYSERR;
	  
	}
	/* write header final, with CRC */
	if(1 != fwrite(header, QM_HEADER_SIZE, 1, fp))
	{
		fclose(fp);
		dsk_report_end();
		return DSK_ERR_SYSERR;
	}
	dsk_report_end();
	return fclose(fp) ? DSK_ERR_SYSERR : DSK_ERR_OK;
}

