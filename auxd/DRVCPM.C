/***************************************************************************
 *                                                                         *
 *    LIBDSK: General floppy and diskimage access library                  *
 *    Copyright (C) 2001,2005  John Elliott <jce@seasip.demon.co.uk>       *
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

/* This is an implementation of the LibDsk API (or rather, the * cut-down 
 * version of it specified in cpmdsk.h) for generic CP/M. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <cpm.h>
#include "auxd.h"
typedef unsigned char byte;
typedef unsigned short int16;
typedef unsigned long  int32;
#include "rpcpack.h"

/* A Disk Parameter Block -- the nearest we'll get to DSK_GEOMETRY.
 * This structure must not contain any packing or alignment bytes!
 *
 * Note that PSH and PHM only apply for CP/M 3.
 */
typedef struct dpb
{
	unsigned      spt;	/*  0   128-byte sectors / track */
	unsigned char bsh;	/*  2   Block shift 3=>1k  4=>2k  5=>4k etc. */
	unsigned char blm;	/*  3   Block mask  7=>1k 15=>2k 31=>4k etc. */
	unsigned char exm;	/*  4   Extent mask */
	unsigned      dsm;	/*  5   Number of blocks in filesystem - 1 */
	unsigned      drm;	/*  7   Number of directory entries - 1 */
	unsigned char al0;	/*  9   Bitmap of directory blocks */
	unsigned char al1;	/* 10   Bitmap of directory blocks */
	unsigned      cks;	/* 11   Size of directory checksum vector */
	unsigned      off;	/* 13   Tracks before directory */
	unsigned char psh;	/* 15   Sector shift 0=>128 1=>256 2=>512 etc.*/
	unsigned char phm;	/* 16   Sector mask  0=>128 1=>256 3=>512 etc.*/
} DPB;

typedef struct bios3
{
	unsigned char function;
	unsigned char a;
	unsigned      bc;	
	unsigned      de;	
	unsigned      hl;	
} BIOS3;

static unsigned char iscpm3;
static void *sectbl;
static DPB *thedpb;

/* ASM wrappers for functions */
dsk_err_t home2();
dsk_err_t seldsk2(unsigned drive, unsigned de);
void      settrk2(unsigned track);
void      setsec2(unsigned sector);
unsigned  sectrn2(unsigned sector, void *table);
void      setdma2(void *dma);
dsk_err_t read2();
dsk_err_t write2();
void	  xmove3(unsigned source, unsigned dest);
void	  move3 (void *dest, void *source, int count);
void	  tron();
void	  troff();
#asm
;
; BIOS interface for CP/M 2
;
	psect data
	global _sectbl

	psect text
	global _home2
_home2:	
	ld	a,21
	jr	bios2
;
	global _seldsk2
_seldsk2:
	pop	af
	pop	bc	;Drive
	pop	de	;Log in or relog?
	push	de
	push	bc
	push	af
	ld	a,24
	call	bios2
	ld	a,h
	or	l
	jr	nz,logged
	ld	hl,-10	;DSK_ERR_NOTRDY
	ret
logged:	
	ld	e,(hl)
	inc	hl
	ld	d,(hl)
	ld	(_sectbl),de	;Sector translation table
	ld	hl,0
	ret
;
	global _setdma2
_setdma2:
	ld	a,33
	defb	1	;Swallow 3E 27
	global _setsec2
_setsec2:
	ld	a,30
	defb	1	;Swallow 3E 27
	global _settrk2
_settrk2:
	ld	a,27
bios1:	pop	de
	pop	bc
	push	bc
	push	de
	jr	bios2
;
	global _write2
_write2:
	ld	a,39
	ld	c,1	;With buffering
	call	bios2
	jr	xlterr2

	global _read2
_read2:	ld	a,36
	call	bios2
xlterr2:
	ld	hl,0
	or	a
	ret	z
	ld	hl,-23	;Map all errors arbitrarily to "controller failed".
	ret	
;
	global _sectrn2
_sectrn2:
	pop	af
	pop	bc	;Sector
	pop	de	;XLT table
	push	de
	push	bc
	push	af
	ld	a,45
bios2:	push	hl	;Call BIOS function; A = value to add to WBOOT
	push	de	;to find the function.
	ld	hl,(1)
	ld	e,a
	ld	d,0
	add	hl,de
	pop	de
	ex	(sp),hl
	ret
;
; BIOS interface for CP/M 3
;
	global	_xmove3
_xmove3:	pop	af
	pop	bc	;Source bank
	pop	de	;Dest bank
	push	de
	push	bc
	push	af
	ld	b,e
	ld	a,84
	jr	bios2
;
	global	_move3
_move3:	pop	af
	pop	hl	;Target
	pop	de	;Source
	pop	bc	;Count
	push	bc
	push	de
	push	hl
	push	af
	ld	a,72
	jr	bios2
;
	global	_tron
_tron:	ld	a,80h
	defb	0EDh,0FEh
	ret
;
	global	_tron
_troff:	ld	a,81h
	defb	0EDh,0FEh
	ret
;

#endasm


static void home3(void )
{
	BIOS3 bios;
	bios.function = 8;
	bdoshl(50, &bios);
}


static void settrk3(unsigned track)
{
	BIOS3 bios;

/*	printf("settrk3(%d)\n", track); */
	bios.function = 10;
	bios.bc = track;
	bdoshl(50, &bios);
}

static void setsec3(unsigned sector)
{
	BIOS3 bios;
/*	printf("setsec3(%d)\n", sector); */
	bios.function = 11;
	bios.bc = sector;
	bdoshl(50, &bios);
}


static void setdma3(void *dma)
{
	BIOS3 bios;
	bios.function = 12;
	bios.bc = (unsigned)dma;
	bdoshl(50, &bios);	/* Set DMA address */
	bios.function = 28;
	bios.a = 1;
	bdoshl(50, &bios);	/* Set dma bank to 1 */
}



static dsk_err_t read3(void)
{
	BIOS3 bios;
	unsigned result;

	bios.function = 13;
	result = bdos(50, &bios);
/*	printf("read3() returned %x\n", result); */
	switch(result)
	{
		case 0: return DSK_ERR_OK;
		case 1: return DSK_ERR_NOTRDY;
		case 2: return DSK_ERR_RDONLY;
		default: return DSK_ERR_UNKNOWN;
	}
}


static dsk_err_t write3(void)
{
	BIOS3 bios;
	unsigned result;

	bios.function = 14;
	bios.bc = 1;
	result = bdos(50, &bios);
/*	printf("write3() returned %x\n", result); */
	switch(result)
	{
		case 0: return DSK_ERR_OK;
		case 1: return DSK_ERR_NOTRDY;
		case 2: return DSK_ERR_RDONLY;
		default: return DSK_ERR_UNKNOWN;
	}
}


static unsigned sectrn3(unsigned sector, void *table)
{
	BIOS3 bios;
	unsigned result;

	bios.function = 16;
	bios.bc = sector;
	bios.de = (unsigned)table;
	result = bdoshl(50, &bios);
/*	printf("sectrn3(%d)=%d\n", sector, result); */
	return result;
}


static void flush3()
{
	BIOS3 bios;
	bios.function = 24;
	bdoshl(50, &bios);
}


/* This function converts a CP/M DPB to a LibDsk geometry. */
static void dpb_to_dg(DSK_GEOMETRY *dg, DPB *dpb)
{
	/* Count of 128-byte sectors on drive. CP/M Plus can go up to 
	 * 512Mb which is comfortably in range of a long. */
	long drive_sectors = (long)(dpb->dsm + 1) * (long)(dpb->blm + 1);
	long drive_cyls    = (drive_sectors + (dpb->spt - 1)) / dpb->spt;
/* We will arbitrarily map this CP/M device to a single-sided floppy (cf 
 * the MYZ80 driver in libdsk proper) */
	dg->dg_sidedness = SIDES_ALT;
	dg->dg_cylinders = drive_cyls + dpb->off;
	dg->dg_heads     = 1;
	dg->dg_sectors   = dpb->spt;	/* Fix later for CP/M Plus */
	dg->dg_secbase   = 0;
	dg->dg_secsize   = 128;
	dg->dg_datarate  = 2;
	dg->dg_rwgap     = 0x2A;
	dg->dg_fmtgap    = 0x52;
	dg->dg_fm        = 0;
	dg->dg_nomulti   = 0;
	dg->dg_noskip    = 0;
	if (iscpm3)
	{
		dg->dg_secsize = 128 * (dpb->phm + 1);
		dg->dg_sectors = (dpb->spt / (dpb->phm + 1));
	}
/* Debugging code: This dumps the DPB and the resulting DSK_GEOMETRY 
	{
		unsigned char *x = (unsigned char *)dpb;
		int n;
		printf("  DPB: ");
		for (n = 0; n < 16; n++)
		{
			printf("%02x ", x[n]);
		}
		printf("\n"); 
		printf("  DG: {%d,%d,%d,%d,%d,%d, %x,%x,%x, %d,%d,%d}\n",
			dg->dg_sidedness,
			dg->dg_cylinders,
			dg->dg_heads,
			dg->dg_sectors,
			dg->dg_secbase,
			dg->dg_secsize,
			dg->dg_datarate,
			dg->dg_rwgap,
			dg->dg_fmtgap,
			dg->dg_fm,
			dg->dg_nomulti,
			dg->dg_noskip);

		fflush(stdout);
	}
	*/

}

/* Workaround for a silly bug in my PIPEMGR-supported version of libc, 
 * which assumes PIPEMGR is installed if the CP/M version is less than 3.
 * The /correct/ fix would be to repair libc, but I haven't got 'round to 
 * doing it yet. */
/* extern unsigned char _piped; */

/* Initialise this miniature LibDsk. */
dsk_err_t dsk_init()
{
	unsigned cpmver = bdoshl(0x0C, 0);
	unsigned char verminor = cpmver & 0xFF;

	/* Check for CP/M Plus. */
	if (verminor >= 0x30)
	{
		iscpm3 = 1;
		bdoshl(0x2D, 0xFE);	/* Don't terminate on errors */
	}
/*	else
	{
		_piped = 0;
	} */
	if (verminor < 0x20)
	{
		fprintf(stderr, "This program won't run on CP/M 1.\n");
		exit(1);
	}
	if (cpmver & 0x100)
	{
		printf("MP/M version is %d.%d\n", (cpmver >> 4) & 0x0F,
				(cpmver & 0x0F));
		printf("WARNING: Not tested on MP/M.\n");
	}
	else
	{
		printf("CP/M version is %d.%d\n", (cpmver >> 4) & 0x0F,
				(cpmver & 0x0F));
	}
	return DSK_ERR_OK;
}

dsk_err_t log_drive(char drive)
{
	dsk_err_t err;
	unsigned rv;
	unsigned char *dph;

	drive = (drive - 'A') & 0x0F;

/*	printf("log_drive(%d)", drive); */
       	rv = bdoshl(0x0E, drive);
/*	printf("=%04x\n", rv); fflush(stdout); */
	thedpb = (DPB *)bdoshl(31, drive);
/*	printf("dpb=%04x\n", thedpb); */
	if ((rv & 0xFF) == 0) 
	{
		if (iscpm3)
		{
			BIOS3 bios;
			bios.function = 9;
			bios.bc = drive;
			bios.de = 0xFFFF;
			dph = (unsigned char *)bdoshl(50, &bios);
			if (dph == NULL) return DSK_ERR_NOTME;
/*			printf("DPH=%04x\n", dph); */
/* Get the address of the skew table. The DPH may be elsebank so use XMOVE to
 * retrieve its first 2 bytes. */
			xmove3(0, 1);
			move3(&sectbl, dph, 2);
/*			printf("sectbl=%04x\n", sectbl);
			fflush(stdout); */
			return DSK_ERR_OK;
		}
		else err = seldsk2(drive, 0xFFFF);
		
		return DSK_ERR_OK;
	}
	switch(rv >> 8)
	{
		case 1: return DSK_ERR_NOTRDY;
		case 2: 
		case 3: return DSK_ERR_RDONLY;
		case 0:
		case 4: return DSK_ERR_NOTME;
	}
	return DSK_ERR_UNKNOWN;
}


dsk_err_t dsk_open(DSK_PDRIVER *handle, char *filename, char *driver,
		                char *compress)
{
	char drive;
	dsk_err_t err;

/* We only support raw access to floppy drives */
	if (driver != NULL && strcmp(driver, "floppy")) return DSK_ERR_NOTME;
	if (compress != NULL) return DSK_ERR_NOTME;
	if (strlen(filename) != 2 || filename[1] != ':') return DSK_ERR_NOTME;
	drive = filename[0];
	if (islower(drive)) drive = toupper(drive);

/* Attempt to select the drive using the BDOS */
	err = log_drive(drive);
	if (err) return err;
	*handle = drive;
	return DSK_ERR_OK;
}


dsk_err_t dsk_close(unsigned short hdriver)
{
	if (iscpm3) { home3(); flush3(); }
	else	      home2();
	bdoshl(0x0D, 0);
	return DSK_ERR_OK;
}

dsk_err_t dsk_pread(DSK_PDRIVER handle, DSK_GEOMETRY *dg, unsigned char *buf,
		unsigned cyl, unsigned head, unsigned sector)
{
	dsk_err_t err;
/*
	printf("dsk_pread(%d, 0x%04x, 0x%04x, %d, %d, %d)\n",
			handle, dg, buf, cyl, head, sector); fflush(stdout); */
	err = log_drive(handle); if (err) return err;

	if (iscpm3)
	{
/*		tron(); */
/*		printf("Calling settrk3(%d)\n", cyl); fflush(stdout); */
		settrk3(cyl);
/*		printf("Calling setsec3(%d)\n", sectrn3(sector,sectbl)); fflush(stdout); */
		setsec3(sectrn3(sector,sectbl));
/*		printf("Calling setdma3(0x%04x)\n", buf); fflush(stdout); */
		setdma3(buf);
/*		printf("Calling read3()\n"); fflush(stdout); */
		err = read3();
/*		troff(); */
/*		printf("err=%d\n", err); fflush(stdout); */
	}
	else
	{
		settrk2(cyl);
		setsec2(sectrn2(sector,sectbl));
		setdma2(buf);
		err = read2();
	}
	return err;
}

dsk_err_t dsk_pwrite(DSK_PDRIVER handle, DSK_GEOMETRY *dg, unsigned char *buf,
		unsigned cyl, unsigned head, unsigned sector)
{
	dsk_err_t err;

	err = log_drive(handle); if (err) return err;

	if (iscpm3)
	{
		settrk3(cyl);
		setsec3(sectrn3(sector,sectbl));
		setdma3(buf);
		err = write3();
	}
	else
	{
		settrk2(cyl);
		setsec2(sectrn2(sector,sectbl));
		setdma2(buf);
		err = write2();
	}
	return err;
}

dsk_err_t dsk_pformat(DSK_PDRIVER handle, DSK_GEOMETRY *dg, 
		unsigned cyl, unsigned head, DSK_FORMAT *format,
		unsigned char filler)
{
	return DSK_ERR_NOTIMPL;
}


dsk_err_t dsk_psecid(DSK_PDRIVER handle, DSK_GEOMETRY *dg, 
		unsigned cyl, unsigned head, DSK_FORMAT *fmt)
{
	return DSK_ERR_NOTIMPL;
}


dsk_err_t dsk_getgeom(DSK_PDRIVER handle, DSK_GEOMETRY *dg)
{
	dsk_err_t err;

	err = log_drive(handle); if (err) return err;

	dpb_to_dg(dg, thedpb);
	return DSK_ERR_OK;
}


dsk_err_t dsk_drive_status(DSK_PDRIVER handle, DSK_GEOMETRY *dg,
		                unsigned head, unsigned char *status)
{
	return DSK_ERR_NOTIMPL;
}


dsk_err_t dsk_pseek(DSK_PDRIVER handle, DSK_GEOMETRY *dg,
		unsigned cylinder, unsigned head)
{
	return DSK_ERR_NOTIMPL;
}


dsk_err_t dsk_ptrackids(DSK_PDRIVER handle, DSK_GEOMETRY *geom,
				unsigned short cylinder, unsigned short head,
				unsigned short *count, DSK_FORMAT *results)
{
	return DSK_ERR_NOTIMPL;
}



dsk_err_t dsk_ptread(DSK_PDRIVER self, DSK_GEOMETRY *geom, unsigned char *b,
	                              unsigned cylinder, unsigned head)
{
	unsigned sec;
	dsk_err_t err;

	for (sec = 0; sec < geom->dg_sectors; sec++)
	{
		err = dsk_pread(self, geom, b + sec * geom->dg_secsize,
				cylinder, head, sec + geom->dg_secbase);
		if (err) return err;
	}
	return DSK_ERR_OK;
}

dsk_err_t dsk_xtread(DSK_PDRIVER self, DSK_GEOMETRY *geom, unsigned char *b,
	                    unsigned cylinder, unsigned head,
			    unsigned cyl_expected, unsigned head_expected)
{
	unsigned sec;
	dsk_err_t err;

	for (sec = 0; sec < geom->dg_sectors; sec++)
	{
		err = dsk_xread(self, geom, b + sec * geom->dg_secsize,
				cylinder, head, cyl_expected, head_expected,
				sec + geom->dg_secbase, geom->dg_secsize, 0);
		if (err) return err;
	}
	return DSK_ERR_OK;
}

dsk_err_t dsk_rtread(DSK_PDRIVER self, DSK_GEOMETRY *geom, unsigned char *b,
	                              unsigned cylinder, unsigned head,
				      unsigned mode, unsigned short *buflen)
{
	*buflen = 0;
	return DSK_ERR_NOTIMPL;
}


dsk_err_t dsk_xread(DSK_PDRIVER handle, DSK_GEOMETRY *geom, unsigned char *buf,
	unsigned cyl, unsigned head, unsigned cyl_expected,
	unsigned head_expected, unsigned sector, unsigned sectorsize,
	unsigned long *deleted)

{
	return DSK_ERR_NOTIMPL;
}


dsk_err_t dsk_xwrite(DSK_PDRIVER handle, DSK_GEOMETRY *geom, unsigned char *buf,
	unsigned cyl, unsigned head, unsigned cyl_expected,
	unsigned head_expected, unsigned sector, unsigned sectorsize,
	unsigned deleted)

{
	return DSK_ERR_NOTIMPL;
}


/* There are no options */
dsk_err_t dsk_option_enum(DSK_PDRIVER handle, unsigned opt, char **value)
{
	*value = NULL;
	return DSK_ERR_BADOPT;
}

dsk_err_t dsk_set_option(DSK_PDRIVER handle, char *option, short value)
{
	return DSK_ERR_BADOPT;
}

dsk_err_t dsk_get_option(DSK_PDRIVER handle, char *option, short *value)
{
	return DSK_ERR_BADOPT;
}


dsk_err_t dsk_properties(DSK_PDRIVER handle, unsigned short **pprops,
		                unsigned short *propcount, char **desc)
{
/* List of implemented functions. Ones which it would be pointless to call
 * are commented out */
	static unsigned short props[] = 
	{
		RPC_DSK_OPEN, RPC_DSK_CREAT, RPC_DSK_CLOSE, 
		RPC_DSK_DRIVE_STATUS, RPC_DSK_PREAD, RPC_DSK_PWRITE,
/*		RPC_DSK_XREAD, RPC_DSK_XWRITE, */RPC_DSK_PTREAD,
		RPC_DSK_XTREAD, /* RPC_DSK_RTREAD, */
/*		RPC_DSK_OPTION_ENUM, RPC_DSK_OPTION_GET, RPC_DSK_OPTION_SET,  
 *		RPC_DSK_GETCOMMENT, RPC_DSK_SETCOMMENT, */
/*		RPC_DSK_PFORMAT, */RPC_DSK_GETGEOM, /* RPC_DSK_PSECID,
		RPC_DSK_PSEEK, RPC_DSK_TRACKIDS, */ RPC_DSK_PROPERTIES
	};
	*pprops = props;
	*propcount = sizeof(props) / sizeof(props[0]);
	*desc = "Generic CP/M driver";
	return DSK_ERR_OK;
}


dsk_err_t dsk_get_comment(DSK_PDRIVER handle, char **comment)
{
	*comment = NULL;
	return DSK_ERR_OK;
}

dsk_err_t dsk_set_comment(DSK_PDRIVER handle, char *comment)
{
	return DSK_ERR_NOTIMPL;
}

