/***************************************************************************
 *                                                                         *
 *    LIBDSK: General floppy and diskimage access library                  *
 *    Copyright (C) 2003, 2017  John Elliott <seasip.webmaster@gmail.com>  *
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

#include "drvi.h"
#include "ldbs.h"

LDPUBLIC32 dsk_err_t LDPUBLIC16 dsk_copy(DSK_GEOMETRY *geom, 
				DSK_PDRIVER source, DSK_PDRIVER dest)
{
	dsk_err_t err;
	DRV_CLASS *dc;
	PLDBS temp = NULL;
	char *creator;
	DSK_GEOMETRY probed_geom;
	LDBS_DPB dpb;
	dsk_err_t (*src_to_ldbs)(DSK_DRIVER *self, struct ldbs **result, DSK_GEOMETRY *geom);
	dsk_err_t (*dest_from_ldbs)(DSK_DRIVER *self, struct ldbs *source, DSK_GEOMETRY *geom);

	/* Does source provide an export to LDBS format? */
	dc = source->dr_class;
	WALK_VTABLE(dc, dc_to_ldbs);
	src_to_ldbs = dc->dc_to_ldbs;
	/* And does destination provide an import from LDBS format? */
	dc = dest->dr_class;
	WALK_VTABLE(dc, dc_from_ldbs);
	dest_from_ldbs = dc->dc_from_ldbs;

	if (!src_to_ldbs || !dest_from_ldbs)
	{
		return DSK_ERR_NOTIMPL;	
	}
	dsk_report("Reading source file...");
	err = (*src_to_ldbs)(source, &temp, geom);
	if (err) 
	{
		dsk_report_end();
		if (temp) ldbs_close(&temp);
		return err;
	}
	dsk_report("Setting creator info");
	/* Set the creator if the exporting driver didn't */
	err = ldbs_get_creator(temp, &creator);
	if (err || !creator)
	{
		ldbs_put_creator(temp, "LIBDSK " LIBDSK_VERSION);
	}
	else ldbs_free(creator);
	/* If the exporting driver saved a geometry record, get it into
	 * probed_geom. If it didn't, probe the source drive and see if 
	 * it can give us any clues. */
	dsk_report("Checking geometry");
	err = ldbs_get_geometry(temp, &probed_geom);
	if (err || (probed_geom.dg_cylinders == 0 && 
			probed_geom.dg_heads == 0))
	{
		if (geom)	/* If a geometry was passed in, store that */
		{
			ldbs_put_geometry(temp, geom);
			memcpy(&probed_geom, geom, sizeof(probed_geom));
		}
		/* No geometry record present. Try to probe it */
		else if (!dsk_getgeom(source, &probed_geom))
		{
			/* Probe was successful; save to file */
			ldbs_put_geometry(temp, &probed_geom);
		}
	}
	/* If there is no CP/M DPB, see if the geometry probe (or caller) 
	 * provided enough information to populate one. */
	dsk_report("Checking DPB");
	err = ldbs_get_dpb(temp, &dpb);	

	/* If the DPB in the file is blank, and the geometry probe found a 
	 * CP/M filesystem, populate the DPB from that */
	if ((err || (dpb.spt == 0 && dpb.dsm == 0 && dpb.drm == 0)) && 
		probed_geom.dg_sectors && probed_geom.dg_secsize)
	{
		int v;

		memset(&dpb, 0, sizeof(dpb));
		dpb.psh = dsk_get_psh(probed_geom.dg_secsize);
		dpb.phm = (1 << dpb.psh) - 1;	
	
		dpb.spt = probed_geom.dg_sectors << dpb.psh;
		if (!dsk_get_option(source, "FS:CP/M:BSH", &v)) dpb.bsh = v;
		if (!dsk_get_option(source, "FS:CP/M:BLM", &v)) dpb.blm = v;
		if (!dsk_get_option(source, "FS:CP/M:EXM", &v)) dpb.exm = v;
		if (!dsk_get_option(source, "FS:CP/M:DSM", &v)) dpb.dsm = v;
		if (!dsk_get_option(source, "FS:CP/M:DRM", &v)) dpb.drm = v;
		if (!dsk_get_option(source, "FS:CP/M:AL0", &v)) dpb.al[0] = v;
		if (!dsk_get_option(source, "FS:CP/M:AL1", &v)) dpb.al[1] = v;
		if (!dsk_get_option(source, "FS:CP/M:CKS", &v)) dpb.cks = v;
		if (!dsk_get_option(source, "FS:CP/M:OFF", &v)) dpb.off = v;

		if (dpb.dsm && dpb.drm && dpb.spt)
		{
			ldbs_put_dpb(temp, &dpb);
		}	
	}			
	err = ldbs_sync(temp);
	if (!err)
	{
		dsk_report("Writing destination...");
		err = (*dest_from_ldbs)(dest, temp, geom);
	}
	dsk_report("Cleaning up");
	if (temp) ldbs_close(&temp);
	dsk_report_end();
	return err;
}
