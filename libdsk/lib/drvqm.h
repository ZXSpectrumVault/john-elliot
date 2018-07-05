/***************************************************************************
 *                                                                         *
 *    LIBDSK: General floppy and diskimage access library                  *
 *    Copyright (C) 2001-2,2005  John Elliott <seasip.webmaster@gmail.com>     *
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

/*
 * Release 2011-04-06
*/
/* Declarations for the CopyQM driver */
typedef enum
{
	QM_DRV_UNKNWN = 0,
	QM_DRV_525DD = 1,
	QM_DRV_525HD = 2,
	QM_DRV_350DD = 3,
	QM_DRV_350HD = 4,
	QM_DRV_350ED = 6,
} qm_drv_t;

typedef struct
{
	LDBSDISK_DSK_DRIVER    qm_super;
	char*         qm_filename;

	/* Fields from the DOS BPB */
	size_t        qm_h_bpb_sector_size;
	unsigned      qm_h_bpb_secclus;
	unsigned      qm_h_bpb_reserved;
	unsigned      qm_h_bpb_fats;
	unsigned      qm_h_bpb_rootentries;
	dsk_lsect_t   qm_h_bpb_total_sectors;
	unsigned char qm_h_bpb_media_id;
	unsigned      qm_h_bpb_secfat;
	unsigned      qm_h_bpb_sectrack;
	unsigned      qm_h_bpb_heads;
	unsigned      qm_h_bpb_hidden;
	int           qm_h_comment_len;
	/* Density - 1 means HD, 2 means QD */
	int           qm_h_density;
	/* Blind transfer or not. */
	int           qm_h_blind;
	dsk_pcyl_t    qm_h_used_cyls;
	dsk_pcyl_t    qm_h_total_cyls;
	/* Interleave */
	int           qm_h_interleave;
	/* Skew. Negative number for skew between sides */
	int           qm_h_skew;
	/* Sector number base. */
	unsigned char qm_h_secbase;
	/* Source drive type */
	qm_drv_t      qm_h_drive;
	/* The crc read from the header */
	unsigned long qm_h_crc;
	/* The crc calculated while the image is read */
	unsigned long qm_calc_crc;
	unsigned int  qm_image_offset;
	/* File handle when saving */
	FILE *        qm_fp;
	unsigned      qm_density[3];
} QM_DSK_DRIVER;

/* Constants for the QM header fields */

#define QM_HEADER_SIZE  133
#define QM_H_BASE         0
#define QM_H_SECSIZE   0x03	/* Start DOS BPB -- from here down to 0x14 */
#define QM_H_SECCLUS   0x05
#define QM_H_RESERVED  0x06
#define QM_H_FATS      0x08
#define QM_H_ROOTENTRIES 0x09
#define QM_H_SECTOTL   0x0b
#define QM_H_MEDIA_ID  0x0d
#define QM_H_SECFAT    0x0e
#define QM_H_SECPTRK   0x10
#define QM_H_HEADS     0x12
#define QM_H_HIDDEN    0x14
#define QM_H_DESCR     0x1c
#define QM_H_DSCR_SIZE   60
#define QM_H_BLIND     0x58
#define QM_H_DENS      0x59
#define QM_H_USED_CYL  0x5a
#define QM_H_TOTL_CYL  0x5b
#define QM_H_DATA_CRC  0x5c
#define QM_H_LABEL     0x60
#define QM_H_LBL_SIZE    11
#define QM_H_TIME      0x6b
#define QM_H_DATE      0x6d
#define QM_H_CMT_SIZE  0x6f
#define QM_H_SECBASE   0x71
#define QM_H_INTLV     0x74
#define QM_H_SKEW      0x75
#define QM_H_DRIVE     0x76
#define QM_H_HEAD_CRC  0x84

#define QM_BLIND_DOS      0
#define QM_BLIND_BLN      1
#define QM_BLIND_HFS      2

#define QM_DENS_DD        0
#define QM_DENS_HD        1
#define QM_DENS_ED        2
