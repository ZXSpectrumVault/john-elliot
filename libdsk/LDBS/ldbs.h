/* LDBS: LibDsk Block Store access functions
 *
 *  Copyright (c) 2016,17 John Elliott <seasip.webmaster@gmail.com>
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a 
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation 
 * the rights to use, copy, modify, merge, publish, distribute, sublicense, 
 * and/or sell copies of the Software, and to permit persons to whom the 
 * Software is furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included 
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS 
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL 
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR 
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, 
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR 
 * OTHER DEALINGS IN THE SOFTWARE. */

#define LDBS_VERSION "0.3"	/* Version of the LDBS spec this library
				 * implements */

/* Structures and functions to manage a LibDsk Block Store disk image, 
 * or other file based on the LDBS structure.
 *
 * This was originally written as a temporary buffer for versions of LibDsk
 * with memory constraints. A number of backends -- particularly those where 
 * the file is held in a compressed format -- work by loading their disc 
 * image into memory, manipulating the memory copy, and then writing it out 
 * in their close() handler.
 *
 * On systems where 640k is enough for anyone, there may not be enough memory
 * to do this. So instead, expand to a 'block store' file on disk, which 
 * allows in-place rewriting. 
 *
 * Once this was done, it was a small step to designing a persistent disk 
 * image format using the same on-disk structure.
 *
 * LDBS functions accordingly come in two groups: One set to implement a
 * generic block store, and the second to implement a persistent disk 
 * image format on top of it.
 *
 * A block store may be temporary or permanent. On systems other than MSDOS,
 * a temporary block store exists only in memory, with no backing disk file.
 */

#ifdef __MSDOS__
#define LDBS_TEMP_IN_MEM  0	/* Temporary blockstores are held on disk */
#else
#define LDBS_TEMP_IN_MEM  1	/* Temporary blockstores are held in memory */
#endif

/* All blocks in the block store are referenced by a 32-bit LDBLOCKID. For a 
 * file that's persisted on disk, this is an offset in the backing file.
 * A temporary store may be implemented in terms of malloc() and free(),
 * in which case its LDBLOCKIDs may be typecast pointers (on a 16- or 32-bit
 * system) or mapped to pointers using a lookup table
 * (on a system with larger pointers).
 *
 * If long is larger than 32 bits that's fine, as long as the value it
 * contains never exceeds 2^31 .
 */

typedef long LDBLOCKID;

/* New block IDs should be initialised to LDBLOCKID_NULL */
#define LDBLOCKID_NULL (0)

/* LDBS can be built as part of libdsk or standalone. If standalone, here 
 * are the bits of libdsk.h it needs */

#ifdef LIBDSK_H
# define ldbs_malloc dsk_malloc
# define ldbs_realloc dsk_realloc
# define ldbs_free dsk_free
#else
typedef          int  dsk_err_t;        /* Error number */
typedef unsigned int  dsk_psect_t;      /* Physical sector */
typedef unsigned int  dsk_pcyl_t;       /* Physical cylinder */
typedef unsigned int  dsk_phead_t;      /* Physical head */
typedef unsigned char dsk_gap_t;        /* Gap length */

# define DSK_ERR_OK       (0)    /* No error */
# define DSK_ERR_BADPTR   (-1)   /* Bad pointer */
# define DSK_ERR_DIVZERO  (-2)   /* Division by zero */
# define DSK_ERR_BADPARM  (-3)   /* Bad parameter */
# define DSK_ERR_NODRVR   (-4)   /* Driver not found */
# define DSK_ERR_NOTME    (-5)   /* File is not in correct format */
# define DSK_ERR_SYSERR   (-6)   /* System error, use errno */
# define DSK_ERR_NOMEM    (-7)   /* Null return from malloc */
# define DSK_ERR_NOADDR   (-15)  /* Missing address mark */
# define DSK_ERR_OVERRUN  (-21)	 /* Overrun */
# define DSK_ERR_CORRUPT  (-32)	 /* Disk image is corrupt */
# define ldbs_malloc malloc
# define ldbs_realloc realloc
# define ldbs_free free

/* Disc sidedness (logical/physical mapping). Use SIDES_ALT for single-sided floppies. */
typedef enum 
{	
	SIDES_ALT, 	/* Track n is cylinder (n/heads) head (n%heads) */
	SIDES_OUTBACK, 	/* Tracks go (head 0) 0,1,2,3,...37,38,39, then
			             (head 1) 39,38,37,...,2,1,0 */
	SIDES_OUTOUT,	/* Tracks go (head 0) 0,1,2,3,...37,38,39, then
			             (head 1) 0,1,2,3,...37,38,39 */
	SIDES_EXTSURFACE/* As SIDES_ALT, but sectors on head 1 identify
			 * as head 0, with numbers in sequence 
			 * eg: Head 0 has sectors 1-9, head 1 has 10-18 */
} dsk_sides_t;

typedef enum
{
	RATE_HD,	/* Data rate for 1.4Mb 3.5"  in 3.5"  drive */
	RATE_DD,	/* Data rate for 360k  5.25" in 1.2Mb drive */
	RATE_SD,	/* Data rate for 720k  3.5"  in 3.5"  drive */
	RATE_ED		/* Data rate for 2.8Mb 3.5"  in 3.5"  drive */
} dsk_rate_t;


typedef enum
{
	/* Low byte of dg_fm: Recording mode */
	RECMODE_MASK     = 0x00FF,
	RECMODE_MFM      = 0x0000,	
	RECMODE_FM       = 0x0001,
	/* High byte of dg_fm: Other data recording flags */
	RECMODE_FLAGMASK = 0xFF00,
	RECMODE_COMPLEMENT = 0x0100	
	
} dsk_recmode_t;


typedef struct dsk_geometry
{
	dsk_sides_t	dg_sidedness;	/* How to handle multisided discs? */
	dsk_pcyl_t  	dg_cylinders;	/* Number of cylinders */
	dsk_phead_t	dg_heads;	/* Number of heads */
	dsk_psect_t	dg_sectors;	/* Sectors per track */
	dsk_psect_t	dg_secbase;	/* First physical sector number */
	size_t		dg_secsize;	/* Sector size in bytes */
	dsk_rate_t	dg_datarate;	/* Data rate */
	dsk_gap_t	dg_rwgap;	/* Read/write gap length */
	dsk_gap_t	dg_fmtgap;	/* Format gap length */
	int		dg_fm;		/* Really a dsk_recmode_t, kept as
					 * int for backward compatibility  */
	int		dg_nomulti;	/* Disable multitrack? */
	int		dg_noskip;	/* Set to 0 to skip deleted data */
} DSK_GEOMETRY;

#endif	/* LIBDSK_H */


/* Public structures: These correspond to individual records in the block
 * store. */

typedef struct ldbs_header	/* File header */
{
	char	magic[4];	/* Magic number, 'LBS\1' */
	char	subtype[4];	/* File subtype, set by ldbs_new() */
	LDBLOCKID used;		/* Offset of first block in 'used' list */
	LDBLOCKID free;		/* Offset of first block in 'free' list */
	LDBLOCKID trackdir;	/* Offset of 'track directory' block */
	int	dirty;		/* 'Dirty' flag, not persisted */
} LDBS_HEADER;

typedef struct ldbs_blockhead	/* Block header */
{
	char	magic[4];	/* Magic number, "LDB\1' */
	char	type[4];	/* Block type, set by ldbs_addblock() 
				 * For blocks referenced in the track
				 * directory, this is unique within the file.
				 * Otherwise it can be non-unique */
	long	dlen;		/* Block length on disk */
	long	ulen;		/* Amount of block in use (<= dlen) */
	LDBLOCKID next;		/* Next block in linked list */
} LDBS_BLOCKHEAD;

/* Each entry in the track directory contains ID and location within the 
 * file */
typedef struct ldbs_trackdir_entry
{
	char id[4];		/* Object identifier */
	LDBLOCKID blockid;	/* Object offset in file */
} LDBS_TRACKDIR_ENTRY;

/* The sector header contains one of these for each sector */
typedef struct ldbs_sector_entry
{
	unsigned char id_cyl;	/* Cylinder number */
	unsigned char id_head;	/* Head number */
	unsigned char id_sec;	/* Sector number */
	unsigned char id_psh;	/* Sector size: 0=>128, 1=>256, 2=>512... */
	unsigned char st1;	/* 8272 status flags 1 */
	unsigned char st2;	/* 8272 status flags 1 */
	unsigned char copies;	/* Number of copies stored. If zero the
       			 	 * sector is entirely filled with filler byte */
	unsigned char filler;	/* Format filler byte */
	LDBLOCKID blockid;
	unsigned short trail;	/* [LDBS 0.3] Number of CRC and GAP#3 bytes
				 *            included in the sector*/
	unsigned short offset;	/* [LDBS 0.3] Offset of sector within track */
} LDBS_SECTOR_ENTRY;

/* Variable-size structures for track directory and track header. 
 * These should be allocated with ldbs_trackdir_alloc() and 
 * ldbs_trackhead_alloc() respectively.
 *
 * It should not be necessary to manipulate the track directory directly --
 * ldbs_getblock_d() and ldbs_putblock_d() should suffice for most 
 * purposes. But I've exposed it anyway.
 */


typedef struct ldbs_trackdir		/* Track directory. */
{
	unsigned short count;		/* Count of entries */
	int dirty;			/* Dirty flag, not persisted */
	LDBS_TRACKDIR_ENTRY entry[1];	/* Entries */	

} LDBS_TRACKDIR;

typedef struct ldbs_trackhead		/* Track header */
{
	unsigned short count;		/* Count of entries */
	unsigned char datarate;		/* Data rate: 
					 *  0 => unknown
					 *  1 => single or double density 
					 *  2 => high density
					 *  3 => extended density */
	unsigned char recmode;		/* Recording mode: 
					 *  0 => unknown
					 *  1 => FM
					 *  2 => MFM */
	dsk_gap_t  gap3;		/* Format GAP3 */
	unsigned char filler;		/* Format filler byte */
	unsigned short total_len;	/* Total size of track including
					 * address marks and gaps */
	int dirty;			/* Dirty flag, not persisted */
	LDBS_SECTOR_ENTRY sector[1];
} LDBS_TRACKHEAD;

/* CP/M 3 disk parameter block */
typedef struct ldbs_dpb
{
	unsigned short spt;		/* Number of 128-byte records / track */
	unsigned char bsh;		/* log2(block size) - 7 */
	unsigned char blm;		/* (block size - 1) / 256 */
	unsigned char exm;		/* Extent mask */
	unsigned short dsm;		/* Number of blocks - 1 */
	unsigned short drm;		/* Number of directory entries - 1 */
	unsigned char al[2];		/* Directory allocation bitmap */
	unsigned short cks;		/* Number of directory entries / 4 */
	unsigned short off;		/* Number of reserved tracks */
	unsigned char psh;		/* log2(sector size) - 7 */
	unsigned char phm;		/* (sector size - 1) / 256 */
} LDBS_DPB;


/* Handle to an LDBS file / memory store */
typedef struct ldbs *PLDBS;

/***************************************************************************/
/************************* BLOCKSTORE FUNCTIONS ****************************/
/***************************************************************************/


/* ldbs_new: Create a new block store. 
 * 
 * filename is NULL to create a temporary file (or in-memory structure), 
 *   non-NULL to create a persistent file with the specified filename.
 * type is a 4-byte code denoting file type.
 *   Pass LDBS_DSK_TYPE for a disk image file.
 *   Pass NULL for a temporary file.
 *
 * On success populates 'result', returns DSK_ERR_OK.
 * On failure *result = NULL, returns error:
 *      DSK_ERR_BADPTR result pointer is NULL
 *      DSK_ERR_NOMEM  if failed to allocate memory
 *      DSK_ERR_SYSERR on I/O error
 */
dsk_err_t ldbs_new(PLDBS *result, const char *filename, const char type[4]);


/* ldbs_open: Open an existing block store. 
 * 
 * filename is the name of the block store on disk.
 * type is a 4-byte buffer that will receive the file type code. 
 * Set *readonly to nonzero to attempt to open read-only.
 *
 * On success returns DSK_ERR_OK.
 *            *result   will be the handle of the new block store.
 *            *readonly will be set to 1 if file opened read-only.
 *
 * On failure sets *result to NULL, returns error:
 *      DSK_ERR_BADPTR result pointer is NULL
 *      DSK_ERR_NOTME  if file is not a valid LDBS block store
 *      DSK_ERR_NOMEM  if failed to allocate memory
 *      DSK_ERR_SYSERR on I/O error
 *      		<or result from ldbs_get_trackdir>
 *
 */
dsk_err_t ldbs_open(PLDBS *result, const char *filename, char type[4], 
			int *readonly);

/* ldbs_close: Close block store. 
 * If it is temporary the backing file will be deleted;
 * if it is held in memory, the memory will be freed. 
 *
 * Returns:
 * 	DSK_ERR_OK on success
 * 	DSK_ERR_BADPTR if pointer is NULL
 * 	DSK_ERR_SYSERR on I/O error
 *      		<or result from ldbs_put_trackdir>
 */
dsk_err_t ldbs_close(PLDBS *blockstore);


/* ldbs_getblockinfo: Get information about a block from the store.
 *
 * Enter with: blockid is the block to retrieve
 *	       type    buffer to be populated with the block type, can be
 *	       	       NULL if you don't care.
 *             len     will be populated with the block length, can be NULL
 *                     if you don't care.
 *
 * On success:
 * 		Populates *len with actual block length
 * 		Populates type with the block type
 * 		Returns DSK_ERR_OK
 *
 * Other errors:
 * 		DSK_ERR_BADPTR	'self' pointer is NULL
 * 		DSK_ERR_BADPARM	blockid is LDBLOCKID_NULL
 *		DSK_ERR_CORRUPT file is corrupt (block header not where it
 *				should be)
 * 		DSK_ERR_SYSERR  I/O error
 */
dsk_err_t ldbs_get_blockinfo(PLDBS self, LDBLOCKID blockid, char type[4],
				size_t *len);

/* ldbs_getblock: Get a block from the store into a user-supplied buffer.
 *
 * Enter with: blockid is the block to retrieve
 *	       type    buffer to be populated with the block type, can be
 *	       	       NULL if you don't care.
 *             data    is the buffer to receive data, can be NULL to retrieve
 *                     block length only
 *             *len    is the maximum size of the receive buffer
 *
 * On success:
 * 		Populates 'data' with the data
 * 		Populates *len with actual block length
 * 		Populates type with the block type
 * 		Returns DSK_ERR_OK
 *
 * If data == NULL or *len == 0:
 * 		Populates *len with actual block length
 * 		Populates type with the block type
 *		Returns DSK_ERR_OVERRUN
 *
 * If *len is nonzero but less than the actual block length:
 * 		Populates 'data' with data up to *len
 * 		Populates *len with actual block length
 * 		Populates type with the block type
 * 		Returns DSK_ERR_OVERRUN
 *
 * Other errors:
 * 		DSK_ERR_BADPTR	'self' or 'len' pointer is NULL
 * 		DSK_ERR_BADPARM	blockid is LDBLOCKID_NULL
 *		DSK_ERR_CORRUPT file is corrupt (block header not where it
 *				should be)
 * 		DSK_ERR_SYSERR  I/O error
 */
dsk_err_t ldbs_getblock(PLDBS self, LDBLOCKID blockid, char type[4],
				void *data, size_t *len);

/* ldbs_getblock_a: Get a block from the store, allocating memory for it.
 *
 * Enter with: self    is the handle to the blockstore
 *             blockid is the block to retrieve
 *	       type    buffer to be populated with the block type, can be
 *	       	       NULL if you don't care.
 *             data    the address of a pointer; this will be allocated if
 *                     successful
 *             *len    the address of a size_t that will be set to block length
 *
 * On success:
 * 		Returns DSK_ERR_OK
 * 		Populates *data with pointer to a buffer containing the data.
 * 			  The buffer will have been allocated with ldbs_alloc()
 * 			  Caller must free it with ldbs_free()
 * 		Populates *len with actual block length
 * 		Populates type with the block type
 *
 * Other errors:
 * 		DSK_ERR_BADPTR	'self' or 'len' pointer is NULL
 * 		DSK_ERR_BADPARM	blockid is LDBLOCKID_NULL
 *		DSK_ERR_CORRUPT file is corrupt (block header not where it
 *				should be)
 * 		DSK_ERR_SYSERR  I/O error
 */
dsk_err_t ldbs_getblock_a(PLDBS self, LDBLOCKID blockid, char *type,
				void **data, size_t *len);

/* ldbs_putblock: Write a block to the store. This covers:
 *   - Adding a new block
 *   - Updating an existing block
 *
 * Enter with: self    is the handle to the blockstore
 *             blockid is the address of a block ID. This should be NULL
 *                     to add a new block, or the ID of an existing block 
 *                     if the block is to be updated.
 *	       type    4-byte block type. 
 *	                  For a new block, this type will be assigned
 *	                  For an existing block, if the type does not 
 *	                  match the type on disk then the existing block
 *	                  will be left untouched and a new block allocated.
 *             data    The contents of the new block
 *             len     The new block length in bytes
 *			If len == 0 a new block will not be added, and an
 *			existing block will be deleted. 
 *
 * Results:
 * 		DSK_ERR_OK	Success
 * 				  blockid will have been populated with the
 * 				  block's new ID. For an update this may
 * 				  have changed if the block had to be moved
 * 				  in the file.
 * 		DSK_ERR_BADPTR	'self' or 'data' pointer is NULL
 * 		DSK_ERR_BADPARM	blockid is LDBLOCKID_NULL
 *		DSK_ERR_CORRUPT file is corrupt (block header not where it
 *				should be)
 * 		DSK_ERR_SYSERR  I/O error
 */
dsk_err_t ldbs_putblock(PLDBS self, LDBLOCKID *blockid, const char *type, 
				const void *data, size_t len);

/* ldbs_delblock: Delete a block.
 *
 * Enter with: self    is the handle to the blockstore
 *             blockid is the block to delete
 *
 * Results:
 * 		DSK_ERR_OK	Success
 * 		DSK_ERR_BADPTR	'self' pointer is NULL
 * 		DSK_ERR_BADPARM	blockid is LDBLOCKID_NULL
 *		DSK_ERR_CORRUPT file is corrupt (block header not where it
 *				should be)
 * 		DSK_ERR_SYSERR  I/O error
 * 
 */
dsk_err_t ldbs_delblock(PLDBS self, LDBLOCKID blockid);


/* ldbs_setroot: Designate a block as the track directory. 
 *
 * Enter with: self    is the handle to the blockstore
 *             blockid is the block to use
 *
 * Results:
 * 		DSK_ERR_OK	Success
 * 		DSK_ERR_BADPTR	'self' pointer is NULL
 */
dsk_err_t ldbs_setroot(PLDBS self, LDBLOCKID blockid);

/* ldbs_getroot: Get the block ID of the track directory 
 *
 * Enter with: self    is the handle to the blockstore
 *             blockid will be set to the block ID.
 *
 * Results:
 * 		DSK_ERR_OK	Success, *blockid holds block ID or 
 * 				LDBLOCKID_NULL if there is no track directory
 * 		DSK_ERR_BADPTR	'self' or 'blockid' pointer is NULL
 */
dsk_err_t ldbs_getroot(PLDBS self, LDBLOCKID *blockid);

/* ldbs_fsck: Rebuild the file's "free" and "used" linked lists.
 *            Should not be necessary to call this.
 *
 * Enter with: self    is the handle to the blockstore
 *             logfile is a text file to which messages can be written,
 *                     can be NULL for no messages.
 * Results:
 * 		DSK_ERR_OK	Success
 * 		DSK_ERR_NOTME   Not a valid LDBS file
 * 		DSK_ERR_SYSERR  I/O error
 * 		DSK_ERR_BADPTR	'self' pointer is NULL
 */
dsk_err_t ldbs_fsck(PLDBS self, FILE *logfile);

/* LDBS 0.2: Ability to empty a blockstore */
dsk_err_t ldbs_clear(PLDBS self);



/***************************************************************************/
/************************* DISK IMAGE FUNCTIONS ****************************/
/***************************************************************************/

/* Below this point, functions are specific to an LDBS disk image 
 * (file type is 'DSK\1') 
 */

/* ldbs_encode_trackid: Encode a cylinder and head into a track ID 
 */
void ldbs_encode_trackid(char trackid[4], dsk_pcyl_t cylinder, dsk_phead_t head);

/* Decode a track ID to cylinder and head: returns 1 if valid, 0 if not 
 */
int ldbs_decode_trackid(const char trackid[4], dsk_pcyl_t *cylinder, dsk_phead_t *head);

/* Encode a cylinder, head and sector into a sector ID */
void ldbs_encode_secid(char secid[4], dsk_pcyl_t cylinder, dsk_phead_t head,
		dsk_psect_t sector);

/* Decode a sector ID: returns 1 if valid, 0 if not */
int ldbs_decode_secid(const char secid[4], dsk_pcyl_t *cylinder, 
		dsk_phead_t *head, dsk_psect_t *sector);


/* ldbs_trackdir_copy: Get a copy of the disk image's track directory. Note 
 * that this is a copy, and will not be updated by subsequent changes. The 
 * following functions may cause the track directory to be updated:
 *
 * 	ldbs_putblock_d
 *	ldbs_put_creator
 *	ldbs_put_comment
 *	ldbs_put_dpb
 *	ldbs_put_geometry
 *      ldbs_put_trackhead
 *
 * Enter with: self    is the handle to the disk image
 *             result  points to a pointer that will receive the track 
 *                     directory.
 * Results:
 * 		DSK_ERR_OK	Success, *result populated. Caller must
 * 	                        free it with ldbs_free().
 * 		DSK_ERR_NOTME   File does not contain a track directory
 * 		DSK_ERR_NOMEM   Cannot allocate memory for result
 * 		DSK_ERR_BADPTR	'self' or 'result' pointer is NULL
 */
dsk_err_t ldbs_trackdir_copy(PLDBS self, LDBS_TRACKDIR **result);


/* ldbs_getblock_d: Look up a block in the directory and load it into
 *                 a user-defined buffer.
 *
 * Enter with: self    is the handle to the blockstore
 *	       type    is the block type, which will be searched for in the
 *		       directory.
 *             data    is the buffer to receive data, can be NULL to retrieve
 *                     block length only
 *             *len    is the maximum size of the receive buffer
 *
 * On success:
 * 		Populates 'data' with the data
 * 		Populates *len with actual block length
 * 		Populates type with the block type
 * 		Returns DSK_ERR_OK
 *
 * If data == NULL or *len == 0:
 * 		Populates *len with actual block length
 * 		Populates type with the block type
 *		Returns DSK_ERR_OVERRUN
 *
 * If *len is nonzero but less than the actual block length:
 * 		Populates 'data' with data up to *len
 * 		Populates *len with actual block length
 * 		Populates type with the block type
 * 		Returns DSK_ERR_OVERRUN
 *
 * Block not found in directory:
 * 		Populates 'data' with NULL
 * 		Populates *len with 0
 * 		Returns DSK_ERR_OK
 *
 * Other errors:
 * 		DSK_ERR_BADPTR	'self' or 'len' pointer is NULL
 * 		DSK_ERR_CORRUPT	directory entry contains invalid block ID
 *		DSK_ERR_NOTME   file does not contain a directory
 * 		DSK_ERR_SYSERR  I/O error
 */
dsk_err_t ldbs_getblock_d(PLDBS self, const char type[4],
				void *data, size_t *len);

/* ldbs_getblock_da: Get a block from the store, allocating memory for it.
 *
 * Enter with: self    is the handle to the blockstore
 *	       type    is the block type, which will be searched for in the
 *		       directory.
 *             data    the address of a pointer; this will be allocated if
 *                     successful
 *             *len    the address of a size_t that will be set to block length
 *
 * On success:
 * 		Returns DSK_ERR_OK
 * 		Populates *data with pointer to a buffer containing the data.
 * 			  The buffer will have been allocated with ldbs_alloc()
 * 			  Caller must free it with ldbs_free()
 * 		Populates *len with actual block length
 *
 * Type not found in directory:
 * 		Returns DSK_ERR_OK
 * 		Populates *data with NULL
 * 		Populates *len with 0
 *
 * 
 * Other errors:
 * 		DSK_ERR_BADPTR	'self' or 'len' pointer is NULL
 * 		DSK_ERR_CORRUPT	directory entry contains invalid block ID
 *		DSK_ERR_NOTME   file does not contain a directory
 * 		DSK_ERR_SYSERR  I/O error
 */
dsk_err_t ldbs_getblock_da(PLDBS self, const char *type,
				void **data, size_t *len);

/* ldbs_putblock_d: Write a block (as ldbs_putblock()) and also record its 
 *                  location in the track directory.
 *
 * Enter with: self    is the handle to the disk image
 *             type    is the block type. This must be unique for each
 *                     entry in the directory.
 *             data    The contents of the new block. If this is NULL
 *                      a new block will not be added, and any existing
 *                      block will be deleted
 *             len     The new block length in bytes
 *
 * Results:
 * 		DSK_ERR_OK	Success
 * 		DSK_ERR_BADPTR	'self' pointer is NULL
 * 		DSK_ERR_CORRUPT	Directory contains invalid block ID 
 * 		DSK_ERR_SYSERR  I/O error
 */
dsk_err_t ldbs_putblock_d(PLDBS self, const char type[4],
				const void *data, size_t len);


/* ldbs_trackdir_alloc: Create a new 'track directory' structure with the 
 * specified number of entries. 
 *
 * Returns a new, initialised track directory if successful, NULL if 
 * ldbs_malloc failed.
 *
 * Free the structure with ldbs_free().
 */
LDBS_TRACKDIR  *ldbs_trackdir_alloc(unsigned entries);

/* ldbs_trackdir_alloc: Create a new 'track header' structure with the 
 * specified number of entries. 
 *
 * Returns a new, initialised track header if successful, NULL if 
 * malloc failed.
 *
 * Free the structure with ldbs_free().
 */
LDBS_TRACKHEAD *ldbs_trackhead_alloc(unsigned entries);

/* Resize a track header structure */
LDBS_TRACKHEAD *ldbs_trackhead_realloc(LDBS_TRACKHEAD *p, unsigned short entries);

/* ldbs_trackdir_add: Add / update an entry in the specified track directory
 * 		      structure. 
 *
 * The structure will be reallocated if necessary to make space. 
 *
 * Enter with: dir     points to a pointer to the LDBS_TRACKDIR structure
 *                     entry in the directory.
 *             type    is the block type, which must be unique in the 
 *                     directory. An existing entry with the same block type
 *                     will be replaced.
 *             blockid the block ID to associate with that block type.
 *
 * Results:
 * 		DSK_ERR_OK	Success
 * 		DSK_ERR_BADPTR  dir or type parameter is NULL
 *		DSK_ERR_NOMEM   Directory is full and could not allocate a 
 *		                larger one. 
 */
dsk_err_t ldbs_trackdir_add(LDBS_TRACKDIR **dir, const char type[4], 
					LDBLOCKID blockid);

/* ldbs_trackdir_find: Look up an entry in the specified track directory
 * 		      structure. 
 *
 * Enter with: dir     points to a pointer to the LDBS_TRACKDIR structure
 *                     entry in the directory.
 *             type    is the block type, which must be unique in the 
 *                     directory. An existing entry with the same block type
 *                     will be replaced.
 *             blockid will be set to the result of the search
 *
 * Results:
 * 		DSK_ERR_OK	Success
 * 					If entry found, *blockid is set
 * 					If not found,   *blockid is NULL
 * 		DSK_ERR_BADPTR  dir, type or blockid parameter is NULL
 */
dsk_err_t ldbs_trackdir_find(LDBS_TRACKDIR *dir, const char type[4], LDBLOCKID *result);

/* ldbs_get_creator: Get the disc image's creator record.
 *
 * Enter with: self    is the handle to the blockstore
 * 	       buffer  points to a char * pointer that will receive the data
 *
 * Results:
 * 		DSK_ERR_OK	Success
 * 				If there is a creator, *buffer holds it
 * 				as an ASCIIZ string. Caller will need to
 * 				free it with ldbs_free().
 * 				If there is not a creator, *buffer is NULL.
 * 		DSK_ERR_BADPTR	'self' or 'len' pointer is NULL
 * 		DSK_ERR_NOTME   File does not contain a track directory
 * 		DSK_ERR_CORRUPT	Directory contains invalid block ID 
 * 		DSK_ERR_SYSERR  I/O error
 */
dsk_err_t ldbs_get_creator(PLDBS self, char **buffer);


/* ldbs_put_creator: Write the disc's creator record.
 *
 * Enter with: self    is the handle to the disk image
 *             creator is the ASCII creator string, "" or NULL if none
 *
 * Results:
 * 		DSK_ERR_OK	Success
 * 		DSK_ERR_BADPTR	'self' is null
 * 		DSK_ERR_NOTME   File does not contain a track directory
 * 		DSK_ERR_CORRUPT	Directory contains invalid block ID 
 * 		DSK_ERR_SYSERR  I/O error
 */
dsk_err_t ldbs_put_creator(PLDBS self, const char *creator);

/* ldbs_get_comment: Get the disc image's comment.
 *
 * Enter with: self    is the handle to the blockstore
 * 	       buffer  points to a char * pointer that will receive the data
 *
 * Results:
 * 		DSK_ERR_OK	Success
 * 				If there is a creator, *buffer holds it
 * 				as an ASCIIZ string. Caller will need to
 * 				free it with ldbs_free().
 * 				If there is not a comment, *buffer is NULL.
 * 		DSK_ERR_BADPTR	'self' or 'len' pointer is NULL
 * 		DSK_ERR_NOTME   File does not contain a track directory
 * 		DSK_ERR_CORRUPT	Directory contains invalid block ID 
 * 		DSK_ERR_SYSERR  I/O error
 */
dsk_err_t ldbs_get_comment(PLDBS self, char **buffer);


/* ldbs_put_comment: Write the disc's comment record.
 *
 * Enter with: self    is the handle to the disk image
 *             comment is the ASCII comment string, "" or NULL if none
 *
 * Results:
 * 		DSK_ERR_OK	Success
 * 		DSK_ERR_BADPTR	'self' is null
 * 		DSK_ERR_NOTME   File does not contain a track directory
 * 		DSK_ERR_CORRUPT	Directory contains invalid block ID 
 * 		DSK_ERR_SYSERR  I/O error
 */
dsk_err_t ldbs_put_comment(PLDBS self, const char *comment);

/* ldbs_get_dpb: Get the disc image's CP/M DPB.
 *
 * Enter with: self    is the handle to the blockstore
 * 	       dpb     points to an LDBS_DPB structure that will receive the
 * 	               data
 *
 * Results:
 * 		DSK_ERR_OK	Success
 * 		                If there is a DPB, the structure will be 
 * 		                populated.
 * 				If there is no DPB, the structure will be 
 * 				filled with zeroes.
 * 		DSK_ERR_NOTME   File does not contain a track directory
 * 		DSK_ERR_BADPTR	'self' or 'dpb' pointer is NULL
 * 		DSK_ERR_CORRUPT	Directory contains invalid block ID
 * 		DSK_ERR_SYSERR  I/O error
 */
dsk_err_t ldbs_get_dpb(PLDBS self, LDBS_DPB *dpb);


/* ldbs_put_dpb: Put the disc image's CP/M DPB record.
 *
 * Enter with: self    is the handle to the blockstore
 * 	       dpb     points to an LDBS_DPB structure to write
 *
 * Results:
 * 		DSK_ERR_OK	Success
 * 		DSK_ERR_NOTME   File does not contain a track directory
 * 		DSK_ERR_BADPTR	'self' or 'dpb' pointer is NULL
 * 		DSK_ERR_CORRUPT	Directory contains invalid block ID 
 * 		DSK_ERR_SYSERR  I/O error
 */
dsk_err_t ldbs_put_dpb(PLDBS self, const LDBS_DPB *dpb);

/* ldbs_get_trackhead: Load the header for a track
 *
 * Enter with: self    is the handle to the blockstore
 * 	       trkh    is the address of a pointer which will be set to the
 *                     result
 *             cyl     is the cylinder for which to load the header
 *             head    is the head for which to load the header
 *
 * Results:
 * 		DSK_ERR_OK	Success
 * 		                If the requested track header is present in
 *                              the blockstore, trkh now points to it. Free
 *                              trkh with ldbs_free() when it is not required.
 * 				If it is not present in the file, trkh will
 *                              be NULL.
 * 		DSK_ERR_NOTME   File does not contain a track directory
 * 		DSK_ERR_BADPTR	'self' or 'trkh' pointer is NULL
 * 		DSK_ERR_CORRUPT	Directory contains invalid block ID 
 * 		DSK_ERR_SYSERR  I/O error
 */
dsk_err_t ldbs_get_trackhead(PLDBS self, LDBS_TRACKHEAD **trkh, 
				dsk_pcyl_t cylinder, dsk_phead_t head);

/* ldbs_put_trackhead: Save the header for a track
 *
 * Enter with: self    is the handle to the blockstore
 * 	       trkh    is the address of the track header to write back
 *                     NULL to delete the header for this track 
 *             cyl     is the cylinder this header is for
 *             head    is the head this header is for
 *
 * Results:
 * 		DSK_ERR_OK	Success
 * 		DSK_ERR_NOTME   File does not contain a track directory
 * 		DSK_ERR_BADPTR	'self' pointer is NULL
 * 		DSK_ERR_CORRUPT	Directory contains invalid block ID 
 * 		DSK_ERR_SYSERR  I/O error
 */
dsk_err_t ldbs_put_trackhead(PLDBS self, const LDBS_TRACKHEAD *trkh,
				dsk_pcyl_t cylinder, dsk_phead_t head);

/* ldbs_get_geometry: Get the disc image's LibDsk geometry, if it has one.
 *
 * Enter with: self    is the handle to the blockstore
 * 	       dg      points to a DSK_GEOMETRY structure that will receive the
 * 	               data
 *
 * Results:
 * 		DSK_ERR_OK	Success
 * 		                If there is a geometry block, the structure 
 * 		                will be populated.
 * 				If there is no geometry, the structure will be 
 * 				filled with zeroes.
 * 		DSK_ERR_NOTME   File does not contain a track directory
 * 		DSK_ERR_BADPTR	'self' or 'dg' pointer is NULL
 * 		DSK_ERR_CORRUPT	Directory contains invalid block ID 
 * 		DSK_ERR_SYSERR  I/O error
 */
dsk_err_t ldbs_get_geometry(PLDBS self, DSK_GEOMETRY *dg);

/* ldbs_put_geometry: Write a LibDsk geometry record to the file.
 *
 * Enter with: self    is the handle to the blockstore
 * 	       dg      points to the DSK_GEOMETRY structure to write, 
 *                     NULL to delete any existing DSK_GEOMETRY structure
 *
 * Results:
 * 		DSK_ERR_OK	Success
 * 		DSK_ERR_NOTME   File does not contain a track directory
 * 		DSK_ERR_BADPTR	'self' pointer is NULL
 * 		DSK_ERR_CORRUPT	Directory contains invalid block ID 
 * 		DSK_ERR_SYSERR  I/O error
 */

dsk_err_t ldbs_put_geometry(PLDBS self, const DSK_GEOMETRY *dg);

/* ldbs_max_cyl_head: Get the range of cylinders / heads covered by the 
 * disk image. These will both be 1 higher than the maximum number 
 * encountered in the track directory.
 *
 * For example, a 720k disc will have cylinders 0-79 and heads 0-1. This
 * function will return *cyls = 80, *heads = 2.
 *
 * Note that there is no guarantee that all intermediate cylinders / heads
 * exist.
 *
 * Enter with: self    is the handle to the blockstore
 * 	       cyls    will be populated with the number of cylinders
 *             heads   will be populated with the number of heads
 *
 * Results:
 * 		DSK_ERR_OK	Success
 * 		DSK_ERR_NOTME   File does not contain a track directory
 * 		DSK_ERR_BADPTR	'self' pointer is NULL
 */
dsk_err_t ldbs_max_cyl_head(PLDBS self, dsk_pcyl_t *cyls, dsk_phead_t *heads);

/* Callback from ldbs_all_tracks().
 *
 * This should return DSK_ERR_OK to continue, any other value to stop 
 * iterating.
 */
typedef dsk_err_t (*LDBS_TRACK_CALLBACK)
	(PLDBS self, 		/* The LDBS file containing this track */
	 dsk_pcyl_t cyl,	/* Physical cylinder */
	 dsk_phead_t head,	/* Physical head */
	 LDBS_TRACKHEAD *th,	/* The track header in question */
	 void *param);		/* Parameter passed by caller */

/* Callback from ldbs_all_sectors()
 *
 * This should return DSK_ERR_OK to continue, any other value to stop 
 * iterating.
 */
typedef dsk_err_t (*LDBS_SECTOR_CALLBACK)
	(PLDBS self, 		/* The LDBS file containing this sector */
	 dsk_pcyl_t cyl,	/* Physical cylinder where sector is located */
	 dsk_phead_t head,	/* Physical head where sector is located */
	 LDBS_SECTOR_ENTRY *se,	/* The sector header for this sector */
	 LDBS_TRACKHEAD *th,	/* The track header this sector lives in */
	 void *param);		/* Parameter passed by caller */


/* ldbs_all_tracks: Iterate over all tracks in the file, 
 * in the specified order
 *
 * Enter with: self    is the handle to the blockstore
 * 	       cbk     will be called once for each track
 *             sides   the order in which tracks will be processed
 *             param   is passed through to cbk
 *
 * Results:
 * 		DSK_ERR_NOTME   File does not contain a track directory
 * 		DSK_ERR_BADPTR	'self' pointer is NULL
 *              Other values as returned from the callback function.
 */
dsk_err_t ldbs_all_tracks(PLDBS self, LDBS_TRACK_CALLBACK cbk, 
		dsk_sides_t sides, void *param);

/* ldbs_all_sectors: Iterate over all sectors in the file, 
 * in the specified order, and sectors in the order they are in the 
 * file. Note that if the sectors are interleaved (eg, held in the order
 * 1,6,2,7,3,8,4,9,5 ) that is the order they will be returned in.
 *
 * Enter with: self    is the handle to the blockstore
 * 	       cbk     will be called once for each sector
 *             sides   the order in which tracks will be processed
 *             param   is passed through to cbk
 *
 * Results:
 * 		DSK_ERR_NOTME   File does not contain a track directory
 * 		DSK_ERR_BADPTR	'self' pointer is NULL
 * 		DSK_ERR_CORRUPT	Directory contains invalid block ID
 * 		DSK_ERR_SYSERR  I/O error
 *              Other values as returned from the callback function.
 */
dsk_err_t ldbs_all_sectors(PLDBS self, LDBS_SECTOR_CALLBACK cbk, 
			dsk_sides_t sides, void *param);

/* ldbs_get_stats: Get statistics for a disc image. These may be useful to
 *                the calling code to determine how 'regular' the disc 
 *                image is -- for example, can it be represented by a 
 *                'flat' format with no metadata?
 *
 * Enter with: self    is the handle to the blockstore
 *             stats   is the structure to populate
 *
 * Results:
 *              DSK_ERR_OK      Stats structure populated
 * 		DSK_ERR_NOTME   File does not contain a track directory
 * 		DSK_ERR_BADPTR	'self' or 'stats' pointer is NULL
 * 		DSK_ERR_CORRUPT	Directory contains invalid block ID 
 * 		DSK_ERR_SYSERR  I/O error
 */
typedef struct ldbs_stats
{
	int	drive_empty;		/* Nonzero if the disk image is empty */
	dsk_pcyl_t max_cylinder;	/* Maximum cylinder number encountered*/
	dsk_pcyl_t min_cylinder;	/* Minimum cylinder number encountered*/
	dsk_phead_t max_head;		/* Maximum head number encountered */
	dsk_phead_t min_head;		/* Minimum head number encountered */
	dsk_psect_t max_spt;		/* Maximum sectors in a track */
	dsk_psect_t min_spt;		/* Minimum sectors in a track */
	size_t max_sector_size;		/* Maximum sector size encountered */
	size_t min_sector_size;		/* Minimum sector size encountered */
	unsigned char max_secid;	/* Maximum sector ID encountered */
	unsigned char min_secid;	/* Minimum sector ID encountered */
} LDBS_STATS;

dsk_err_t ldbs_get_stats(PLDBS self, LDBS_STATS *stats);

/* LDBS 0.2: Ability to load a whole track in one go. 
 * Enter with: self    is the handle to the blockstore
 * 	       trkh    is the track header
 *             buf     will be populated on return with the buffer
 *		       containing the sector data
 *             buflen  will be populated on return with the number of 
 *	               bytes in the buffer
 *             sector_size  if nonzero, sectors will be padded/truncated to
 *                          that size. 
 *
 * Results:
 * 		DSK_ERR_NOMEM   Cannot allocate buffer
 * 		DSK_ERR_BADPTR	'self', 'buf' or 'buflen' pointer is NULL
 * 		DSK_ERR_CORRUPT	track header contains an invalid block ID
 * 		DSK_ERR_SYSERR  I/O error
 *
 * If track has no sectors, returns DSK_ERR_OK with *buf = NULL, *buflen = 0
 */
dsk_err_t ldbs_load_track(PLDBS self, const LDBS_TRACKHEAD *trkh, void **buf, 
			size_t *buflen, size_t sector_size);

/* LDBS 0.2: Ability to clone a disk image */
dsk_err_t ldbs_clone(PLDBS source, PLDBS target);

/* LDBS 0.2: Write back any memory buffers associated with this blockstore */
dsk_err_t ldbs_sync(PLDBS self);



/* Magic numbers */
#define LDBS_HEADER_MAGIC    "LBS\1"
#define LDBS_BLOCKHEAD_MAGIC "LDB\1"

/* File types */
#define LDBS_DSK_TYPE        "DSK\2"
#define LDBS_DSK_TYPE_V1     "DSK\1"

/* Block types */
#define LDBS_DIR_TYPE        "DIR\1"
#define LDBS_INFO_TYPE       "INFO"
#define LDBS_DPB_TYPE        "DPB "
#define LDBS_GEOM_TYPE       "GEOM"
#define LDBS_CREATOR_TYPE    "CREA"

/* Helper functions to store / retrieve 16- and 32-bit little-endian values */
void ldbs_poke4(unsigned char *dest, unsigned long val);
unsigned long ldbs_peek4(unsigned char *src);

void ldbs_poke2(unsigned char *dest, unsigned short val);
unsigned short ldbs_peek2(unsigned char *src);

