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
#include "psfio.h"

/* Create and initialise a psf_unicode_buffer struct. Its entries will
 * form a linked list; the tail is psfb_dirents[0] and the head is 
 * psfb_dirents[PSF_ENTRIES_PER_BUFFER - 1] */
psf_unicode_buffer *psf_malloc_unicode_buffer(void)
{
	int n;
	psf_unicode_buffer *b = malloc(sizeof(psf_unicode_buffer));
	
	if (!b) return NULL;
	
	for (n = 1; n < PSF_ENTRIES_PER_BUFFER; n++) 
	{
		b->psfb_dirents[n].psfu_next = &b->psfb_dirents[n-1];
	}
	b->psfb_dirents[0].psfu_next = NULL;
	b->psfb_next = NULL;
	return b;
}

/* Initialise the Unicode structures of a font file */
psf_errno_t psf_file_create_unicode(PSF_FILE *f)
{
	int n, nchars = psf_count_chars(f);
/* Always allocate enough slots for a PSF1 to output correctly */
	if (nchars < 256) nchars = 256;
	if (nchars > 256 && nchars < 512) nchars = 512;	
	/* Clear out any existing unicode directory */
	psf_file_delete_unicode(f);

	f->psf_flags |= 1;
	f->psf_dirents_used = malloc(nchars * sizeof (psf_unicode_dirent *));
	if (!f->psf_dirents_used) return PSF_E_NOMEM;
	for (n = 0; n < nchars; n++) f->psf_dirents_used[n] = NULL;

	f->psf_dirents_buffer = psf_malloc_unicode_buffer();
	if (!f->psf_dirents_buffer) return PSF_E_NOMEM;
	f->psf_dirents_nused = 0;
	f->psf_dirents_nfree = PSF_ENTRIES_PER_BUFFER;
	f->psf_dirents_free  = 
	    &(f->psf_dirents_buffer->psfb_dirents[PSF_ENTRIES_PER_BUFFER - 1]);
	return PSF_E_OK;
}

/* Remove any unicode directory from a PSF file */
void psf_file_delete_unicode(PSF_FILE *f)
{
        psf_unicode_buffer *b, *bnew;
        if (f->psf_dirents_used) free(f->psf_dirents_used);

        b = f->psf_dirents_buffer;
        while(b)
        {
                bnew = b->psfb_next;
                free(b);
                b = bnew;
        }
        f->psf_dirents_used = NULL;
        f->psf_dirents_free = NULL;
        f->psf_dirents_buffer = NULL;
        f->psf_dirents_nused = 0;
        f->psf_dirents_nfree = 0;
        f->psf_flags &= ~1;
}

psf_errno_t psf_unicode_banned(psf_dword token)
{
	/* The range FDD0-FDEF aren't valid Unicode chars */
	if (token >= 0xFDD0 && token <= 0xFDEF) return PSF_E_BANNED;

	/* Any character ending FFFE or FFFF isn't valid. However, PSF
	 * allows FFFE to be written to the directory; it behaves as
	 * a separator. 
	if ((token & 0xFFFF) == 0xFFFE) return PSF_E_BANNED;
	*/
	if ((token & 0xFFFF) == 0xFFFF) return PSF_E_BANNED;

	return PSF_E_OK;
}

/* Add a chain to the Unicode directory from the map */
psf_errno_t psf_unicode_addmap(PSF_FILE *f, psf_word destchar,
	                                PSF_MAPPING *m, psf_word slot)
{
	psf_dword *pw;
	psf_errno_t rv;

	if (slot > 256) return PSF_E_OK;
       	pw = m->psfm_tokens[slot];
	if (!pw) return PSF_E_OK;

	while (*pw != 0xFFFF && *pw != 0x1FFFF)
	{
		rv = psf_unicode_add(f, destchar, *pw);
		if (rv) return rv;
		++pw;	
	}
	return PSF_E_OK;
}

/* Add a (char, ucs) pair to the unicode directory. Does not check if
 * the entry exists already */

psf_errno_t psf_unicode_add(PSF_FILE *f, psf_word nchar, psf_dword token)
{
	psf_unicode_dirent *ude, *ude2;

	if (nchar >= f->psf_length) return PSF_E_RANGE;
	if (!psf_is_unicode(f)) return PSF_E_ASCII;

	if (psf_unicode_banned(token)) return PSF_E_BANNED;

	if (!f->psf_dirents_nfree)	/* Need to allocate some more */
					/* memory for the Unicode directory */
	{
		psf_unicode_buffer *nbuf = psf_malloc_unicode_buffer();
		if (!nbuf) return PSF_E_NOMEM;		
		/* Prepend the new buffer to the buffer list */

		nbuf->psfb_next = f->psf_dirents_buffer;
		f->psf_dirents_buffer = nbuf;

		/* Add the new free entries to the free entries list, which 
		 * should be empty */
		 nbuf->psfb_dirents[0].psfu_next = f->psf_dirents_free;
		f->psf_dirents_free  = 
		    &(nbuf->psfb_dirents[PSF_ENTRIES_PER_BUFFER - 1]);	
		f->psf_dirents_nfree += PSF_ENTRIES_PER_BUFFER;	 
			
	}
	/* Construct the new entry and take it off the free list */
	ude = f->psf_dirents_free;
	f->psf_dirents_free = ude->psfu_next;
	ude->psfu_next  = NULL;
	ude->psfu_token = token;

	/* Update "used" and "free" counts */
	--f->psf_dirents_nfree;
	++f->psf_dirents_nused;
	/* Go to the end of the linked list of existing entries to append
         * the new one */
	ude2 = f->psf_dirents_used[nchar];
	if (!ude2) 
	{
		f->psf_dirents_used[nchar] = ude;	
		return PSF_E_OK;
	}
	else while (ude2->psfu_next) ude2 = ude2->psfu_next;
	ude2->psfu_next = ude;
	return PSF_E_OK;
}

/* Delete a (nchar, token) pair from the Unicode directory - if it's 
 * present */
psf_errno_t psf_unicode_delete(PSF_FILE *f, psf_word nchar, psf_dword token)
{
	psf_unicode_dirent *ude, *ude2, *ude3;
	
	if (!psf_is_unicode(f)) return PSF_E_ASCII;
	if (psf_unicode_banned(token)) return PSF_E_BANNED;

	ude  = f->psf_dirents_used[nchar];
	ude2 = NULL; 
	while (ude)
	{
		if (ude->psfu_token == token)
		{
			/* Take entry off "used" and add it to "free". */
			ude3 = ude->psfu_next;

			if (ude2) ude2->psfu_next = ude->psfu_next;
			else      f->psf_dirents_used[nchar] = ude->psfu_next;
			
			ude->psfu_next = f->psf_dirents_free;
			f->psf_dirents_free = ude;
			--f->psf_dirents_nused;
			++f->psf_dirents_nfree;
			ude = ude3;
			continue;
		}
		ude2 = ude;
		ude  = ude->psfu_next;
	}
	return PSF_E_OK;
	
}


psf_errno_t psf_unicode_lookupmap(PSF_FILE *f, PSF_MAPPING *m, psf_word slot, 
		psf_dword *nchar, psf_dword *found)
{
	psf_dword *pw;
	psf_errno_t rv;

	if (slot > 256) return PSF_E_NOTFOUND;
       	pw = m->psfm_tokens[slot];
	if (!pw) return PSF_E_NOTFOUND;

	while (*pw != 0xFFFF)
	{
		if (*pw == 0x1FFFF) { ++pw; continue; }
		rv = psf_unicode_lookup(f, *pw, nchar);
		if (!rv) 
		{
			if (found) *found = *pw;
			return rv;
		}
		++pw;
	}
	return PSF_E_NOTFOUND;	
}

/* Look up a token in the Unicode directory */
psf_errno_t psf_unicode_lookup(PSF_FILE *f, psf_dword token, psf_dword *nchar)
{
	psf_unicode_dirent *ude;
	psf_dword n;
	
	if (!psf_is_unicode(f)) return PSF_E_ASCII;
	if (psf_unicode_banned(token)) return PSF_E_BANNED;

	for (n = 0; n < f->psf_length; n++)
	{
		for (ude = f->psf_dirents_used[n]; 
		     ude != NULL;
		     ude = ude->psfu_next)
		{
/* Don't look in multibyte sequences, only single-byte */
			if (ude->psfu_token == 0xFFFE) break;
			if (ude->psfu_token == token) 
			{
				if (nchar) *nchar = n;
				return PSF_E_OK;
			}
		}
	}
	return PSF_E_NOTFOUND;
}
