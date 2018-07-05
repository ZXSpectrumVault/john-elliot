/***************************************************************************
 *                                                                         *
 *    LIBDSK: General floppy and diskimage access library                  *
 *    Copyright (C) 2001  John Elliott <seasip.webmaster@gmail.com>            *
 *                                                                         *
 *    This library is free software; you can redistribute it and/or        *
 *    modify it under the terms of the GNU Library General Public          *
 *    License as published by the Free Software Foundation; either         *
 *    version 2 of the License, or (at your option) any later version.     *
 *                                                                         *
 *    This library is distributed in the hope that it will be useful,      *
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of       *
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU    *
 *    Library General Public License for more details.                     *
 *                                                                         *
 *    You should have received a copy of the GNU Library General Public    *
 *    License along with this library; if not, write to the Free           *
 *    Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,      *
 *    MA 02111-1307, USA                                                   *
 *                                                                         *
 ***************************************************************************/

/* Declarations for the CPCEMU driver */

typedef struct
{
        LDBSDISK_DSK_DRIVER cpc_super;
	char *cpc_filename;
	int cpc_extended;
        FILE *cpc_fp;
} CPCEMU_DSK_DRIVER;

/* v0.9.0: Use subclassing to create separate drivers for normal and 
 * extended .DSK images. This way we can create extended images by 
 * using "-type edsk" or similar */
dsk_err_t cpcext_open(DSK_DRIVER *self, const char *filename);
dsk_err_t cpcext_creat(DSK_DRIVER *self, const char *filename);
dsk_err_t cpcemu_open(DSK_DRIVER *self, const char *filename);
dsk_err_t cpcemu_creat(DSK_DRIVER *self, const char *filename);
dsk_err_t cpcemu_close(DSK_DRIVER *self);

