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

#include "ldbs.h"

extern DRV_CLASS dc_ldbsdisk;

typedef struct
{
        DSK_DRIVER ld_super;		/* Base class */
	PLDBS ld_store;			/* Underlying block store */

	int   ld_readonly;
	int   ld_sector;		/* Last sector used for DD READ ID, or*/
       				 	/* -1 if valid last sector not seen */
	dsk_pcyl_t  ld_cur_cyl;		/* Cylinder / head currently loaded */
	dsk_phead_t ld_cur_head;	/* (if any) */
	LDBS_TRACKHEAD *ld_cur_track;	/* And the associated track */
	DSK_GEOMETRY ld_lastgeom;	/* Last geometry written */
	LDBS_DPB ld_dpb;		/* CP/M DPB */

} LDBSDISK_DSK_DRIVER;

/* For subclasses. The subclass should call ldbsdisk_attach() having
 * loaded a disk image into ld_store, and ldbsdisk_detach() before
 * attempting to write back the contents of ld_store */
dsk_err_t ldbsdisk_attach(DSK_DRIVER *self);
dsk_err_t ldbsdisk_detach(DSK_DRIVER *self);

dsk_err_t ldbsdisk_open(DSK_DRIVER *self, const char *filename);
dsk_err_t ldbsdisk_creat(DSK_DRIVER *self, const char *filename);
dsk_err_t ldbsdisk_close(DSK_DRIVER *self);
dsk_err_t ldbsdisk_read(DSK_DRIVER *self, const DSK_GEOMETRY *geom,
                              void *buf, dsk_pcyl_t cylinder,
                              dsk_phead_t head, dsk_psect_t sector);
dsk_err_t ldbsdisk_write(DSK_DRIVER *self, const DSK_GEOMETRY *geom,
                              const void *buf, dsk_pcyl_t cylinder,
                              dsk_phead_t head, dsk_psect_t sector);
dsk_err_t ldbsdisk_format(DSK_DRIVER *self, DSK_GEOMETRY *geom,
                                dsk_pcyl_t cylinder, dsk_phead_t head,
                                const DSK_FORMAT *format, unsigned char filler);
dsk_err_t ldbsdisk_getgeom(DSK_DRIVER *self, DSK_GEOMETRY *geom);
dsk_err_t ldbsdisk_secid(DSK_DRIVER *self, const DSK_GEOMETRY *geom,
                                dsk_pcyl_t cylinder, dsk_phead_t head,
                                DSK_FORMAT *result);
dsk_err_t ldbsdisk_xseek(DSK_DRIVER *self, const DSK_GEOMETRY *geom,
                                dsk_pcyl_t cylinder, dsk_phead_t head);
dsk_err_t ldbsdisk_xread(DSK_DRIVER *self, const DSK_GEOMETRY *geom, void *buf,
                              dsk_pcyl_t cylinder, dsk_phead_t head,
                              dsk_pcyl_t cyl_expected, dsk_phead_t head_expected,
                              dsk_psect_t sector, size_t sector_size, int *deleted);
dsk_err_t ldbsdisk_xwrite(DSK_DRIVER *self, const DSK_GEOMETRY *geom, const void *buf,
                              dsk_pcyl_t cylinder, dsk_phead_t head,
                              dsk_pcyl_t cyl_expected, dsk_phead_t head_expected,
                              dsk_psect_t sector, size_t sector_size, int deleted);
dsk_err_t ldbsdisk_trackids(DSK_DRIVER *self, const DSK_GEOMETRY *geom,
                            dsk_pcyl_t cylinder, dsk_phead_t head,
                            dsk_psect_t *count, DSK_FORMAT **result);
dsk_err_t ldbsdisk_status(DSK_DRIVER *self, const DSK_GEOMETRY *geom,
                  dsk_phead_t head, unsigned char *result);

dsk_err_t ldbsdisk_option_enum(DSK_DRIVER *self, int idx, char **optname);

dsk_err_t ldbsdisk_option_set(DSK_DRIVER *self, const char *optname, int value);
dsk_err_t ldbsdisk_option_get(DSK_DRIVER *self, const char *optname, int *value);

/* Convert to LDBS format. */
dsk_err_t ldbsdisk_to_ldbs(DSK_DRIVER *self, struct ldbs **result, DSK_GEOMETRY *geom);

/* Convert from LDBS format. */
dsk_err_t ldbsdisk_from_ldbs(DSK_DRIVER *self, struct ldbs *source, DSK_GEOMETRY *geom);

