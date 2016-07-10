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

int write_word(FILE *fp, psf_word x)
{
    if (fputc(  x       & 0xFF, fp) == EOF) return -1; 
    if (fputc( (x >> 8) & 0xFF, fp) == EOF) return -1; 
    return 0;
}

int write_dword(FILE *fp, psf_dword x)
{
    if (write_word(fp, (psf_word) ( x        & 0xFFFF))) return -1;
    if (write_word(fp, (psf_word) ((x >> 16) & 0xFFFF))) return -1;
    return 0;
}

int write_byte(FILE *fp, psf_byte x)
{
    return (fputc(x, fp) == EOF);
}


