/*

    CPMREDIR: CP/M filesystem redirector
    Copyright (C) 1998, John Elliott <jce@seasip.demon.co.uk>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public
    License along with this library; if not, write to the Free
    Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

    This file deals with drive-based functions.
*/

#include "cpmint.h"

cpm_byte fcb_reset(void)
{
#ifdef __MSDOS__
	bdos(0x0D, 0, 0);
#endif

	redir_l_drives  = 0;
	redir_cpmdrive  = 0;	/* A reset forces current drive to A: */
/*	redir_ro_drives = 0; Software write protect not revoked by func 0Dh  */
	return 0;
}


cpm_word fcb_drive (cpm_byte drv)
{
	if (redir_drive_prefix[drv][0])
	{
		redir_cpmdrive = drv;
		redir_log_drv(drv);
		return 0;
	}
	else return 0x04FF;	/* Drive doesn't exist */
}

cpm_byte fcb_getdrv(void)
{
	return redir_cpmdrive;
}


cpm_byte fcb_user  (cpm_byte usr)
{
	if (usr != 0xFF) redir_cpmuser = usr % 16;

	redir_Msg("User: parameter %d returns %d\r\n", usr, redir_cpmuser);

	return redir_cpmuser;
}



cpm_word fcb_logvec(void)
{
	return redir_l_drives;
}


cpm_word fcb_rovec(void)
{
	return redir_ro_drives;
}


cpm_word fcb_rodisk(void)
{
	cpm_word mask = 1;

	if (redir_cpmdrive) mask = mask << redir_cpmdrive;

	redir_ro_drives |= mask;
	return 0;
}


cpm_word fcb_resro(cpm_word bitmap)
{
	redir_ro_drives &= ~bitmap;

	return 0;
}


cpm_word fcb_sync(cpm_byte flag)
{
#ifdef WIN32
	return 0;
#else
	sync(); return 0;	/* Apparently some sync()s are void not int */
#endif
}


cpm_word fcb_purge()
{
#ifdef WIN32
	return 0;
#else
	sync(); return 0;	/* Apparently some sync()s are void not int */
#endif
}


static cpm_byte exdpb[0x11] = {
	0x80, 0, 	/* 128 records/track */
	0x04, 0x0F,	/* 2k blocks */
	0x00,		/* 16k / extent */
	0xFF, 0x0F,	/* 4095 blocks */
	0xFF, 0x03,	/* 1024 dir entries */
	0xFF, 0xFF,	/* 16 directory blocks */
	0x00, 0x80,	/* Non-removable media */
	0x00, 0x00,	/* No system tracks */
	0x02, 0x03	/* 512-byte sectors */
};

cpm_word fcb_getdpb(cpm_byte *dpb)
{
#ifndef WIN32
/* XXX In WIN32, use some function or other to do this */

        struct statfs buf;
	cpm_word nfiles;

	/* Get DPB for redir_cpmdrive. Currently just returns a dummy. */
        if (!statfs(redir_drive_prefix[redir_cpmdrive], &buf))
	{
		/* Store correct directory entry count */

		if (buf.f_files >= 0x10000L) nfiles = 0xFFFF;
		else                         nfiles = buf.f_files;

		exdpb[7] = nfiles & 0xFF;
		exdpb[8] = nfiles >> 8;
	}
#endif
	
	memcpy(dpb, &exdpb, 0x11);
	return 0x11;
}


/* Create an entirely bogus ALV
 * TODO: Make it a bit better */

cpm_word fcb_getalv(cpm_byte *alv, cpm_word max)
{
	if (max > 1024) max = 1024;

	memset(alv,             0xFF, max / 2);
	memset(alv + (max / 2), 0,    max / 2);
	
	return max;
}

/* Get disk free space */

cpm_word fcb_dfree (cpm_byte drive, cpm_byte *dma)
{
#ifdef WIN32
        DWORD secclus, bps, freeclus, totclus, dfree;

        if (!redir_drive_prefix[drive]) return 0x01FF;  /* Can't select */

        if (!GetDiskFreeSpace(redir_drive_prefix[drive], &secclus, &bps, &freeclus, &totclus))
		return 0x01FF;	/* Can't select */
	dfree = freeclus * (secclus * bps / 128);

	if (dfree < freeclus ||		/* Calculation has wrapped round */
	    dfree > 4194303L)           /* Bigger than max CP/M drive size */
	{
		dfree = 4194303L;
	}
#else
	struct statfs buf;
	long dfree;

	if (!redir_drive_prefix[drive]) return 0x01FF;	/* Can't select */
	
	if (statfs(redir_drive_prefix[drive], &buf)) return 0x01FF;

	dfree = (buf.f_bavail * (buf.f_bsize / 128));

	if (dfree < buf.f_bavail ||	/* Calculation has wrapped round */
	    dfree > 4194303L)           /* Bigger than max CP/M drive size */
	{
		dfree = 4194303L;
	}
#endif

	redir_wr24(dma, dfree);
	return 0;
}



