/***************************************************************************
 *                                                                         *
 *    LIBDSK: General floppy and diskimage access library                  *
 *    Copyright (C) 2001-3, 2017  John Elliott <seasip.webmaster@gmail.com>*
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

/* Declarations for the APRIDISK driver. We handle this like the CFI 
 * driver, by decompressing into memory. Or rather, since libdsk 1.5.3,
 * into an LDBS blockstore. 
 */



typedef struct
{
	LDBSDISK_DSK_DRIVER adisk_super;
	char *adisk_filename;
	FILE *adisk_fp;
} ADISK_DSK_DRIVER;

dsk_err_t adisk_open(DSK_DRIVER *self, const char *filename);
dsk_err_t adisk_creat(DSK_DRIVER *self, const char *filename);
dsk_err_t adisk_close(DSK_DRIVER *self);
