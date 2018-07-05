/* LDBS: LibDsk Block Store access functions
 *
 *  Copyright (c) 2016 John Elliott <seasip.webmaster@gmail.com>
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
 * Simple standalone LDBS -> EDSK converter. 
 *
 * EDSK format as specified at 
 *        http://www.cpcwiki.eu/index.php/Format:DSK_disk_image_file_format 
 * LDBS 0.1 as specified in ldbs.html
 */

#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ldbs.h"

int verbose;

/* Header of the DSK file being generated */
unsigned char dsk_header[256];

unsigned char *offset_rec = NULL, *offset_ptr = NULL;
int any_offset = 0;


/* Migrate a track from LDBS to CPCEMU .DSK */
dsk_err_t migrate_track(PLDBS infile, FILE *fpo, int cyl, int head)
{
	int track = (cyl * dsk_header[0x31]) + head;
	dsk_err_t err = 0;
	int n;
/* A Track-Info header is normally 256 bytes. However if a track has more 
 * than 29 sectors, Track-Info has to be increased */
	unsigned char trackinfo[2304];
	LDBS_TRACKHEAD *ldbs_track;	/* LDBS track header */
	size_t tracklen, secsize, tilen, m;
	size_t expected = ldbs_peek2(dsk_header + 0x32);

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
		if (dsk_header[0] == 'E')
		{
			dsk_header[0x34 + track] = 1;
		}
		if (fwrite(trackinfo, 1, 256, fpo) < 256) return DSK_ERR_SYSERR;
		/* If not an EDSK, all tracks are the same length, so write
		 * padding bytes to the right size */
		if (dsk_header[0] != 'E')
		{
			for (m = 256; m < expected; m++)
			{
				fputc(trackinfo[0x17], fpo);
			}
		}
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

	if (offset_ptr)
	{
		ldbs_poke2(offset_ptr, ldbs_track->total_len);
		if (ldbs_track->total_len) any_offset = 1;
		offset_ptr += 2;
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

		if (offset_ptr)
		{
			ldbs_poke2(offset_ptr, ldbs_track->sector[n].offset);
			if (ldbs_track->sector[n].offset) any_offset = 1;
			offset_ptr += 2;
		}
		secsize = 128 << ldbs_track->sector[n].id_psh;
		if (ldbs_track->sector[n].copies > 0)
		{
			secsize += ldbs_track->sector[n].trail; /* 0.3 */
			secsize *= ldbs_track->sector[n].copies;
		}
		if (dsk_header[0] == 'E')
		{
			ldbs_poke2(trackinfo + 0x1E + 8 * n, secsize);
		}
	}
	tilen = 256;
	if (ldbs_track->count > 29)
	{
		tilen = (ldbs_track->count * 8) + 0x18;
		/* Round up to a multiple of 256 bytes */
		tilen = (tilen + 255) & (~255);
	}
/* Write the Track-Info block */
	if (fwrite(trackinfo, 1, tilen, fpo) < tilen) 
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
				if (fputc(ldbs_track->sector[n].filler, fpo) == EOF)
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
			if (fwrite(secbuf, 1, len, fpo) < len)
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
		if (fputc(0, fpo) == EOF)
		{
			ldbs_free(ldbs_track);
			return DSK_ERR_SYSERR;
		}
		++tracklen;
	}
	if (dsk_header[0] == 'E')
	{
		dsk_header[0x34 + track] = tracklen / 256;
	}
	else
	{
		if (tracklen != expected)
		{
			fprintf(stderr, "ERROR: Cylinder %d head %d "
					"unexpected track length\n",
					cyl, head);
		}
	}
	return DSK_ERR_OK;
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

int convert_file(const char *filename)
{
	char type[4];
	int readonly;
	char *target = malloc(10 + strlen(filename));
	char *dot;
	FILE *fpo;
	PLDBS infile;
	LDBS_STATS stats;
	OFFSET_COUNT oc;
	dsk_pcyl_t c;
	dsk_phead_t h;
	dsk_err_t err;
	char *creator;

	/* Generate target filename: .ldbs -> .dsk */

	if (!target)
	{
		return DSK_ERR_NOMEM;
	}

	strcpy(target, filename);

	dot = strrchr(target, '.');
	if (dot) strcpy(dot, ".dsk");
	else	strcat(target, ".dsk");

	if (verbose) printf("%s -> %s\n", filename, target);

	readonly = 1;
	err = ldbs_open(&infile, filename, type, &readonly);
	if (err) return err;

	if (verbose) printf("%s opened.\n", filename);

	if (memcmp(type, LDBS_DSK_TYPE, 4))
	{
		ldbs_close(&infile);
		return DSK_ERR_NOTME;
	}
	/* Get stats */
	err = ldbs_get_stats(infile, &stats);
	if (err)
	{
		ldbs_close(&infile);
		return err;
	}
	if (verbose)
	{
		printf("max_cylinder=%d\n", stats.max_cylinder);
		printf("min_cylinder=%d\n", stats.min_cylinder);
		printf("max_head    =%d\n", stats.max_head);
		printf("min_head    =%d\n", stats.min_head);
		printf("max_spt     =%d\n", stats.max_spt);
		printf("min_spt     =%d\n", stats.min_spt);
		printf("max_secid   =%d\n", stats.max_secid);
		printf("min_secid   =%d\n", stats.min_secid);
		printf("max_secsize =%d\n", (int)stats.max_sector_size);
		printf("min_secsize =%d\n", (int)stats.min_sector_size);
	}

	/* Initialise the .DSK header */
	memset(dsk_header, 0, sizeof(dsk_header));

	/* Check for presence of offset info; that forces EDSK */
	memset(&oc, 0, sizeof(oc));
	err = ldbs_all_sectors(infile, count_sector_callback, SIDES_ALT, &oc);
	if (err)
	{
		ldbs_close(&infile);
		return err;
	}

	/* Is this a reasonably regular DSK with the same number of sectors
	 * in each track and all sectors the same size? */
	if (stats.max_spt == stats.min_spt &&
	    stats.max_sector_size == stats.min_sector_size &&
	    stats.max_spt <= 29 &&
	    !oc.uses_offsets)
	{
		strcpy((char *)dsk_header, "MV - CPCEMU Disk-File\r\nDisk-Info\r\n");
		if (verbose) printf("Output format will be .DSK\n");
	}
	else
	{
		strcpy((char *)dsk_header, "EXTENDED CPC DSK File\r\nDisk-Info\r\n");
		if (verbose) printf("Output format will be extended .DSK\n");
	}
	/* Migrate the creator if there is one */
	err = ldbs_get_creator(infile, &creator);
	if (err)
	{
		ldbs_close(&infile);
       		return err;
	}
	if (creator)
	{
		strncpy((char *)dsk_header + 0x22, creator, 14);
		ldbs_free(creator);
	}

	/* Get the track directory */
	err = ldbs_max_cyl_head(infile, &c, &h);
      	if (err)
	{
		ldbs_close(&infile);
		return err;
	}
	if (verbose) printf("Converting %d cylinders, %d heads\n", c, h);
	fpo = fopen(target, "w+b");
	if (!fpo)
	{
		perror(target);
		ldbs_close(&infile);
		return DSK_ERR_SYSERR;
	}	
	/* Compute the maximum cylinder and head */
	dsk_header[0x30] = c;
	dsk_header[0x31] = h;

	any_offset = 0;
	offset_rec = NULL;
	offset_ptr = NULL;
	if (dsk_header[0] == 'E' && oc.uses_offsets)
	{
		offset_rec = ldbs_malloc( 15 + ((c * h + oc.sectors) * 2));
		if (offset_rec)
		{
			memcpy(offset_rec, "Offset-Info\r\n", 14);
			offset_rec[14] = 0;
			offset_ptr = offset_rec + 15;
		}
	}
	if (dsk_header[0] != 'E')
	{
		/* Fixed-length tracks: 256 byte header + secsize * spt
		 * sectors */
		ldbs_poke2(dsk_header + 0x32,
				256 + stats.max_sector_size * stats.max_spt);
	}

	/* Write an (incomplete) EDSK header */
	if (fwrite(dsk_header, 1, 256, fpo) < 256)
	{
		perror(target);
		ldbs_close(&infile);
		return DSK_ERR_SYSERR;
	}

	for (c = 0; c < dsk_header[0x30]; c++)
	{
		for (h = 0; h < dsk_header[0x31]; h++)
		{
			if (verbose) printf("Migrating cylinder %d head %d\n",
					c, h);
			err = migrate_track(infile, fpo, c, h);
			if (err) break;
		}
		if (err) break;
	}
	/* Write the offset record if it exists */
	if (!err && offset_rec && any_offset)
	{
		size_t offset_len = (offset_ptr - offset_rec);

		if (fwrite(offset_rec, 1, offset_len, fpo) < offset_len)
		{
			err = DSK_ERR_SYSERR;
		}
	}
	if (offset_rec)
	{
		ldbs_free(offset_rec);	
	}
	offset_rec = NULL;
	offset_ptr = NULL;

/* Rewrite the updated header */
	if (!err)
	{
		if (verbose) printf("Writing the updated header\n");
		if (fseek(fpo, 0, SEEK_SET) ||
		    fwrite(dsk_header, 1, 256, fpo) < 256) 
		{
			err = DSK_ERR_SYSERR;
		}

	}
	if (err)
	{
		fclose(fpo); 
		ldbs_close(&infile);	
		remove(target);
		free(target); 
		return err; 
	}
	if (verbose) printf("Complete.\n");


/* Clean up and shut down */
	if (fclose(fpo))
	{
		perror(target);
	}
	err = ldbs_close(&infile);
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
		if (!strcmp(argv[n], "-verbose") || 
		    !strcmp(argv[n], "-v") ||
		    !strcmp(argv[n], "--verbose"))
		{
			verbose = 1;
			continue;
		}

		switch (err = convert_file(argv[n]))
		{
			case DSK_ERR_OK:
				break;
			case DSK_ERR_NOTME: 
				fprintf(stderr, "%s: File is not in LDBS disk format\n", argv[n]); 
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
