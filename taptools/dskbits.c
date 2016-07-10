/************************************************************************

    TAPTOOLS v1.1.0 - Tapefile manipulation utilities

    Copyright (C) 1996, 2014  John Elliott <jce@seasip.demon.co.uk>

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

*************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "config.h"
#include "dskbits.h"

#ifndef HAVE_LIBDSK_H

#ifndef SEEK_SET
#define SEEK_SET 0
#endif


dsk_err_t dsk_creat(DSK_PDRIVER *self, const char *name, const char *type,
		const char *compress)
{
	FILE *fp = fopen(name, "wb");
	*self = fp;
	if (!fp) return DSK_ERR_SYSERR;
	return DSK_ERR_OK;
}

dsk_err_t dsk_open(DSK_PDRIVER *self, const char *name, const char *type,
		const char *compress)
{
	FILE *fp = fopen(name, "rb");
	*self = fp;
	if (!fp) return DSK_ERR_SYSERR;
	return DSK_ERR_OK;
}


dsk_err_t dg_pcwgeom(DSK_GEOMETRY *dg, const unsigned char *bootsec)
{
	dg->dg_sectors = bootsec[3];
	dg->dg_secsize = 128 << bootsec[4];
	return DSK_ERR_OK;
}

dsk_err_t dsk_alform(DSK_PDRIVER fp, DSK_GEOMETRY *dg, dsk_ltrack_t track, 
		unsigned char filler)
{
	int n;
	fseek(fp, track * dg->dg_secsize * dg->dg_sectors, SEEK_SET);
	for (n = 0; n < (dg->dg_secsize * dg->dg_sectors); n++) 
	{
		if (fputc(filler, fp) == EOF) return DSK_ERR_SYSERR;	
	}
	return DSK_ERR_OK;
}

dsk_err_t dsk_lwrite(DSK_PDRIVER fp, DSK_GEOMETRY *dg, void *buf, int sector)
{
	fseek(fp, sector * dg->dg_secsize, SEEK_SET);
	if (fwrite(buf, 1, dg->dg_secsize, fp) < dg->dg_secsize) 
		return DSK_ERR_SYSERR;
	return DSK_ERR_OK;
}

const char *dsk_strerror(int err)
{
	switch(err)
	{
		case DSK_ERR_NOMEM:  return "Out of memory.";
		case DSK_ERR_BADPTR: return "Bad pointer.";
		case DSK_ERR_SYSERR: return strerror(errno);
		case DSK_ERR_OK:     return "No error.";
	}
	return "Unknown error.";
}

dsk_err_t dsk_close(DSK_PDRIVER *fp)
{
	fclose(*fp);
	*fp = NULL;
	return DSK_ERR_OK;
}

void dsk_reportfunc_set(void *f1, void *f2) { }
void dsk_report(const char *s) { }
void dsk_report_end() { }

dsk_err_t dg_stformat(DSK_GEOMETRY *self, dsk_format_t *fmt,
			dsk_cchar_t *s1, dsk_cchar_t *s2)
{
	return DSK_ERR_BADPTR;
}

dsk_err_t dsk_set_retry(DSK_PDRIVER fp, int tries) { return DSK_ERR_OK; }
dsk_err_t dsk_set_option(DSK_PDRIVER fp, 
		const char *opt, int value) { return DSK_ERR_OK; }

dsk_err_t dsk_getgeom(DSK_PDRIVER fp, DSK_GEOMETRY *dg)
{
	return DSK_ERR_BADPTR;
}


unsigned char dsk_get_psh(size_t secsize)
{
        unsigned char psh;

        for (psh = 0; secsize > 128; secsize /= 2) psh++;
        return psh;
}

dsk_err_t dg_stdformat(DSK_GEOMETRY *self, dsk_format_t formatid,
	dsk_cchar_t *name, dsk_cchar_t *desc)
{
	return DSK_ERR_BADPTR;
}

#endif
