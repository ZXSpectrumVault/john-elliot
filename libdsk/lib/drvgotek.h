/***************************************************************************
 *                                                                         *
 *    LIBDSK: General floppy and diskimage access library                  *
 *    Copyright (C) 2001, 2019  John Elliott <seasip.webmaster@gmail.com>  *
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

/* Declarations for the Gotek driver */

typedef struct
{
        DSK_DRIVER gotek_super;
	char *gotek_filename;
        FILE *gotek_fp;
	int   gotek_readonly;
	unsigned long  gotek_filesize;
	unsigned long  gotek_base;
	unsigned long  gotek_gap;
/* If, under Windows, accessing a raw USB device directly */
#ifdef WIN32FLOPPY
	HANDLE	gotek_hVolume;	/* Handle for the partition in question */
	LPVOID  gotek_buffer;	/* Correctly-aligned transfer buffer */
#endif
} GOTEK_DSK_DRIVER;

dsk_err_t gotek_open(DSK_DRIVER *self, const char *filename);
dsk_err_t gotek_creat(DSK_DRIVER *self, const char *filename);
dsk_err_t gotek_close(DSK_DRIVER *self);
dsk_err_t gotek_read(DSK_DRIVER *self, const DSK_GEOMETRY *geom,
                              void *buf, dsk_pcyl_t cylinder,
                              dsk_phead_t head, dsk_psect_t sector);
dsk_err_t gotek_write(DSK_DRIVER *self, const DSK_GEOMETRY *geom,
                              const void *buf, dsk_pcyl_t cylinder,
                              dsk_phead_t head, dsk_psect_t sector);
dsk_err_t gotek_format(DSK_DRIVER *self, DSK_GEOMETRY *geom,
                                dsk_pcyl_t cylinder, dsk_phead_t head,
                                const DSK_FORMAT *format, unsigned char filler);
dsk_err_t gotek_xseek(DSK_DRIVER *self, const DSK_GEOMETRY *geom,
                                dsk_pcyl_t cylinder, dsk_phead_t head);
dsk_err_t gotek_status(DSK_DRIVER *self, const DSK_GEOMETRY *geom,
                                dsk_phead_t head, unsigned char *result);
dsk_err_t gotek_getgeom(DSK_DRIVER *self, DSK_GEOMETRY *geom);
dsk_err_t gotek_to_ldbs(DSK_DRIVER *self, struct ldbs **result, DSK_GEOMETRY *geom);
dsk_err_t gotek_from_ldbs(DSK_DRIVER *self, struct ldbs *source, DSK_GEOMETRY *geom);

