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
	f->psf_magic   = PSF_MAGIC;
	f->psf_version = 0;
	f->psf_hdrlen  = 32;
	f->psf_flags   = 0;
	f->psf_length  = 0;	
	f->psf_charlen = 0;
	f->psf_height  = 0;
	f->psf_width   = 0;
	f->psf_data    = NULL;
	
	f->psf_dirents_used = NULL;
	f->psf_dirents_free = NULL;
	f->psf_dirents_buffer = NULL;
	f->psf_dirents_nused = 0;
	f->psf_dirents_nfree = 0;

}


/* Initialise the structure for a new font 
 * height = height of characters, bytes
 * type   = 0 for 256 characters, 1 for 512, 2 for 256+Unicode, 3 for  
 *           512+Unicode */

psf_errno_t psf_file_create(PSF_FILE *f, psf_dword width, psf_dword height,
                                psf_dword nchars, psf_byte unicode)
{
	psf_dword charlen;

	psf_file_delete(f);
	charlen = height * ((width + 7)/8);
	f->psf_data = (psf_byte *)malloc(nchars * charlen);
	if (!f->psf_data) return PSF_E_NOMEM;
	memset(f->psf_data, 0, nchars * charlen);
	f->psf_height  = height;
	f->psf_width   = width;
	f->psf_charlen = charlen;
	f->psf_length  = nchars;
	if (unicode) return psf_file_create_unicode(f);
	return PSF_E_OK;
}

static int psf_read_ucs_dir(PSFIO *io, int cchar)
{
	psf_byte  utf8;
	psf_word  ucs16;
	psf_dword ucs32;
	int err;

	if (io->psf->psf_magic == PSF1_MAGIC)
	{
		if (psfio_get_word(io, &ucs16)) return PSF_E_UNICODE;
        	while (ucs16 != 0xFFFF)
	        {
       	 		err = psf_unicode_add(io->psf, cchar, ucs16);
	/* Drop banned characters silently */
			if (err == PSF_E_BANNED) err = PSF_E_OK;
    	   		if (err != PSF_E_OK) return err;
			if (psfio_get_word(io, &ucs16)) return PSF_E_UNICODE;
		}
		return PSF_E_OK;
	}
	if (psfio_get_byte(io, &utf8)) return PSF_E_UNICODE;
	while (utf8 != 0xFF)
	{
		if (utf8 == 0xFE) ucs32 = 0xFFFE;
		else if (psfio_get_utf8(io, utf8, &ucs32)) return PSF_E_UNICODE;
 		err = psf_unicode_add(io->psf, cchar, ucs32);
	/* Drop banned characters silently */
		if (err == PSF_E_BANNED) err = PSF_E_OK;
   		if (err != PSF_E_OK) return err;
		if (psfio_get_byte(io, &utf8)) return PSF_E_UNICODE;
	}	
	return PSF_E_OK;
}



/* Load a PSF from memory or disc*/
psf_errno_t psf_read(PSFIO *io)
{
        psf_byte *ndata;
        int err, cchar;
	int psfver = 0;
	psf_dword magic;
	psf_dword len, version, headersize, flags, nchars, charlen, h, w;

	/* Check the magic number. If this is PSF1, it loads the whole
	 * header into a dword. */
	err = psfio_get_dword(io, &magic); if (err) return err;

	if      (magic == PSF_MAGIC)            psfver = 2;
	else if ((magic & 0xFFFF) == PSF1_MAGIC) psfver = 1;
        else return PSF_E_NOTPSF;
	
        if (psfver == 1 && (magic & 0xFF0000L) > 0x30000L) return PSF_E_NOTIMPL;

	if (psfver == 1) 
	{
		version = 0;
		headersize = 4;
		flags = (magic & 0x020000L) ? 1 : 0;
		nchars = (magic & 0x010000L) ? 512 : 256;
		charlen = ((magic >> 24) & 0xFF);
		h = charlen;
		w = 8;	
		magic = PSF1_MAGIC;
	}
	else
	{
		if (psfio_get_dword(io, &version))    return PSF_E_NOTPSF;
		if (version > 0) return PSF_E_NOTIMPL;
		if (psfio_get_dword(io, &headersize)) return PSF_E_NOTPSF;
		if (psfio_get_dword(io, &flags))      return PSF_E_NOTPSF;
		if (psfio_get_dword(io, &nchars))     return PSF_E_NOTPSF;
		if (psfio_get_dword(io, &charlen))    return PSF_E_NOTPSF;
		if (psfio_get_dword(io, &h))          return PSF_E_NOTPSF;
		if (psfio_get_dword(io, &w))          return PSF_E_NOTPSF;
	}
	len = charlen * nchars;

        ndata = (psf_byte *)malloc(len);
        if (!ndata) return PSF_E_NOMEM;
        psf_file_delete(io->psf);

	/* OK. Font data allocated. Set up the headers */

	io->psf->psf_magic   = magic;
	io->psf->psf_version = version;
	io->psf->psf_hdrlen  = headersize;
	io->psf->psf_flags   = flags;
	io->psf->psf_length  = nchars;
	io->psf->psf_charlen = charlen;
	io->psf->psf_height  = h;
	io->psf->psf_width   = w;
        io->psf->psf_data   = ndata;

	/* Skip any extra header bits */
	while (headersize > 32)
	{
		err = psfio_get_byte(io, ndata);
		--headersize;
	}

	err = psfio_get_bytes(io, ndata, len);
	if (err) return PSF_E_ERRNO;
        if (!psf_is_unicode(io->psf)) return PSF_E_OK;

	err = psf_file_create_unicode(io->psf); 
	if (err != PSF_E_OK) return err; 
        cchar = 0;

        while (cchar < nchars)
        {
		err = psf_read_ucs_dir(io, cchar);
		if (err) return err;
        	++cchar; 
        }
        return PSF_E_OK;
}	


/* Write PSF to file */
int psf_write (PSFIO *io)
{
	unsigned len, nchars, n;
	int err, cchar;

	/* Empty PSF */
	if (!io->psf->psf_data || !io->psf->psf_height) return PSF_E_EMPTY;

	/* Write a v1 file */
	if (io->psf->psf_magic == PSF1_MAGIC)
	{
		psf_byte type = 0;

		if (io->psf->psf_length > 256) type = 1;
		if (io->psf->psf_flags & 1)    type |= 2;

		psfio_put_word(io,  io->psf->psf_magic);
		psfio_put_byte(io,  type);
		psfio_put_byte(io,  io->psf->psf_charlen);
		nchars = (io->psf->psf_length > 256) ? 512 : 256;
	}
	else
	{
		psfio_put_dword(io, io->psf->psf_magic);
		psfio_put_dword(io, io->psf->psf_version);
		psfio_put_dword(io, io->psf->psf_hdrlen);
		psfio_put_dword(io, io->psf->psf_flags);
		psfio_put_dword(io, io->psf->psf_length);
		psfio_put_dword(io, io->psf->psf_charlen);
		psfio_put_dword(io, io->psf->psf_height);
		psfio_put_dword(io, io->psf->psf_width);
		nchars = io->psf->psf_length;
	}
        len = io->psf->psf_charlen * io->psf->psf_length;
	/* Truncation for long v1 files. */
	/* 1.0.1: Hang on, that should be psf_length not psf_charlen! */
	if (nchars < io->psf->psf_length) len = nchars * io->psf->psf_charlen;

	/* Write the file */
	if (psfio_put_bytes(io, io->psf->psf_data, len)) return PSF_E_ERRNO;

	/* Packing for short v1 files */
	if (nchars > io->psf->psf_length)
	{
		len = (nchars - io->psf->psf_length)	* io->psf->psf_charlen;
		for (n = 0; n < len; n++)
			if (psfio_put_byte(io, 0)) return PSF_E_ERRNO;
	}

	/* Write Unicode directory */

	if (psf_is_unicode(io->psf))
	{
		nchars = psf_count_chars(io->psf);
		for (cchar = 0; cchar < nchars; cchar++)
		{
			psf_unicode_dirent *e = io->psf->psf_dirents_used[cchar];
		
			while(e)
			{
				if (io->psf->psf_magic == PSF1_MAGIC)
					err = psfio_put_word(io, e->psfu_token);
				else
				{
					if (e->psfu_token == 0xFFFE)
					     err = psfio_put_byte(io, 0xFE);
					else err = psfio_put_utf8(io, e->psfu_token);
				}
				if (err) return err;
				e = e->psfu_next;
			}
			if (io->psf->psf_magic == PSF1_MAGIC)
				err = psfio_put_word(io, 0xFFFF);
			else	err = psfio_put_byte(io, 0xFF);
			if (err) return err;
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





