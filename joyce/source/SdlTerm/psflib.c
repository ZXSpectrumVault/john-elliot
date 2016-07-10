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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef CPM
# ifndef __PACIFIC__
#  include <errno.h>
# endif
#endif
#include "psflib.h"

#ifdef CPM
/* Hi-Tech C for CP/M prototypes strerror(), but doesn't seem to 
 * provide it. Here it is. */

extern char *sys_err[];
extern int sys_ner;

char *strerror(int errno)
{
	if (errno < 0 || errno > sys_ner) return "Unknown error";
	return sys_err[errno];	
}

#endif

/* Initialise the structure as empty */
void psf_file_new(PSF_FILE *f)
{
	f->psf_magic  = PSF_MAGIC;
	f->psf_type   = 0;
	f->psf_height = 0;
	f->psf_data   = NULL;
	
	f->psf_dirents_used = NULL;
	f->psf_dirents_free = NULL;
	f->psf_dirents_buffer = NULL;
	f->psf_dirents_nused = 0;
	f->psf_dirents_nfree = 0;

}

/* Create and initialise a psf_unicode_buffer struct. Its entries will
 * form a linked list; the tail is psfb_dirents[0] and the head is 
 * psfb_dirents[PSF_ENTRIES_PER_BUFFER - 1] */
static psf_unicode_buffer *psf_malloc_unicode_buffer(void)
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
	
	/* Clear out any existing unicode directory */
	psf_file_delete_unicode(f);

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

/* Add a (char, ucs) pair to the unicode directory. Does not check if
 * the entry exists already */

psf_errno_t psf_unicode_add(PSF_FILE *f, psf_word nchar, psf_word ucs16)
{
	psf_unicode_dirent *ude, *ude2;
	
	if (!psf_is_unicode(f)) return PSF_E_ASCII;
	
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
	ude->psfu_token = ucs16;

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

/* Delete a (nchar, ucs16) pair from the Unicode directory - if it's 
 * present */
psf_errno_t psf_unicode_delete(PSF_FILE *f, psf_word nchar, psf_word ucs16)
{
	psf_unicode_dirent *ude, *ude2;
	
	if (!psf_is_unicode(f)) return PSF_E_ASCII;

	ude  = f->psf_dirents_used[nchar];
	ude2 = NULL; 
	while (ude)
	{
		if (ude->psfu_token == ucs16)
		{
			/* Take entry off "used" and add it to "free". */
			
			if (ude2) ude2->psfu_next = ude->psfu_next;
			else      f->psf_dirents_used[nchar] = ude->psfu_next;
			
			ude->psfu_next = f->psf_dirents_free;
			f->psf_dirents_free = ude;
			--f->psf_dirents_nused;
			++f->psf_dirents_nfree;
		}
		ude2 = ude;
		ude  = ude->psfu_next;
	}
	return PSF_E_OK;
	
}

/* Return number of characters in the file */
int psf_count_chars(PSF_FILE *f)
{
	return 256 * (1 + (f->psf_type & 1));
}

/* Initialise the structure for a new font 
 * height = height of characters, bytes
 * type   = 0 for 256 characters, 1 for 512, 2 for 256+Unicode, 3 for  
 *           512+Unicode */

int psf_file_create(PSF_FILE *f, psf_byte height, psf_byte type)
{
	if (type > 3) return PSF_E_NOTIMPL;

	psf_file_delete(f);
	f->psf_data = (psf_byte *)malloc(height * 256 * (1+(type&1)));
	if (!f->psf_data) return PSF_E_NOMEM;
	f->psf_type   = type;
	f->psf_height = height;
	if (type > 1) return psf_file_create_unicode(f);
	return PSF_E_OK;
}

/* Load a PSF from memory */
psf_errno_t psf_memory_read(PSF_FILE *f, psf_byte *data)
{
        psf_byte *ndata;
        int len, err, nchars, cchar;

	/* Check the magic number */
	if (data[0] != (PSF_MAGIC & 0xFF) ||
            data[1] != (PSF_MAGIC >> 8)) return PSF_E_NOTPSF;
        if (data[2] >  3) return PSF_E_NOTIMPL;

	/* Allocate space for font */
	nchars = 256 * (1 + (data[2] & 1));
        len = data[3] * nchars;
        ndata = (psf_byte *)malloc(len);
        if (!ndata) return PSF_E_NOMEM;

	memcpy(ndata, data + 4, len);
        /* OK, we've got a PSF loaded. Assign the header values
         * to our header copy. */
        psf_file_delete(f);
        f->psf_type   = data[2];
        f->psf_height = data[3];
        f->psf_data   = ndata;
        if (!psf_is_unicode(f)) return PSF_E_OK;

	err = psf_file_create_unicode(f); 
	if (err != PSF_E_OK) return err; 
        cchar = 0;
        ndata = data + 4 + len;
        while (cchar < nchars)
        {
        	psf_word ucs16 = ndata[0] + 256*ndata[1];
        	if (ucs16 != 0xFFFF)
        	{
        		err = psf_unicode_add(f, cchar, ucs16);
        		if (err != PSF_E_OK) return err;
        	}
        	else ++cchar; 
        }
        return PSF_E_OK;
}	

/* Save a PSF to memory */
psf_errno_t psf_memory_write(PSF_FILE *f, psf_byte *data)
{
	int nchars, cchar;
	int len;

        if (!f->psf_data || !f->psf_height) return PSF_E_EMPTY;
	/* Write out header */
        data[0] =  f->psf_magic       & 0xFF;
        data[1] = (f->psf_magic >> 8) & 0xFF;
        data[2] =  f->psf_type;
        data[3] =  f->psf_height;

	/* Write out data */
        len = f->psf_height * psf_count_chars(f);

	memcpy(data + 4, f->psf_data, len);

	data += (len + 4);
	/* Write Unicode directory */
	if (psf_is_unicode(f))
	{
		nchars = psf_count_chars(f);
		for (cchar = 0; cchar < nchars; cchar++)
		{
			psf_unicode_dirent *e = f->psf_dirents_used[cchar];
			
			while(e)
			{
				data[0] = e->psfu_token & 0xFF;
				data[1] = (e->psfu_token >> 8) & 0xFF;
				data += 2;
				e = e->psfu_next;
			}
			data[0] = 0xFF;
			data[1] = 0xFF;
			data += 2;
		}
	}


        return PSF_E_OK;
}


/* Load a PSF from a stream */
int psf_file_read  (PSF_FILE *f, FILE *fp)
{
	int err, cchar, nchars;
	psf_byte header[4];
	psf_byte *data;
	unsigned len;

	/* Read the 4-byte header */
	if (fread(header, 1, 4, fp) < 4)
	{
		if (ferror(fp)) return PSF_E_ERRNO;
		return PSF_E_NOTPSF;
	}
	/* Check the magic number */
	if (header[0] != (PSF_MAGIC & 0xFF) ||
            header[1] != (PSF_MAGIC >> 8)) return PSF_E_NOTPSF;
	if (header[2] >  3) return PSF_E_NOTIMPL;	

	/* Work out the file length & allocate in-memory image */
	nchars = 256 * (1 + (header[2] & 1));
	len = header[3] * nchars;
	data = (psf_byte *)malloc(len);
	if (!data) return PSF_E_NOMEM;

	/* Load the data */
	if (fread(data, 1, len, fp) < len) 
	{
		free(data);
		if (ferror(fp)) return PSF_E_ERRNO;
		return PSF_E_NOTPSF;
	}
	/* OK, we've got a PSF loaded. Only now do we assign the new values.
	 * Thus if a load fails, any data already present in the PSF will be
	 * left intact. */
	psf_file_delete(f);
	f->psf_type   = header[2];
	f->psf_height = header[3];
	f->psf_data   = data;


        if (!psf_is_unicode(f)) return PSF_E_OK;

	err = psf_file_create_unicode(f); 
	if (err != PSF_E_OK) return err; 
        cchar = 0;
        while (cchar < nchars)
        {
        	psf_word ucs16;
        	if (read_word(fp, &ucs16)) return PSF_E_ERRNO;
        	if (ucs16 != 0xFFFF)
        	{
        		err = psf_unicode_add(f, cchar, ucs16);
        		if (err != PSF_E_OK) return err;
        	}
        	else ++cchar; 
        }
        return PSF_E_OK;
}


/* Write PSF to stream */
int psf_file_write (PSF_FILE *f, FILE *fp)
{
	unsigned len;
	psf_byte header[4];
	int nchars, cchar;

	/* Empty PSF */
	if (!f->psf_data || !f->psf_height) return PSF_E_EMPTY;

	/* Generate the header */
	header[0] =  f->psf_magic       & 0xFF;
	header[1] = (f->psf_magic >> 8) & 0xFF;
	header[2] =  f->psf_type;
	header[3] =  f->psf_height;

	/* Work out data length */
        len = f->psf_height * psf_count_chars(f);

	/* Write the file */
	if (fwrite(header,      1, 4,   fp) < 4)   return PSF_E_ERRNO;
	if (fwrite(f->psf_data, 1, len, fp) < len) return PSF_E_ERRNO;

	/* Write Unicode directory */
	if (psf_is_unicode(f))
	{
		nchars = psf_count_chars(f);
		for (cchar = 0; cchar < nchars; cchar++)
		{
			psf_unicode_dirent *e = f->psf_dirents_used[cchar];
			
			while(e)
			{
				if (write_word(fp, e->psfu_token))
					return PSF_E_ERRNO;
				e = e->psfu_next;
			}
			if (write_word(fp, 0xFFFF)) return PSF_E_ERRNO;
		}
	}

	return PSF_E_OK;
}



/* Free any memory associated with a PSF file */
void psf_file_delete (PSF_FILE *f)
{
	psf_file_delete_unicode(f);
	if (f->psf_data) free(f->psf_data);
	psf_file_new(f);	/* Reset to empty */
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
}






/* Expand error number returned by other routines, to string */
char *psf_error_string(psf_errno_t err)
{
	switch(err)
	{
		case PSF_E_OK:		return "No error";
		case PSF_E_NOMEM:	return "Out of memory";
		case PSF_E_NOTIMPL:	return "Unknown PSF font file version";
		case PSF_E_NOTPSF:	return "File is not a PSF file";
		case PSF_E_EMPTY:	return "Attempt to save an empty file";
		case PSF_E_ASCII:	return "Not a Unicode PSF file";
		case PSF_E_ERRNO:	return strerror(errno);
	}
	return "Unknown error";
}

