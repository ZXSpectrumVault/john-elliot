/***************************************************************************
 *                                                                         *
 *    LIBDSK: General floppy and diskimage access library                  *
 *    Copyright (C) 2001-2016  John Elliott <seasip.webmaster@gmail.com>   *
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

/* Declarations for the QRST driver. This is the first attempt at writing
 * an LDBS subclass */

typedef struct
{
        LDBSDISK_DSK_DRIVER qrst_super;
	char *qrst_filename;

	FILE *qrst_fp;
	/* The following variables hold state when saving, and are only
	 * used within qrst_close() */
	size_t qrst_tracklen;
	unsigned long qrst_bias;
	unsigned long qrst_checksum;

} QRST_DSK_DRIVER;

dsk_err_t qrst_open(DSK_DRIVER *self, const char *filename);
dsk_err_t qrst_creat(DSK_DRIVER *self, const char *filename);
dsk_err_t qrst_close(DSK_DRIVER *self);

