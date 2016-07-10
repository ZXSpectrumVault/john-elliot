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

/* Get/set pixel in a character bitmap */
psf_errno_t psf_get_pixel(PSF_FILE *f, psf_dword ch, psf_dword x, psf_dword y, psf_byte *pix)
{
	psf_byte * b;

	if (!f->psf_data) return PSF_E_EMPTY;
	*pix = 0;
	if (ch >= f->psf_length || x >= f->psf_width || y >= f->psf_height) 
		return -999;	/* XXX */	

	b = &f->psf_data[ch * f->psf_charlen];	
	b += (y * ((f->psf_width + 7)/8)) + (x/8);

	*pix = *b & (0x80 >> (x & 7));	
	return PSF_E_OK;
}


psf_errno_t psf_set_pixel(PSF_FILE *f, psf_dword ch, psf_dword x, psf_dword y, psf_byte pix)
{
	psf_byte * b;

	if (!f->psf_data) return PSF_E_EMPTY;
	if (ch >= f->psf_length || x >= f->psf_width || y >= f->psf_height) 
		return -999;	/* XXX */	

	b = &f->psf_data[ch * f->psf_charlen];	
	b += (y * ((f->psf_width + 7)/8)) + (x/8);

	if (pix) *b |= (0x80 >> (x & 7));	
	else     *b &= ~(0x80 >> (x & 7));
	return PSF_E_OK;
}

