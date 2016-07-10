/*
    psftools: Manipulate console fonts in the .PSF format
    Copyright (C) 2003, 2005  John Elliott

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
*/
#include <stdio.h>
#include <string.h>
#ifndef CPM
# ifndef __PACIFIC__
#  include <errno.h>
# endif
#endif
#include "zxflib.h"

/* Functions to manipulate Spectrum +3DOS fonts */

void zxf_file_new(ZXF_FILE *f)
{
	memset(f->zxf_header, 0, sizeof(f->zxf_header));
	memset(f->zxf_data,   0, sizeof(f->zxf_data));
	memset(f->zxf_udgs,   0, sizeof(f->zxf_udgs));

/* Format of +3DOS font header:
0	DB	'PLUS3DOS',26	;+3DOS header magic
9	DB	1,0		;+3DOS header version; always 1.0
11	DD	filesize	;inclusive of header
15	DB	filetype	;fonts are always type 3
16	DW	datasize	;file size less header
18	DW	load address	;Not important for fonts
20	DW	variables	;Not used for fonts
22	DB	spare		;Spare; we will set it to 'f' for "Font".
-- zeroes --
127	DB	checksum	;128-byte checksum of header */

	memcpy(f->zxf_header, ZXF_MAGIC, 11);
	f->zxf_header[11] = 0x80; f->zxf_header[12] = 0x03;  /* Length 0x380 */
	f->zxf_header[16] = 0x00; f->zxf_header[17] = 0x03;  /* Size   0x300 */
	f->zxf_header[18] = 0x00; f->zxf_header[19] = 0x3D;  /* Load addr */ 
	f->zxf_header[22] = 'f';

	f->zxf_header[15] = 0;	/* Mark file as empty */
}

/* Initialise the structure for a new font*/
zxf_errno_t zxf_file_create(ZXF_FILE *f)
{
	zxf_file_delete(f);
	f->zxf_header[15] = 3;
	return ZXF_E_OK;
}

/* This buffer holds from the start of the SNA to the end of the UDG 
 * system variable */
static unsigned char snabuf[7320];

static zxf_errno_t find_snafont(ZXF_FILE *f, FILE *fpin, FILE *fpout)
{
	unsigned chars, udg, ch, ud, fpos;
	int c, hdrlen;

	hdrlen = 0;
	if (!fpout) 
	{
		hdrlen = 128;
		memcpy(snabuf, f->zxf_header, 128);
		zxf_file_new(f);
		zxf_file_create(f);
	}
	if (fread(snabuf + hdrlen, 1, sizeof(snabuf) - hdrlen, fpin) < 
			sizeof(snabuf) - hdrlen)
	{
		if (ferror(fpin)) return ZXF_E_ERRNO;
		return ZXF_E_NOTP3FNT;
	}
	chars = snabuf[7249] + 256 * snabuf[7250] + 256;
	udg   = snabuf[7318] + 256 * snabuf[7319];

	if (chars < 0x4000) 
	{
		fprintf(stderr, "Warning: CHARS is in ROM, "
			"character set not present\n");
		ch = 0;
	}
	else	ch = chars - 0x3FE5;
	if (udg < 0x4000) 
	{
		fprintf(stderr, "Warning: UDG is in ROM, "
			"user-defined graphics not present\n");
		ud = 0;	
	}
	else	ud = udg - 0x3FE5;
	/* Now get/put the font */
	for ( fpos = 0; fpos < 49179; fpos++)
	{
		/* 1. Get the next byte. */
		if (fpos < sizeof(snabuf)) c = snabuf[fpos];
		else 			   c = fgetc(fpin);
		if (c == EOF) 
		{
			if (ferror(fpin)) return ZXF_E_ERRNO;
			return ZXF_E_NOTP3FNT;
		}
		/* 2. Are we in the font? */
		if (ch != 0 && fpos >= ch && fpos <= (ch + 768))
		{
			if (fpout) c = f->zxf_data[fpos - ch];
			else	   f->zxf_data[fpos - ch] = c;
		}
		/* 3. Are we in the UDGs? */
		if (ud != 0 && fpos >= ud && fpos <= (ud + 168))
		{
/* Don't actually save UDGs, because psf2zx does not populate them */
			if (fpout) /*c = f->zxf_udgs[fpos - ud] */;
			else	   f->zxf_udgs[fpos - ud] = c;
		}
		/* 4. Write new char? */
		if (fpout && fputc(c, fpout) == EOF) return ZXF_E_ERRNO;
	}
	if (!fpout)
	{
		f->zxf_format = ZXF_SNA;
	}
	return ZXF_E_OK;
};

static unsigned char rom_signature[16] = 
{
	0xA1, 0x03, 0x8C, 0x10, 0xB2, 0x13, 0x0E, 0x55, 
	0xE4, 0x8D, 0x58, 0x39, 0xBC, 0x5B, 0x98, 0xFD
};

static zxf_errno_t find_romfont(FILE *fpin, FILE *fpout)
{
	static unsigned char buf[128];
	int countdown = -1;

	/* Read the ROM file in 128-byte chunks. When we come to the signature
	 * (which in a real Spectrum ROM is at 3800h) we know the font is
	 * 500h bytes further on. */
	while (fread(buf, 1, sizeof(buf), fpin) == sizeof(buf))
	{
		if (fpout)  
		{
			if (fwrite(buf, 1, sizeof(buf), fpout) < (int)sizeof(buf))
				return ZXF_E_ERRNO;
		}
		if (!memcmp(buf, rom_signature, 16)) 
			countdown = 0x500;
		if (countdown > 0)
		{
			countdown -= 0x80;
			if (!countdown) break;
		}
	}
	if (!countdown) return 0;
	return ZXF_E_NOTP3FNT;
}

/* Load a Spectrum +3 font from a stream */
zxf_errno_t zxf_file_read  (ZXF_FILE *f, FILE *fp, ZXF_FORMAT format)
{
	int n;
	zxf_byte b = 0;
	zxf_byte head[128];

	if (fread(head, 1, 128, fp) < 128) 
	{
		if (ferror(fp)) return ZXF_E_ERRNO;
		return ZXF_E_NOTP3FNT;
	}
	for (n = 0; n < 127; n++) b += head[n];
	if (b == head[n] && !memcmp(head, ZXF_MAGIC, 11)) 
	{
		/* It's +3DOS */	
		f->zxf_format = ZXF_P3DOS;
		if (head[11] != 0x80     || head[12] != 0x03 ||
		    head[13] || head[14] || head[15] != 0x03 ||
		    head[16]             || head[17] != 0x03) 
			return ZXF_E_NOTP3FNT; /* Length or filetype is wrong */

		memcpy(f->zxf_header, head, 128);
		if (fread(f->zxf_data, 1, 768, fp) < 768)
		{
			if (ferror(fp)) return ZXF_E_ERRNO;
			return ZXF_E_NOTP3FNT;
		}
		return ZXF_E_OK;
	}
	/* Possible TAP file */
	else if (head[0] == 0x13 && head[1] == 0 && head[2] == 0)
	{
		f->zxf_format = ZXF_TAP;
		if (head[3] != 3 || head[14] != 0x00 || head[15] != 0x03)
			return ZXF_E_NOTP3FNT; /* Length or filetype is wrong */

		memset(f->zxf_header, 0, sizeof(f->zxf_header));
		memcpy(f->zxf_header, ZXF_MAGIC, 11);
		f->zxf_header[11] = 0x80; f->zxf_header[12] = 0x03;  /* Length 0x380 */
		/* Copy tape header to +3DOS header */
		f->zxf_header[15] = head[3];
		memcpy(f->zxf_header + 16, head + 14, 6);
		f->zxf_header[22] = 'f';
		memcpy(f->zxf_data, head + 0x18, 0x68);
		if (fread(f->zxf_data + 0x68, 1, 768 - 0x68, fp) < 768 - 0x68)
		{
			if (ferror(fp)) return ZXF_E_ERRNO;
			return ZXF_E_NOTP3FNT;
		}
		return ZXF_E_OK;
	}
	if (format == ZXF_ROM)
	{
		n = find_romfont(fp, NULL);
		if (n) return n;
		zxf_file_new(f);
		zxf_file_create(f);
		f->zxf_format = ZXF_ROM;
		if (fread(f->zxf_data, 1, 768, fp) < 768)
		{
			if (ferror(fp)) return ZXF_E_ERRNO;
			return ZXF_E_NOTP3FNT;
		}
		return ZXF_E_OK;	
	}
	if (format == ZXF_SNA)
	{
		return find_snafont(f, fp, NULL);
	}

	/* Assume it's a naked font file */
	zxf_file_new(f);
	zxf_file_create(f);
	f->zxf_format = ZXF_NAKED;
	memcpy(f->zxf_data, head, 128);
	if (fread(f->zxf_data + 128, 1, 640, fp) < 640)
	{
		if (ferror(fp)) return ZXF_E_ERRNO;
		return ZXF_E_NOTP3FNT;
	}
	return ZXF_E_OK;
}

static zxf_byte tap_head[] =
{
	0x13, 0x00,	/* Length of Spectrum file header block */
	0x00, 		/* Block type */ 
	0x03, 		/* Header type */
	'T', ':', 'f', 'o', 'n', 't', '.', 'b', 'i', 'n', /* Filename */
	0x00, 0x03, 	/* Length */
	0x00, 0x3D, 	/* Load address */
	0x00, 0x80,	/* Unused */
	0x8B,		/* Checksum */
	0x02, 0x03,	/* Length of Spectrum file data block */
	0xFF,		/* Block type */
};

/* Write Spectrum +3 font to stream */
zxf_errno_t zxf_file_write (ZXF_FILE *f, FILE *fp, ZXF_FORMAT format, char *auxfile)
{
	int n;
	zxf_byte b = 0;
	FILE *fpin;

	if (!f->zxf_header[15]) return ZXF_E_EMPTY;


	switch(format)
	{
		case ZXF_SNA:
		fpin = fopen(auxfile, "rb");
		if (!fpin) return ZXF_E_ERRNO;
		n = find_snafont(f, fpin, fp);
		fclose(fpin);
		return n;

		case ZXF_ROM:
		fpin = fopen(auxfile, "rb");
		if (!fpin) return ZXF_E_ERRNO;
		n = find_romfont(fpin, fp);
		fclose(fpin);
		if (n) return n;
		if (fwrite(f->zxf_data,   1, 768, fp) < 768) return ZXF_E_ERRNO;
		for (n = 0; n < 768; n++)
		{
			if (fgetc(fpin) == EOF) return ZXF_E_OK;
		}
		while ((n = fgetc(fpin)) != EOF)
		{
			if (fputc(n, fp) == EOF) return ZXF_E_ERRNO;
		}
		return ZXF_E_OK;

		case ZXF_NAKED:
		if (fwrite(f->zxf_data,   1, 768, fp) < 768) return ZXF_E_ERRNO;
		break;

		case ZXF_TAP:
		/* Write header */
		if (fwrite(tap_head, 1, sizeof(tap_head), fp) < (int)sizeof(tap_head)) 
			return ZXF_E_ERRNO;
		/* Write data */
		if (fwrite(f->zxf_data,   1, 768, fp) < 768) return ZXF_E_ERRNO;
		/* Write checksum */
		b = 0xFF;
		for (n = 0; n < 768; n++) b ^= f->zxf_data[n];
		if (fputc(b, fp) == EOF) return ZXF_E_ERRNO;	
		break;

		case ZXF_P3DOS:
		/* Checksum the header */
		for (n = 0; n < 127; n++) b += f->zxf_header[n];
		f->zxf_header[n] = b;
		/* Write out the data */
		if (fwrite(f->zxf_header, 1, 128, fp) < 128) return ZXF_E_ERRNO;
		if (fwrite(f->zxf_data,   1, 768, fp) < 768) return ZXF_E_ERRNO;
		break;
	}
	return ZXF_E_OK;
}


/* Free any memory associated with a +3 font file (dummy) */
void zxf_file_delete (ZXF_FILE *f)
{
	zxf_file_new(f);
}

char *zxf_error_string(zxf_errno_t err)
{
        switch(err)
        {
                case ZXF_E_OK:          return "No error";
                case ZXF_E_NOTP3FNT:    return "File is not a Spectrum +3 font file";
                case ZXF_E_EMPTY:       return "Attempt to save an empty file";
                case ZXF_E_ERRNO:       return strerror(errno);
        }
        return "Unknown error";

}



