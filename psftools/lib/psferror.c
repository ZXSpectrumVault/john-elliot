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
#include <stdlib.h>
#include <string.h>
#include "config.h"
#ifdef HAVE_ERRNO_H
# include <errno.h>
#endif
#include "psflib.h"


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
		case PSF_E_V2:		return "This program does not support the PSF2 file format";
		case PSF_E_NOTFOUND:	return "Code point not found";
		case PSF_E_BANNED:	return "Code point is not permitted for interchange";
		case PSF_E_PARSE:	return "Unicode string is not valid";
		case PSF_E_RANGE:	return "Character index out of range";
		case PSF_E_ERRNO:	return strerror(errno);
	}
	return "Unknown error";
}

