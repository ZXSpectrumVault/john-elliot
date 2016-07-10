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

/* Minidsk is a cut-down LibDsk, intended for use in the CP/M LibDsk server
 * (auxd). It's similar to the full-blown LibDsk API, but with a few changes.
 * 
 * > The expectation is that there are no multiple drivers; instead, that
 *  minidsk will provide access to raw drives only.
 *
 * > Because of this, DSK_PDRIVER is typedeffed as a short int, since that's
 *  plenty to hold a drive number. 
 *
 *  > No const. Hi-Tech C doesn't support it. 
 */

typedef unsigned short DSK_PDRIVER;

/* Error codes */
#define DSK_ERR_OK       (0)    /* No error */
#define DSK_ERR_BADPTR   (-1)   /* Bad pointer */
#define DSK_ERR_DIVZERO  (-2)   /* Division by zero */
#define DSK_ERR_BADPARM  (-3)   /* Bad parameter */
#define DSK_ERR_NODRVR   (-4)   /* Driver not found */
#define DSK_ERR_NOTME    (-5)   /* Driver rejects disc */
#define DSK_ERR_SYSERR   (-6)   /* System error, use errno */
#define DSK_ERR_NOMEM    (-7)   /* Null return from malloc */
#define DSK_ERR_NOTIMPL  (-8)   /* Function not implemented */
#define DSK_ERR_MISMATCH (-9)   /* Check sectors: No match*/
#define DSK_ERR_NOTRDY   (-10)  /* Drive not ready */
#define DSK_ERR_RDONLY   (-11)  /* Read-only disc */
#define DSK_ERR_SEEKFAIL (-12)  /* Seek fail */
#define DSK_ERR_DATAERR  (-13)  /* CRC data error */
#define DSK_ERR_NODATA   (-14)  /* No data */
#define DSK_ERR_NOADDR   (-15)  /* Missing address mark */
#define DSK_ERR_BADFMT   (-16)  /* Bad format */
#define DSK_ERR_CHANGED  (-19)  /* Disc changed */
#define DSK_ERR_ECHECK   (-20)  /* Equipment check */
#define DSK_ERR_OVERRUN  (-21)  /* Overrun */
#define DSK_ERR_ACCESS   (-22)  /* Access denied */
#define DSK_ERR_CTRLR    (-23)  /* Failed controller */
#define DSK_ERR_COMPRESS (-24)  /* Compressed file is corrupt */
#define DSK_ERR_RPC      (-25)  /* Error encoding / decoding RPC */
#define DSK_ERR_BADOPT   (-26)  /* Requested optional feature not present */
#define DSK_ERR_BADVAL   (-27)  /* Bad value for requested option */
#define DSK_ERR_ABORT    (-28)  /* User abort requested */
#define DSK_ERR_TIMEOUT  (-29)  /* Communications timed out */
#define DSK_ERR_UNKRPC   (-30)	/* RPC server does not recognise function */
#define DSK_ERR_UNKNOWN  (-99)  /* Unknown error */

/* Sidedness */
#define SIDES_ALT     0
#define SIDES_OUTBACK 1
#define SIDES_OUTOUT  2

/* Rates */
#define RATE_HD 0
#define RATE_DD 1
#define RATE_SD 2
#define RATE_ED 3

typedef int dsk_err_t;

dsk_err_t read_packet(unsigned char  *pkt, int *len);
dsk_err_t write_packet(unsigned char *pkt, int len);
dsk_err_t dsk_rpc_server(unsigned char *input, int inp_len, 
		         unsigned char *output, int *out_len,
			 int *ref_count);

/* DSK_GEOMETRY holds information used to convert physical to/from logical
 * sectors and to access the disc
 *
 * In the CP/M-80 version, everything that can be is a byte
 */ 
typedef struct
{
	unsigned char	dg_sidedness;	/* How to handle multisided discs? */
	unsigned  	dg_cylinders;	/* Number of cylinders */
	unsigned char	dg_heads;	/* Number of heads */
	unsigned char	dg_sectors;	/* Sectors per track */
	unsigned char	dg_secbase;	/* First physical sector number */
	size_t		dg_secsize;	/* Sector size in bytes */
	unsigned char	dg_datarate;	/* Data rate */
	unsigned char	dg_rwgap;	/* Read/write gap length */
	unsigned char	dg_fmtgap;	/* Format gap length */
	unsigned char	dg_fm;		/* FM mode? The only thing I know that uses FM mode
					 * is the BBC Micro DFS format. */
	unsigned char	dg_nomulti;	/* Disable multitrack? */
	unsigned char   dg_noskip;	/* Set to 0 to skip deleted data */
} DSK_GEOMETRY;

typedef struct
{
	unsigned char fmt_cylinder;
	unsigned char fmt_head;
	unsigned char fmt_sector;
	size_t fmt_secsize;
} DSK_FORMAT;

/* Functions */

dsk_err_t dsk_init();

dsk_err_t dsk_open(DSK_PDRIVER *handle, char *filename, char *driver,
		char *compress);
dsk_err_t dsk_close(DSK_PDRIVER handle);

dsk_err_t dsk_pread(DSK_PDRIVER handle, DSK_GEOMETRY *dg, unsigned char *buf,
		unsigned cyl, unsigned head, unsigned sector);
dsk_err_t dsk_pwrite(DSK_PDRIVER handle, DSK_GEOMETRY *dg, unsigned char *buf,
		unsigned cyl, unsigned head, unsigned sector);
dsk_err_t dsk_pformat(DSK_PDRIVER handle, DSK_GEOMETRY *dg, unsigned cyl, 
		unsigned head, DSK_FORMAT *fmt, unsigned char filler);
dsk_err_t dsk_getgeom(DSK_PDRIVER handle, DSK_GEOMETRY *dg);
dsk_err_t dsk_psecid(DSK_PDRIVER handle, DSK_GEOMETRY *dg, 
		unsigned cyl, unsigned head, DSK_FORMAT *fmt);
dsk_err_t dsk_drive_status(DSK_PDRIVER handle, DSK_GEOMETRY *dg, 
		unsigned head, unsigned char *status);
dsk_err_t dsk_pseek(DSK_PDRIVER handle, DSK_GEOMETRY *dg, 
		unsigned cyl, unsigned head);
dsk_err_t dsk_properties(DSK_PDRIVER handle, unsigned short **props,
		unsigned short *propcount, char **desc);
dsk_err_t dsk_ptrackids(DSK_PDRIVER handle, DSK_GEOMETRY *dg, 
		unsigned short cylinder, unsigned short head, 
		unsigned short *count, DSK_FORMAT *results);
dsk_err_t dsk_xread(DSK_PDRIVER handle, DSK_GEOMETRY *dg, unsigned char *buf,
		unsigned cyl, unsigned head, unsigned cyl_expected,
		unsigned head_expected, unsigned sector, unsigned sectorsize,
		unsigned long *deleted);
dsk_err_t dsk_xwrite(DSK_PDRIVER handle, DSK_GEOMETRY *dg, unsigned char *buf,
		unsigned cyl, unsigned head, unsigned cyl_expected,
		unsigned head_expected, unsigned sector, unsigned sectorsize,
		unsigned deleted);
dsk_err_t dsk_ptread(DSK_PDRIVER handle, DSK_GEOMETRY *dg, unsigned char *buf,
		unsigned cyl, unsigned head);
dsk_err_t dsk_rtread(DSK_PDRIVER handle, DSK_GEOMETRY *dg, unsigned char *buf,
		unsigned cyl, unsigned head, unsigned mode, unsigned short *bufsize);
dsk_err_t dsk_xtread(DSK_PDRIVER handle, DSK_GEOMETRY *dg, unsigned char *buf,
		unsigned cyl, unsigned head, unsigned cyl_expected, 
		unsigned head_expected);
dsk_err_t dsk_option_enum(DSK_PDRIVER handle, unsigned opt, char **value);
dsk_err_t dsk_set_option(DSK_PDRIVER handle, char *option, short value);
dsk_err_t dsk_get_option(DSK_PDRIVER handle, char *option, short *value);
dsk_err_t dsk_get_comment(DSK_PDRIVER handle, char **comment);
dsk_err_t dsk_set_comment(DSK_PDRIVER handle, char *comment);

