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

/* This file prototypes functions to load and save fonts in the PSF format */

#ifndef PSFLIB_H_INCLUDED

#define PSFLIB_H_INCLUDED

#include <stddef.h>	/* For size_t */

#define PSF_MAGIC 0x0436	/* PSF file magic number */

typedef unsigned char  psf_byte;
typedef unsigned short psf_word;
typedef unsigned long  psf_dword;

typedef int psf_errno_t;

/* In-memory representation of a .PSF unicode directory entry. Rather than
 * malloc these one at a time, we malloc them in blocks and keep a linked
 * list of the blocks allocated (psf_unicode_buffer).
 *
 * The entries themselves also take the form of a linked list; there will
 * be 256 or 512 chains for characters, and one for free blocks.
 */
typedef struct psfu_entry
{
	struct psfu_entry *psfu_next;
	psf_word psfu_token;	
} psf_unicode_dirent;

#define PSF_ENTRIES_PER_BUFFER 32	/* Can be tuned */

typedef struct psfu_buffer
{	
	struct psfu_buffer *psfb_next;
	psf_unicode_dirent psfb_dirents[PSF_ENTRIES_PER_BUFFER];
} psf_unicode_buffer;

/* In-memory representation of a .PSF file */
typedef struct
{
	psf_word psf_magic;
	psf_byte psf_type;
	psf_byte psf_height;	
	psf_byte *psf_data;	
	psf_unicode_dirent **psf_dirents_used;
	psf_unicode_dirent *psf_dirents_free;
	psf_unicode_buffer *psf_dirents_buffer;
	size_t psf_dirents_nused;
	size_t psf_dirents_nfree;	/* total malloced is always used+free */
} PSF_FILE;

/* Initialise the structure as empty */
void psf_file_new(PSF_FILE *f);
/* Initialise the structure for a new font*/
psf_errno_t psf_file_create(PSF_FILE *f, psf_byte height, psf_byte type);
/* Add Unicode directory to a font */
psf_errno_t psf_file_create_unicode(PSF_FILE *f);

/* Add an entry to the Unicode directory */
psf_errno_t psf_unicode_add(PSF_FILE *f, psf_word nchar, psf_word ucs16);
/* Remove an entry from the Unicode directory */
psf_errno_t psf_unicode_delete(PSF_FILE *f, psf_word nchar, psf_word ucs16);

/* Load a PSF from memory */
psf_errno_t psf_memory_read(PSF_FILE *f, psf_byte *data);
/* Save a PSF to memory */
psf_errno_t psf_memory_write(PSF_FILE *f, psf_byte *data);
/* Load a PSF from a stream 
 * nb: If the file pointer is important to you, you have to save it. */
psf_errno_t psf_file_read  (PSF_FILE *f, FILE *fp);
/* Write PSF to stream */
psf_errno_t psf_file_write (PSF_FILE *f, FILE *fp);
/* Free any memory associated with a PSF file */
void psf_file_delete (PSF_FILE *f);
/* Remove any unicode directory from a PSF file */
void psf_file_delete_unicode(PSF_FILE *f);
/* Count characters in a font */
int psf_count_chars(PSF_FILE *f);
/* Expand error number returned by other routines, to string */
char *psf_error_string(psf_errno_t err);

#define psf_is_unicode(f)  ( ((f)->psf_type) & 2 )

/* errors */
#define PSF_E_OK        ( 0)	/* OK */
#define PSF_E_NOMEM	(-1)	/* Out of memory */
#define PSF_E_NOTIMPL	(-2)	/* Unsupported PSF file format */
#define PSF_E_NOTPSF	(-3)	/* Attempt to load a non-PSF file */
#define PSF_E_ERRNO	(-4)	/* Error in fread() or fwrite() */
#define PSF_E_EMPTY	(-5)	/* Attempt to save an empty file */
#define PSF_E_ASCII     (-6)    /* Not a Unicode PSF */

#include "psfutils.h"

#endif /* PSFLIB_H_INCLUDED */

