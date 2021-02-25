/***************************************************************************
 *                                                                         *
 *    LIBDSK: General floppy and diskimage access library                  *
 *    Copyright (C) 2001, 2018  John Elliott <seasip.webmaster@gmail.com>  *
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

/* Declarations for the SAP driver. */

typedef struct
{
	LDBSDISK_DSK_DRIVER sap_super;
	char *sap_filename;

	/* These variables hold state when loading, and have no meaning
	 * outside sap_open() */
	DSK_GEOMETRY sap_geom;
	/* These variables hold state when saving, and have no meaning
	 * outside sap_close() */
	FILE *sap_fp;

} SAP_DSK_DRIVER;

dsk_err_t sap_open(DSK_DRIVER *self, const char *filename);
dsk_err_t sap_creat(DSK_DRIVER *self, const char *filename);
dsk_err_t sap_close(DSK_DRIVER *self);
dsk_err_t sap_getgeom(DSK_DRIVER *self, DSK_GEOMETRY *geom);
