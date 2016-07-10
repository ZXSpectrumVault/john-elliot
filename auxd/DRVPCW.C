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

/* This is a PCW-specific implementation of the LibDsk API (or rather, the
 * cut-down version of it specified in cpmdsk.h */

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

#define IMPLEMENT_GETGEOM 0

/* An eXtended Disk Parameter Block -- Amstrad CP/M equivalent of the 
 * DSK_GEOMETRY structure. The first 15 or so bytes are really only used
 * by the CP/M filesystem; the low-level stuff is at psh onward. 
 *
 * This structure must not contain any packing or alignment bytes!
 */
typedef struct xdpb
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
	unsigned char sides;	/* 17   Bits 0-1: Sidedness
					  0: Single-sided
					  1: SIDES_ALT
					  2: SIDES_OUTBACK
					Bit 6: High density (PcW16 only)
					Bit 7: 0 for 48tpi, 1 for 96tpi */
	unsigned char cyls;	/* 18   Number of cylinders / side */
	unsigned char secs;	/* 19   Number of sectors / track */
	unsigned char sec1;	/* 20   First sector number */
	unsigned      secsize;	/* 21   Size of sector */
	unsigned char rwgap;	/* 23   Read/Write gap */
	unsigned char fmtgap;	/* 24   Format gap */
	unsigned char multi;	/* 25   Bit 7: Multisector
					Bit 6: MFM
					Bit 5: Skip deleted data */
	unsigned char freeze;	/* 26   00h to use CP/M geometry probe
				   	0FFh not to */
} XDPB;

typedef struct dd_l_buf
{
	unsigned char bank;
	unsigned char *address;
	unsigned      datalen;
	unsigned char commandlen;
	unsigned char command[20];
} DD_L_BUF;

/* PCW extended BIOS wrappers. These are implemented below as asm. */
extern unsigned char cd_version();
extern unsigned char cd_info();
extern void          dd_init(void);
extern dsk_err_t     dd_read_sector(XDPB *xdpb, unsigned drive,
		unsigned track, unsigned sector, unsigned char *buf);
extern dsk_err_t     dd_write_sector(XDPB *xdpb, unsigned drive,
		unsigned track, unsigned sector, unsigned char *buf);
extern dsk_err_t     dd_check_sector(XDPB *xdpb, unsigned drive,
		unsigned track, unsigned sector, unsigned char *buf);
extern dsk_err_t     dd_format      (XDPB *xdpb, unsigned drive,
		unsigned track, unsigned filler, unsigned char *buf);
extern dsk_err_t     dd_login     (XDPB *xdpb, unsigned drive);
extern dsk_err_t     dd_sel_format(XDPB *xdpb, unsigned fmt);
extern unsigned char dd_drive_status(unsigned drive);
extern dsk_err_t     dd_read_id   (XDPB *xdpb, unsigned drive, 
		unsigned track, unsigned char **buf);
extern void dd_l_on_motor();
extern void dd_l_t_off_motor();
extern void dd_l_off_motor();
extern dsk_err_t dd_l_seek(XDPB *xdpb, unsigned drive, unsigned cylinder);
extern unsigned char *dd_l_read(DD_L_BUF *buf);
extern unsigned char *dd_l_write(DD_L_BUF *buf);
#asm
	psect text
;
; Helper routine: Call the Amstrad extended BIOS
;
	global	userf
userf:	jp	userf1
userf1:	push	hl
	push	de
	ld	hl,(1)
	ld	de,57h
	add	hl,de
	ld	(userf+1),hl	;Skip this calculation in future calls
	pop	de
	ex	(sp),hl
	ret
;
; Extended BIOS wrappers
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; Reset the disk subsystem.
;
	global _dd_init
_dd_init:
	call	userf
	defw	80h
	ret
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; Read / write sector. These have the same signature and so I can implement
; them like this.
;
	global	_dd_read_sector
	global	_dd_write_sector
	global	_dd_check_sector

_dd_read_sector:
	ld	hl,86h		; DD READ SECTOR
	ld	(dd_func),hl
	jr	dd_generic

_dd_write_sector:
	ld	hl,89h		; DD WRITE SECTOR
	ld	(dd_func),hl
	jr	dd_generic

_dd_check_sector:
	ld	hl,8Ch		; DD CHECK SECTOR
	ld	(dd_func),hl
 	jr	dd_generic

_dd_format:
	ld	hl,8Fh		; DD FORMAT
	ld	(dd_func),hl
;;;	jr	dd_generic

dd_generic:
	push	iy
	ld	iy,0
	add	iy,sp	;IY=frame pointer
			;   IY+0 = old IY
			;   IY+2 = return address
			;   IY+4 = XDPB
			;   IY+6 = drive
			;   IY+8 = track
			;   IY+10= sector
			;   IY+12= buffer
	push	ix
	ld	l,(iy+4)
	ld	h,(iy+5)
	push	hl
	pop	ix	;IX->XDPB
	ld	b,1	;TPA
	ld	c,(iy+6);Drive
	ld	d,(iy+8);Track
	ld	e,(iy+10);Sector
	ld	l,(iy+12)
	ld	h,(iy+13);Buffer
	call	userf
dd_func:
	defw	86h	;DD READ SECTOR
	pop	ix
	pop	iy
	push	af
	ld	a,(dd_func)
	cp	8Ch	  ; If it was DD CHECK SECTOR
	jr	nz,dd_xlt
	pop	af	 
	jr	nc,dd_xlt ; And it wasn't an error
	jr	z,dd_xlt  ; And it wasn't successful
	ld	hl,-9	  ; Then it must be a mismatch. 
	ret

dd_xlt:	pop	af
	jp	xlterr	;Translate error to LibDsk error.
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; Log in a disc.
;
	global	_dd_login
_dd_login:
	pop	de	;Return address
	pop	hl	;XDPB
	pop	bc	;Drive
	push	bc
	push	hl
	push	de
;
; C = drive, HL->XDPB
;
	push	ix	;Save frame pointer
	push	hl
	pop	ix	;IX -> XDPB
	call	userf
	defw	92h
	pop	ix	;Restore frame pointer
	jp	xlterr
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; Initialise an XDPB.
;
	global	_dd_sel_format
_dd_sel_format:
	pop	bc	;Return address
	pop	hl	;XDPB
	pop	de	;Format ID
	push	de
	push	hl
	push	bc
;
; E = format ID, HL->XDPB
;
	push	ix	;Save frame pointer
	push	hl
	pop	ix	;IX -> XDPB
	ld	a,e	;A = format ID
	call	userf
	defw	95h
	pop	ix	;Restore frame pointer
	jp	xlterr
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; Get drive status.
;
	global	_dd_drive_status
_dd_drive_status:
	pop	de	;Return address
	pop	bc	;Drive
	push	bc	
	push	de
	call	userf
	defw	98h
	ld	l,a
	ld	h,0
	ret
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; Read a sector ID.
;
	global	_dd_read_id
_dd_read_id:
	push	iy
	ld	iy,0
	add	iy,sp	;IY=frame pointer
			;   IY+0 = old IY
			;   IY+2 = return address
			;   IY+4 = XDPB
			;   IY+6 = drive
			;   IY+8 = track
			;   IY+10=buffer
	push	ix
	ld	l,(iy+4)
	ld	h,(iy+5)
	push	hl
	pop	ix	;IX->XDPB
	ld	c,(iy+6);Drive
	ld	d,(iy+8);Track
	push	iy
	call	userf
	defw	9Bh	;DD READ ID
	pop	iy
	ex	de,hl	;DE = result
	ld	l,(iy+10)
	ld	h,(iy+11) ;HL->buffer pointer
	ld	(hl),e
	inc	hl
	ld	(hl),d
	pop	ix
	pop	iy
;
; Translate PCW error to LibDsk error
;
xlterr:	ld	hl,0
	ret	c
	add	a,10		;Due to a stupid mistake on my part,
	cp	18		;"disk changed" in LibDsk is -19 not -18;
				;and "unsuitable media" is -31.
	jr	c,not18		;So map 8 and 9 to -19 and -31.
	jr	z,not19
	ld	a,30		;This will become 31.
not19:	inc	a
not18:	neg
	ld	l,a
	dec	h	;HL = error
	ret
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; Turn the drive motor(s) on
;
	global	_dd_l_on_motor
_dd_l_on_motor:
	call	userf
	defw	00A4h
	ret
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; Start countdown for motors off
;
	global	_dd_l_t_off_motor
_dd_l_t_off_motor:
	call	userf
	defw	00A7h
	ret
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; Turn the drive motors off
;
	global	_dd_l_off_motor
_dd_l_off_motor:
	call	userf
	defw	00AAh
	ret
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; Seek to a track.
;
	global	_dd_l_seek
_dd_l_seek:
	pop	af	;Return address
	pop	hl	;XDPB
	pop	bc	;Drive
	pop	de	;Cylinder
	push	de
	push	bc
	push	hl
	push	af
;
; HL->XDPB, C=drive, E=cylinder.
;
	push	ix
	push	hl
	pop	ix	;IX->XDPB
	ld	d,e	;D=cylinder
	call	userf
	defw	00B3h
	pop	ix
	jp	xlterr
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; Low level uPD765A command interface.
;
	global	_dd_l_read
_dd_l_read:
	pop	de
	pop	hl
	push	hl
	push	de
	call	userf
	defw	00ADh
	ret
;
	global	_dd_l_write
_dd_l_write:
	pop	de
	pop	hl
	push	hl
	push	de
	call	userf
	defw	00B0h
	ret
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; Get BIOS version and machine type.
;
	global _cd_version
_cd_version:
	push	hl
	call	userf
	defw	00E3h
	pop	hl
	ld	l,a
	ret
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; Get count of floppies - L=0 for 1, 0FFh for 2
;
	global _cd_info
_cd_info:
	call	userf
	defw	00E6h
	ld	l,a
	ret
#endasm

static unsigned char twodrives;

/* Convert a sector size to a sector shift */
static unsigned char dsk_get_psh(unsigned secsize)
{
	unsigned char psh;

	for (psh = 0; secsize > 128; secsize /= 2) ++psh;
	return psh;
}

/* This program's use of the XDPB relies on the stack being above 0C000h 
 * and hence in common memory. */
static void dg_to_xdpb(DSK_GEOMETRY *dg, XDPB *xdpb)
{
	/* Init XDPB */
	dd_sel_format(xdpb, 0);

	xdpb->spt   = (dg->dg_secsize / 128) * dg->dg_sectors;	
	if (dg->dg_heads < 2)
	{
		xdpb->sides = 0;
	}
	else switch(dg->dg_sidedness)
	{
		case SIDES_ALT: xdpb->sides = 1; break;
		default:	xdpb->sides = 2; break;
	}
	if (dg->dg_cylinders > 45) xdpb->sides |= 0x80;
	if (dg->dg_datarate == RATE_HD) xdpb->sides |= 0x40;

	xdpb->phm = (dg->dg_secsize / 128) - 1;
	xdpb->psh = dsk_get_psh(dg->dg_secsize);
	xdpb->cyls = dg->dg_cylinders;
	xdpb->secs = dg->dg_sectors;
	xdpb->sec1 = dg->dg_secbase;
	xdpb->secsize = dg->dg_secsize;
	xdpb->rwgap   = dg->dg_rwgap;
	xdpb->fmtgap  = dg->dg_fmtgap;
	xdpb->multi   = 0;
/* PCW doesn't get on well with multitrack */
/*	if (!dg->dg_nomulti) xdpb->multi |= 0x80; */
	if (!dg->dg_fm)      xdpb->multi |= 0x40;
	if (!dg->dg_noskip)  xdpb->multi |= 0x20;
	xdpb->freeze = 0xFF;
/* Debugging code: This dumps a DSK_GEOMETRY structure and the XDPB generated
 * from it 
	{
		unsigned char *x = (unsigned char *)xdpb;
		int n;
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

		printf("  XDPB: ");
		for (n = 0; n < 27; n++)
		{
			printf("%02x ", x[n]);
		}
		printf("\n"); 
		fflush(stdout);
	}
	*/
}


#if IMPLEMENT_GETGEOM
/* This function reverses dg_to_xdpb(). It only applies if you recompile
 * this driver to use the PCW's geometry probe rather than LibDsk's own */
static void xdpb_to_dg(DSK_GEOMETRY *dg, XDPB *xdpb)
{
	if ((xdpb->sides & 3) == 2) dg->dg_sidedness = SIDES_OUTBACK;
	else			    dg->dg_sidedness = SIDES_ALT;
	if ((xdpb->sides & 3) == 0) dg->dg_heads = 1;
	else			    dg->dg_heads = 2;

	dg->dg_cylinders = xdpb->cyls;
	dg->dg_sectors   = xdpb->secs;
	dg->dg_secbase   = xdpb->sec1;
	dg->dg_secsize   = xdpb->secsize;
	dg->dg_datarate  = (dg->dg_sidedness & 0x40) ? RATE_HD : RATE_SD;
	dg->dg_rwgap     = xdpb->rwgap;
	dg->dg_fmtgap    = xdpb->fmtgap;
	dg->dg_fm        = (xdpb->multi & 0x40) ? 0 : 1;
	dg->dg_nomulti   = (xdpb->multi & 0x80) ? 0 : 1;
	dg->dg_noskip    = (xdpb->multi & 0x20) ? 0 : 1;
/* Debugging code: This dumps the XDPB and the resulting DSK_GEOMETRY 
	{
		unsigned char *x = (unsigned char *)xdpb;
		int n;
		printf("  XDPB: ");
		for (n = 0; n < 27; n++)
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

#endif	/* IMPLEMENT_GETGEOM */

/* Temporary buffer for sector IDs and such. */
static unsigned char tmpbuf[1024];
static unsigned char status[4][3];
static unsigned char doublestep[4];
static          char forcehead[4];

/* Given an XDPB, a cylinder and a head, return the track number corresponding
 * to that cylinder and head. */
static unsigned calc_track(XDPB *xdpb, unsigned cyl, unsigned head)
{
	switch(xdpb->sides & 3)
	{
		default:
		case 0:	return cyl;
		case 1:	return cyl * 2 + head;
		case 2: if (!head) return cyl;
			return ((xdpb->cyls * 2) - 1) - cyl;
	}
}

/* Initialise this miniature LibDsk. */
dsk_err_t dsk_init()
{
	/* Check for CP/M Plus. */
	if ((bdos(0x0C, 0) & 0xFF) != 0x31)
	{
		fprintf(stderr, "This program requires Amstrad CP/M Plus\n");
		return DSK_ERR_NOTME;	
	}
	/* Disable scrolling (for speed). Do this before clearing the screen,
	 * so that if this is run on an old version of CP/M that doesn't 
	 * support ESC 4, then "41" will be printed but then the clear screen
	 * will take it away. */
	printf("%c41%cE%cH", 27,27,27);

	/* Call USERF to get system type. */
	switch(cd_version())
	{
		case 0:   printf("System is CPC series\n"); break;
		case 1:   printf("System is PCW series\n"); break;
		case 3:   printf("System is Spectrum +3\n"); break;
		case 'A': printf("System is PcW16\n"); break;
		default:  printf("System is unknown(%d)\n", cd_version()); 
			  break;
	}
	/* Enumerate floppies. */
	twodrives = cd_info();
	return DSK_ERR_OK;
}


dsk_err_t dsk_open(DSK_PDRIVER *handle, char *filename, char *driver,
		                char *compress)
{
	char drive;

/* We only support raw access to floppy drives - so that's A: a: B: and b: */
	if (driver != NULL && strcmp(driver, "floppy")) return DSK_ERR_NOTME;
	if (compress != NULL) return DSK_ERR_NOTME;
	if (strlen(filename) != 2 || filename[1] != ':') return DSK_ERR_NOTME;
	drive = filename[0];
	if (islower(drive)) drive = toupper(drive);

	dd_init();
	switch(drive)
	{
		case '0': case 'A': 
			*handle = 'A';
			doublestep[0] = 0;
			forcehead[0]  = -1;
			return DSK_ERR_OK;
		case '1': case 'B':
			if (!twodrives) return DSK_ERR_NOTME;
			*handle = 'B';
			doublestep[1] = 0;
			forcehead[1] = -1;
			return DSK_ERR_OK;
	}
	return DSK_ERR_NOTME;	
}


dsk_err_t dsk_close(unsigned short hdriver)
{
	/* No cleanup needed */
	return DSK_ERR_OK;
}

dsk_err_t dsk_pread(DSK_PDRIVER handle, DSK_GEOMETRY *dg, unsigned char *buf,
		unsigned cyl, unsigned head, unsigned sector)
{
	XDPB xdpb;
	unsigned track;

	dg_to_xdpb(dg, &xdpb);
	track  = calc_track(&xdpb, cyl, head);
/*	printf("cyl=%d head=%d sector=%d sec1=%d\n", cyl, head, sector, xdpb.sec1); */
	return dd_read_sector(&xdpb, handle - 'A', track, 
			sector - xdpb.sec1, buf);
}

dsk_err_t dsk_pwrite(DSK_PDRIVER handle, DSK_GEOMETRY *dg, unsigned char *buf,
		unsigned cyl, unsigned head, unsigned sector)
{
	XDPB xdpb;
	unsigned track;

	dg_to_xdpb(dg, &xdpb);
	track  = calc_track(&xdpb, cyl, head);
/*	printf("cyl=%d head=%d sector=%d sec1=%d\n", cyl, head, sector, xdpb.sec1); 
	printf("Writing sector, buffer starts %02x %02x %02x\n",
			buf[0], buf[1], buf[2]);
*/
	return dd_write_sector(&xdpb, handle - 'A', track, 
			sector - xdpb.sec1, buf);
}


dsk_err_t dsk_pformat(DSK_PDRIVER handle, DSK_GEOMETRY *dg, 
		unsigned cyl, unsigned head, DSK_FORMAT *format,
		unsigned char filler)
{
	XDPB xdpb;
	unsigned track, sec;
	dsk_err_t err;

	dg_to_xdpb(dg, &xdpb);
/* Populate the format buffer */
/*	printf("Format buffer: "); */
	for (sec = 0; sec < dg->dg_sectors; sec++)
	{
		tmpbuf[sec*4  ] = format[sec].fmt_cylinder;
		tmpbuf[sec*4+1] = format[sec].fmt_head;
		tmpbuf[sec*4+2] = format[sec].fmt_sector;
		tmpbuf[sec*4+3] = dsk_get_psh(format[sec].fmt_secsize);
/*		printf("%02x %02x %02x %02x  ",
				tmpbuf[sec*4], tmpbuf[sec*4+1],
				tmpbuf[sec*4+2], tmpbuf[sec*4+3]); */
	}
	track  = calc_track(&xdpb, cyl, head);
/*	printf("Entering dd_format with track=%d filler=%02x\n", track, filler);
	fflush(stdout); */
	err = dd_format(&xdpb, handle - 'A', track, filler, tmpbuf);
/*	printf("dd_l_format returned %d\n", err);
	fflush(stdout); */
	return err;
}


dsk_err_t dsk_psecid(DSK_PDRIVER handle, DSK_GEOMETRY *dg, 
		unsigned cyl, unsigned head, DSK_FORMAT *fmt)
{
	XDPB xdpb;
	unsigned track;
	unsigned char *buf;
	dsk_err_t err;

	dg_to_xdpb(dg, &xdpb);
	track  = calc_track(&xdpb, cyl, head);
	err = dd_read_id(&xdpb, handle - 'A', track, &buf); 	
	if (err == 0)
	{
		fmt->fmt_cylinder = buf[4];
		fmt->fmt_head     = buf[5];
		fmt->fmt_sector   = buf[6];
		fmt->fmt_secsize  = (128 << buf[7]);
	}
	return err;
}


dsk_err_t dsk_getgeom(DSK_PDRIVER handle, DSK_GEOMETRY *dg)
{
#if IMPLEMENT_GETGEOM
/*
 * Here is the native geometry probe if you want to use it
 */
	XDPB xdpb;
	dsk_err_t err;

	err = dd_login(&xdpb, handle - 'A');
	xdpb_to_dg(dg, &xdpb);
	return err;
#else
	/* The PCW driver supports LibDsk's own geometry probe, which is
	 * more comprehensive (supporting DOS and CP/M-86 bootsectors). */
	if (gl_verbose) printf("drvpcw: Not overriding geometry probe.\n");
	return DSK_ERR_NOTME;
#endif
}


dsk_err_t dsk_drive_status(DSK_PDRIVER handle, DSK_GEOMETRY *dg,
		                unsigned head, unsigned char *status)
{
	unsigned drive = (handle - 'A') & 3;

	if ((forcehead[drive] == -1 && head != 0) ||
	    (forcehead[drive] == 1))			drive |= 4;
	*status = dd_drive_status(drive);
	return DSK_ERR_OK;
}




/* PASSES is the number of times a sector header is seen before we deduce 
 * that all the sectors on the disc have been seen by the scan. Higher
 * values of PASSES may make for more accurate results but slower scans. 
 *
 * This function and the two that follow are not PCW-specific and could be 
 * copied verbatim to any driver that supports dsk_psecid, dsk_pread and 
 * dsk_xread respectively.
 */ 
#define PASSES 3	

dsk_err_t dsk_ptrackids(DSK_PDRIVER handle, DSK_GEOMETRY *geom,
				unsigned short cylinder, unsigned short head,
				unsigned short *count, DSK_FORMAT *results)
{
	dsk_err_t err;
	DSK_FORMAT fmt;
	DSK_GEOMETRY gtemp;
	int lcount = 0;
	unsigned sector = 0;
	int n;

	if (!geom || !count || !results)
	       	return DSK_ERR_BADPTR;

	memcpy(&gtemp, geom, sizeof(gtemp));

/* Start by forcing a read error. We set sector size to 256
 * i) because with any luck, the floppy we're scanning won't have 256-byte 
 *   sectors.
 * ii) because the buffer we're using is only 256 bytes long. 
 *
 * dsk_xread is used here because we don't get the whole 
 * 'retry 15 times per read' thing.
 * */
	gtemp.dg_secsize = 256;
	do
	{
/*
		err = dsk_pread(handle,&gtemp,tmpbuf,cylinder,head,
				sector++); */
		err = dsk_xread(handle,&gtemp,tmpbuf,cylinder,head,
				cylinder, head, sector++, 256, NULL); 
	}
	while (err == DSK_ERR_OK);
/* After the read error, we should be right after the index hole. So read
 * off the sector IDs in order. But in case we miss one, go around a few
 * times. */
 
/* This doesn't seem to work.	memset(tmpbuf, 0, 256); */
	for (n = 0; n < 256; n++) tmpbuf[n] = 0;
	for (;;)
	{
		err = dsk_psecid(handle,geom,cylinder,head,&fmt);
		if (err) return err;
		fmt.fmt_sector &= 0xFF;
/*
		printf("Scanning.. found %2d %2d %2d %4d (%d/%d) %d\n",
				fmt.fmt_cylinder,
				fmt.fmt_head,
				fmt.fmt_sector,
				fmt.fmt_secsize,
				tmpbuf[fmt.fmt_sector],
				PASSES,
				lcount); */
		if (!tmpbuf[fmt.fmt_sector])
		{
			memcpy(&results[lcount], &fmt, sizeof(fmt));
			++lcount;
		}
		tmpbuf[fmt.fmt_sector]++;
		if (tmpbuf[fmt.fmt_sector] > PASSES) break;
	}
	if (!lcount) return DSK_ERR_NOADDR;	/* Track blank */

	*count = lcount;
	return DSK_ERR_OK;
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
				sec + geom->dg_secbase, geom->dg_secsize, NULL);
		if (err) return err;
	}
	return DSK_ERR_OK;
}

dsk_err_t dsk_rtread(DSK_PDRIVER self, DSK_GEOMETRY *geom, unsigned char *b,
	                              unsigned cylinder, unsigned head,
				      unsigned mode, unsigned short *bufsize)
{
	*bufsize = 0;
	return DSK_ERR_NOTIMPL;
}


/* Status register bits from <linux/fd.h> */
/* Bits of FD_ST0 */
#define ST0_DS		0x03		/* drive select mask */
#define ST0_HA		0x04		/* Head (Address) */
#define ST0_NR		0x08		/* Not Ready */
#define ST0_ECE		0x10		/* Equipment check error */
#define ST0_SE		0x20		/* Seek end */
#define ST0_INTR	0xC0		/* Interrupt code mask */
/* Bits of FD_ST1 */
#define ST1_MAM		0x01		/* Missing Address Mark */
#define ST1_WP		0x02		/* Write Protect */
#define ST1_ND		0x04		/* No Data - unreadable */
#define ST1_OR		0x10		/* OverRun */
#define ST1_CRC		0x20		/* CRC error in data or addr */
#define ST1_EOC		0x80		/* End Of Cylinder */
/* Bits of FD_ST2 */
#define ST2_MAM		0x01		/* Missing Address Mark (again) */
#define ST2_BC		0x02		/* Bad Cylinder */
#define ST2_SNS		0x04		/* Scan Not Satisfied */
#define ST2_SEH		0x08		/* Scan Equal Hit */
#define ST2_WC		0x10		/* Wrong Cylinder */
#define ST2_CRC		0x20		/* CRC error in data field */
#define ST2_CM		0x40		/* Control Mark = deleted */

/* Manually translate the error from a 765 command to LibDsk format */
static dsk_err_t error_765(int unit, unsigned char *st)
{
	char n;

	/* Save a copy of the status registers */
	for (n = 0; n < 3; n++)
	{
	/*	printf("Set reg(%d,%d):=%02x\n", unit, n, st[n]); */
		status[unit][n] = st[n];
	}
	switch(st[0] & ST0_INTR)
	{
		case 0:		return DSK_ERR_OK;
		case 0x80:	return DSK_ERR_UNKNOWN;	/* Bad command */
		case 0xC0: 	return DSK_ERR_NOTRDY;
	}
/* If we get here, ST0 high bits must be 0x40 */
	if (st[0] & ST0_NR)  return DSK_ERR_NOTRDY;
	if (st[0] & ST0_ECE) return DSK_ERR_ECHECK;
	if (st[1] & ST1_OR)  return DSK_ERR_OVERRUN;
	if (st[1] & ST1_CRC) return DSK_ERR_DATAERR;
	if (st[1] & ST1_ND)  return DSK_ERR_NODATA;
	if (st[1] & ST1_WP)  return DSK_ERR_RDONLY;
	if (st[1] & ST1_MAM) return DSK_ERR_NOADDR;
	if (st[2] & ST2_CRC) return DSK_ERR_DATAERR;
	if (st[2] & ST2_WC)  return DSK_ERR_SEEKFAIL;
	if (st[2] & ST2_CM)  return DSK_ERR_NODATA;
	if (st[2] & ST2_BC)  return DSK_ERR_SEEKFAIL;
	if (st[2] & ST2_MAM) return DSK_ERR_NOADDR;
	return DSK_ERR_UNKNOWN;
}


/*
static dsk_err_t debug_error_765(int unit, unsigned char *st)
{
	dsk_err_t err;

	printf("error_765(%02x %02x %02x) = ", st[0], st[1], st[2]);
	err = error_765(unit, st);
	printf("%d\n", err);
	return err;
}
#define error_765 debug_error_765
*/
dsk_err_t dsk_pseek(DSK_PDRIVER handle, DSK_GEOMETRY *dg,
		unsigned cylinder, unsigned head)
{
	dsk_err_t err;
	unsigned drive = (handle - 'A') & 3;
/* Version using the dd_l_seek call */
	XDPB xdpb;

	dg_to_xdpb(dg, &xdpb);
	if (doublestep[drive]) cylinder *= 2;
	if ((forcehead[drive] == -1 && head != 0) ||
	    (forcehead[drive] == 1))			drive |= 4;
	dd_l_on_motor();
	err = dd_l_seek(&xdpb, drive, cylinder);
	dd_l_t_off_motor(); 

/* Version using direct FDC access -- DOES NOT WORK 
	DD_L_BUF lbuf;
	unsigned char tmp, *result;

	if (doublestep[drive]) cylinder *= 2;
	if ((forcehead[drive] == -1 && head != 0) ||
	    (forcehead[drive] == 1))			drive |= 4;
	lbuf.bank = 1;
	lbuf.address = &tmp;
	lbuf.datalen = 0;
	lbuf.commandlen = 3;
	lbuf.command[0] = 0x0F;	
	lbuf.command[1] = drive;
	lbuf.command[2] = cylinder;
	dd_l_on_motor();
	result = dd_l_read(&lbuf);
	if (result[0] < 3) err = DSK_ERR_UNKNOWN;
	else err = error_765(drive & 3, result+1);	
	dd_l_t_off_motor();	*/
	return err;
}


/* dsk_xread and dsk_xwrite are implemented using direct access to the 
 * host system's floppy controller (compare drvlinux.c). We could do as
 * drvlinux does and implement dsk_pread / dsk_pwrite on top of the x versions
 * but since I implemented the dsk_p* ones first and they work, I don't see
 * the need to change them. */
dsk_err_t dsk_xread(DSK_PDRIVER handle, DSK_GEOMETRY *geom, unsigned char *buf,
	unsigned cyl, unsigned head, unsigned cyl_expected,
	unsigned head_expected, unsigned sector, unsigned sectorsize,
	unsigned long *deleted)

{
	DD_L_BUF lbuf;
	dsk_err_t err;
	unsigned char mask = 0xFF;
	unsigned char unit = (handle - 'A') & 3;
	unsigned char *result;
	int del = 0;

	if (deleted) del = *deleted;
	if ((forcehead[unit] == -1 && head != 0) ||
	    (forcehead[unit] == 1))			unit |= 4;
        if (geom->dg_noskip)  mask &= ~0x20;    /* Don't skip deleted data */
        if (geom->dg_fm)      mask &= ~0x40;    /* FM recording mode */
/*      if (geom->dg_nomulti) */mask &= ~0x80;    /* Disable multitrack */

	lbuf.bank = 1;
	lbuf.address = buf;
	lbuf.datalen = sectorsize;
	lbuf.commandlen = 9;
	if (del) lbuf.command[0] = 0xEC & mask;
	else     lbuf.command[0] = 0xE6 & mask;
	lbuf.command[1] = unit;
	lbuf.command[2] = cyl_expected;
	lbuf.command[3] = head_expected;
	lbuf.command[4] = sector;
	lbuf.command[5] = dsk_get_psh(geom->dg_secsize);
	lbuf.command[6] = sector;
	lbuf.command[7] = geom->dg_rwgap;
	lbuf.command[8] = (sectorsize < 256) ? sectorsize : 0xFF;

	dd_l_on_motor();
	err = dsk_pseek(handle, geom, cyl, head);
	/* err = dd_l_seek(&xdpb, unit, cyl); */
	if (!err)
	{
/* int n;
printf("Calling dd_l_read (deleted=%d) with ", del);
for (n = 0; n < 6 + lbuf.commandlen; n++) printf("%02x ", ((unsigned char *)&lbuf)[n]);
printf("\n"); */

		result = dd_l_read(&lbuf);
/* printf("It returned ");
for (n = 1; n <= result[0]; n++) printf("%02x ", result[n]);
printf("\nBuffer holds ");
for (n = 0; n < 20; n++) printf("%02x ", buf[n]);
printf("\n"); */
		if (result[0] < 3) err = DSK_ERR_UNKNOWN;
		else err = error_765(unit & 3, result+1);
	}	
	dd_l_t_off_motor();
	return err;
}



dsk_err_t dsk_xwrite(DSK_PDRIVER handle, DSK_GEOMETRY *geom, unsigned char *buf,
	unsigned cyl, unsigned head, unsigned cyl_expected,
	unsigned head_expected, unsigned sector, unsigned sectorsize,
	unsigned deleted)

{
	DD_L_BUF lbuf;
	dsk_err_t err;
	unsigned char mask = 0xFF;
	unsigned char unit = (handle - 'A') & 3;
	unsigned char *result;

	if ((forcehead[unit] == -1 && head != 0) ||
	    (forcehead[unit] == 1))			unit |= 4;
        if (geom->dg_noskip)  mask &= ~0x20;    /* Don't skip deleted data */
        if (geom->dg_fm)      mask &= ~0x40;    /* FM recording mode */
/*      if (geom->dg_nomulti) */mask &= ~0x80;    /* Disable multitrack */

	lbuf.bank = 1;
	lbuf.address = buf;
	lbuf.datalen = sectorsize;
	lbuf.commandlen = 9;
	if (deleted) lbuf.command[0] = 0xE9 & mask;
	else	     lbuf.command[0] = 0xC5 & mask;
	lbuf.command[1] = unit;
	lbuf.command[2] = cyl_expected;
	lbuf.command[3] = head_expected;
	lbuf.command[4] = sector;
	lbuf.command[5] = dsk_get_psh(geom->dg_secsize);
	lbuf.command[6] = sector;
	lbuf.command[7] = geom->dg_rwgap;
	lbuf.command[8] = (sectorsize < 256) ? sectorsize : 0xFF;

	dd_l_on_motor();
	err = dsk_pseek(handle, geom, cyl, head);
	/* err = dd_l_seek(&xdpb, unit, cyl); */
	if (!err)
	{
/* int n;
printf("Calling dd_l_write with ");
for (n = 0; n < 6 + lbuf.commandlen; n++) printf("%02x ", ((unsigned char *)&lbuf)[n]);
printf("\n"); */
		result = dd_l_write(&lbuf);
/* printf("It returned ");
for (n = 1; n <= result[0]; n++) printf("%02x ", result[n]);
printf("\n"); */
		if (result[0] < 3) err = DSK_ERR_UNKNOWN;
		else err = error_765(unit & 3, result+1);
	}	
	dd_l_t_off_motor();
	return err;
}



/* Valid options are the first 3 status registers */
dsk_err_t dsk_option_enum(DSK_PDRIVER handle, unsigned opt, char **value)
{
	switch(opt)
	{
/* Disable the HEAD and DOUBLESTEP options. They won't work with functions
 * other than dsk_xread and dsk_xwrite. *
		case 0: *value = "DOUBLESTEP"; return DSK_ERR_OK;
		case 1: *value = "HEAD"; return DSK_ERR_OK;
		case 2: *value = "ST0"; return DSK_ERR_OK;
		case 3: *value = "ST1"; return DSK_ERR_OK;
		case 4: *value = "ST2"; return DSK_ERR_OK; */
		case 0: *value = "ST0"; return DSK_ERR_OK;
		case 1: *value = "ST1"; return DSK_ERR_OK;
		case 2: *value = "ST2"; return DSK_ERR_OK; 
	}
	*value = NULL;
	return DSK_ERR_BADOPT;
}

dsk_err_t dsk_set_option(DSK_PDRIVER handle, char *option, short value)
{

/* Disable the HEAD and DOUBLESTEP options. They won't work with functions
 * other than dsk_xread and dsk_xwrite. *
	char unit = (handle - 'A') & 3;
	if (!strcmp(option, "HEAD"))
	{
		if (value == 0 || value == 1 || value == -1)
		{
			forcehead[unit] = value;
			return DSK_ERR_OK;		
		}
		return DSK_ERR_BADVAL;
	}
	if (!strcmp(option, "DOUBLESTEP"))
	{
		if (value == 0 || value == 1)
		{
			doublestep[unit] = value;
			return DSK_ERR_OK;		
		}
		return DSK_ERR_BADVAL;
	}
*/

	if (option[0] != 'S' || option[1] != 'T') return DSK_ERR_BADOPT;
	if (option[2] < '0'  || option[2] >  '2') return DSK_ERR_BADOPT;
	return DSK_ERR_BADVAL;	/* Read-only options */
}

/* Return the values of the status registers */
dsk_err_t dsk_get_option(DSK_PDRIVER handle, char *option, short *value)
{
	int y;
	char unit = (handle - 'A') & 3;

/* Disable the HEAD and DOUBLESTEP options. They won't work with functions
 * other than dsk_xread and dsk_xwrite. *
	if (!strcmp(option, "HEAD"))
	{
		v[0] = forcehead[unit];
		return DSK_ERR_OK;
	}
	if (!strcmp(option, "DOUBLESTEP"))
	{
		v[0] = doublestep[unit];
		return DSK_ERR_OK;
	} */
	if (option[0] != 'S' || option[1] != 'T') return DSK_ERR_BADOPT;
	if (option[2] < '0'  || option[2] >  '2') return DSK_ERR_BADOPT;

	y = option[2] -'0';
	value[0] = status[unit][y];
/*	printf("Get register (%d,%d)=%02x %d\n", unit, y, status[unit][y], value[0]); */

	return DSK_ERR_OK;
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
		RPC_DSK_XREAD, RPC_DSK_XWRITE, RPC_DSK_PTREAD,
		RPC_DSK_XTREAD, /* RPC_DSK_RTREAD, */
		RPC_DSK_OPTION_ENUM, RPC_DSK_OPTION_GET, RPC_DSK_OPTION_SET,  
		/* RPC_DSK_GETCOMMENT, RPC_DSK_SETCOMMENT, */
		RPC_DSK_PFORMAT, /*RPC_DSK_GETGEOM, */ RPC_DSK_PSECID,
		RPC_DSK_PSEEK, RPC_DSK_TRACKIDS, RPC_DSK_PROPERTIES
	};
	*pprops = props;
	*propcount = sizeof(props) / sizeof(props[0]);
	*desc = "Amstrad XBIOS driver";
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

