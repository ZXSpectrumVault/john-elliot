/***************************************************************************
 *                                                                         *
 *    LIBDSK: General floppy and diskimage access library                  *
 *    Copyright (C) 2001-2,2007  John Elliott <seasip.webmaster@gmail.com>     *
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

/* Declarations for the Teledisk driver */

/* This structure (for the file header) is based on:
 * <http://www.fpns.net/willy/wteledsk.htm> 
 * <ftp://bitsavers.informatik.uni-stuttgart.de/pdf/sydex/Teledisk_1.05_Sep88.pdf>
 * <http://www.classiccmp.org/dunfield/img54306/td0notes.txt>
 *
 * In LibDsk 1.5.3, rewritten to do a Teledisk -> LDBS conversion and 
 * hand off to the LDBS driver.
 */

typedef unsigned char tele_byte;
typedef unsigned short tele_word;

/* The header and comment block of a Teledisk file have quite a bit of 
 * information that LDBS can't hold or calculate. To hold it, the TELE_HEADER 
 * will be saved to the blockstore as a 'utel' block. 
 *
 * Note: Binary compatibility should not be a problem because all fields 
 * are single bytes.
 */

typedef struct
{
	char magic[3];
	tele_byte volume_seq;	/* 0 for first file in sequence, 1 for 2nd... */
	tele_byte volume_id;	/* All files must have the same volume ID */
	tele_byte ver;		/* Version of Teledisk used to create file */
	tele_byte datarate;	/* 0=250Kbps (360k/720k) 1=300Kbps (1.2M)
				   2=500Kbps (1.4M). Bit 7 set for FM. */
	tele_byte drivetype;	/* 1=360k 2=1.2M 3=720k 4=1.4M */
	tele_byte doubletrack;	/* 0=media matched drive
				   1=48tpi media, 96tpi drive
				   2=96tpi media, 48tpi drive
				  
				   Bit 7 set if comment present. */
	tele_byte dosmode;	/* Only allocated sectors backed up? */
	tele_byte sides;	/* Number of heads */

} TELE_HEADER;


typedef struct
{
	LDBSDISK_DSK_DRIVER	tele_super;
	char 		*tele_filename;
	TELE_HEADER	tele_head;	
	FILE		*tele_fp;

	/* Stats used when saving */
	unsigned tele_fm;	/* Number of FM tracks */
	unsigned tele_mfm;	/* Number of MFM tracks */
	unsigned tele_rate[3];	/* Number of tracks at various data rates */

} TELE_DSK_DRIVER;


dsk_err_t tele_open(DSK_DRIVER *self, const char *filename);
dsk_err_t tele_creat(DSK_DRIVER *self, const char *filename);
dsk_err_t tele_close(DSK_DRIVER *self);

