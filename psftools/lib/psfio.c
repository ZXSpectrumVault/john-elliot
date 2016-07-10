/*
    psftools: Manipulate console fonts in the .PSF format
    Copyright (C) 2002, 2007  John Elliott

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
#include <stdlib.h>
#include <string.h>
#include "psfio.h"

#define UNKNOWN 0xFFFD		/* Unknown character (unicode) */

static int file_get_byte(PSFIO *io)
{
	return fgetc(io->data.fp);
}

static int array_get_byte(PSFIO *io)
{
        PSFIO_ARRAY *ap = &io->data.arr;
        int c;

        if (ap->len)
        {
                --ap->len;
                c = (ap->data)[0];
                ++ap->data;
                return c;
        }
        return EOF;
}

static int file_put_byte(PSFIO *io, psf_byte b)
{
	return fputc(b, io->data.fp);
}


static int array_put_byte(PSFIO *io, psf_byte b)
{
        PSFIO_ARRAY *ap = &io->data.arr;

        if (ap->len)
        {
                --ap->len;
                (ap->data)[0] = b;
                ++ap->data;
                return 0;
        }
        return EOF;
}


psf_errno_t psfio_get_byte(PSFIO *io, psf_byte *b)
{
	int c = (*io->readfunc)(io);

	if (c == EOF) return PSF_E_NOTPSF;
	*b = c;
	return PSF_E_OK;
}



psf_errno_t psfio_get_bytes(PSFIO *io, psf_byte *b, unsigned len)
{
	while (len)
	{
		if (psfio_get_byte(io, b)) return PSF_E_NOTPSF;
		++b;
		--len;
	}
	return PSF_E_OK;
}



psf_errno_t psfio_get_word(PSFIO *io, psf_word *w)
{
	psf_byte l, h;

	if (psfio_get_byte(io, &l)) return PSF_E_NOTPSF;
	if (psfio_get_byte(io, &h)) return PSF_E_NOTPSF;
	*w = ((psf_word)h) << 8 | l;
	return PSF_E_OK;
}


psf_errno_t psfio_get_dword(PSFIO *io, psf_dword *w)
{
	psf_word l, h;

	if (psfio_get_word(io, &l)) return PSF_E_NOTPSF;
	if (psfio_get_word(io, &h)) return PSF_E_NOTPSF;
	*w = ((psf_dword)h) << 16 | l;
	return PSF_E_OK;
}



psf_errno_t psfio_get_utf8 (PSFIO *io, psf_byte first, psf_dword *b)
{
	psf_byte buf[6];
	psf_dword ch;
	unsigned len, n;

	*b = UNKNOWN;
	buf[0] = first;

	if (first < 0x80) { *b = first;   return PSF_E_OK; }
	if (first < 0xC0) return PSF_E_UNICODE; 
	if      (first < 0xE0) len = 2;
	else if (first < 0xF0) len = 3;
	else if (first < 0xF8) len = 4;
	else if (first < 0xFC) len = 5;	
	else if (first < 0xFE) len = 6;
	else return PSF_E_UNICODE;
	
	if (psfio_get_bytes(io, buf+1, len-1)) return PSF_E_UNICODE;
	for (n = 1; n < len; n++) if ((buf[n] & 0xC0) != 0x80) return PSF_E_UNICODE;

	switch(len)
	{
		case 2: ch = (buf[0] & 0x1F);
			ch = ch << 6 | (buf[1] & 0x3F);
			*b = ch;
			break;
		case 3: ch = (buf[0] & 0x0F);
			ch = ch << 6 | (buf[1] & 0x3F);
			ch = ch << 6 | (buf[2] & 0x3F);
			*b = ch;
			break;
		case 4: ch = (buf[0] & 0x07);
			ch = ch << 6 | (buf[1] & 0x3F);
			ch = ch << 6 | (buf[2] & 0x3F);
			ch = ch << 6 | (buf[3] & 0x3F);
			*b = ch;
			break;
		case 5: ch = (buf[0] & 0x03);
			ch = ch << 6 | (buf[1] & 0x3F);
			ch = ch << 6 | (buf[2] & 0x3F);
			ch = ch << 6 | (buf[3] & 0x3F);
			ch = ch << 6 | (buf[4] & 0x3F);
			*b = ch;
			break;
		case 6: ch = (buf[0] & 0x01);
			ch = ch << 6 | (buf[1] & 0x3F);
			ch = ch << 6 | (buf[2] & 0x3F);
			ch = ch << 6 | (buf[3] & 0x3F);
			ch = ch << 6 | (buf[4] & 0x3F);
			ch = ch << 6 | (buf[5] & 0x3F);
			*b = ch;
			break;
	}
	return PSF_E_OK;
}

psf_errno_t psfio_put_byte(PSFIO *io, psf_byte b)
{
	int c = (*io->writefunc)(io, b);

	if (c == EOF) return PSF_E_ERRNO;
	else return PSF_E_OK;
}


psf_errno_t psfio_put_word(PSFIO *io, psf_word w)
{
        if (psfio_put_byte(io, w & 0xFF)) return PSF_E_ERRNO;
        if (psfio_put_byte(io, (w >> 8))) return PSF_E_ERRNO;
        return PSF_E_OK;
}


psf_errno_t psfio_put_dword(PSFIO *io, psf_dword w)
{
        if (psfio_put_word(io, w & 0xFFFF)) return PSF_E_ERRNO;
        if (psfio_put_word(io, w >> 16)) return PSF_E_ERRNO;
        return PSF_E_OK;
}



psf_errno_t psfio_put_bytes(PSFIO *io, psf_byte *b, unsigned len)
{
	while (len)
	{
		if (psfio_put_byte(io, *b)) return PSF_E_ERRNO;
		++b;
		--len;
	}
	return PSF_E_OK;
}

psf_errno_t psfio_put_utf8 (PSFIO *io, psf_dword u32)
{
	psf_byte buf[6];
	unsigned len;

	if      (u32 < 0x80)      { buf[0] = u32 & 0x7F; len = 1; }
	else if (u32 < 0x800)     { buf[1] = (u32 & 0x3F) | 0x80; u32 = u32 >> 6;
			 	    buf[0] = (u32 & 0x1F) | 0xC0; len = 2; }
	else if (u32 < 0x10000)   { buf[2] = (u32 & 0x3F) | 0x80; u32 = u32 >> 6;
				    buf[1] = (u32 & 0x3F) | 0x80; u32 = u32 >> 6;
				    buf[0] = (u32 & 0x0F) | 0xE0; len = 3; }
	else if (u32 < 0x200000)  { buf[3] = (u32 & 0x3F) | 0x80; u32 = u32 >> 6;
				    buf[2] = (u32 & 0x3F) | 0x80; u32 = u32 >> 6;
				    buf[1] = (u32 & 0x3F) | 0x80; u32 = u32 >> 6;
				    buf[0] = (u32 & 0x07) | 0xF0; len = 4; }
	else if (u32 < 0x4000000) { buf[4] = (u32 & 0x3F) | 0x80; u32 = u32 >> 6;
				    buf[3] = (u32 & 0x3F) | 0x80; u32 = u32 >> 6;
				    buf[2] = (u32 & 0x3F) | 0x80; u32 = u32 >> 6;
				    buf[1] = (u32 & 0x3F) | 0x80; u32 = u32 >> 6;
				    buf[0] = (u32 & 0x03) | 0xF8; len = 4; }
	else                      { buf[5] = (u32 & 0x3F) | 0x80; u32 = u32 >> 6;
				    buf[4] = (u32 & 0x3F) | 0x80; u32 = u32 >> 6;
				    buf[3] = (u32 & 0x3F) | 0x80; u32 = u32 >> 6;
				    buf[2] = (u32 & 0x3F) | 0x80; u32 = u32 >> 6;
				    buf[1] = (u32 & 0x3F) | 0x80; u32 = u32 >> 6;
				    buf[0] = (u32 & 0x03) | 0xFC; len = 5; }

	return psfio_put_bytes(io, buf, len);	
}



/* Load a PSF from a stream */
int psf_file_read  (PSF_FILE *f, FILE *fp)
{
        PSFIO io;

        io.psf = f;
        io.readfunc = file_get_byte;
	io.writefunc = NULL;
	io.data.fp = fp;

	return psf_read(&io);
}


/* Load a PSF from memory */
int psf_memory_read  (PSF_FILE *f, psf_byte *data, unsigned len)
{
        PSFIO io;

        io.psf = f;
        io.readfunc = array_get_byte;
	io.writefunc = NULL;
	io.data.arr.data = data;
	io.data.arr.len  = len;

	return psf_read(&io);
}



/* Load a PSF from a stream */
int psf_file_write  (PSF_FILE *f, FILE *fp)
{
        PSFIO io;

        io.psf = f;
        io.readfunc = NULL;
	io.writefunc = file_put_byte;
	io.data.fp = fp;

	return psf_write(&io);
}


/* Load a PSF from memory */
int psf_memory_write (PSF_FILE *f, psf_byte *data, unsigned len)
{
        PSFIO io;

        io.psf = f;
        io.readfunc = NULL;
	io.writefunc = array_put_byte;
	io.data.arr.data = data;
	io.data.arr.len  = len;

	return psf_write(&io);
}





