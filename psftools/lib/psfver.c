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

/* Force a PSF file to be v1 or v2 */
psf_errno_t psf_force_v1(PSF_FILE *f)
{
	psf_byte *ndata;
	int nc, srccount, dstcount, y, wb;
	psf_unicode_dirent **de, *ude, *ude2;

	if (f->psf_magic == PSF1_MAGIC) return PSF_E_OK;
	if (!f->psf_data || !f->psf_height) return PSF_E_EMPTY;

	dstcount = 256;
	if (f->psf_length > 256) dstcount = 512;

	srccount = f->psf_length;
	/* dstcount = number of chars in PSF1 file */
	/* srccount = number of chars in PSF2 file */

	ndata = malloc(dstcount * f->psf_height);
	memset(ndata, 0, dstcount * f->psf_height);

	if (!ndata) return PSF_E_NOMEM;
	if (f->psf_flags & 1)	/* It's unicode */
	{
/* v1.0.1: Resize the unicode directory to match the font. Firstly, 
 * allocate a new set of pointers with the right number of characters. */
		de = malloc(dstcount * sizeof(psf_unicode_dirent *));
		if (!de)
		{
			free(ndata);
			return PSF_E_NOMEM;
		}
/* Secondly, copy across the pointers we're going to keep */
		for (nc = 0; nc < dstcount; nc++)
		{
			if (nc < srccount)
			{
				de[nc] = f->psf_dirents_used[nc];
			}
			else
			{
				de[nc] = NULL;
			}
		}
/* Thirdly, free the ones we aren't.  */
		for (; nc < srccount; nc++)
		{
			ude = f->psf_dirents_used[nc]; 	
			while (ude)
			{
/* Move entry to the free list, and adjust counts appropriately */
				ude2 = ude->psfu_next;
				ude->psfu_next = f->psf_dirents_free;
				f->psf_dirents_free = ude;
				--f->psf_dirents_nused;
				++f->psf_dirents_nfree;
				ude = ude2;
			}
		}
/* Finally, swap in the new list */
		free(f->psf_dirents_used);
		f->psf_dirents_used = de;
	}
/* Now force the characters to be 1 byte wide */
	wb = (f->psf_width + 7) / 8;
/* Copy such character bitmaps as fit */
	for (nc = 0; nc < srccount; nc++)
	{
		if (nc >= dstcount) break;
		for (y = 0; y < f->psf_height; y++)
		{
			ndata[f->psf_height * nc + y] =
				f->psf_data[f->psf_charlen * nc + y * wb]; 
		}
	}
/* Zero any extra character bitmaps */
	for (; nc < dstcount; nc++)
	{
		for (y = 0; y < f->psf_height; y++)
		{
			ndata[f->psf_height * nc + y] = 0;
		}
	}
/* Swap in the new font data, and tamper with the header */
	free(f->psf_data);
	f->psf_data = ndata;
	f->psf_magic = PSF1_MAGIC;
	f->psf_version = 0;
	f->psf_hdrlen = 4;
	f->psf_length = dstcount;
	f->psf_charlen = f->psf_height;
	f->psf_width = 8;

	return PSF_E_OK;
}



psf_errno_t psf_force_v2(PSF_FILE *f)
{
	f->psf_magic = PSF_MAGIC;
	f->psf_version = 0;
	f->psf_hdrlen = 32;
	return PSF_E_OK;
}

