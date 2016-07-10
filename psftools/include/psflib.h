/*
    psftools: Manipulate console fonts in the .PSF (v2) format
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

/* This file prototypes functions to load and save fonts in the PSF format 
 * (versions 1 and 2) */

#ifndef PSFLIB_H_INCLUDED

#define PSFLIB_H_INCLUDED

#define PSF_MAGIC 0x864ab572L	/* PSF file magic number */
#define PSF_MAGIC0     0x72
#define PSF_MAGIC1     0xb5
#define PSF_MAGIC2     0x4a
#define PSF_MAGIC3     0x86


#ifndef PSF1_MAGIC
#define PSF1_MAGIC 0x0436       /* PSF file magic number */

#define PSF_E_OK        ( 0)    /* OK */
#define PSF_E_NOMEM     (-1)    /* Out of memory */
#define PSF_E_NOTIMPL   (-2)    /* Unsupported PSF file format */
#define PSF_E_NOTPSF    (-3)    /* Attempt to load a non-PSF file */
#define PSF_E_ERRNO     (-4)    /* Error in fread() or fwrite() */
#define PSF_E_EMPTY     (-5)    /* Attempt to save an empty file */
#define PSF_E_ASCII     (-6)    /* Not a Unicode PSF */
#define PSF_E_UNICODE   (-7)    /* Failed to load Unicode directory */
#define PSF_E_V2        (-8)    /* PSF is in version 2 */
#define PSF_E_NOTFOUND  (-9)	/* Code point not found */
#define PSF_E_BANNED    (-10)	/* Invalid Unicode code point */
#define PSF_E_PARSE     (-11)   /* Invalid Unicode sequence string */
#define PSF_E_RANGE	(-12)	/* Character index out of range */

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
	psf_dword psfu_token;	
} psf_unicode_dirent;

#define PSF_ENTRIES_PER_BUFFER 32	/* Can be tuned */

typedef struct psfu_buffer
{	
	struct psfu_buffer *psfb_next;
	psf_unicode_dirent psfb_dirents[PSF_ENTRIES_PER_BUFFER];
} psf_unicode_buffer;

#endif

/* In-memory representation of a .PSF file (v1 or v2) */
typedef struct
{
	psf_dword psf_magic;
	psf_dword psf_version;
	psf_dword psf_hdrlen;	/* Size of header */
	psf_dword psf_flags;
	psf_dword psf_length;	/* Number of characters */	
	psf_dword psf_charlen;	/* Length of a character bitmap */
	psf_dword psf_height;	/* Height of a glyph */
	psf_dword psf_width;	/* Width of a glyph */	
	psf_byte *psf_data;	/* Font bitmaps */
	psf_unicode_dirent **psf_dirents_used;
	psf_unicode_dirent *psf_dirents_free;
	psf_unicode_buffer *psf_dirents_buffer;
	size_t psf_dirents_nused;
	size_t psf_dirents_nfree;	/* total malloced is always used+free */
} PSF_FILE;


typedef struct array
{
        psf_byte *data;
        unsigned long len;
} PSFIO_ARRAY;


/* Stream I/O support */
typedef struct psfio
{
        PSF_FILE *psf;
        int (*readfunc )(struct psfio *io);
        int (*writefunc)(struct psfio *io, psf_byte b);
        union
        {
                FILE *fp;
                PSFIO_ARRAY arr;
		void *general;
        } data;
} PSFIO;

typedef struct psf_mapping
{
	char *psfm_name;
	psf_dword *psfm_tokens[256];
} PSF_MAPPING;

/* Initialise the structure as empty */
void psf_file_new(PSF_FILE *f);
/* Initialise the structure for a new font*/
psf_errno_t psf_file_create(PSF_FILE *f, psf_dword width, psf_dword height,
				psf_dword nchars, psf_byte unicode);
/* Add Unicode directory to a font */
psf_errno_t psf_file_create_unicode(PSF_FILE *f);

/* Add an entry to the Unicode directory */
psf_errno_t psf_unicode_add(PSF_FILE *f, psf_word nchar, psf_dword token);
/* Add a chain to the Unicode directory from the map */
psf_errno_t psf_unicode_addmap(PSF_FILE *f, psf_word destchar, 
				PSF_MAPPING *m, psf_word srcchar);
/* Remove an entry from the Unicode directory */
psf_errno_t psf_unicode_delete(PSF_FILE *f, psf_word nchar, psf_dword token);
/* Find a token in the Unicode directory */
psf_errno_t psf_unicode_lookup(PSF_FILE *f, psf_dword token, psf_dword *nchar);
/* Find a token in the Unicode directory, based on chain slot in mapping m */
psf_errno_t psf_unicode_lookupmap(PSF_FILE *f, PSF_MAPPING *m, psf_word slot, psf_dword *nchar, psf_dword *found);
/* Is 'token' a character that can't go in the Unicode mapping table? */
psf_errno_t psf_unicode_banned(psf_dword token);
/* Convert a Unicode chain to a string */
psf_errno_t psf_unicode_to_string(psf_unicode_dirent *chain, char **str);
/* ... and back */
psf_errno_t psf_unicode_from_string(PSF_FILE *f, psf_word nchar, const char *str);


/* Allocate and initialise a psf_unicode_buffer */
psf_unicode_buffer *psf_malloc_unicode_buffer(void);

/* Load a PSF from memory */
psf_errno_t psf_memory_read(PSF_FILE *f, psf_byte *data, unsigned len);
/* Save a PSF to memory */
psf_errno_t psf_memory_write(PSF_FILE *f, psf_byte *data, unsigned len);
/* Load a PSF from a stream 
 * nb: If the file pointer is important to you, you have to save it. */
psf_errno_t psf_file_read  (PSF_FILE *f, FILE *fp);
/* Write PSF to stream */
psf_errno_t psf_file_write (PSF_FILE *f, FILE *fp);
/* Free any memory associated with a PSF file */
void psf_file_delete (PSF_FILE *f);
/* Remove any unicode directory from a PSF file */
void psf_file_delete_unicode(PSF_FILE *f);
/* Expand error number returned by other routines, to string */
char *psf_error_string(psf_errno_t err);

/* Find an ASCII->Unicode mapping */
PSF_MAPPING *psf_find_mapping(char *name);
void psf_list_mappings(FILE *fp);

/* Get/set pixel in a character bitmap */
psf_errno_t psf_get_pixel(PSF_FILE *f, psf_dword ch, psf_dword x, psf_dword y, psf_byte *pixel);
psf_errno_t psf_set_pixel(PSF_FILE *f, psf_dword ch, psf_dword x, psf_dword y, psf_byte pixel);

/* Input/output from/to generic streams */
psf_errno_t psf_read (PSFIO *io);
psf_errno_t psf_write(PSFIO *io);

/* Force a PSF file to be v1 or v2 */
psf_errno_t psf_force_v1(PSF_FILE *f);
psf_errno_t psf_force_v2(PSF_FILE *f);

#define psf_is_unicode(f)  ( ((f)->psf_flags) & 1 )
#define psf_count_chars(f) ( (f)->psf_length )
#include "psfutils.h"

#endif /* PSFLIB_H_INCLUDED */

