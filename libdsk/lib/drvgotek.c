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

/* Access a Gotek-formatted USB key. This stores flat 1.4Mb disk images at 
 * 0x1800000-byte intervals.  */

#include <stdio.h>
#include <assert.h>
#include "libdsk.h"
#include "drvi.h"
#include "ldbs.h"
#include "drvgotek.h"
#include "drvwin32.h"	/* For Win32 raw device access */

#ifdef WIN32FLOPPY
#include <winioctl.h>
#endif

DRV_CLASS dc_gotek = 
{
	sizeof(GOTEK_DSK_DRIVER),
	NULL,		/* superclass */
	"gotek\0gotek1440\0",
	"Gotek disc image collection ",
	gotek_open,	/* open */
	gotek_creat,	/* create new */
	gotek_close,	/* close */
	gotek_read,	/* read sector, working from physical address */
	gotek_write,	/* write sector, working from physical address */
	gotek_format,	/* format track, physical */
	NULL,		/* get geometry */
	NULL,		/* sector ID */
	gotek_xseek,	/* seek to track */
	gotek_status,	/* drive status */
	NULL, 		/* xread */
	NULL, 		/* xwrite */
	NULL, 		/* tread */
	NULL, 		/* xtread */
	NULL,		/* option_enum */
	NULL,		/* option_set */
	NULL,		/* option_get */
	NULL,		/* trackids */
	NULL,		/* rtread */
	gotek_to_ldbs,	/* export as LDBS */
	gotek_from_ldbs	/* import as LDBS */
};


#define CHECK_CLASS(s) \
	if (s->dr_class != &dc_gotek)  return DSK_ERR_BADPTR; \
						\
	gxself = (GOTEK_DSK_DRIVER *)s;



static dsk_err_t gotek_open_create(DSK_DRIVER *self, const char *filename, int create)
{
	GOTEK_DSK_DRIVER *gxself;
	char *p;
	
	/* Sanity check: Is this meant for our driver? */
	CHECK_CLASS(self);

	/* Filename passed is of the format: gotek:device,number
 	 * where device is the file (expected to be a device special) 
	 * containing the images, and number is 0-999 */
	if (strncmp(filename, "gotek:", 6) && strncmp(filename, "gotek144:", 9))
		return DSK_ERR_NOTME;

	/* Filename begins "gotek:", so try to parse it */
	gxself->gotek_filename = dsk_malloc(30 + strlen(filename));
	if (!gxself->gotek_filename) return DSK_ERR_NOMEM;

	if (!strncmp(filename, "gotek:", 6))
	{
		strcpy(gxself->gotek_filename, filename + 6);
	}
	else if (!strncmp(filename, "gotek144:", 9))
	{
		strcpy(gxself->gotek_filename, filename + 9);
	}
	else	strcpy(gxself->gotek_filename, filename); /* Shouldn't happen */
	p = strchr(gxself->gotek_filename, ',');
	if (p == NULL || !isdigit(p[1]) || atoi(p + 1) < 0 || atoi(p + 1) > 999)
	{
		dsk_free(gxself->gotek_filename);
		return DSK_ERR_NOTME;
	}
	*p = 0;
	++p;
	gxself->gotek_gap  = 0x180000L;	/* Gap between each disk image */
	gxself->gotek_base = gxself->gotek_gap * atoi(p);
	gxself->gotek_fp   = NULL;

#ifdef WIN32FLOPPY
	gxself->gotek_hVolume = INVALID_HANDLE_VALUE;

	if (win32_is_device(gxself->gotek_filename))
	{
		__int64 size;
		DISK_GEOMETRY dg;

		gxself->gotek_buffer = VirtualAlloc(NULL, 512, MEM_COMMIT, PAGE_READWRITE);
		if (!gxself->gotek_buffer)
		{
			dsk_free(gxself->gotek_filename);
			return DSK_ERR_NOMEM;
		}
/* Open the device we've been passed - probably the first image on the 
 * Gotek, treated as a volume */
		gxself->gotek_hVolume = win32_open_device
			(gxself->gotek_filename, &gxself->gotek_readonly);
		if (INVALID_HANDLE_VALUE == gxself->gotek_hVolume)
		{
			dsk_free(gxself->gotek_filename);
			VirtualFree(gxself->gotek_buffer, 0, MEM_RELEASE);
			return DSK_ERR_NOTME;
		}
/* By default access is limited only to those sectors covered by the 
 * volume (ie, the first 1.4Mb). Remove that restriction */
		{
#ifndef FSCTL_ALLOW_EXTENDED_DASD_IO
#define FSCTL_ALLOW_EXTENDED_DASD_IO 0x00090083
#endif
			DWORD dwBytesRead;

			if (!DeviceIoControl(gxself->gotek_hVolume, 
				FSCTL_ALLOW_EXTENDED_DASD_IO, NULL, 0, 
				NULL, 0, &dwBytesRead, NULL))
			{
				CloseHandle(gxself->gotek_hVolume);
				dsk_free(gxself->gotek_filename);
				VirtualFree(gxself->gotek_buffer, 0, MEM_RELEASE);
				return DSK_ERR_NOTME;
			}
		}
		if (!win32_lock_volume(gxself->gotek_hVolume))
		{
			CloseHandle(gxself->gotek_hVolume);
			dsk_free(gxself->gotek_filename);
			VirtualFree(gxself->gotek_buffer, 0, MEM_RELEASE);
			return DSK_ERR_ACCESS;
		}
		/* Get the capacity of the drive (rather than the filesystem on it, which 
		 * is most likely that of a single 1.4M floppy image) */
		if (!win32_get_geometry(gxself->gotek_hVolume, &dg))
		{
			win32_unlock_volume(gxself->gotek_hVolume);
			CloseHandle(gxself->gotek_hVolume);
			dsk_free(gxself->gotek_filename);
			VirtualFree(gxself->gotek_buffer, 0, MEM_RELEASE);
			return DSK_ERR_ACCESS;
		}
		/* Calculate number of sectors in partition */
		size = (dg.TracksPerCylinder * dg.SectorsPerTrack * dg.Cylinders.QuadPart);
		/* Cap at enough sectors to hold 1000 disk images */
		if (size > (gxself->gotek_gap / dg.BytesPerSector * 1000))
		{
			size = (gxself->gotek_gap / dg.BytesPerSector * 1000);
		}
		size *= dg.BytesPerSector;
	
		gxself->gotek_filesize = (long)size;
		return DSK_ERR_OK;
	}

#endif

	/* Start by trying to open the file read/write. If that fails,
 	 * open read-only. If that fails, create it. */	
	gxself->gotek_fp = fopen(gxself->gotek_filename, "r+b");
	if (!gxself->gotek_fp) 
	{
		gxself->gotek_readonly = 1;
		gxself->gotek_fp = fopen(filename, "rb");
	}
	if (create && !gxself->gotek_fp)
	{
		gxself->gotek_fp = fopen(gxself->gotek_filename, "w+b");
	}
	if (!gxself->gotek_fp) 
	{
		dsk_free(gxself->gotek_filename);
		return DSK_ERR_NOTME;
	}
        if (fseek(gxself->gotek_fp, 0, SEEK_END)) return DSK_ERR_SYSERR;
        gxself->gotek_filesize = ftell(gxself->gotek_fp);

	return DSK_ERR_OK;
}

dsk_err_t gotek_open(DSK_DRIVER *self, const char *filename)
{
	return gotek_open_create(self, filename, 0);
}

dsk_err_t gotek_creat(DSK_DRIVER *self, const char *filename)
{
	return gotek_open_create(self, filename, 1);
}




dsk_err_t gotek_close(DSK_DRIVER *self)
{
	GOTEK_DSK_DRIVER *gxself;

	CHECK_CLASS(self);

	dsk_free(gxself->gotek_filename);
	gxself->gotek_filename = NULL;
#ifdef WIN32FLOPPY
	if (gxself->gotek_hVolume != INVALID_HANDLE_VALUE)
	{
		win32_dismount_volume(gxself->gotek_hVolume);
		win32_unlock_volume(gxself->gotek_hVolume);
		CloseHandle(gxself->gotek_hVolume);
		gxself->gotek_hVolume = INVALID_HANDLE_VALUE;	
		VirtualFree(gxself->gotek_buffer, 0, MEM_RELEASE);
	}
#endif
	if (gxself->gotek_fp) 
	{
		if (fclose(gxself->gotek_fp) == EOF) return DSK_ERR_SYSERR;
		gxself->gotek_fp = NULL;
	}
	return DSK_ERR_OK;	
}


static unsigned long gotek_offset(GOTEK_DSK_DRIVER *gxself, 
			dsk_pcyl_t cylinder, dsk_phead_t head, 
			dsk_psect_t sector)
{
	unsigned long offset = 0;

	/* Sectors in a Gotek image are stored in a fixed geometry 
	 * (18 x 512-byte sectors numbered 1-18, SIDES_ALT) */
	offset = (cylinder * 2) + head;
	offset *= 18;
	if (sector >= 1) /* which it should always be */ 
		offset += (sector - 1);
	offset *= 512;
	offset += gxself->gotek_base;
	return offset;
}

dsk_err_t gotek_read(DSK_DRIVER *self, const DSK_GEOMETRY *geom,
                             void *buf, dsk_pcyl_t cylinder,
                              dsk_phead_t head, dsk_psect_t sector)
{
	GOTEK_DSK_DRIVER *gxself;
	unsigned long offset;
	size_t secsize = geom->dg_secsize;

	if (!buf || !self || !geom) return DSK_ERR_BADPTR;
	CHECK_CLASS(self);


	offset = gotek_offset(gxself, cylinder, head, sector);

#ifdef WIN32FLOPPY
	if (gxself->gotek_hVolume != INVALID_HANDLE_VALUE)
	{
		DWORD bytesread;
		int res;

		if (SetFilePointer(gxself->gotek_hVolume, offset, NULL,
			FILE_BEGIN) == INVALID_FILE_SIZE) return DSK_ERR_SYSERR;
		res = ReadFile(gxself->gotek_hVolume, gxself->gotek_buffer,
			512, &bytesread, NULL);
		if (!res) return DSK_ERR_NOADDR;
		if (bytesread >= geom->dg_secsize) bytesread = geom->dg_secsize;
		memcpy(buf, gxself->gotek_buffer, bytesread);
		if (bytesread < geom->dg_secsize) return DSK_ERR_DATAERR;
		return DSK_ERR_OK;
	}
#endif

	if (!gxself->gotek_fp) return DSK_ERR_NOTRDY;
	if (fseek(gxself->gotek_fp, offset, SEEK_SET)) return DSK_ERR_SYSERR;

	if (secsize > 512) secsize = 512;
	if (fread(buf, 1, secsize, gxself->gotek_fp) < geom->dg_secsize)
	{
		return DSK_ERR_NOADDR;
	}
	if (geom->dg_secsize > 512) return DSK_ERR_DATAERR;
	return DSK_ERR_OK;
}


static dsk_err_t seekto(GOTEK_DSK_DRIVER *self, unsigned long offset)
{
#ifdef WIN32FLOPPY
	if (self->gotek_hVolume != INVALID_HANDLE_VALUE) 
	{
		if (SetFilePointer(self->gotek_hVolume, offset, NULL,
			FILE_BEGIN) == INVALID_FILE_SIZE) return DSK_ERR_SYSERR;
		return DSK_ERR_OK;
	}
#endif
	/* 0.9.5: Fill any "holes" in the file with 0xE5. Otherwise, UNIX would
	 * fill them with zeroes and Windows would fill them with whatever
	 * happened to be lying around */
	
	if (self->gotek_filesize < offset)
	{
		if (fseek(self->gotek_fp, self->gotek_filesize, SEEK_SET)) 
			return DSK_ERR_SYSERR;
		while (self->gotek_filesize < offset)
		{
			if (fputc(0xE5, self->gotek_fp) == EOF) return DSK_ERR_SYSERR;
			++self->gotek_filesize;
		}
	}
	if (fseek(self->gotek_fp, offset, SEEK_SET)) return DSK_ERR_SYSERR;
	return DSK_ERR_OK;
}

dsk_err_t gotek_write(DSK_DRIVER *self, const DSK_GEOMETRY *geom,
                             const void *buf, dsk_pcyl_t cylinder,
                              dsk_phead_t head, dsk_psect_t sector)
{
	GOTEK_DSK_DRIVER *gxself;
	unsigned long offset;
	dsk_err_t err;
	size_t secsize;

	if (!buf || !self || !geom) return DSK_ERR_BADPTR;

	CHECK_CLASS(self);

	if (gxself->gotek_readonly) return DSK_ERR_RDONLY;
	if (sector < 1 || sector > 18)
		return DSK_ERR_NOADDR;

	offset = gotek_offset(gxself, cylinder, head, sector);

	err = seekto(gxself, offset);
	if (err) return err;

	secsize = geom->dg_secsize;
	if (secsize > 512) secsize = 512;

#ifdef WIN32FLOPPY
	if (gxself->gotek_hVolume != INVALID_HANDLE_VALUE)
	{
		DWORD byteswritten;
		int res;

		memset(gxself->gotek_buffer, 0xE5, 512);
		memcpy(gxself->gotek_buffer, buf, secsize);
		res = WriteFile(gxself->gotek_hVolume, gxself->gotek_buffer,
			512, &byteswritten, NULL);
		if (!res || byteswritten < 512) return DSK_ERR_NOADDR;
		return DSK_ERR_OK;
	}
#endif

	if (!gxself->gotek_fp) return DSK_ERR_NOTRDY;
	if (fwrite(buf, 1, secsize, gxself->gotek_fp) < secsize)
	{
		return DSK_ERR_NOADDR;
	}
	if (gxself->gotek_filesize < offset + secsize)
		gxself->gotek_filesize = offset + secsize;
	return DSK_ERR_OK;
}


dsk_err_t gotek_format(DSK_DRIVER *self, DSK_GEOMETRY *geom,
                                dsk_pcyl_t cylinder, dsk_phead_t head,
                                const DSK_FORMAT *format, unsigned char filler)
{
/*
 * Note that we completely ignore the "format" parameter, since Gotek
 * images have a fixed format.
 */
	GOTEK_DSK_DRIVER *gxself;
	unsigned long offset;
	unsigned long trklen;
	dsk_err_t err;

   (void)format;
	if (!self || !geom) return DSK_ERR_BADPTR;
	CHECK_CLASS(self);

	if (gxself->gotek_readonly) return DSK_ERR_RDONLY;

	offset = gotek_offset(gxself, cylinder, head, 1);

	trklen = 18 * 512;	/* Always format 18 512-byte sectors */
	err = seekto(gxself, offset);
	if (err) return err;


#ifdef WIN32FLOPPY
	if (gxself->gotek_hVolume != INVALID_HANDLE_VALUE)
	{
		DWORD byteswritten;
		int res;
		int n;

		for (n = 0; n < 18; n++)
		{
			memset(gxself->gotek_buffer, filler, 512);
			res = WriteFile(gxself->gotek_hVolume, gxself->gotek_buffer,
				512, &byteswritten, NULL);
			if (!res || byteswritten < 512) return DSK_ERR_SYSERR;
		}
		return DSK_ERR_OK;
	}
#endif
	if (!gxself->gotek_fp) return DSK_ERR_NOTRDY;

	if (gxself->gotek_filesize < offset + trklen)
		gxself->gotek_filesize = offset + trklen;

	for (++trklen; trklen > 1; trklen--)
		if (fputc(filler, gxself->gotek_fp) == EOF) return DSK_ERR_SYSERR;	

	return DSK_ERR_OK;
}

	

dsk_err_t gotek_xseek(DSK_DRIVER *self, const DSK_GEOMETRY *geom,
                      dsk_pcyl_t cylinder, dsk_phead_t head)
{
	GOTEK_DSK_DRIVER *gxself;
	long offset;

	if (!self || !geom) return DSK_ERR_BADPTR;
	CHECK_CLASS(self);


	if (cylinder >= geom->dg_cylinders || head >= geom->dg_heads)
		return DSK_ERR_SEEKFAIL;

	offset = (cylinder * 2) + head;	/* Drive track */
	offset *= 18 * 512;
	offset += gxself->gotek_base;
#ifdef WIN32FLOPPY
	if (gxself->gotek_hVolume != INVALID_HANDLE_VALUE) 
	{
		if (SetFilePointer(gxself->gotek_hVolume, offset, NULL,
			FILE_BEGIN) == INVALID_FILE_SIZE) return DSK_ERR_SYSERR;
		return DSK_ERR_OK;
	}
#endif
	
	if (!gxself->gotek_fp) return DSK_ERR_NOTRDY;
	if (fseek(gxself->gotek_fp, offset, SEEK_SET)) return DSK_ERR_SEEKFAIL;

	return DSK_ERR_OK;
}

dsk_err_t gotek_status(DSK_DRIVER *self, const DSK_GEOMETRY *geom,
                      dsk_phead_t head, unsigned char *result)
{
	GOTEK_DSK_DRIVER *gxself;

	if (!self || !geom) return DSK_ERR_BADPTR;
	CHECK_CLASS(self);

#ifdef WIN32FLOPPY
	if ((gxself->gotek_hVolume == INVALID_HANDLE_VALUE) && !gxself->gotek_fp)
#else
	if (!gxself->gotek_fp) 
#endif
	{
		*result &= ~DSK_ST3_READY;
	}
	if (gxself->gotek_readonly) *result |= DSK_ST3_RO;
	return DSK_ERR_OK;
}

dsk_err_t gotek_getgeom(DSK_DRIVER *self, DSK_GEOMETRY *geom)
{
	unsigned char bootblock[512];
	DSK_GEOMETRY bootgeom;
	dsk_err_t err;

	if (!self || !geom) return DSK_ERR_BADPTR;
	if (self->dr_class != &dc_gotek)  return DSK_ERR_BADPTR;

	/* Default to 1.4M geometry */
	dg_stdformat(geom, FMT_1440K, NULL, NULL);
	geom->dg_cylinders = 85;	/* There's enough space for 85 
					 * cylinders in 0x180000 bytes */

	err = gotek_read(self, geom, bootblock, 0, 0, 1);
	/* Any error causes the 1.4M format to be returned */
	if (err) return DSK_ERR_OK;
	
	/* Is there a recognisable boot block, and does it match the
	 * parameters that can't be changed? */
	if (!dg_bootsecgeom(&bootgeom, bootblock) &&
		bootgeom.dg_secsize == geom->dg_secsize &&
		bootgeom.dg_secbase == geom->dg_secbase &&
		bootgeom.dg_sectors <= geom->dg_sectors &&
		bootgeom.dg_heads   <= geom->dg_heads &&
		bootgeom.dg_cylinders <= geom->dg_cylinders &&
		bootgeom.dg_datarate == geom->dg_datarate)
	{
/* The boot sector geometry may specify a smaller disc image than is actually
 * present on the Gotek (eg, a 1.2Mb disc image). This is OK. */
		geom->dg_cylinders = bootgeom.dg_cylinders;
		geom->dg_heads     = bootgeom.dg_heads;
		geom->dg_sectors   = bootgeom.dg_sectors;
	}
	return DSK_ERR_OK;
}


dsk_err_t gotek_to_ldbs(DSK_DRIVER *self, struct ldbs **result, DSK_GEOMETRY *geom)
{
	DSK_GEOMETRY bootgeom;
	dsk_err_t err;
	dsk_pcyl_t cyl;
	dsk_phead_t head;
	dsk_psect_t sec;
	unsigned char *secbuf;
	LDBS_TRACKHEAD *th;
	int n;

	if (!self || !result) return DSK_ERR_BADPTR;

	if (geom == NULL)
	{
		err = gotek_getgeom(self, &bootgeom);
		if (err) return err;

		geom = &bootgeom;	
	}
	secbuf = dsk_malloc(geom->dg_secsize);
	if (!secbuf) return DSK_ERR_NOMEM;

	err = ldbs_new(result, NULL, LDBS_DSK_TYPE);
	if (err)
	{
		dsk_free(secbuf);
		return err;
	}
	/* If a geometry was provided, save it in the file */
	err = ldbs_put_geometry(*result, geom);
	if (err)
	{
		ldbs_close(result);
		dsk_free(secbuf);
		return err;
	}
	for (cyl = 0; cyl < geom->dg_cylinders; cyl++)
	    for (head = 0; head < geom->dg_heads; head++)
	{
		th = ldbs_trackhead_alloc(geom->dg_sectors);
		if (!th)
		{
			dsk_free(secbuf);
			ldbs_close(result);
			return DSK_ERR_NOMEM;
		}
		for (sec = 0; sec < geom->dg_sectors; sec++)
		{
			err = gotek_read(self, geom, secbuf, cyl, head, 
					sec + geom->dg_secbase);
			if (err)
			{
				ldbs_free(th);
				dsk_free(secbuf);
				ldbs_close(result);
				return err;
			}
			th->sector[sec].id_cyl  = cyl;
			th->sector[sec].id_head = head;
			th->sector[sec].id_sec  = sec + geom->dg_secbase;
			th->sector[sec].id_psh  = dsk_get_psh(geom->dg_secsize);
			th->sector[sec].copies = 0;
			for (n = 1; n < (int)(geom->dg_secsize); n++)
			{
				if (secbuf[n] != secbuf[0])
				{
					th->sector[sec].copies = 1;
					break;
				}
			}
			if (!th->sector[sec].copies)
			{
				th->sector[sec].filler = secbuf[0];
			}
			else
			{
				char secid[4];
			
				ldbs_encode_secid(secid, cyl, head, 	
						sec + geom->dg_secbase);
				err = ldbs_putblock(*result, 
						&th->sector[sec].blockid,
							secid, secbuf,
							geom->dg_secsize);
				if (err)
				{
					ldbs_free(th);
					dsk_free(secbuf);
					ldbs_close(result);
					return err;
				}
			}	
		}	/* End of loop over sectors */
		err = ldbs_put_trackhead(*result, th, cyl, head);
		ldbs_free(th);
		if (err)
		{
			dsk_free(secbuf);
			ldbs_close(result);
			return err;
		}
	}	/* End of loop over cyls / heads */
	dsk_free(secbuf);	
	return ldbs_sync(*result);
}

static dsk_err_t gotek_from_ldbs_callback(PLDBS ldbs, dsk_pcyl_t cyl,
	dsk_phead_t head, LDBS_SECTOR_ENTRY *se, LDBS_TRACKHEAD *th, 
	void *param)
{
	GOTEK_DSK_DRIVER *gxself = param;
	dsk_err_t err;
	size_t len;
	long offset;

	if (gxself->gotek_readonly) return DSK_ERR_RDONLY;

	/* Only store IDs supported by the fixed Gotek format */
	if (cyl >= 85 || head >= 2)
	{
		return DSK_ERR_OK;
	}
	if (se->id_sec >= 1 && se->id_sec <= 18)
	{
		return DSK_ERR_OK;
	}

	offset = gotek_offset(gxself, cyl, head, se->id_sec);
	len = 512;

	/* Ensure the file is big enough to hold the sector */
	err = seekto(gxself, offset + len);
	if (err) return err;
	/* Then seek to the start of the sector */
	err = seekto(gxself, offset);
	if (err) return err;

	/* Do we have sector data? */
	if (se->copies)
	{
		unsigned char *secbuf;

		err = ldbs_getblock_a(ldbs, se->blockid, NULL,
						(void **)&secbuf, &len);
		if (err) return err;
		if (len > 512) len = 512;

#ifdef WIN32FLOPPY
		if (INVALID_HANDLE_VALUE != gxself->gotek_hVolume)
		{
			int res;
			DWORD byteswritten;

			memset(gxself->gotek_buffer, se->filler, 512);
			memcpy(gxself->gotek_buffer, secbuf, len);
			res = WriteFile(gxself->gotek_hVolume, 
				gxself->gotek_buffer, 512, &byteswritten, NULL);
			if (!res || byteswritten < len)
			{
				ldbs_free(secbuf);
				return DSK_ERR_SYSERR;
			}
		} 
		else
#endif
		{
			if (fwrite(secbuf, 1, len, gxself->gotek_fp) < len)
			{
				ldbs_free(secbuf);
				return DSK_ERR_SYSERR;
			}
			/* Pad to 512 bytes */
			while (len < 512)
			{
				if (fputc(se->filler, gxself->gotek_fp) == EOF)
				{
					return DSK_ERR_SYSERR;
				}
				++len;
			}
		}
		ldbs_free(secbuf);
	}
	else	/* No copies, write the filler byte */
	{
		if (len > 512) len = 512;
#ifdef WIN32FLOPPY
		if (INVALID_HANDLE_VALUE != gxself->gotek_hVolume)
		{
			int res;
			DWORD byteswritten;

			memset(gxself->gotek_buffer, se->filler, 512);
			res = WriteFile(gxself->gotek_hVolume, 
				gxself->gotek_buffer, 512, &byteswritten, NULL);
			if (!res || byteswritten < len)
			{
				return DSK_ERR_SYSERR;
			}
		} 
		else
#endif
		{
			while (len > 0)
			{
				if (fputc(se->filler, gxself->gotek_fp) == EOF)
				{
					return DSK_ERR_SYSERR;
				}
				--len;
			}
		}		
	}
	return DSK_ERR_OK;
}




dsk_err_t gotek_from_ldbs(DSK_DRIVER *self, struct ldbs *source, DSK_GEOMETRY *geom)
{
	GOTEK_DSK_DRIVER *gxself;
	long pos;

	if (!self || !source) return DSK_ERR_BADPTR;
	CHECK_CLASS(self);

	/* Erase anything existing in the file */
	if (fseek(gxself->gotek_fp, gxself->gotek_base, SEEK_SET)) 
		return DSK_ERR_SYSERR;

	for (pos = 0; pos < (long)(gxself->gotek_gap); pos++)
	{
		if (fputc(0xE5, gxself->gotek_fp) == EOF) return DSK_ERR_SYSERR;
	}
	if (fseek(gxself->gotek_fp, gxself->gotek_base, SEEK_SET)) 
		return DSK_ERR_SYSERR;

	/* And populate with whatever is in the blockstore */	
	return ldbs_all_sectors(source, gotek_from_ldbs_callback,
				SIDES_ALT, gxself);
}


