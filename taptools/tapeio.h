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

/* Supported tape file formats: */
typedef enum {

	TIOF_TAP,		/* TAP, Spectrum timings are assumed */
	TIOF_ZXT,		/* ZXT, Spectrum timings are assumed */	
	TIOF_TZX_SPECTRUM,	/* TZX, Spectrum timings */
	TIOF_TZX_IBM,		/* TZX, 'authentic' IBM timings */
	TIOF_PZX_SPECTRUM,	/* PZX, Spectrum timings */
	TIOF_PZX_IBM,		/* PZX, 'authentic' IBM timings */

} TIO_FORMAT;

typedef struct tio_file *TIO_PFILE;

/* Block types returned by tio_readblock() */
typedef enum {

	TIO_ZX_HEADER,	/* Parsed as Spectrum header */
	TIO_ZX_DATA,	/* Parsed as Spectrum data */
	TIO_ZX_SNA,	/* Parsed as Spectrum snapshot */
	TIO_IBM_HEADER,	/* Parsed as IBM header */
	TIO_IBM_DATA,	/* Parsed as IBM data */
	TIO_TZXOTHER,	/* TZX unsupported block type */
	TIO_UNKNOWN,	/* Unknown headerless block */
	TIO_EOF		/* End of file, no block found */

} TIO_STATUS;


/* The result of a tio_readblock(): */
typedef struct tio_header
{
	TIO_STATUS status;		/* What sort of block we got */
	unsigned char header[18];	/* The first 18 bytes of it,
					  including the sync byte */
	unsigned length;		/* Length of the block */
	unsigned char *data;		/* The data (if requested) */
} TIO_HEADER;

/* Expand a TIO_FORMAT to a text description */
const char *tio_format_desc(TIO_FORMAT fmt);

/* These functions generally return NULL on success, error message on 
 * error. */

/* Parse an ASCII format to a TIO_FORMAT */
const char *tio_parse_format(const char *s, TIO_FORMAT *fmt);

/* Create a new tape file */
const char *tio_creat(TIO_PFILE *pfp, const char *filename, TIO_FORMAT fmt);

/* Create a new temporary tape file using mkstemp() */
const char *tio_mktemp(TIO_PFILE *pfp, TIO_FORMAT fmt);

/* Close temporary tape file and spew its contents to fp */
const char *tio_closetemp(TIO_PFILE *pfp, FILE *fp);

/* Open an existing tape file, and determine its type 
 *
 * If file is 0 bytes long, it will be initialised with *fmt
 */
const char *tio_open(TIO_PFILE *pfp, const char *filename, TIO_FORMAT *fmt);

/* Seek to end of file (for appending) */
const char *tio_seek_end(TIO_PFILE pfp);

/* Called by tio_readdata() for each data byte */
typedef char *(*TIO_READ_CALLBACK)(void *param, int value);

/* Load a block from a tape file; all leader and trailer bytes will be loaded.
 *
 * hdr should be initialised to all zeroes. On return, it will be populated.
 * If headeronly is set, only a maximum of 18 bytes will be read from the block.
 */
const char *tio_readraw(TIO_PFILE fp, TIO_HEADER *hdr, int headeronly);

/* Call immediately after a Spectrum or IBM tape header has been loaded, 
 * to read and reassemble the corresponding data block(s). Leaders, trailers
 * and checksums will be removed.
 *
 * hdr contains the header block.
 *     The callback
 */
const char *tio_readdata(TIO_PFILE fp, const TIO_HEADER *hdr, void *param,
		TIO_READ_CALLBACK cbk);

/* Remove leader / trailer / checksum bytes from a block read by 
 * tio_readraw() */
void tio_compact(TIO_HEADER *data);

/* Write a block to tape, generating Spectrum leader and trailer. 
 *
 * type = 0 for header, 0xFF for data
 * blk = data to write
 * count = count of bytes
 */
const char *tio_writezx(TIO_PFILE fp, unsigned char type, unsigned char *blk, unsigned long count);

/* Write a block to tape, generating IBM leader, trailer and CRC. 
 *
 * blk = data to write
 * count = count of bytes
 */
const char *tio_writeibm(TIO_PFILE fp, unsigned char *blk, size_t count);

/* Close a tape file. */
const char *tio_close(TIO_PFILE *pfp);

/* Generate a CRC using the same algorithm that the IBM 5150 BIOS uses */
unsigned short tio_crc(unsigned char* data_p, short length);

