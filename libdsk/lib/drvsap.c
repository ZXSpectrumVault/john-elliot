/***************************************************************************
 *                                                                         *
 *    LIBDSK: General floppy and diskimage access library                  *
 *    Copyright (C) 2001-2,2018  John Elliott <seasip.webmaster@gmail.com> *
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
#include "drvsap.h"

/* This driver implements the SAP disk image format:
 * 
 * Header: 66 bytes. First byte is the disk type (low 7 bits):
 *                   0: 80 tracks, single density
 *                   1: 80 tracks, double density
 *                   2: 40 tracks, single density
 *                   3: 40 tracks, double density
 *                   Top bit set if double-sided.
 * 
 *                   Remainder of header is the magic number.
 *
 * The tracks follow the header; each track is apparently always 16 sectors.
 * Sectors have a 4-byte header:
 *		First byte is sector size / recording mode:
 *		  0 => MFM, 256 bytes
 * 		  1 => FM,  128 bytes
 *		  2 => MFM, 1024 bytes
 * 		  3 => MFM, 512 bytes
 * 		Second byte is protection type
 *              Third byte is cylinder ID
 *              Fourth byte is sector ID
 *              Sector data followed by CRC.
 */

static const int SAP_SECTORS = 16;


static const char sap_magic[] = "SYSTEME D'ARCHIVAGE PUKALL "
				"S.A.P. (c) Alexandre PUKALL Avril 1998";

/* This struct contains function pointers to the driver's functions, and the
 * size of its DSK_DRIVER subclass */

DRV_CLASS dc_sap = 
{
	sizeof(SAP_DSK_DRIVER),
	&dc_ldbsdisk,	/* superclass */
	"sap\0SAP\0",
	"SAP file driver",
	sap_open,	/* open */
	sap_creat,	/* create new */
	sap_close,	/* close */
	NULL,		/* read */
	NULL,		/* write */
	NULL,		/* format */
	sap_getgeom,	/* get geometry */	
};

unsigned short sap_crc(unsigned char *b, unsigned short len);

dsk_err_t sap_open(DSK_DRIVER *self, const char *filename)
{
	FILE *fp;
	SAP_DSK_DRIVER *sapself;
	dsk_err_t err;	
	dsk_ltrack_t nt, tracks;
	dsk_lsect_t sec;
	LDBS_TRACKHEAD *trkh;
	char header[66];
	unsigned char secbuf[1030];
	size_t secsize;
	dsk_phead_t head = 0;
	dsk_pcyl_t cylinder = 0;
	int n, allsame;
	int recmode[3];
	
	/* Sanity check: Is this meant for our driver? */
	if (self->dr_class != &dc_sap) return DSK_ERR_BADPTR;
	sapself = (SAP_DSK_DRIVER *)self;

	fp = fopen(filename, "r+b");
	if (!fp) 
	{
		sapself->sap_super.ld_readonly = 1;
		fp = fopen(filename, "rb");
	}
	if (!fp) return DSK_ERR_NOTME;

	/* Load the header */
	if (fread(header, 1, sizeof(header), fp) < (int)sizeof(header))
	{
		fclose(fp);
		return DSK_ERR_NOTME;
	}
	/* Check that the drive type is 00-03 or 80-83, and the magic number 
	 * is present at offset 1. */
	if ((header[0] & 0x7C) ||
	    memcmp(header + 1, sap_magic, 65))
	{
		fclose(fp);
		return DSK_ERR_NOTME;
	}
	/* Work out how many tracks to expect */
	if (header[0] & 2)    tracks = 40; else tracks = 80;
	if (header[0] & 0x80) tracks *= 2;

	/* OK, we're pretty sure this is a SAP image by now */
	/* Keep a copy of the filename; when writing back, we will need it */
	sapself->sap_filename = dsk_malloc_string(filename);
	if (!sapself->sap_filename) 
	{
		fclose(fp);
		return DSK_ERR_NOMEM;
	}
	/* Initialise the blockstore */
	err = ldbs_new(&sapself->sap_super.ld_store, NULL, LDBS_DSK_TYPE);
	if (err)
	{
		dsk_free(sapself->sap_filename);
		fclose(fp);
		return err;
	}
	/* Now to load the tracks */
	nt = 0;
	dsk_report("Loading SAP file into memory");
	for (nt = 0; nt < tracks; nt++)
	{
		if ((header[0] & 0x80) && (nt * 2) >= tracks)
		{
			head = 1;
			cylinder = nt - (tracks / 2);
		}
		else
		{
			head = 0;
			cylinder = nt;
		}
		trkh = ldbs_trackhead_alloc(SAP_SECTORS);
		if (!trkh) 
		{
			fclose(fp);
			return DSK_ERR_NOMEM;
		}
		trkh->recmode = (header[0] & 1) ? 2 : 1;
		trkh->filler  = 0xE5;
		err = DSK_ERR_OK;
		recmode[0] = recmode[1] = recmode[2] = 0;
		for (sec = 0; sec < SAP_SECTORS; sec++)
		{
			unsigned short crc, dcrc;

			if (fread(secbuf, 1, 4, fp) < 4)
			{
				err = DSK_ERR_NOTME;
				break;
			}
			switch(secbuf[0] & 3)
			{
				case 0: secsize = 256; 
					++recmode[2]; 
					trkh->gap3 = 0x36;
					break;
				case 1: secsize = 128; 
					++recmode[1]; 
					trkh->gap3 = 0x1B;
					break;
				case 2: secsize = 1024; 
					++recmode[2]; 
					trkh->gap3 = 0x74;
					break;
				case 3: secsize = 512; 
					++recmode[2]; 	
					trkh->gap3 = 0x54;
					break;
			}		
			if (fread(secbuf + 4, 1, secsize + 2, fp) < secsize + 2)
			{
				err = DSK_ERR_NOTME;
				break;
			}
			/* Sector loaded. Unobfuscate it and add it to the 
			 * track */
			for (n = 0; n < secsize; n++) secbuf[n + 4] ^= 0xb3;
			/* Check the CRC. If it's different set the 'bad CRC'
			 * bit and record what the actual CRC was */
			crc  = sap_crc(secbuf, secsize + 4);
			dcrc = (secbuf[secsize + 4] << 8) | secbuf[secsize + 5];
			if (crc != dcrc)
			{
				trkh->sector[sec].st1 |= 0x20; /* CRC error */
				trkh->sector[sec].st2 |= 0x20; /*   in data */
			}

			trkh->sector[sec].id_cyl = secbuf[2];
			trkh->sector[sec].id_head = head;
			trkh->sector[sec].id_sec = secbuf[3];
			trkh->sector[sec].id_psh = dsk_get_psh(secsize);
			for (n = 0, allsame = 1; n < secsize; n++)
			{
				if (secbuf[n+4] != secbuf[4]) 
				{
					allsame = 0;
					break;
				}
			}	
			if (allsame && (crc == dcrc)) 
			{
				trkh->sector[sec].copies = 0;
				trkh->sector[sec].filler = secbuf[4];
			}
			else
			{
				char secid[4];

				trkh->sector[sec].copies = 1;
				ldbs_encode_secid(secid, secbuf[2], head,
						secbuf[3]);
				/* If CRC is wrong record the on-disk CRC */
				if (crc != dcrc)
				{
					trkh->sector[sec].trail = 2;
					secsize += 2;
				}
				err = ldbs_putblock(sapself->sap_super.ld_store,
					&trkh->sector[sec].blockid,
					secid, secbuf + 4, secsize);
				if (err) break;
			}
			/* If no MFM sectors, or more FM than MFM, track
			 * is assumed to be FM. Otherwise it's MFM. */
			if (recmode[1] > recmode[2] || recmode[2] == 0)
				trkh->recmode = 1;
			else	trkh->recmode = 2;
		}
		if (!err) ldbs_put_trackhead(sapself->sap_super.ld_store,
				trkh, cylinder, head);
		dsk_free(trkh);
		if (err) break;
	}
	if (err) 
	{
		dsk_free(sapself->sap_filename);
		dsk_report_end();
		fclose(fp);
		return err;
	} 
	dsk_report_end();
	fclose(fp);
	return ldbsdisk_attach(self);
}


dsk_err_t sap_creat(DSK_DRIVER *self, const char *filename)
{
	SAP_DSK_DRIVER *sapself;
	dsk_err_t err;
	FILE *fp;

	/* Sanity check: Is this meant for our driver? */
	if (self->dr_class != &dc_sap) return DSK_ERR_BADPTR;
	sapself = (SAP_DSK_DRIVER *)self;

	/* Save the filename, we'll want it when doing output */
	sapself->sap_filename = dsk_malloc_string(filename);
	if (!sapself->sap_filename) return DSK_ERR_NOMEM;

	/* Create a 0-byte file, just to be sure we can */
	fp = fopen(filename, "wb");
	if (!fp) return DSK_ERR_SYSERR;
	fclose(fp);

	/* OK, create a new empty blockstore */
	err = ldbs_new(&sapself->sap_super.ld_store, NULL, LDBS_DSK_TYPE);
	if (err)
	{
		dsk_free(sapself->sap_filename);
		return err;
	}
	/* Finally, hand the blockstore to the superclass so it can provide
	 * all the read/write/format methods */
	return ldbsdisk_attach(self);
}


/* SAP_CRC function based on code provided by Emulix75: */
/* NEW CODE FREE FOR ALL EVEN COMMERCIAL APPLICATIONS */
/* NO RESTRICTION TO USE THIS CODE */
/* EMULIX75 04-08-2018 */

unsigned short sap_crc(unsigned char *b, unsigned short len)
{
	unsigned int idx,i;
	unsigned short int crc_pukall;
	unsigned char data;

	static unsigned short int crc_pukall_tbl_sap[]={
	   0x0000, 0x1081, 0x2102, 0x3183,
	   0x4204, 0x5285, 0x6306, 0x7387,
	   0x8408, 0x9489, 0xa50a, 0xb58b,
	   0xc60c, 0xd68d, 0xe70e, 0xf78f
	};


	crc_pukall = 0xFFFF; /* for a new CRC must be init to 0xffff */

	for (i = 0; i < len; i++)
	{
		data = b[i];
		idx = (crc_pukall ^ data) & 0xf;
		crc_pukall = ((crc_pukall>>4) & 0xfff) ^ crc_pukall_tbl_sap[idx];
		data = data >> 4;
		idx = (crc_pukall ^ data) & 0xf;
		crc_pukall = ((crc_pukall>>4) & 0xfff) ^ crc_pukall_tbl_sap[idx];
	}	
	return crc_pukall;
}


/* Analyse the disc image and determine what recording mode(s) are used on
 * all tracks */
static dsk_err_t sap_recmode_callback
        (PLDBS self,            /* The LDBS file containing this track */
         dsk_pcyl_t cyl,        /* Physical cylinder */
         dsk_phead_t head,      /* Physical head */
         LDBS_TRACKHEAD *th,    /* The track header in question */
         void *param)           /* Parameter passed by caller */
{
	int *recmode = (int *)param;

	switch (th->recmode)
	{
		default: ++recmode[0]; break;
		case 1: ++recmode[1]; break;
		case 2: ++recmode[2]; break;
	}
	return DSK_ERR_OK;
}

static dsk_err_t sap_save_track
        (PLDBS self,            /* The LDBS file containing this track */
         dsk_pcyl_t cyl,        /* Physical cylinder */
         dsk_phead_t head,      /* Physical head */
         LDBS_TRACKHEAD *th,    /* The track header in question */
         void *param)           /* Parameter passed by caller */
{
	int nsec, n;
	unsigned seclen = 0;
	int maxsec = 0;
	unsigned char secbuf[1030];
	dsk_err_t err;
	LDBS_SECTOR_ENTRY *se = NULL;
	dsk_pcyl_t lastcyl = 0;
	SAP_DSK_DRIVER *sapself = (SAP_DSK_DRIVER *)param;
	unsigned short crc;

	for (nsec = 0; nsec < SAP_SECTORS; nsec++)
	{
		memset(secbuf, 0, sizeof(secbuf));
		if (nsec >= th->count)	/* Not enough real sectors. Write a 
					 * dummy */
		{
			secbuf[0] = 0; 
			seclen = 256;
			secbuf[1] = 0;	/* ?? Protection */
			secbuf[2] = lastcyl;
			secbuf[3] = ++maxsec;
			memset(secbuf + 4, 0xE5, seclen);
		}
		else		/* Real sector */
		{
			se = &th->sector[nsec];

			/* Encode sector size */
			switch(se->id_psh)
			{
				case 0: secbuf[0] = 1; seclen = 128; break; 
				case 1: secbuf[0] = 0; seclen = 256; break;
				case 2: secbuf[0] = 3; seclen = 512; break;
				case 3: secbuf[0] = 2; seclen = 1024; break; 
				default: secbuf[0] = 0;	seclen = 256; break;
			}
			secbuf[1] = 0;	/* ?? Protection */
			secbuf[2] = lastcyl = se->id_cyl;
			secbuf[3] = se->id_sec;
			if (se->id_sec > maxsec) maxsec = se->id_sec;
			/* Encode sector data */
			if (se->copies == 0)
				memset(secbuf + 4, se->filler, seclen);
			else
			{
				size_t len = seclen;

				/* If a CRC is stored, load it too */
				if (se->trail >= 2)
				{
					len += 2;
				}
				err = ldbs_getblock(self, se->blockid,
					NULL, secbuf + 4, &len);
				if (err && err != DSK_ERR_OVERRUN)
					return err;
			}
		}
		/* Calculate CRC before obfuscating */
		crc = sap_crc(secbuf, 4 + seclen);
		for (n = 0; n < seclen; n++) secbuf[n + 4] ^= 0xB3;
		if (se && (se->st2 & 0x20) && (se->trail >= 2))
		{
			/* Sector had 'bad CRC' and an actual CRC is present.
			 * Just write the 'bad' CRC rather than recalculating
			 * a 'good' value. */
		}
		else
		{
			secbuf[4 + seclen] = crc >> 8;
			secbuf[5 + seclen] = crc & 0xFF;
		}
		if (fwrite(secbuf, 1, 6 + seclen, sapself->sap_fp) < 
			6 + seclen) return DSK_ERR_SYSERR;
	}
	return DSK_ERR_OK;
}

dsk_err_t sap_close(DSK_DRIVER *self)
{
	SAP_DSK_DRIVER *sapself;
	dsk_err_t err = DSK_ERR_OK;
/* LDBS_STATS does not pick up recording mode, so we'll have to scan for
 * that as well. */
	int recmode[3];
	LDBS_STATS stats;
	char header[66];

	if (self->dr_class != &dc_sap) return DSK_ERR_BADPTR;
	sapself = (SAP_DSK_DRIVER *)self;

	/* Firstly, ensure any pending changes are flushed to the LDBS 
	 * blockstore. Once this has been done we own the blockstore again 
	 * and have to close it after we've finished with it. */
	err = ldbsdisk_detach(self); 
	if (err)
	{
		dsk_free(sapself->sap_filename);
		ldbs_close(&sapself->sap_super.ld_store);
		return err;
	}

	/* If this disc image has not been written to, just close it and 
	 * dispose thereof. */
	if (!self->dr_dirty)
	{
		dsk_free(sapself->sap_filename);
		return ldbs_close(&sapself->sap_super.ld_store);
	}
	/* Trying to save changes but source is read-only */
	if (sapself->sap_super.ld_readonly)
	{
		dsk_free(sapself->sap_filename);
		ldbs_close(&sapself->sap_super.ld_store);
		return DSK_ERR_RDONLY;
	}
	/* OK, write out a SAP file */
	sapself->sap_fp = fopen(sapself->sap_filename, "wb");
	if (!sapself->sap_fp) err = DSK_ERR_SYSERR;
	else
	{
		dsk_report("Analysing SAP file");

		recmode[0] = recmode[1] = recmode[2] = 0;

		err = ldbs_get_stats(sapself->sap_super.ld_store, &stats);
		if (!err) err = ldbs_all_tracks(sapself->sap_super.ld_store,
			sap_recmode_callback, SIDES_OUTOUT, recmode);

		memset(header, 0, sizeof(header));
		if (stats.max_cylinder <= 40)        header[0] |= 0x02;
		if (stats.max_head - stats.min_head) header[0] |= 0x80;
		if (recmode[1] == 0 || recmode[2] > recmode[1])
						     header[0] |= 0x01;
		memcpy(header + 1, sap_magic, 65);
		if (!err)
		{
			if (fwrite(header, 1, 66, sapself->sap_fp) < 66)
				err = DSK_ERR_SYSERR;
		}
		if (!err)
		{ 
			dsk_report("Generating SAP file");
			err = ldbs_all_tracks(sapself->sap_super.ld_store,
					sap_save_track, SIDES_OUTOUT, sapself);
		}
		if (fclose(sapself->sap_fp)) err = DSK_ERR_SYSERR;
	
		dsk_report_end();
	}
	if (sapself->sap_filename) 
	{
		dsk_free(sapself->sap_filename);
		sapself->sap_filename = NULL;
	}
	if (!err) err = ldbs_close(&sapself->sap_super.ld_store);
	else            ldbs_close(&sapself->sap_super.ld_store);
	return err;
}


dsk_err_t sap_getgeom(DSK_DRIVER *self, DSK_GEOMETRY *geom)
{
	dsk_err_t err = ldbsdisk_getgeom(self, geom);

	if (err) return err;

	/* SAP discs are written in SIDES_OUTOUT order, not SIDES_ALT */
	geom->dg_sidedness = SIDES_OUTOUT;
	return err;
}
