/* LDBS: LibDsk Block Store access functions
 *
 *  Copyright (c) 2016, 2017 John Elliott <seasip.webmaster@gmail.com>
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a 
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation 
 * the rights to use, copy, modify, merge, publish, distribute, sublicense, 
 * and/or sell copies of the Software, and to permit persons to whom the 
 * Software is furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included 
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS 
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL 
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR 
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, 
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR 
 * OTHER DEALINGS IN THE SOFTWARE. */

/* LDBS example software: 
 * Simple standalone DSK -> LDBS converter. 
 *
 * DSK format as specified at 
 *        http://www.cpcwiki.eu/index.php/Format:DSK_disk_image_file_format 
 * LDBS 0.3 as specified in ldbs.html
 */

#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ldbs.h"


/* Header of the DSK file being converted */
unsigned char dsk_header[256];

static unsigned char *offset_rec;
static unsigned char *offset_ptr;


LDBS_DPB ldbs_dpb;		/* CP/M DPB, if we can determine it. 
				 * Ideally we should have the full libdsk
				 * geometry probe here, but we'll probably
				 * have to make do with something simpler. */

static unsigned char boot_pcw180[] = 
{
	 0,    0, 40, 9, 2, 1, 3, 2, 0x2A, 0x52
};

static unsigned char boot_cpcsys[] = 
{
	 0,    0, 40, 9, 2, 2, 3, 2, 0x2A, 0x52
};

static unsigned char boot_cpcdata[] =
{
	 0,    0, 40, 9, 2, 0, 3, 2, 0x2A, 0x52
};


void probe_dpb(unsigned char *buf, int psh)
{
	unsigned bsh, blocksize, secsize, dirblocks, drm, off, dsm, al;
	unsigned tracks, sectors, exm;
	static unsigned char alle5[10]  = { 0xE5, 0xE5, 0xE5, 0xE5, 0xE5,
					    0xE5, 0xE5, 0xE5, 0xE5, 0xE5 };

	if (!memcmp(buf, alle5, 10)) buf = boot_pcw180;

	if (buf[0] == 0xE9 || buf[0] == 0xEA || buf[0] == 0xEB)
	{
		if (memcmp(buf + 0x2B, "CP/M", 4) ||
		    memcmp(buf + 0x33, "DSK", 3)  ||
		    memcmp(buf + 0x7C, "CP/M", 4)) return;
		/* Detected PCW16 boot+root, disc spec at 80h */
		buf += 0x80;
	}
	/* Generate the DPB */
	bsh = buf[6];
	blocksize = 128 << bsh;
	secsize   = 128 << buf[4];
	dirblocks = buf[7];
	drm = dirblocks * (blocksize / 32);
	off = buf[5];
	al = (1L << 16) - (1L << (16 - dirblocks));
	tracks = buf[2];
	if (buf[1] & 3) tracks *= 2;	/* Double-sided */
	sectors = buf[3];
	dsm = ((long)tracks - off) * sectors * secsize / blocksize; 

	if (dsm <= 256) exm = (blocksize / 1024) - 1;
	else		exm = (blocksize / 2048) - 1;

	ldbs_dpb.spt = sectors << psh;	
	ldbs_dpb.bsh = bsh;
	ldbs_dpb.blm = (1 << bsh) - 1;
	ldbs_dpb.exm = exm;
	ldbs_dpb.dsm = dsm - 1;
	ldbs_dpb.drm = drm - 1;
	ldbs_dpb.al[0] =  (al >> 8) & 0xFF;	/* AL0 */
	ldbs_dpb.al[1] = al& 0xFF;		/* AL1 */
	ldbs_dpb.cks = drm / 4;
	ldbs_dpb.off = off;
	ldbs_dpb.psh = psh;
	ldbs_dpb.phm = (1 << psh) - 1;
}


/* Migrate a track from CPCEMU .DSK to LDBS format. 
 *
 * For simplicity's sake, read the whole track into memory in one go, 
 * rather than one sector at a time.
 */
dsk_err_t migrate_track(FILE *fpi, PLDBS outfile, int cyl, int head)
{
	int track = (cyl * dsk_header[0x31]) + head;
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
	if (dsk_header[0] != 'E')
	{
		dsk_trklen = ldbs_peek2(dsk_header + 0x32);
	}
	else	/* In an extended DSK it is held per track */
	{
		dsk_trklen = dsk_header[0x34 + track] * 256;
	}
	/* Allocate and load the track */
	dsk_track = malloc(dsk_trklen);
	if (!dsk_track) return DSK_ERR_NOMEM;

	memset(dsk_track, 0, dsk_trklen);

	if (fread(dsk_track, 1, dsk_trklen, fpi) < dsk_trklen)
	{
		fprintf(stderr, "Warning: Short read on track %d\n", track);
	}
	/* Check that the track header has the correct magic */
	if (memcmp(dsk_track, "Track-Info\r\n", 12))
	{
		fprintf(stderr, "Track-Info block %d not found\n", track);
		free(dsk_track);
		return DSK_ERR_NOTME;
	}
	/* Create an LDBS track header */
	ldbs_track = ldbs_trackhead_alloc(dsk_track[0x15]);
	if (!ldbs_track)
	{
		free(dsk_track);
		return DSK_ERR_NOMEM;
	}
	/* Generate the fixed part of the header */
	ldbs_track->count    = dsk_track[0x15];	/* Count of sectors */
	ldbs_track->datarate = dsk_track[0x12];	/* Data rate */
	ldbs_track->recmode  = dsk_track[0x13];	/* Recording mode */
	ldbs_track->gap3     = dsk_track[0x16];	/* GAP#3 length */
	ldbs_track->filler   = dsk_track[0x17];	/* Format filler */
	if (offset_ptr)
	{
		ldbs_track->total_len = ldbs_peek2(offset_ptr);
		offset_ptr += 2;
	}

	source = 256;
	/* In theory a DSK could exist with more than 29 sectors, and 
	 * in that case Track-info would be longer than 256 bytes. */
	if (dsk_track[0x15] > 29)
	{
		source += 8 * (dsk_track[0x15] - 29);
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
		if (offset_ptr)
		{
			cursec->offset = ldbs_peek2(offset_ptr);
			offset_ptr += 2;
		}

		/* If we find cylinder 0 head 0 sector 1 in the right 
		 * place, do a drive geometry probe on it */
		if (cyl == 0 && head == 0 && 
		    cursec->id_cyl   == 0 &&
		    cursec->id_head  == 0 &&
		    cursec->id_sec   == 1 &&
		    cursec->id_psh   == 2)
		{
			probe_dpb(dsk_track + source, 2);
		}
		/* Also check for CPC 'system' and 'data' formats */
		if (cyl == 0 && head == 0 && 
		    cursec->id_cyl   == 0 &&
		    cursec->id_head  == 0 &&
		    cursec->id_sec   == 0x41 &&
		    cursec->id_psh   == 2)
		{
			probe_dpb(boot_cpcsys, 2);
		}		
		if (cyl == 0 && head == 0 && 
		    cursec->id_cyl   == 0 &&
		    cursec->id_head  == 0 &&
		    cursec->id_sec   == 0xC1 &&
		    cursec->id_psh   == 2)
		{
			probe_dpb(boot_cpcdata, 2);
		}		
	

		/* Size of theoretical sector record */
		dsk_secsize = (128 << dsk_track[0x1B + sector * 8]);
	
		/* Size of actual on-disk record */
		if (dsk_header[0] != 'E') 
		{
			dsk_seclen = dsk_secsize;
		}
		else	
		{
			dsk_seclen = ldbs_peek2(dsk_track + 0x1E + sector*8);
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


		err = ldbs_putblock(outfile, &cursec->blockid, block_id,
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
	err = ldbs_put_trackhead(outfile, ldbs_track, cyl, head);
	ldbs_free(ldbs_track);
	free(dsk_track);
	return err;
}



int convert_file(const char *filename)
{
	char *target = malloc(10 + strlen(filename));
	char *dot;
	FILE *fpi;
	PLDBS outfile;
	int c, h;
	dsk_err_t err;

	/* Generate target filename: .dsk -> .ldbs */

	if (!target)
	{
		return DSK_ERR_NOMEM;
	}

	strcpy(target, filename);

	dot = strrchr(target, '.');
	if (dot) strcpy(dot, ".ldbs");
	else	strcat(target, ".ldbs");

	/* Load the .DSK header */
	fpi = fopen(filename, "rb");
	if (!fpi)
	{
		free(target); 
		return DSK_ERR_SYSERR;
	}
	if (fread(dsk_header, 1, sizeof(dsk_header), fpi) < sizeof(dsk_header))
	{
		/* If file is shorter than 256 bytes, zap the header so
		 * it doesn't meet the magic number check */
		memset(dsk_header, 0, sizeof(dsk_header));
	}
	if (memcmp(dsk_header, "MV - CPC", 8 ) &&
	    memcmp(dsk_header, "EXTENDED", 8))
	{
		fclose(fpi);
		free(target);
		return DSK_ERR_NOTME;
	}
	err = ldbs_new(&outfile, target, LDBS_DSK_TYPE);
	if (err)
	{
		fclose(fpi);
		free(target);
		return err;
	}

	/* Initialise CP/M DPB to blank */
	memset(&ldbs_dpb, 0, sizeof(ldbs_dpb));

	/* Migrate the creator record, if there is one */
	if (dsk_header[0x22])
	{
		char creator[15];

		/* Creator length in the DSK header is a packed string and
		 * may not have a terminating NUL. Use strncpy to extract
		 * it. */
		memset(creator, 0, sizeof(creator));
		strncpy(creator, (char *)(dsk_header + 0x22), 14);

		/* Add to file */
		err = ldbs_put_creator(outfile, creator);
	}
	/* Find the "Offset-Info" record, if there is one */
	offset_ptr = NULL;
	if (!err)
	{
		long offset = 0x100;

		if (dsk_header[0] == 'E')
		{
			int t = 0;
			for (c = 0; c < dsk_header[0x30]; c++)
			{
				for (h = 0; h < dsk_header[0x31]; h++)
				{
					offset += 256L * dsk_header[0x34 + t];
					++t;
				}
			}
		}
		else
		{
			offset += (long)ldbs_peek2(dsk_header + 0x32)
					* dsk_header[0x30]	/* *cyls */
					* dsk_header[0x31];	/* *heads */
		}
		if (!fseek(fpi, 0, SEEK_END))
		{
			long max = ftell(fpi);
			if (max > offset && !fseek(fpi, offset, SEEK_SET))
			{
				offset_rec = ldbs_malloc(max - offset);
				if (offset_rec)
				{
					memset(offset_rec, 0, max - offset);
					if (fread(offset_rec, 1, 
						max - offset, fpi) == max - offset)
					{
						if (!memcmp(offset_rec, "Offset-Info\r\n", 13)) offset_ptr = offset_rec + 15;

					}	
				}
			}
		}
		if (fseek(fpi, 0x100, SEEK_SET)) err = DSK_ERR_SYSERR;
	}

	/* Migrate the tracks, one by one */
	if (!err)
	{
		for (c = 0; c < dsk_header[0x30]; c++)
		{
			for (h = 0; h < dsk_header[0x31]; h++)
			{
				err = migrate_track(fpi, outfile, c, h);
				if (err) break;
			}
			if (err) break;
		}
	}
	/* Write the DPB if one was found */	
	if (ldbs_dpb.spt && !err)
	{
		err = ldbs_put_dpb(outfile, &ldbs_dpb);
	}

	if (err)
	{
		fclose(fpi); 
		ldbs_close(&outfile);	
		remove(target);
		free(target); 
		return err; 
	}
/* Clean up and shut down */
	if (fclose(fpi))
	{
		perror(filename);
	}
	err = ldbs_close(&outfile);
	free(target);
	return err;
}



int main(int argc, char **argv)
{
	int n;
	dsk_err_t err;

	if (argc < 2)
	{
		fprintf(stderr, "%s: Syntax is %s <dskfile> <dskfile> ...\n",
				argv[0], argv[0]);
		exit(1);
	}
	for (n = 1; n < argc; n++)
	{
		switch (err = convert_file(argv[n]))
		{
			case DSK_ERR_OK:
				break;
			case DSK_ERR_NOTME: 
				fprintf(stderr, "%s: File is not in CPCEMU DSK / EDSK format\n", argv[n]); 
				break;
			case DSK_ERR_SYSERR:
				perror(argv[n]);
				break;
			case DSK_ERR_NOMEM:
				fprintf(stderr, "Out of memory\n");
				break;
			default:
				fprintf(stderr, "%s: LibDsk error %d\n", 
					argv[n], err);
				break;
		}
	}
	return 0;
}
