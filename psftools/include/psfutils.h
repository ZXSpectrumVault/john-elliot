/*
    psftools: Manipulate console fonts in the .PSF format
    Copyright (C) 2000  John Elliott

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

/* psfutil1.c */
psf_byte flip(psf_byte b);			/* Reverse bits in a byte */
psf_byte crush(psf_byte b1);			/* 8 bits become 4 (the top 
						 * 4) */

/* These read/write functions are used to read/write 8-bit, 16-bit and 32-bit 
 * little-endian values from/to a file (the 8-bit ones are included for 
 * consistency). They all return 0 if successful and -1 if the read/write 
 * failed.
 */ 

/* psfutil2.c */
int write_byte (FILE *fp, psf_byte v);		/* 1 byte */
int write_word (FILE *fp, psf_word v);		/* 2 bytes */
int write_dword(FILE *fp, psf_dword v);		/* 4 bytes */

/* psfutil3.c */
int read_byte  (FILE *fp, psf_byte *v);		/* 1 byte */
int read_word  (FILE *fp, psf_word *v);		/* 2 bytes */
int read_dword (FILE *fp, psf_dword *v);	/* 4 bytes */
