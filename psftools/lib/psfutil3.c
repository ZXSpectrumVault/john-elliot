/*
    psftools: Manipulate console fonts in the .PSF format
    Copyright (C) 2000, 2007  John Elliott

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
#include "psflib.h"

int read_word(FILE *fp, psf_word *x)
{
    psf_byte h,l;

    if (read_byte(fp, &l)) return -1;
    if (read_byte(fp, &h)) return -1;

    *x = ((psf_word)h) << 8 | l;
    return 0;
}

int read_dword(FILE *fp, psf_dword *x)
{
    psf_word h,l;

    if (read_word(fp, &l)) return -1;
    if (read_word(fp, &h)) return -1;
    *x = ((psf_dword)h) << 16 | l;
    return 0;
}

int read_byte(FILE *fp, psf_byte *x)
{
    int b = fgetc(fp);
    if (b == EOF) return -1;
    *x = (psf_byte)b;
    return 0;
}


