/*
    psftools: Manipulate console fonts in the .PSF format
    Copyright (C) 2002  John Elliott

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
#ifndef CPM
# ifndef __PACIFIC__
#  include <errno.h>
# endif
#endif
#include "psflib.h"


/* psfio.c */
psf_errno_t psfio_get_byte (PSFIO *io, psf_byte *b);
psf_errno_t psfio_get_bytes(PSFIO *io, psf_byte *b, unsigned len);
psf_errno_t psfio_get_word (PSFIO *io, psf_word *b);
psf_errno_t psfio_get_dword(PSFIO *io, psf_dword *b);
psf_errno_t psfio_get_utf8 (PSFIO *io, psf_byte first, psf_dword *b);

psf_errno_t psfio_put_byte (PSFIO *io, psf_byte b);
psf_errno_t psfio_put_bytes(PSFIO *io, psf_byte *b, unsigned len);
psf_errno_t psfio_put_word (PSFIO *io, psf_word b);
psf_errno_t psfio_put_dword(PSFIO *io, psf_dword b);
psf_errno_t psfio_put_utf8 (PSFIO *io, psf_dword b);

/* libpsf.c */

psf_errno_t psf2_read(PSFIO *io);
psf_errno_t psf2_write(PSFIO *io);

