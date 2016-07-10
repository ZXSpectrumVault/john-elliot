/************************************************************************

    TAPTOOLS v1.1.0 - Tapefile manipulation utilities

    Copyright (C) 1996, 2014  John Elliott <jce@seasip.demon.co.uk>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

*************************************************************************/

#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#ifdef HAVE_LIMITS_H
#include <limits.h>
#endif
#ifdef HAVE_SYS_H
#include <sys.h>
#endif
#include "tapeio.h"

#ifndef SEEK_SET
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2
#endif

#ifndef PATH_MAX
#ifdef WINDOWS
#define PATH_MAX 260
#endif
#endif

#define IBM_TRAILER 4	/* A real PC writes 4 trailer bytes of 0xFF */

/* Magic number for .ZXT file */
static const unsigned char tzx_magic[] = "ZXTape!\032\001\012";
/* Magic number for .PZX file */
static const unsigned char pzx_magic[] = "PZXT\002\000\000\000\001\000";

/* Corresponds to an open file */
typedef struct tio_file
{
	unsigned char header[128];	/* +3DOS header (for .ZXT files) */
	char filename[PATH_MAX];	/* Filename */
	TIO_FORMAT format;		/* File format */
	FILE *fp;			/* Underlying file */
	int readonly;			/* Nonzero if opened read-only */
	int dirty;
} TIO_FILE;

/* Buffer for error messages */
static char msgbuf[PATH_MAX + 120];

/* Parse little-endian 16-bit, 24-bit and 32-bit integers */
static unsigned peek2(const unsigned char *d)
{
	return ( ((unsigned)d[0]) | (((unsigned)d[1]) << 8));
}

static unsigned long peek3(const unsigned char *d)
{
	return (  ((unsigned long)d[0]) | 
		 (((unsigned long)d[1]) << 8) |
		 (((unsigned long)d[2]) << 16));
}

static unsigned long peek4(const unsigned char *d)
{
	return (  ((unsigned long)d[0]) | 
		 (((unsigned long)d[1]) <<  8) |
		 (((unsigned long)d[2]) << 16) |
		 (((unsigned long)d[3]) << 24) );
}


static void poke4(unsigned char *d, unsigned long v)
{
	d[0] = v & 0xFF;
	d[1] = (v >>  8) & 0xFF;
	d[2] = (v >> 16) & 0xFF;
	d[3] = (v >> 24) & 0xFF;
}


/* Expand a TIO_FORMAT to a text description */
const char *tio_format_desc(TIO_FORMAT fmt)
{
	switch (fmt)
	{
		case TIOF_TAP: return "Spectrum TAP format";
		case TIOF_ZXT: return "Spectrum +3 ZXT format";
		case TIOF_TZX_IBM: 
			      return "TZX format, IBM timings";
		case TIOF_TZX_SPECTRUM: 
			      return "TZX format";
		case TIOF_PZX_IBM: 
			      return "PZX format, IBM timings";
		case TIOF_PZX_SPECTRUM: 
			      return "PZX format";
		default:      sprintf(msgbuf, "Unknown format 0x%02x", fmt);
			      return msgbuf;
	}
}

/* Parse a text format specification to a TIO_FORMAT. */
const char *tio_parse_format(const char *s, TIO_FORMAT *fmt)
{
	char buf[5];
	int n;

	sprintf(buf, "%-4.4s", s);
	for (n = 0; n < 5; n++)
	{
		if (isupper(buf[n])) buf[n] = tolower(buf[n]);
	}		
	if (!strcmp(buf, "tap ")) { *fmt = TIOF_TAP; return NULL; }
	if (!strcmp(buf, "zxt ")) { *fmt = TIOF_ZXT; return NULL; }
	if (!strcmp(buf, "tzx ")) { *fmt = TIOF_TZX_SPECTRUM; return NULL; }
	if (!strcmp(buf, "tzxi")) { *fmt = TIOF_TZX_IBM; return NULL; }
	if (!strcmp(buf, "pzx ")) { *fmt = TIOF_PZX_SPECTRUM; return NULL; }
	if (!strcmp(buf, "pzxi")) { *fmt = TIOF_PZX_IBM; return NULL; }

	sprintf(msgbuf, "Unknown format: %s", s);
	return msgbuf;
}


/* Initialise a new, empty tapefile. Called when a new file is created, or a 
 * 0-byte file is opened. */
static char *tio_init(TIO_FILE **pfp, TIO_FORMAT fmt)
{
	TIO_FILE *self = (*pfp);
	int n, hdrlen = 0;

	switch (fmt)
	{
		case TIOF_TAP:
		default:
			return NULL;	/* No header */

		/* ZXT files have a 128-byte +3DOS header */
		case TIOF_ZXT:
			memset(self->header, 0, sizeof(self->header));
			memcpy(self->header, "PLUS3DOS\032\001", 10);
			memcpy(self->header + 15, "TAPEFILE", 8);
			self->header[11] = 128;
			for (n = 0; n < 127; n++) 
				self->header[127] += self->header[n];
			hdrlen = 128;
			break;
		/* TZX files have a 10-byte header */
		case TIOF_TZX_SPECTRUM:
		case TIOF_TZX_IBM:
			memcpy(self->header, tzx_magic, 10);
			hdrlen = 10;
			break;
		/* as do PZX files (at least, the ones we generate do) */
		case TIOF_PZX_SPECTRUM:
		case TIOF_PZX_IBM:
			memcpy(self->header, pzx_magic, 10);
			hdrlen = 10;
			break;
	}
	if (!hdrlen) return NULL;

	/* If there is a header, write it */
	if (fwrite(self->header, 1, hdrlen, self->fp) < hdrlen)
	{
/* If header write fails, fail file creation */
		fclose(self->fp);
		remove(self->filename);
		free(self);
		*pfp = NULL;
		sprintf(msgbuf, "Failed to write header to file: %s", 
			self->filename);
		return msgbuf;
	}
	return NULL;
}


/* Create a temporary file. Used for programs like sna2tap, which have to
 * support sending output down a pipe; we create the output file and 
 * then stream it out when it's closed. 
 */
const char *tio_mktemp(TIO_FILE **pfp, TIO_FORMAT fmt)
{
#ifdef HAVE_MKSTEMP
	int fd;
#endif
	TIO_FILE *self;
	char filename[20];

	*pfp = NULL;
	self = malloc(sizeof(TIO_FILE));
	if (!self)
	{
		sprintf(msgbuf, "Cannot create %s: Out of memory "
			"(%d bytes needed)\n", filename, (int)sizeof(TIO_FILE));
		return msgbuf;
	}
	strcpy(filename, "TAPEIOXXXXXX");
#ifdef HAVE_MKSTEMP
	fd = mkstemp(filename);
	if (fd < 0)
	{
		free(self);
		return "Failed to create temporary tape file.";
	}
	memset(self, 0, sizeof(TIO_FILE));
	self->format = fmt;
	strcpy(self->filename, filename);
	self->fp = fdopen(fd, "r+b");
#else
	mktemp(filename);
	if (filename[0] == 0)
	{
		free(self);
		return "Failed to create temporary tape file.";
	}
	memset(self, 0, sizeof(TIO_FILE));
	self->format = fmt;
	strcpy(self->filename, filename);
	self->fp = fopen(self->filename, "r+b");
#endif
	if (!self->fp)
	{
		free(self);
		return "Failed to create temporary tape file.";
	}
	*pfp = self;
	return tio_init(pfp, fmt);
}

/* Create a new output file. */
const char *tio_creat(TIO_FILE **pfp, const char *filename, TIO_FORMAT fmt)
{
	TIO_FILE *self;

	*pfp = NULL;
	/* Create the TIO_FILE object */
	self = malloc(sizeof(TIO_FILE));
	if (!self)
	{
		sprintf(msgbuf, "Cannot create %s: Out of memory "
			"(%d bytes needed)\n", filename, (int)sizeof(TIO_FILE));
		return msgbuf;
	}
	memset(self, 0, sizeof(TIO_FILE));
	self->format = fmt;
	/* Create the output file */
	self->fp = fopen(filename, "wb");
	if (!self->fp)
	{
		sprintf(msgbuf, "Cannot open file for write: %s", filename);
		free(self);
		return msgbuf;
	}
	*pfp = self;
	/* Write the header, if there is one. */
	return tio_init(pfp, fmt);
}

/* Read in a block from a PZX file. 
 * On entry hdr should be all-zeroes. If successful, hdr->status will be set
 * to TIO_TZXOTHER and hdr->header and hdr->data will be populated with the 
 * contents of the chunk. 
 *
 * If end of file reached, hdr->status will be set to TIO_EOF.
 *
 * If headonly is nonzero, hdr->header will hold the first 18 bytes of the
 * chunk and hdr->data will be NULL.
 *
 */
static const char *pzx_loadblock(TIO_FILE *self, TIO_HEADER *hdr, int headonly)
{
	unsigned long len;
/*	long pos = ftell(self->fp); */

	if (hdr->data != NULL)
	{
		free(hdr->data);
		hdr->data = NULL;
	}
	/* Load the first 8 bytes of the chunk, which are consistent */
	if (fread(hdr->header, 1, 8, self->fp) < 8)
	{
		hdr->status = TIO_EOF;
		return NULL;
	}
/*	fprintf(stderr, "PZX block %-4.4s @ %08lx\n", hdr->header, pos); */
	hdr->status = TIO_TZXOTHER;
	/* Get the length of the remainder of the chunk */	
	len = peek4(hdr->header + 4);
	if (headonly)
	{
	/* We're only loading the first 18 bytes of the chunk. We already
	 * have 8, so load up to 10 more */
		unsigned more = (len < 10) ? len : 10;
		if (fread(hdr->header + 8, 1, more, self->fp) < more)
		{
			hdr->status = TIO_EOF;
			return NULL;
		}
	/* and skip the rest */
		if (len > 10)
		{
			if (fseek(self->fp, len - 10, SEEK_CUR))
			{
				sprintf(msgbuf, "Cannot seek in file %s",
					self->filename);
				return msgbuf;
			}
		}
		return NULL;
	}
/* We're loading the whole chunk. Malloc memory for it */
	hdr->data = malloc(len + 8);
	if (hdr->data == NULL)
	{
		hdr->length = 0;
		return NULL;
	}
/* Copy in the 8 bytes we read so far */
	memcpy(hdr->data, hdr->header, 8);		
/* And pull in the rest. */
	if (fread(hdr->data + 8, 1, len, self->fp) < len)
	{
		hdr->status = TIO_EOF;
		return NULL;
	}
	hdr->length = len;
/* Copy the first 18 bytes (or less if the block is shorter) to hdr->header */
	if (len + 8 < 18) memcpy(hdr->header, hdr->data, len + 8);
	else	          memcpy(hdr->header, hdr->data, 18);
	return NULL;
}


/* Scan a PZX file for a block with IBM timings (used to distinguish 
 * TIOF_PZX_SPECTRUM from TIOF_PZX_IBM) */
static const char *pzx_findibm(TIO_FILE *self)
{
	TIO_HEADER hdr;
	const char *boo;

	while (1)
	{
		memset(&hdr, 0, sizeof(hdr));
		/* Load the block */
		boo = pzx_loadblock(self, &hdr, 0);
		if (boo) 
		{
			if (hdr.data) free(hdr.data);
			return boo;
		}
		/* Return if we've reached EOF */
		if (hdr.status == TIO_EOF)
		{
			if (hdr.data) free(hdr.data);
			return NULL;
		}
		/* DATA block found. Get the timings. */
		if (!memcmp(hdr.header, "DATA", 4) &&
			peek4(hdr.header + 4) >= 16 &&
			hdr.header[14] == 2 &&
			hdr.header[15] == 2)
		{
			unsigned l1 = peek2(hdr.data + 16);	
			unsigned l2 = peek2(hdr.data + 18);	
			unsigned h1 = peek2(hdr.data + 20);	
			unsigned h2 = peek2(hdr.data + 22);	

			if (l1 == l2 && h1 == h2)
			{
				if (hdr.data) free(hdr.data);
				if (l1 == 875 && h1 == 1750) 	
					self->format = TIOF_PZX_IBM;
				return NULL;	
			}
		}
		if (hdr.data) free(hdr.data);
	}
	return NULL;
}



/* Open a tape file */
const char *tio_open(TIO_FILE **pfp, const char *filename, TIO_FORMAT *fmt)
{
	int n;
	TIO_FILE *self;
	const char *boo;

	*pfp = NULL;
	self = malloc(sizeof(TIO_FILE));
	if (!self)
	{
		sprintf(msgbuf, "Cannot open %s: Out of memory "
			"(%d bytes needed)\n", filename, (int)sizeof(TIO_FILE));
		return msgbuf;
	}
	memset(self, 0, sizeof(TIO_FILE));
	self->dirty = 0;
	/* Open read/write */
	self->fp = fopen(filename, "r+b");
	if (!self->fp)
	{
		/* If that failed, open read-only */
		self->readonly = 1;
		self->fp = fopen(filename, "rb");
	}
	if (!self->fp)
	{
		sprintf(msgbuf, "Cannot open file for read: %s", filename);
		free(self);
		return msgbuf;
	}
	/* Load the first 128 bytes and determine the file type */
	n = fread( self->header, 1, sizeof(self->header), self->fp);

	/* If nothing was read, the file is 0 bytes long: initialise it 
 	 * with the passed-in format. */
	if (n == 0 && !self->readonly)
	{
		return tio_init(pfp, *fmt);
	}

	/* If it's at least 128 bytes long, it could be .ZXT. Check for the
	 * appropriate +3DOS header and checksum. */
	if (n == 128)
	{
		unsigned char cksum = 0;

		for (n = 0; n < 127; n++) cksum += self->header[n];
		if (!memcmp(self->header, "PLUS3DOS\032", 9) &&
		    !memcmp(self->header + 15, "TAPEFILE", 8) &&
		    cksum == self->header[127])
		{
			if (fseek(self->fp, 128, SEEK_SET))
			{
				sprintf(msgbuf, "Cannot seek in file %s",
					self->filename);
				return msgbuf;
			}
			*fmt = self->format = TIOF_ZXT;
			*pfp = self;
			return NULL;
		}
	}
	/* If it's at least 10 bytes long, it could be .PZX or .TZX. */
	if (n >= 10)
	{
		if (!memcmp(self->header, pzx_magic, 4))
		{
			/* Calculate the offset of the first block after 
			 * the header. */
			unsigned long pzxblk = peek4(self->header + 4) + 8;

			/* It's PZX. Check that the major version is 1 
			 * (a PZX implementation MUST fail if the major
			 * version number is too high) */
			if (self->header[8] > 1)
			{
				sprintf(msgbuf, "%s: Unsupported PZX version %d", self->filename, self->header[8]);
				return msgbuf;
			}
			/* Seek to the first block after the header */
			if (fseek(self->fp, pzxblk, SEEK_SET))
			{
				sprintf(msgbuf, "Cannot seek in file %s",
					self->filename);
				return msgbuf;
			}
			/* Determine if it's in IBM format */
			self->format = TIOF_PZX_SPECTRUM;
			boo = pzx_findibm(self);
			if (boo) return boo;

			/* Seek back to the first block after the header */
			if (fseek(self->fp, pzxblk, SEEK_SET))
			{
				sprintf(msgbuf, "Cannot seek in file %s",
					self->filename);
				return msgbuf;
			}
			*fmt = self->format;
			*pfp = self;
			return NULL;
		}
		/* Compare with TZX magic. */
		if (!memcmp(self->header, tzx_magic, 8))
		{
			if (fseek(self->fp, 10, SEEK_SET))
			{
				sprintf(msgbuf, "Cannot seek in file %s",
					self->filename);
				return msgbuf;
			}
			/* We detect IBM timings by looking at the first
			 * chunk. If it's Turbo and the leader/sync pulses
			 * are the same timings as one/zero pulses, assume
			 * IBM. */
			self->format = TIOF_TZX_SPECTRUM;
			if (n >= 22 && self->header[10] == 0x11)
			{
				unsigned leader = peek2(self->header + 11);
				unsigned sync1  = peek2(self->header + 13);
				unsigned sync2  = peek2(self->header + 15);
				unsigned zero   = peek2(self->header + 17);
				unsigned one    = peek2(self->header + 19);

				if (sync1 == sync2 && leader == one && 
				    sync1 == zero)
				{
					self->format = TIOF_TZX_IBM;
				}
			}
			*fmt = self->format;
			*pfp = self;
			return NULL;
		}
	}
	/* No magic number found. Seek to the beginning and assume .TAP. */
	if (fseek(self->fp, 0, SEEK_SET))
	{
		sprintf(msgbuf, "Cannot seek in file %s",
			self->filename);
		return msgbuf;
	}
	*fmt = self->format = TIOF_TAP;
	*pfp = self;
	return NULL;
}



/* tio_open() left the file pointer at the beginning, ready to read 
 * blocks. If we want to append, the file pointer should be placed at
 * the end of the file. */
const char *tio_seek_end(TIO_FILE *self)
{
	int ret;

	/* On a ZXT file, trust the file length given in the +3DOS header 
	 * over the real file length (+3DOS pads file sizes to the nearest
	 * 128 bytes) */
	if (self->format == TIOF_ZXT)
	{
		ret = fseek(self->fp, peek4(self->header + 11), SEEK_SET);
	}
	else
	{
		ret = fseek(self->fp, 0, SEEK_END);
	}
	if (ret)
	{
		sprintf(msgbuf, "Cannot seek in file %s", self->filename);
		return msgbuf;
	}
	return NULL;
}


/* Skip over a TZX chunk. This is annoying because the length of a TZX chunk
 * is not stored consistently; some are fixed, some are variable. 
 * 
 * hdr:  TIO_HEADER which will be populated with the first 18 bytes of the
 *       chunk.
 * type: The type of the chunk. 
 * len:  The length of the fixed part of the chunk (less the first byte) */
const char *tzx_skip(TIO_FILE *self, TIO_HEADER *hdr, unsigned char type,
			int len)
{
	unsigned char header[21];
	int hdrlen;
	unsigned long skip = 0;

 	hdrlen = (len < 20) ? len : 20;
	memset(header, 0, sizeof(header));
	hdr->header[0] = header[0] = type;
	hdr->status = TIO_TZXOTHER;
	/* 1-byte chunk? */
	if (!hdrlen)
	{
		return NULL;
	}
	/* Load the rest of the fixed part */
	if (fread(header + 1, 1, hdrlen, self->fp) < hdrlen)
	{
		hdr->status = TIO_EOF;
		return NULL;
	} 
	hdr->length = hdrlen;
	/* Get the length of the variable part */
	switch (header[0])
	{
		case 0x13: skip = 2 * header[1]; break;
		case 0x15: skip = peek3(header + 6);
		case 0x16: 
		case 0x17: 
			   skip = peek4(header + 1);
			   skip -= 4;
			   break;
		case 0x18:
		case 0x19: 
		case 0x2A:
		case 0x2B:
			   skip = peek4(header + 1);
			   break;
		case 0x21: skip = header[1]; break;
		case 0x26: skip = 2 * peek2(header + 1); break;
		case 0x32:
		case 0x28: skip = peek2(header + 1); break;
		case 0x30: skip = header[1]; break;
		case 0x31: skip = header[2]; break;
		case 0x33: skip = header[1] * 3; break;
		case 0x35: skip = peek4(header + 17); break;
		case 0x40: skip = peek3(header + 2); break;
	}
	memcpy(hdr->header, header, 18);
	hdr->length += skip;
	/* Skip over the variable part */
	if (skip)
	{
		if (fseek(self->fp, skip, SEEK_CUR))
		{
			sprintf(msgbuf, "Cannot seek in file %s",
				self->filename);
			return msgbuf;
		}
	}
	return NULL;
}



/* A TIO_HEADER has been loaded. See if it can be recognised as any of:
 * > A Spectrum file header
 * > A Spectrum data block
 * > A Spectrum snapshot created by sna2tap
 * > An IBM file header
 * > An IBM data record
 */
static void parse_header(TIO_HEADER *hdr, unsigned long blklen)
{
	switch (hdr->header[0])
	{
		/* Spectrum headers: First byte is 0, length = 19 */
		case 0x00: if (blklen == 19)
				hdr->status = TIO_ZX_HEADER; 
			   else hdr->status = TIO_UNKNOWN;
			   break;
		/* IBM records: First byte is 0x16. */
		case 0x16: if (blklen < 0x107)
				hdr->status = TIO_UNKNOWN;
/* If this is a single block of 256 bytes (+ headers & trailers) and its
 * second byte is 0xA5, may be a header */
			   else if ((blklen == 0x107) && hdr->header[1] == 0xA5)
			   {
				hdr->status = TIO_IBM_HEADER;
			   }
			   else hdr->status = TIO_IBM_DATA; 
			   break;
		/* Snapshots: Type=S, length=49181 */
		case 'S':  if (blklen == 49181UL)
				hdr->status = TIO_ZX_SNA; 
			   else hdr->status = TIO_UNKNOWN;
			   break;
		/* First byte is 0xFF: Spectrum data block */
		case 0xFF: hdr->status = TIO_ZX_DATA; break;
		default:   hdr->status = TIO_UNKNOWN; break;
	}
}


/* Read a data block from the file into hdr. 
 *
 * If headeronly is nonzero, read only the first 18 bytes.
 */
const char *tio_readraw(TIO_FILE *self, TIO_HEADER *hdr, int headeronly)
{
	unsigned char blkhead[20];
	unsigned long blklen = 0;
	int c;
	long pos = ftell(self->fp);
	unsigned long hlen;
	const char *boo;

	if (hdr->data)
	{
		free(hdr->data);
		hdr->data = NULL;
	}

	memset(hdr->header, 0, sizeof(hdr->header));
	switch (self->format)
	{
		default:
			hdr->status = TIO_EOF;
			return NULL;

		case TIOF_ZXT:
/* In a ZXT file, check length against header */
			hlen = peek4(self->header + 11);
			if (pos >= (hlen - 3))
			{
				hdr->status = TIO_EOF;
				return NULL;
			}
			/* FALL THROUGH */
		case TIOF_TAP:
			/* Read the block length (2 bytes) */
			if (fread(blkhead, 1, 2, self->fp) < 2)
			{
				hdr->status = TIO_EOF;
				return NULL;
			}
			blklen = peek2(blkhead);
			break;
		case TIOF_PZX_IBM:
		case TIOF_PZX_SPECTRUM:
			boo = pzx_loadblock(self, hdr, 0);
			if (boo) return boo;
			if (hdr->status == TIO_EOF) return NULL;
			/* We have a PZX block. Is it DATA? */
			if (memcmp(hdr->header, "DATA", 4))
			{
				/* If not, return it as-is */
				hdr->status = TIO_TZXOTHER;
				return NULL;
			}
			/* If it is DATA, extract the payload from the 
		 	 * waveform definition */
			blklen = peek4(hdr->header + 4);
			hlen = 16 + 2 * (hdr->header[14] + hdr->header[15]);
			if (hdr->data)
			{
				memmove(hdr->data, hdr->data + hlen,
					blklen + 8 - hlen);
				hdr->length = blklen + 8 - hlen;
				memset(hdr->header, 0, sizeof(hdr->header));
				memcpy(hdr->header, hdr->data,
					(hdr->length < 18) ? hdr->length : 18);
				if (headeronly)
				{
					free(hdr->data);
					hdr->data = NULL;
				}
			}
			/* Parse it to see what we got */
			parse_header(hdr, blklen + 8 - hlen);
			return NULL;	

		case TIOF_TZX_IBM:
		case TIOF_TZX_SPECTRUM:
			/* Get the TZX block type */
			c = fgetc(self->fp);
			if (c == EOF)
			{
				hdr->status = TIO_EOF;
				return NULL;
			}
			blkhead[0] = c;
			switch (c)
			{
				case 0x10:	/* Standard block */
					if (fread(blkhead + 1, 1, 4, self->fp) < 4)
					{
						hdr->status = TIO_EOF;
						return NULL;
					}
					blklen = peek2(blkhead + 3);
					break;	

				case 0x11:	/* Turbo block */
					if (fread(blkhead + 1, 1, 18, self->fp) < 18)
					{
						hdr->status = TIO_EOF;
						return NULL;
					}
					blklen = peek3(blkhead + 16);
					break;	
				case 0x14:	/* Pure data block */
					if (fread(blkhead + 1, 1, 10, self->fp) < 10)
					{
						hdr->status = TIO_EOF;
						return NULL;
					}
					blklen = peek3(blkhead + 8);
					break;	

				/* These we don't support, except to skip */
				case 0x12: return tzx_skip(self, hdr, c, 4);
				case 0x13: return tzx_skip(self, hdr, c, 1);
				case 0x15: return tzx_skip(self, hdr, c, 8);
				case 0x20: return tzx_skip(self, hdr, c, 2);
				case 0x21: return tzx_skip(self, hdr, c, 1);
				case 0x22: return tzx_skip(self, hdr, c, 0);
				case 0x23: return tzx_skip(self, hdr, c, 2);
				case 0x24: return tzx_skip(self, hdr, c, 2);
				case 0x25: return tzx_skip(self, hdr, c, 0);
				case 0x26: return tzx_skip(self, hdr, c, 2);
				case 0x27: return tzx_skip(self, hdr, c, 0);
				case 0x28: return tzx_skip(self, hdr, c, 2);
				case 0x16:
				case 0x17:
				case 0x18:
				case 0x19:
				case 0x29: 
				case 0x2A: 
				case 0x2B: return tzx_skip(self, hdr, c, 4);
				case 0x30: return tzx_skip(self, hdr, c, 1);
				case 0x31: return tzx_skip(self, hdr, c, 2);
				case 0x32: return tzx_skip(self, hdr, c, 3);
				case 0x33: return tzx_skip(self, hdr, c, 1);
				case 0x34: return tzx_skip(self, hdr, c, 8);
				case 0x35: return tzx_skip(self, hdr, c, 20);
				case 0x40: return tzx_skip(self, hdr, c, 4);
				case 'Z':  return tzx_skip(self, hdr, 'Z',  9);
				default:
					sprintf(msgbuf, "%s: Unsupported TZX block type: 0x%02x\n", self->filename, blkhead[0]);
					fprintf(stderr, "Unsupported TZX block type: 0x%02x\n", blkhead[0]);
					return msgbuf;	
			}
					
	}
	/* OK. We got here with (hopefully) blklen = length of data block
	 * and file pointer pointing to data block */
	hdr->length = blklen;
	/* Malloc memory for the data; if that fails, fall back to loading
	 * only the header. */
	if (!headeronly)
	{
		hdr->data = malloc(blklen);
		if (hdr->data == NULL) headeronly = 1;
	}
	if (!headeronly)
	{
		/* Read the entire block */
		if (fread(hdr->data, 1, blklen, self->fp) < blklen)
		{
			free(hdr->data);

			hdr->status = TIO_EOF;
		}
		memcpy(hdr->header, hdr->data, (blklen < 18) ? blklen : 18);
	}
	else
	{
		/* Read the first 18 bytes (or less if there's less) */
		int len = (blklen < 18) ? blklen : 18;

		if (fread(hdr->header, 1, len, self->fp) < len)
		{
			hdr->status = TIO_EOF;
		}
		/* And skip the rest, if any */
		if (blklen > 18)
		{
			if (fseek(self->fp, blklen - 18, SEEK_CUR))
			{
				sprintf(msgbuf, "Cannot seek in file %s",
					self->filename);
				return msgbuf;
			}
		}
	}
	parse_header(hdr, blklen);
	return NULL;

}

/* Read the next data block, skipping unknown TZX / PZX blocks */
static const char *tio_nextblock(TIO_FILE *self, TIO_HEADER *data)
{
	TIO_HEADER dtemp;
	const char *boo;

	do	
	{
		memset(&dtemp, 0, sizeof(dtemp));
		boo = tio_readraw(self, &dtemp, 0);
		if (boo) return boo;
	
		if (dtemp.status != TIO_TZXOTHER)
		{
			memcpy(data, &dtemp, sizeof(dtemp));
			return NULL;
		}

		/* Skip TZX blocks of unknown type */
		if (dtemp.data) free(dtemp.data);
		dtemp.data = NULL;
		
	}
	while (dtemp.status == TIO_TZXOTHER);

	data->status = TIO_EOF;
	return NULL;
}

/* Remove any head, trail and checksum bytes from data->data */
void tio_compact(TIO_HEADER *data)
{
/* The data records loaded by tio_readraw() from IBM tapes have 2-byte 
 * checksums every 256 bytes. Compact the block to remove these */
	if (data->status == TIO_IBM_DATA || data->status == TIO_IBM_HEADER)
	{
		unsigned len = data->length;
		unsigned src = 1;	/* Skip lead byte */
		unsigned dest = 0;

		while (len >= 256)
		{
			memmove(data->data + dest, data->data + src, 256);
			dest += 256;
			len -= 256;
			src += 258;	/* Skip checksum */
		}
		data->length = dest;
	}
	else	/* Remove head and trail bytes from Spectrum block */
	{
		if (data->length > 2)
		{
			memmove(data->data, data->data + 1, data->length - 2);
			data->length -= 2;
		}
	}
}



/* As tio_compact() above, except that we have the BASIC header for the file,
 * which gives the exact size */
static void ibm_compact(const TIO_HEADER *hdr, TIO_HEADER *data)
{
	unsigned len = peek2(hdr->header + 11);
	unsigned src = 1;
	unsigned dest = 0;
	unsigned count;

	while (len)
	{
		count = (len < 256) ? len : 256;
		memmove(data->data + dest, data->data + src, count);
		dest += count;
		len -= count;
		src += 258;	/* Skip checksum */
	}
	data->length = dest;
}


/* Read an ASCII-format file from an IBM tape. The file is composed of a 
 * number of 256-byte records. The first byte is 0 if this is not the last
 * record (with 255 bytes of data following) or 1 + length of record if it
 * is the last. 
 *
 * This uses a streaming interface to return the data, rather than repeatedly
 * realloc()ing the memory necessary to hold everything.
 */
static const char *ibm_read_ascii(TIO_FILE *self, void *param, TIO_READ_CALLBACK cbk)
{
	TIO_HEADER dtemp;
	unsigned blklen;
	const char *boo;
	unsigned char record[256];
	int n;

	record[0] = 0;
	do
	{
		memset(&dtemp, 0, sizeof(dtemp));
		/* Read the next data block */
		boo = tio_nextblock(self, &dtemp);
		if (boo) return boo;

		/* Unexpected block type: treat as EOF. 
		 * Note that TIO_IBM_HEADER is allowed, because a final
		 * block with 164 bytes in it will detect as TIO_IBM_HEADER
		 * rather than TIO_IBM_DATA.
		 */
		if (dtemp.status != TIO_IBM_DATA && 
		    dtemp.status != TIO_IBM_HEADER)
		{
			if (dtemp.data) free(dtemp.data);
			return NULL;
		}
		/* OK: we have a usable data block */
		if (dtemp.data == NULL)
		{
			/* ... it's empty? */
			memset(record, 0, 256);
			record[0] = 1;
		}
		else
		{
			/* Copy that block so we can quickly free() its 
			 * contents */
			memcpy(record, dtemp.data + 1, 256);
			free(dtemp.data);
			dtemp.data = NULL;
		}
		if (record[0] == 0) blklen = 255;
		else	            blklen = record[0]- 1;

		/* Output the contents of the block. */
		for (n = 0; n < blklen; n++)
		{
			boo = (*cbk)(param, record[n + 1]);
			if (boo) return boo;
		}
	}
	while (record[0] == 0);
	return NULL;
}


/* Read a Spectrum- or IBM- format data block from the file, stripping 
 * out all header, trailer and checksum bytes.
 *
 * hdr is the header last read in; its corresponding data blocks are assumed
 * to follow straight on.
 */
const char *tio_readdata(TIO_FILE *self, const TIO_HEADER *hdr, 
		void *param, TIO_READ_CALLBACK cbk)
{
	const char *boo;
	TIO_HEADER dtemp;
	size_t n;

	memset(&dtemp, 0, sizeof(dtemp));

	switch (hdr->status)
	{
		case TIO_ZX_HEADER:
			/* Load the next block, which ought to be TIO_ZX_DATA */
			boo = tio_nextblock(self, &dtemp);
			if (boo) return boo;
			if (dtemp.status == TIO_ZX_DATA &&
				dtemp.length > 2 && dtemp.data)
			{
/* If it is, pass the contents down the callback interface. */
				for (n = 1; n < dtemp.length - 1; n++)
				{
					boo = (*cbk)(param, dtemp.data[n]);
					if (boo)
					{
						free(dtemp.data);
						return boo;
					}
				}
			}
			if (dtemp.data) free (dtemp.data);
			return NULL;

		case TIO_IBM_HEADER:
/* How we handle this depends if it's an IBM or an ASCII file */
			if (hdr->header[0x0A] & 0xA1) /* binary */
			{
				boo = tio_nextblock(self, &dtemp);
				if (boo) return boo;
				if ((dtemp.status == TIO_IBM_DATA ||
				     dtemp.status == TIO_IBM_HEADER) &&
				     dtemp.data != NULL)
				{
/* Remove checksums, headers and trailers */
					ibm_compact(hdr, &dtemp);
					for (n = 0; n < dtemp.length; n++)
					{
/* And return the results */
						boo = (*cbk)(param, dtemp.data[n]);
						if (boo)
						{
							free(dtemp.data);
							return boo;
						}
					}
				}
				if (dtemp.data) free(dtemp.data);
				return NULL;
			}
			/* Not binary, so must be ASCII */
			return ibm_read_ascii(self, param, cbk);
	}

	return "Programming error: tio_readdata() called "
	       "but no Spectrum or IBM header passed ";
}



static const unsigned char ibmturbo[] =
{

/* 'Turbo' TZX block with IBM timings. Note that 'normal' Spectrum timings
 * are within the tolerances of the 5150's tape loader, so there isn't 
 * any actual _need_ to use this; it's just for correctness' sake. */
	0x11,		/* 00 'Turbo' block */
	0xD6, 0x06, 	/* 01 Length of pilot pulse */
	0x6B, 0x03,	/* 03 Length of first sync pulse */	
	0x6B, 0x03,	/* 05 Length of second sync pulse */
	0x6B, 0x03,	/* 07 Length of zero pulse */
	0xD6, 0x06,	/* 09 Length of one pulse */
	0x00, 0x08,	/* 0B Length of pilot tone */
	0x08, 		/* 0D Number of valid bits in last byte */
	0xE8, 0x03,	/* 0E Pause after this block */
	0x07, 0x01, 0x00, /* 10 Length of block = 0x107 
			   * Comprising:     1 sync byte
			   *             0x100 data bytes
                           *                 2 checksum bytes
                           *                 4 trailer bytes */
};

/* PZX chunks describing a Spectrum data block at standard timings */
static unsigned char pzx_spectrum[] =
{
	'P',  'U',  'L',  'S',		/* 0000 magic */
	0x08, 0x00, 0x00, 0x00,		/* 0004 length of block */
	0x7F, 0x9F, 0x78, 0x08,		/* 0008 leader tones */
	0x9B, 0x02, 0xDF, 0x02,		/* 000c sync pulse */
	'D',  'A',  'T',  'A',		/* 0010 magic */
	0x23, 0x00, 0x00, 0x00,		/* 0014 length */
	0x98, 0x00, 0x00, 0x80,		/* 0018 count of pulses */
	0xB1, 0x03, 0x02, 0x02,		/* 001c waveform definition */
	0x57, 0x03, 0x57, 0x03,		/* 0020 zero waveform */
	0xAE, 0x06, 0xAE, 0x06,		/* 0024 one waveform */
};


/* PZX chunks describing an IBM data block at 5150 timings */
static unsigned char pzx_ibm[] =
{
	'P',  'U',  'L',  'S',		/* 0000 magic */
	0x08, 0x00, 0x00, 0x00,		/* 0004 length of block */
	0x00, 0x88, 0xD6, 0x06,		/* 0008 leader tones */
	0x02, 0x80, 0x6B, 0x03,		/* 000c sync pulse */
	'D',  'A',  'T',  'A',		/* 0010 magic */
	0x23, 0x00, 0x00, 0x00,		/* 0014 length */
	0x98, 0x00, 0x00, 0x80,		/* 0018 count of pulses */
	0xB1, 0x03, 0x02, 0x02,		/* 001c waveform definition */
	0x6B, 0x03, 0x6B, 0x03,		/* 0020 zero waveform */
	0xD6, 0x06, 0xD6, 0x06,		/* 0024 one waveform */
};


/* PZX chunk giving a 1-second delay */
static unsigned char pzx_pause[] =
{
	'P', 'A', 'U', 'S',		/* 0000 magic */
	0x04, 0x00, 0x00, 0x00,		/* 0004 length of block */
	0xE0, 0x67, 0x35, 0x00,		/* 0008 duration */
};

/* Write a block to the tape in Spectrum format, adding lead and trail bytes */
const char *tio_writezx(TIO_FILE *self, unsigned char type, unsigned char *blk, unsigned long count)
{
	unsigned char cbt = type;
	size_t n, hdrlen = 0;
	unsigned char hdr[50];

	/* Generate the checkbittoggle */
	for (n = 0; n < count; n++)
	{
		cbt ^= blk[n];
	}
	switch (self->format)
	{
		/* TAP / ZXT have a 2-byte block length + lead byte */
		case TIOF_TAP:
		case TIOF_ZXT:
			hdr[0] = (count + 2) & 0xFF;
			hdr[1] = (count + 2) >> 8;
			hdr[2] = type;
			hdrlen = 3;
			break;
		/* TZX for Spectrum has a 5-byte header + lead byte */
		case TIOF_TZX_SPECTRUM:
			hdr[0] = 0x10;	/* Standard block */
			hdr[1] = 0xe8;	/* Pause 1 second after it */
			hdr[2] = 0x03;
			hdr[3] = (count + 2) & 0xFF;
			hdr[4] = (count + 2) >> 8;
			hdr[5] = type;
			hdrlen = 6;
			break;
		/* TZX for IBM has a 'turbo' header, see ibmturbo */
		case TIOF_TZX_IBM:
			memcpy(hdr, ibmturbo, sizeof(ibmturbo));
			hdr[16] = (count + 2) & 0xFF;
			hdr[17] = (count + 2) >> 8;
			hdr[18] = (count + 2) >> 16;
			hdr[19] = type;
			hdrlen = sizeof(ibmturbo) + 1;
			break;
		/* PZX files have a PULS chunk followed by a DATA chunk */
		case TIOF_PZX_SPECTRUM:
		case TIOF_PZX_IBM:
			if (self->format == TIOF_PZX_IBM)
			{
				memcpy(hdr, pzx_ibm, sizeof(pzx_ibm));
			}
			else
			{
				memcpy(hdr, pzx_spectrum, sizeof(pzx_spectrum));
				/* Length of leader tone depends on block
				 * type */
				if (type < 0x80) 
					{ hdr[8] = 0x7F; hdr[9] = 0x9F; }
				else	{ hdr[8] = 0x97; hdr[9] = 0x8C; }
			}
			/* Count of bytes in block */
			poke4(hdr + 0x14, count + 18L);
			/* Count of bits in payload */
			poke4(hdr + 0x18, (count + 2L) * 8);
			/* +1 lead byte */
			hdrlen = sizeof(pzx_spectrum) + 1;
			hdr[sizeof(pzx_spectrum)] = type;
			break;	
	}
	if (fwrite(hdr, 1, hdrlen, self->fp) < hdrlen ||
	    fwrite(blk, 1, count,  self->fp) < count  ||
	    fputc(cbt, self->fp) == EOF)
	{
		sprintf(msgbuf, "Write error on file: %s", self->filename);
		return msgbuf;
	}
	/* For PZX files, the delay after each block needs to be specified
	 * manually. */
	switch (self->format)
	{
		case TIOF_PZX_SPECTRUM:
		case TIOF_PZX_IBM:
			if (fwrite(pzx_pause, 1, sizeof(pzx_pause), self->fp) 
				< (int)sizeof(pzx_pause))
			{
				sprintf(msgbuf, "Write error on file: %s", self->filename);
				return msgbuf;
			}
			break;
	}
	self->dirty = 1;
	poke4(self->header + 11, ftell(self->fp));
	return NULL;
}


/* Checksum an area of memory using the same algorithm that the IBM 5150 
 * BIOS uses */
unsigned short tio_crc(unsigned char* data_p, short length)
{
	unsigned char x;
	unsigned short crc = 0xFFFF;
	int n;

	while (length--)
	{
		x = *data_p++;

		for (n = 0; n < 8; n++, x = x << 1)
		{
			unsigned char bit = x & 0x80;

			if ((bit == 0 && (crc & 0x8000) != 0) ||
			    (bit != 0 && (crc & 0x8000) == 0))
			{
				crc = ((crc ^ 0x810) << 1) | 1;
			}
			else
			{
				crc = crc << 1;
			}
		}	
	}
	return ~crc;
}


/* Write a block to the tape in IBM format, adding lead and trail bytes and
 * checksums. */
const char *tio_writeibm(TIO_FILE *self, unsigned char *blk, size_t count)
{
	unsigned blocks;
	size_t hdrlen = 0;
	unsigned char hdr[50];
	unsigned long blklen;
	unsigned char block[258];

	/* Round up to nearest 256 bytes */
	blocks = (count + 255) / 256;
	/* 1 lead byte, 258 bytes per block, 4 trail bytes */
 	blklen = 1 + (blocks * 258) + IBM_TRAILER;

	switch (self->format)
	{
/* All this is as in tio_writezx give or take a few timing parameters */
		case TIOF_TAP:
		case TIOF_ZXT:
			hdr[0] = blklen & 0xFF;
			hdr[1] = blklen >> 8;
			hdr[2] = 0x16;
			hdrlen = 3;
			break;
		case TIOF_TZX_SPECTRUM:
			hdr[0] = 0x10;	/* Standard block */
			hdr[1] = 0xe8;
			hdr[2] = 0x03;	/* 1 sec pause */
			hdr[3] = blklen & 0xFF;
			hdr[4] = blklen >> 8;
			hdr[5] = 0x16;
			hdrlen = 6;
			break;
		case TIOF_TZX_IBM:
			memcpy(hdr, ibmturbo, sizeof(ibmturbo));
			hdr[16] = blklen & 0xFF;
			hdr[17] = blklen >> 8;
			hdr[18] = blklen >> 16;
			hdr[19] = 0x16;
			hdrlen = sizeof(ibmturbo) + 1;
			break;
		case TIOF_PZX_SPECTRUM:
		case TIOF_PZX_IBM:
			if (self->format == TIOF_PZX_IBM)
			{
				memcpy(hdr, pzx_ibm, sizeof(pzx_ibm));
			}
			else
			{
				memcpy(hdr, pzx_spectrum, sizeof(pzx_spectrum));
				hdr[8] = 0x7F; 
				hdr[9] = 0x9F; 
			}
			poke4(hdr + 0x14, blklen + 16L);
			poke4(hdr + 0x18, blklen * 8);
			hdrlen = sizeof(pzx_spectrum) + 1;
			hdr[sizeof(pzx_spectrum)] = 0x16;
			break;		
	}
	if (fwrite(hdr, 1, hdrlen, self->fp) < hdrlen)
	{
		sprintf(msgbuf, "Write error on file: %s", self->filename);
		return msgbuf;
	}
	/* Header written. Now write the data, in 256-byte blocks */
	while (count)
	{
		unsigned short crc;

		/* Generate a 256-byte data block */
		if (count >= 256)
		{
			memcpy(block, blk, 256);
			count -= 256;
			blk += 256;
		}
		else
		{
			memcpy(block, blk, count);
			memset(block + count, 0, 256 - count);
			count = 0;
		}
		/* Checksum it */
		crc = tio_crc(block, 256);
		block[256] = crc >> 8;		/* Checksum stored big-endian!*/
		block[257] = crc & 0xFF;
		/* Write it out */
		if (fwrite(block, 1, 258, self->fp) < 258)
		{
			sprintf(msgbuf, "Write error on file: %s", self->filename);
			return msgbuf;
		}
	}
	/* Write the 4 trailer bytes */
	memset(block, 0xFF, IBM_TRAILER);
	if (fwrite(block, 1, IBM_TRAILER, self->fp) < IBM_TRAILER)
	{
		sprintf(msgbuf, "Write error on file: %s", self->filename);
		return msgbuf;
	}
	/* In PZX format, append the delay afterwards. */
	switch (self->format)
	{
		case TIOF_PZX_SPECTRUM:
		case TIOF_PZX_IBM:
			if (fwrite(pzx_pause, 1, sizeof(pzx_pause), self->fp) 
				< (int)sizeof(pzx_pause))
			{
				sprintf(msgbuf, "Write error on file: %s", self->filename);
				return msgbuf;
			}
			break;
	}
	self->dirty = 1;
	poke4(self->header + 11, ftell(self->fp));
	return NULL;
}



/* Update the file header on closing */
static char *tio_finish(TIO_FILE *self)
{
	int n;

	/* For a ZXT file, update the header if the length has changed */
	switch ( self->format )
	{
		case TIOF_ZXT:
			if (self->dirty && !self->readonly)
			{
				self->header[127] = 0;
				for (n = 0; n < 127; n++) 
					self->header[127] += self->header[n];
				if (fseek(self->fp, 0, SEEK_SET))
				{
					sprintf(msgbuf, 
						"Cannot seek in file %s",
						self->filename);
					return msgbuf;
				}
				if (fwrite(self->header, 1, 128, self->fp) < 128)
				{
					sprintf(msgbuf, "%s: Failed to update ZXT header", self->filename);
					return msgbuf;
				}	
			}
			break;
	}
	return NULL;
}


/* Close a tapefile. */
const char *tio_close(TIO_FILE **pfp)
{
	TIO_FILE *self = *pfp;
	const char *boo;

	/* Update the header. */
	boo = tio_finish(self);
	if (boo)
	{
		fclose(self->fp);
		free(self);
		*pfp = NULL;
		return boo;
	}
	/* Close the file */
	if (fclose(self->fp))
	{
		sprintf(msgbuf, "%s: Failed to close file", self->filename);
		free(self);
		*pfp = NULL;
		return msgbuf;
	}
	free(self);
	*pfp = NULL;
	return NULL;
}


/* Close a temporary tapefile, write its contents to fp, and delete it. */
const char *tio_closetemp(TIO_FILE **pfp, FILE *fp)
{
	TIO_FILE *self = *pfp;
	const char *boo;
	int c;

	boo = tio_finish(self);
	if (boo)
	{
		fclose(self->fp);
		remove(self->filename);
		free(self);
		*pfp = NULL;
		return boo;
	}
	if (fseek(self->fp, 0, SEEK_SET))
	{
		sprintf(msgbuf, "Cannot seek in file %s", self->filename);
		fclose(self->fp);
		remove(self->filename);
		free(self);
		*pfp = NULL;
		return msgbuf;
	}

	while ( (c = fgetc(self->fp)) != EOF)
	{
		if (fputc(c, fp) == EOF)
		{
			fclose(self->fp);
			remove(self->filename);
			free(self);
			*pfp = NULL;
			return "Write error on final output file.";
		}
	}	
	if (fclose(self->fp))
	{
		sprintf(msgbuf, "%s: Failed to close file", self->filename);
		remove(self->filename);
		free(self);
		*pfp = NULL;
		return msgbuf;
	}
	remove(self->filename);
	free(self);
	*pfp = NULL;
	return NULL;
}

