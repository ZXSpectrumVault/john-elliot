/* LDBS: LibDsk Block Store access functions
 *
 *  Copyright (c) 2016, 2017 John Elliott <seasip.webmaster@gmail.com>
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

#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include "ldbs.h"

/* Pacific C doesn't define these */
#ifndef SEEK_SET
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2
#define isprint(c) ( (c) >= 0x20 && (c) <= 0x7E)
#endif

#ifdef __MSDOS__
#include <sys.h>	/* for mktemp() */
#endif

#if defined(__MSDOS__) && !defined(PATH_MAX)
#define PATH_MAX 81
#endif

#if defined(WINDOWS) && !defined(PATH_MAX)
#include <windows.h>
#define PATH_MAX _MAX_PATH
#endif

/**/
#define FSEEK fseek
#define FREAD fread
#define FWRITE fwrite
/*

#define FSEEK(fp, o, w) debug_fseek(fp, #fp, o, w)
#define FREAD(buf, s, c, fp) debug_fread(buf, s, c, fp, #fp)
#define FWRITE(buf, s, c, fp) debug_fwrite(buf, s, c, fp, #fp)

static int debug_fseek(FILE *fp, const char *s, long offset, int whence)
{
	int r;
	printf("fseek(%s,%lx,%d)=", s, offset, whence);
	r = fseek(fp, offset, whence);
	printf("%d\n", r);
	return r;	
}

static size_t debug_fread(void *buf, size_t size, size_t count, FILE *fp, const char *s)
{
	size_t r;
	printf("[%08lx] fread(...,%u,%u,%s)=", ftell(fp), size, count, s);
	r = fread(buf, size, count, fp);
	printf("%u\n", r);
	return r;	
}


static size_t debug_fwrite(void *buf, size_t size, size_t count, FILE *fp, const char *s)
{
	size_t r;

	printf("[%08lx] fwrite([%02x%02x%02x%02x...],%u,%u,%s)=", 
		ftell(fp), 
		0xFF & ((unsigned char *)buf)[0], 
		0xFF & ((unsigned char *)buf)[1], 
		0xFF & ((unsigned char *)buf)[2], 
		0xFF & ((unsigned char *)buf)[3], 
		size, count, s);
	r = fwrite(buf, size, count, fp);
	printf("%u\n", r);
	return r;	
}

*/

/* LibDsk Block Store (on-disk implementation)
 * 
 * File format: 20-byte header:
 * 
 *	DB	'LBS\1'	;Magic number
 * 	DB	'xxxx'	;File subtype, set by ldbs_new()
 * 	DD	used	;Offset of first block in 'used' linked list, 0 if none
 * 	DD	free	;Offset of first block in 'free' linked list, 0 if none
 * 	DD 	root	;Offset of root directory block, 0 if none
 *
 * Then one of more blocks, each with a 20-byte header:
 *
 *	DB	'LDB\1'	;Block 
 *	DB	'xxxx'	;Block type as passed to ldbs_addblock, all zeroes
			;for a free block
 *	DD	len	;Allocated length
 *	DD	ulen	;Used length
 * 	DD	prev	;Next block in used / free linked list, 0 if none
 *
 */

#define HEADER_LEN 20		/* On-disk length of file header */
#define BLOCKHEAD_LEN 20	/* On-disk length of block header */

/* Implementation: This covers all the bits we need to manage an LDBS file
 */
typedef struct ldbs
{
	LDBS_HEADER header;
	FILE *fp;
	long filesize;
	char *filename;
	int istemp;
	int version;	/* File format: Version 1 has shorter track/sector */
			/* headers */
#if LDBS_TEMP_IN_MEM
	int ismem;
	void **idmap;
	int idmapmax;
#endif
	LDBS_TRACKDIR *dir;
} LDBS;

static const unsigned char FREEBLOCK[4] = {0,0,0,0};
static const unsigned char USEDBLOCK[4] = {0,1,0,1};

static dsk_err_t ldbs_get_trackdir(PLDBS self, LDBS_TRACKDIR **pdir, LDBLOCKID blockid);
static dsk_err_t ldbs_put_trackdir(PLDBS self, LDBS_TRACKDIR *dir, LDBLOCKID *blkid);


#define TMPDIR "/tmp"

/***************************************************************************/
/************************* BLOCKSTORE FUNCTIONS ****************************/
/***************************************************************************/
/* All functions that access the underlying file should be concentrated in */
/* this area of the program. They are the only ones which should need to   */
/* consider whether there is an underlying file at all, or whether         */
/* everything is being held in memory.                                     */
/***************************************************************************/


/* Helper functions to copy / compare an LDBLOCKID.
 *
 * If we have the option to maintain the blockstore in memory, we need to 
 * know which field of the LDBLOCKID is significant; on a system with (for 
 * example) 32-bit longs and 64-bit pointers, we only need to do a 32-bit 
 * comparison on the longs and a 64-bit comparison on the pointers.
 */
#if LDBS_TEMP_IN_MEM

/* Because a pointer may not fit into 32 bits, for an all-memory object
 * we will need to have a mapping between pointers and 32-bit ints.
 *
 * Map the null pointer to 0.
 */
static LDBLOCKID encode_ptr(LDBS *self, void *p)
{
	int n;
	void **p2;

	/* Map NULL to zero */
	if (p == NULL) return 0;
	/* If a pointer will fit in 32 bits, shove it in there */
	if (sizeof(void *) <= 4)
	{
		return (long)p;
	}
	/* OK, we need to store a (probably 64-bit) pointer in a 32-bit long */

	/* Create the ID map, if it doesn't already exist */
	if (!self->idmap)
	{
		self->idmap = ldbs_malloc(8 * sizeof(void *));
		if (!self->idmap) return 0;
		for (n = 0; n < 8; n++)
		{
			self->idmap[n] = NULL;	
		}
		self->idmapmax = 8;
	}

	/* Is the suggested pointer already in the map? */
	for (n = 0; n < self->idmapmax; n++)
	{
		if (self->idmap[n] == p) 
		{
			return n + 1;
		}
		if (self->idmap[n] == 0)
		{
			self->idmap[n] = p;
			return n + 1;
		}
	}
	/* The map must be full. Double it. 
	 * Note: The map will never decrease in size, because blocks once
	 * allocated are never freed, only zeroed */
	p2 = ldbs_realloc(self->idmap, 2 * self->idmapmax * sizeof(void *));
	if (!p2) return 0;
	for (n = self->idmapmax; n < self->idmapmax * 2; n++)
	{
		p2[n] = NULL;	
	}
	self->idmap = p2;
	self->idmap[self->idmapmax] = p;
	self->idmapmax *= 2;
	return (self->idmapmax / 2) + 1;
}


/* Decode an integer ID to a pointer */
static void *decode_ptr(LDBS *self, long l)
{
	if (l == 0) return NULL;
	
	/* If a pointer will fit in 32 bits, it was encoded thus */
	if (sizeof(void *) <= 4) 
	{
		return (void *)l;
	}
	/* Otherwise look it up in the ID map */
	return self->idmap[l - 1];
}


#endif



#if LDBS_TEMP_IN_MEM == 0

#if (!defined(HAVE_MKSTEMP) && !defined(HAVE_GETTEMPFILENAME))

void remktemp(char *s)
{
	srand(ldbs_peek2(s));
	while (*s) 
	{
		*s = (rand() % 36) + 'A';
		if (*s > 'Z') *s = (*s - 'Z' - 1) + '0';
		++s;
	}
}
#endif


/* Create a temporary file */    
static dsk_err_t ldbs_mktemp(LDBS *self)
{
	char *tdir;
	int fd;
	char tmpdir[PATH_MAX];

	self->filename = ldbs_malloc(PATH_MAX);
	if (!self->filename) return DSK_ERR_NOMEM; 

/* Win32: Create temp file using GetTempFileName() */
#ifdef HAVE_GETTEMPFILENAME
        GetTempPath(PATH_MAX, tmpdir);
        if (!GetTempFileName(tmpdir, "ldbs", 0, self->filename))
	{
		ldbs_free(self->filename);
		self->filename = NULL;
		return DSK_ERR_SYSERR;
	}
        self->fp = fopen(self->filename, "w+b");
        (void)fd;
        (void)tdir;
#else /* def HAVE_GETTEMPFILENAME */

/* Modern Unixes: Use mkstemp() */
#ifdef HAVE_MKSTEMP
	tdir = getenv("TMPDIR");
	if (tdir) sprintf(tmpdir, "%s/ldbsXXXXXXXX", tdir);
	else      sprintf(tmpdir, TMPDIR "/ldbsXXXXXXXX");

	fd = mkstemp(tmpdir);
	self->fp = NULL;
	if (fd != -1) self->fp = fdopen(fd, "w+b");
	strcpy(self->filename, tmpdir);
#else   /* def HAVE_MKSTEMP */
/* Old Unixes, old xenix clones such as DOS */ 
	tdir = getenv("TMP");
	if (!tdir) tdir = getenv("TEMP");
	if (tdir) sprintf(tmpdir, "%s/LBXXXXXX", tdir);
	else	  sprintf(tmpdir, "./LBXXXXXX");

	(void)fd;
	strcpy(self->filename, mktemp(tmpdir));
	while (1)
	{
		FILE *fp;


		fp = fopen(self->filename, "rb");
		if (fp)	/* Chosen temp file already exists! (Pacific C mktemp()
			 * seems to base temp filenames on the system clock,
			 * so two temp files created at the same time will
			 * have the same name). Generate a new temp name and
			 * repeat. */
		{
			char *s = self->filename + strlen(self->filename) - 6;
			remktemp(s);
			fclose(fp);
			continue;		
		}
		self->fp = fopen(self->filename, "w+b");
		break;
	}
#endif /* HAVE_MKSTEMP */                  
#endif /* HAVE_GETTEMPFILENAME */
	if (!self->fp)   
	{
		ldbs_free(self->filename);
		self->filename = NULL;
		return DSK_ERR_SYSERR;
	}
	return DSK_ERR_OK;
}

#endif

/* On a real DOS system, we could get away with just dumping the
 * memory structure out. But let's serialise it properly, so 
 * the files are portable between libdsks. 
 * 
 * All dwords will be stored little-endian.
 */

void ldbs_poke4(unsigned char *dest, unsigned long val)
{
	dest[0] = (unsigned char)(val & 0xFF);
	dest[1] = (unsigned char)((val >> 8) & 0xFF);
	dest[2] = (unsigned char)((val >> 16) & 0xFF);
	dest[3] = (unsigned char)((val >> 24) & 0xFF);
}


unsigned long ldbs_peek4(unsigned char *src)
{
	unsigned long u = src[3];
	u = (u << 8) | src[2];
	u = (u << 8) | src[1];
	u = (u << 8) | src[0];
	return u;
}

void ldbs_poke2(unsigned char *dest, unsigned short val)
{
	dest[0] = val & 0xFF;
	dest[1] = (val >> 8) & 0xFF;
}


unsigned short ldbs_peek2(unsigned char *src)
{
	unsigned short u = src[1];
	u = (u << 8) | src[0];
	return u;
}



/* Read the file header */
static dsk_err_t ldbs_read_header(PLDBS self)
{
	unsigned char header[HEADER_LEN];

#if LDBS_TEMP_IN_MEM
	if (self->ismem) 
	{
		return DSK_ERR_OK;
	}
#endif
	if (FSEEK(self->fp, 0L, SEEK_SET))
	{
		return DSK_ERR_SYSERR;
	}
	if (FREAD(header, 1, HEADER_LEN, self->fp) < HEADER_LEN) 
	{
		return DSK_ERR_SYSERR;
	}
	memcpy(self->header.magic,   header, 4);
	memcpy(self->header.subtype, header + 4, 4);
	self->header.used     = ldbs_peek4(header + 8);
	self->header.free     = ldbs_peek4(header + 12);
	self->header.trackdir = ldbs_peek4(header + 16);
	self->header.dirty = 0;
	return DSK_ERR_OK;
}


/* Read a block header */
static dsk_err_t ldbs_read_blockhead(PLDBS self, LDBS_BLOCKHEAD *bh, 
		LDBLOCKID blockid)
{
	unsigned char header[BLOCKHEAD_LEN];

	if (blockid == LDBLOCKID_NULL) return DSK_ERR_BADPARM;

#if LDBS_TEMP_IN_MEM
	if (self->ismem)
	{
		void *ptr = decode_ptr(self, blockid);
		memcpy(bh, ptr, sizeof(LDBS_BLOCKHEAD));
		return DSK_ERR_OK;
	}
#endif

	if (FSEEK(self->fp, blockid, SEEK_SET))
	{
		return DSK_ERR_SYSERR;
	}
	if (FREAD(header, 1, BLOCKHEAD_LEN, self->fp) < BLOCKHEAD_LEN) 
	{
		return DSK_ERR_SYSERR;
	}
	if (memcmp(header, LDBS_BLOCKHEAD_MAGIC, 4))
	{
		return DSK_ERR_CORRUPT;	/* Not a valid block header */
	}
	memcpy(bh->magic,  header, 4);
	memcpy(bh->type,   header + 4, 4);
	bh->dlen = ldbs_peek4(header + 8);
	bh->ulen = ldbs_peek4(header + 12);
	bh->next = ldbs_peek4(header + 16);
	return DSK_ERR_OK;
}


/* Write the header out */
static dsk_err_t ldbs_write_header(PLDBS self)
{
	unsigned char header[HEADER_LEN];

#if LDBS_TEMP_IN_MEM
	if (self->ismem) 
	{
		return DSK_ERR_OK;
	}
#endif

	if (FSEEK(self->fp, 0L, SEEK_SET))
	{
		return DSK_ERR_SYSERR;
	}
	memcpy(header,      self->header.magic,   4);
	memcpy(header + 4,  self->header.subtype, 4);
	ldbs_poke4 (header + 8,  self->header.used);
	ldbs_poke4 (header + 12, self->header.free);
	ldbs_poke4 (header + 16, self->header.trackdir);
	if (FWRITE(header, 1, HEADER_LEN, self->fp) < HEADER_LEN) 
		return DSK_ERR_SYSERR;
	self->header.dirty = 0;
	if (self->filesize < HEADER_LEN)
	{
		self->filesize = HEADER_LEN;
	}
	return DSK_ERR_OK;
}

static dsk_err_t ldbs_write_blockhead(PLDBS self, LDBS_BLOCKHEAD *bh, 
		LDBLOCKID blockid)
{
	unsigned char header[BLOCKHEAD_LEN];

	if (blockid == LDBLOCKID_NULL) return DSK_ERR_BADPARM;
#if LDBS_TEMP_IN_MEM
	if (self->ismem)
	{
		void *ptr = decode_ptr(self, blockid);
		memcpy(ptr, bh, sizeof(LDBS_BLOCKHEAD));
		return DSK_ERR_OK;
	}
#endif

	if (FSEEK(self->fp, blockid, SEEK_SET))
	{
		return DSK_ERR_SYSERR;
	}
	memcpy(header, LDBS_BLOCKHEAD_MAGIC, 4);
	memcpy(header + 4, bh->type, 4);
	ldbs_poke4(header + 8,  bh->dlen);
	ldbs_poke4(header + 12, bh->ulen);
	ldbs_poke4(header + 16, bh->next);

	if (FWRITE(header, 1, BLOCKHEAD_LEN, self->fp) < BLOCKHEAD_LEN) 
		return DSK_ERR_SYSERR;

	if (self->filesize < (blockid + BLOCKHEAD_LEN)) 
	{
		self->filesize = blockid + BLOCKHEAD_LEN;
	}

	return DSK_ERR_OK;
}


static dsk_err_t ldbs_write_payload(PLDBS self,
		LDBLOCKID blockid, const void *data, size_t len)
{
	if (0 == blockid)
	{
		return DSK_ERR_BADPTR;
	}
#if LDBS_TEMP_IN_MEM
	if (self->ismem)
	{
		unsigned char *dest = decode_ptr(self, blockid);

		memcpy(dest + sizeof(LDBS_BLOCKHEAD), data, len);
		return DSK_ERR_OK;
	}
#endif

	if (FWRITE(data, 1, len, self->fp) < len)
	{
		return DSK_ERR_SYSERR;
	}
	return DSK_ERR_OK;
}


static dsk_err_t ldbs_newblock(PLDBS self, size_t size, LDBLOCKID *result)
{
#if LDBS_TEMP_IN_MEM
	if (self->ismem)
	{
		void *p = malloc(sizeof(LDBS_BLOCKHEAD) + size);
		if (!p) return DSK_ERR_NOMEM;
		*result = encode_ptr(self, p);
		return DSK_ERR_OK;
	}
#endif
	*result = self->filesize;
	self->filesize += size + BLOCKHEAD_LEN;
	return DSK_ERR_OK;
}


/* Create a new block store. 
 * 
 * filename is NULL to create a temporary file, non-NULL to create with thee
 * specified filename.
 *
 * On success populates 'result', returns DSK_ERR_OK.
 * On failure *result = NULL, returns error.
 */
dsk_err_t ldbs_new(PLDBS *result, const char *filename, const char *type)
{
	LDBS temp, *pres = NULL;
	dsk_err_t err;
	char st[5];

	if (result == NULL) return DSK_ERR_BADPTR;
	if (!type) 
	{
		memset(st, 0, sizeof(st));
	}
	else
	{
		memcpy(st, type, 4);
	}
	*result = 0;
	memset(&temp, 0, sizeof(temp));
	if (filename != NULL)
	{
		temp.fp = fopen(filename, "w+b");	
		if (!temp.fp) return DSK_ERR_SYSERR;
		temp.filename = ldbs_malloc(1 + strlen(filename));
		if (!temp.filename)
		{
			fclose(temp.fp);
			remove(filename);
			return DSK_ERR_NOMEM;	
		}
		strcpy(temp.filename, filename);
		temp.istemp = 0;
	}
	else
	{
#if LDBS_TEMP_IN_MEM
		temp.ismem = 1;
		temp.fp = NULL;
#else
		err = ldbs_mktemp(&temp);
		if (err) return err;
#endif
		temp.istemp = 1;
	}
	/* temp.fp and temp.filename successfully populated. Write an empty
	 * header */
	temp.filesize = HEADER_LEN;
	memcpy(temp.header.magic, LDBS_HEADER_MAGIC, 4);
	memcpy(temp.header.subtype, st, 4);
	temp.header.used = temp.header.trackdir = temp.header.free = LDBLOCKID_NULL;
	err = ldbs_write_header(&temp);	
	if (!err)
	{
		pres = ldbs_malloc(sizeof(LDBS));
		if (!pres) err = DSK_ERR_NOMEM;
	}
	/* If this is a disk image, give it an empty DIR record 
 	 * This is a layering violation: In theory the block layer shouldn't 
	 * know about the track directory. But it's more covenient to do it 
	 * here than at the point the directory is accessed. */
	if (!memcmp(st, LDBS_DSK_TYPE, 4) || !memcmp(st, LDBS_DSK_TYPE_V1, 4))
	{
		/* Allow 176 entries: a disc with 84 tracks, 2 heads and all
		 * optional blocks will have 173 directory entries, so this
		 * seems a reasonable figure. The size can always be 
		 * increased by ldbs_trackdir_add() if necessary. */
		temp.version = st[3];
		temp.dir = ldbs_trackdir_alloc(176);
		if (!temp.dir) 
		{
			free(pres);
			err = DSK_ERR_NOMEM;	
		}
	}

	if (err)
	{
		fclose(temp.fp);
		remove(temp.filename);
		ldbs_free(temp.filename);
		return err;
	}
	if (pres)
	{
		memcpy(pres, &temp, sizeof(LDBS));
	}
	*result = pres;
	return DSK_ERR_OK;
}


/* Open an existing block store. 
 * 
 * filename is the name of the block store on disk.
 *
 * On success populates 'result', returns DSK_ERR_OK.
 * On failure *result = NULL, returns error.
 */
dsk_err_t ldbs_open(PLDBS *result, const char *filename, char *type, 
		int *readonly)
{
	LDBS temp, *pres;
	dsk_err_t err;

	if (result == NULL || filename == NULL) return DSK_ERR_BADPTR;
	*result = 0;
	memset(&temp, 0, sizeof(temp));

	if (readonly && *readonly)	/* R/O access requested, don't try */
	{				/* to open R/W */
		temp.fp = NULL;
	}
	else				/* R/W access requested */
	{
		temp.fp = fopen(filename, "r+b");	
	}
	if (!temp.fp) 	/* Didn't open R/W. Try to open R/O */
	{
		if (readonly) *readonly = 1;
		temp.fp = fopen(filename, "rb");
	}
	if (!temp.fp) return DSK_ERR_SYSERR;
	temp.filename = ldbs_malloc(1 + strlen(filename));
	if (!temp.filename)
	{
		fclose(temp.fp);
		return DSK_ERR_NOMEM;	
	}
	strcpy(temp.filename, filename);
	temp.istemp = 0;
	/* temp.fp and temp.filename successfully populated. Load the 
	 * header */
	err = ldbs_read_header(&temp);
	if (!err)
	{
		if (memcmp(temp.header.magic, LDBS_HEADER_MAGIC, 4))
		{
			fclose(temp.fp);
			ldbs_free(temp.filename);
			return DSK_ERR_NOTME;
		}
		if (FSEEK(temp.fp, 0, SEEK_END)) 
		{
			err = DSK_ERR_SYSERR;
		}
		else
		{
			temp.filesize = ftell(temp.fp);
		}
	}
	if (!err)
	{
		pres = ldbs_malloc(sizeof(LDBS));
		if (!pres) err = DSK_ERR_NOMEM;
	}
/* If this is a DSK type file, load its track directory. 
 * This is a layering violation: In theory the block layer shouldn't 
 * know about the track directory. But it's more covenient to do it here. */
	if (!err && temp.header.trackdir != LDBLOCKID_NULL &&
		(!memcmp(temp.header.subtype, LDBS_DSK_TYPE, 4) ||
		 !memcmp(temp.header.subtype, LDBS_DSK_TYPE_V1, 4)))
	{
		temp.version = temp.header.subtype[3];
		err = ldbs_get_trackdir(&temp, &temp.dir, temp.header.trackdir);
	}
	if (err)
	{
		fclose(temp.fp);
		ldbs_free(temp.filename);
		return err;
	}
	if (type)
	{
		memcpy(type, temp.header.subtype, 4);
	}
	memcpy(pres, &temp, sizeof(LDBS));
	*result = pres;
	return DSK_ERR_OK;
}


/* Blank a range of bytes in a file */
static dsk_err_t zero_range(PLDBS self, long pos, long len)
{
	/* Seek to the first byte to remove */
	if (FSEEK(self->fp, pos, SEEK_SET)) return DSK_ERR_SYSERR;
	while (len)
	{
		if (fputc(0, self->fp) == EOF) return DSK_ERR_SYSERR;
		--len;
	}
	return DSK_ERR_OK;
}

/* Write back any pending data to the block store. */
dsk_err_t ldbs_sync(PLDBS self)
{
	LDBS_BLOCKHEAD blockhead;
	dsk_err_t result = DSK_ERR_OK;

	if (self == NULL) return DSK_ERR_BADPTR;

	/* If there is a directory, write it */
	if (self->dir)
	{
		result = ldbs_put_trackdir(self, self->dir,
				&self->header.trackdir);
	}

	/* Scrub any blank areas (but don't bother on a temporary blockstore) */
	if (!self->istemp)
	{
		LDBLOCKID blockid = self->header.free;

		while (0 != blockid)
		{
			result = ldbs_read_blockhead(self, &blockhead, blockid);
			if (result) break;

			if (!memcmp(blockhead.type, FREEBLOCK, 4))
			{
				zero_range(self, 
					blockid + BLOCKHEAD_LEN, 
					blockhead.dlen);
			}
			blockid = blockhead.next;
		}
		blockid = self->header.used;

		while (0 != blockid)
		{
			result = ldbs_read_blockhead(self, &blockhead, blockid);
			if (result) break;

			if (blockhead.dlen > blockhead.ulen)
			{
				zero_range(self, 
					blockid + BLOCKHEAD_LEN + blockhead.ulen, 
					blockhead.dlen - blockhead.ulen);
			}
			blockid = blockhead.next;
		}

	}

	/* Flush the header if it needs updating */
	if (self->header.dirty)
	{
		result = ldbs_write_header(self);
	}
	return result;
}

/* Close block store. If it is temporary the backing file will be deleted. */
dsk_err_t ldbs_close(PLDBS *self)
{
	LDBS_BLOCKHEAD blockhead;
	dsk_err_t result = DSK_ERR_OK;

	ldbs_sync(self[0]);

#if LDBS_TEMP_IN_MEM
	/* Free all blocks */
	if (self[0]->ismem)
	{
		LDBLOCKID blockid = self[0]->header.free;

		while (0 != blockid)
		{	
			result = ldbs_read_blockhead(self[0], &blockhead, blockid);
			if (result) break;

			ldbs_free(decode_ptr(self[0], blockid));
			blockid = blockhead.next;
		}
		blockid = self[0]->header.used;

		while (0 != blockid)
		{
			result = ldbs_read_blockhead(self[0], &blockhead, blockid);
			if (result) break;

			ldbs_free(decode_ptr(self[0], blockid));
			blockid = blockhead.next;
		}

	}
#endif

	/* Close the backing file */
	if (self[0]->fp && fclose(self[0]->fp))
	{
		result = DSK_ERR_SYSERR;
	}
	/* If this is a temporary file, remove the backing file */
	if (self[0]->istemp && self[0]->filename)
	{
		if (remove(self[0]->filename)) 
		{
		/* Failure to delete a temporary file is not sufficient cause
		 * to return an error */
		/*	result = DSK_ERR_SYSERR; */
		}
	}
	if (self[0]->dir)   ldbs_free(self[0]->dir);
#if LDBS_TEMP_IN_MEM
	if (self[0]->idmap) ldbs_free(self[0]->idmap);
#endif
	ldbs_free(self[0]->filename);
	ldbs_free(self[0]);
	self[0] = NULL;

	return result;
}


/* Get the size/type of a block from the store. */
dsk_err_t ldbs_get_blockinfo(PLDBS self, LDBLOCKID blockid, char type[4],
					size_t *len)
{
	dsk_err_t err;
	LDBS_BLOCKHEAD blockhead;

	if (self == NULL)
	{
		return DSK_ERR_BADPTR;
	}

	/* Assume blockid is correct. */
	err = ldbs_read_blockhead(self, &blockhead, blockid);
	if (err) return err;
	/* Block not found */
	if (!memcmp(blockhead.type, FREEBLOCK, 4)) return DSK_ERR_CORRUPT;

	if (type != NULL)
	{
		memcpy(type, blockhead.type, 4);
	}
	if (len != NULL)
	{
		*len = blockhead.ulen;
	}
	return DSK_ERR_OK;
}




/* Get a block from the store. */
dsk_err_t ldbs_getblock(PLDBS self, LDBLOCKID blockid, char type[4],
					void *data, size_t *len)
{
	dsk_err_t err;
	LDBS_BLOCKHEAD blockhead;

	if (self == NULL || len == NULL)
	{
		return DSK_ERR_BADPTR;
	}

	/* Assume blockid is correct. */
	err = ldbs_read_blockhead(self, &blockhead, blockid);
	if (err) return err;
	/* Block not found */
	if (!memcmp(blockhead.type, FREEBLOCK, 4)) return DSK_ERR_CORRUPT;

	if (type != NULL)
	{
		memcpy(type, blockhead.type, 4);
	}

	/* Are we just getting its size? */
	if (data == NULL || *len == 0)
	{
		*len = blockhead.ulen;
		return DSK_ERR_OVERRUN;
	}
	/* The buffer is too small. Read what can be read. */
 	if ((long)(*len) < blockhead.ulen)
	{
#if LDBS_TEMP_IN_MEM
		if (self->ismem)
		{
			unsigned char *src = decode_ptr(self, blockid);
			memcpy(data, src + sizeof(LDBS_BLOCKHEAD), *len);
			*len = blockhead.ulen;
			return DSK_ERR_OVERRUN;
		}
#endif
		if (FREAD(data, 1, *len, self->fp) < *len)
		{
			return DSK_ERR_SYSERR;
		}

		*len = blockhead.ulen;
		return DSK_ERR_OVERRUN;
	}


	*len = blockhead.ulen;
#if LDBS_TEMP_IN_MEM
	if (self->ismem)
	{
		unsigned char *src = decode_ptr(self, blockid);
		memcpy(data, src + sizeof(LDBS_BLOCKHEAD), *len);
		return DSK_ERR_OK;
	}
#endif
	if (FREAD(data, 1, *len, self->fp) < *len)
	{
		return DSK_ERR_SYSERR;
	}
	return DSK_ERR_OK;
}


/* Get a block from the store, allocating memory for it.
 *
 * Enter with: blockid is the block to retrieve
 *	       type will be populated with the block type
 *
 * On success populates '*data' with a pointer to the data, *len with 
 * length actually retrieved, type with the block type
 * 
 */
dsk_err_t ldbs_getblock_a(PLDBS self, LDBLOCKID blockid, char *type,
					void **data, size_t *len)
{
	dsk_err_t err;
	LDBS_BLOCKHEAD blockhead;

	if (self == NULL || len == NULL)
	{
		return DSK_ERR_BADPTR;
	}

	/* Assume blockid is correct. */
	err = ldbs_read_blockhead(self, &blockhead, blockid);
	if (err) return err;

/*	printf("ldbs_getblock_a(%lx): blockhead.type=%02x%02x%02x%02x\n",
			blockid, blockhead.type[0], blockhead.type[1],
			blockhead.type[2], blockhead.type[3]);
*/
	/* Block not found */
	if (!memcmp(blockhead.type, FREEBLOCK, 4)) return DSK_ERR_CORRUPT;

	if (type != NULL)
	{
		memcpy(type, blockhead.type, 4);
	}
	*data = malloc(blockhead.ulen);
	if (!*data) return DSK_ERR_NOMEM;

	*len = blockhead.ulen;
#if LDBS_TEMP_IN_MEM
	if (self->ismem)
	{
		unsigned char *src = decode_ptr(self, blockid);
		memcpy(*data, src + sizeof(LDBS_BLOCKHEAD), *len);
		return DSK_ERR_OK;
	}
#endif
	if (FREAD(*data, 1, *len, self->fp) < *len)
	{
		return DSK_ERR_SYSERR;
	}
	return DSK_ERR_OK;
}




/* Add a new block to the store */
static dsk_err_t ldbs_addblock(PLDBS self, LDBLOCKID *result, const char *type, 
				const void *data, size_t len)
{
	LDBLOCKID blockid = LDBLOCKID_NULL;
	LDBLOCKID found   = LDBLOCKID_NULL;
	LDBS_BLOCKHEAD blockhead, bh2;
	dsk_err_t err;	
	char tb[5];
	unsigned long foundlen = 0;

	if (type)
	{
		memcpy(tb, type, 4);
	}
	else
	{
		memcpy(tb, USEDBLOCK, 4);
	}

	/* Allocating a 0-byte block is possible but uninteresting */
	if (len == 0)
	{
		*result = LDBLOCKID_NULL;
		return DSK_ERR_OK;
	}	
	if (!self || !data) return DSK_ERR_BADPTR;

	/* Walk the free chain looking for a block that's exactly the
	 * right size */
	blockid = self->header.free;
	found = LDBLOCKID_NULL;
	foundlen = 0L;
	while (0 != blockid)
	{
		err = ldbs_read_blockhead(self, &blockhead, blockid);
		if (err) return err;

		if (memcmp(blockhead.type, FREEBLOCK, 4))
		{
			/* Non-free block on free list, skip */
			blockid = blockhead.next;
			continue;	
		}
		if ((unsigned)blockhead.dlen == len)	/* Block is just the right */
		{				/* size! */
			found = blockid;
			break;
		}
		blockid = blockhead.next;
	}
	if (0 == found)	/* No exact-sized block found */
	{
		blockid = self->header.free;
		while (0 != blockid)
		{
			err = ldbs_read_blockhead(self, &blockhead, blockid);
			if (err) return err;

			if (memcmp(blockhead.type, FREEBLOCK, 4))
			{
				/* Non-free block on free list, skip */
				blockid = blockhead.next;
				continue;	
			}
/* Is this block (a) long enough, and (b) shorter than any previous 
 * candidate? */
			if (blockhead.dlen >= (long)len && 
				(foundlen == 0 || (long)foundlen > blockhead.dlen))
			{
				found    = blockid;
				foundlen = blockhead.dlen;
			}
			blockid = blockhead.next;
		}
	}
	if (0 != blockid)
	{
		/* Load the block header we want */
		err = ldbs_read_blockhead(self, &blockhead, found);
		if (err) return err;
		
		/* Remove block from free list */
		blockid = self->header.free;
		while (0 != blockid)
		{
			err = ldbs_read_blockhead(self, &bh2, blockid);
			if (err) return err;

			if (bh2.next == found)
			{
				bh2.next = blockhead.next;
				err = ldbs_write_blockhead(self, &bh2, blockid);
				if (err) return err;
			}
			blockid = bh2.next;
		}
		if (self->header.free == found)
		{
			self->header.free = blockhead.next;
			self->header.dirty = 1;
		}
		/* Rewrite actual block */
		blockhead.ulen = len;
		memcpy(blockhead.type, tb, 4);
		blockhead.next = self->header.used;
		err = ldbs_write_blockhead(self, &blockhead, found);
		if (err) return err;
		err = ldbs_write_payload(self, found, data, len);
		if (err) return err;
		self->header.used = found;
		self->header.dirty = 1;	
		*result = found;
	}
	else	/* No suitable block found */
	{
		err = ldbs_newblock(self, len, &blockid);
		if (err) return err;

		memset(&blockhead, 0, sizeof(blockhead));
		blockhead.dlen = blockhead.ulen = len;
		memcpy(blockhead.type, tb, 4);
		blockhead.next = self->header.used;

		err = ldbs_write_blockhead(self, &blockhead, blockid);
		if (err) return err;
		err = ldbs_write_payload(self, blockid, data, len);
		if (err) return err;
		self->header.used = blockid;
		self->header.dirty = 1;
		*result = blockid;
	}
	return DSK_ERR_OK;
}

/* In-place update a block 
 *
 * Enter with: blockid is the block's ID
 *             data is the new data to write 
 *             len  is the length of the data to write
 *
 * On success returns DSK_ERR_OK.
 * On failure returns error, including:
 * 	DSK_ERR_OVERRUN: Trying to write more data than will fit
 *	DSK_ERR_CORRUPT: Invalid block ID
 *
 */
static dsk_err_t ldbs_rewriteblock(PLDBS self, LDBLOCKID blockid, 
					const void *data, size_t len)
{
	LDBS_BLOCKHEAD blockhead;
	dsk_err_t err;

	err = ldbs_read_blockhead(self, &blockhead, blockid);
	if (err) return err;

	if (blockhead.dlen < (long)len) return DSK_ERR_OVERRUN;
	blockhead.ulen = len;
	err = ldbs_write_blockhead(self, &blockhead, blockid);
	if (err) return err;
	return ldbs_write_payload(self, blockid, data, len);
}

/* Replace a block -- in-place if possible, allocating a new block if 
 * not. 
 *
 * Parameters as for ldbs_addblock except that 'blockid' needs to be 
 * populated on entry with the ID of the block to replace.
 */
dsk_err_t ldbs_putblock(PLDBS self, LDBLOCKID *blockid, const char *t,
				const void *data, size_t len)
{	
	dsk_err_t err;
	size_t cur_len;
	char cur_type[5];
	char type[5];

	if (t) 
	{
		memcpy(type, t, 4);
	}
	else
	{
		memcpy(type, USEDBLOCK, 4);
	} 	

	/* Block does not exist, just add it */
	if (0 == *blockid)
	{
		return ldbs_addblock(self, blockid, type, data, len);
	}

	/* See if it exists with the correct type */
	err = ldbs_getblock(self, *blockid, cur_type, NULL, &cur_len);
	if (err && err != DSK_ERR_OVERRUN) return err;

	if (!memcmp(cur_type, type, 4) && cur_len >= len && len != 0)
	{
		return ldbs_rewriteblock(self, *blockid, data, len);
	}

	/* Oh dear. Type is different, or size doesn't conform. We need to
	 * delete the old block and create a new one. */
		
	err = ldbs_delblock(self, *blockid);
	*blockid = LDBLOCKID_NULL;
	if (err) return err;
	return ldbs_addblock(self, blockid, type, data, len);
}



dsk_err_t ldbs_fsck(PLDBS self, FILE *logfile)
{
	dsk_err_t err;
	LDBS_BLOCKHEAD blockhead;
	LDBLOCKID pos = LDBLOCKID_NULL;
	unsigned char buf[BLOCKHEAD_LEN];

	if (!self) return DSK_ERR_BADPTR;

	/* Flush any pending changes */
	if (self->header.dirty)
	{
		err = ldbs_write_header(self);
		if (err) return err;
	}
	/* Seek to just after the header */
	err = ldbs_read_header(self);
	if (err) return err;
	pos = ftell(self->fp);

	self->header.used = LDBLOCKID_NULL;
	self->header.free = LDBLOCKID_NULL;
	self->header.dirty = 1;

	/* See if there's another block header; if so, read it */
	while (FREAD(buf, 1, BLOCKHEAD_LEN, self->fp) == BLOCKHEAD_LEN)
	{
		/* Oh dear, block header not where it should be. Let's try
		 * for a manual resync: creep ahead, one byte at a time,
		 * and hope we pick up the scent again. */
		if (memcmp(buf, LDBS_BLOCKHEAD_MAGIC, 4))
		{
			if (logfile)
			{
				fprintf(logfile, "Block header not found "
					"at 0x%08lx, attempting resync\n", 
					pos);
			}
			if (FSEEK(self->fp, 1 - BLOCKHEAD_LEN, SEEK_CUR)) 
				return DSK_ERR_SYSERR;
			continue;
		}
		/* Load the block header */
		err = ldbs_read_blockhead(self, &blockhead, pos);
		if (err) return err;

		/* See if it's designated free or not */
		if (!memcmp(blockhead.type, FREEBLOCK, 4))
		{
			if (logfile) fprintf(logfile, "Free block at 0x%08lx\n",
					pos);
			blockhead.next = self->header.free;
			self->header.free = pos;
		}
		else
		{
			if (logfile) fprintf(logfile, "Block type %-4.4s at "
				"0x%08lx\n", blockhead.type, pos);
			blockhead.next = self->header.used;
			self->header.used = pos;
		}
		/* Skip over the block header and the block */
		pos += (BLOCKHEAD_LEN + blockhead.dlen);
		if (FSEEK(self->fp, pos, SEEK_SET))
		{
			return DSK_ERR_SYSERR;
		}	
	}
	/* See if the track directory is valid */
	err = ldbs_read_blockhead(self, &blockhead, self->header.trackdir);
	if (err == DSK_ERR_CORRUPT)
	{
		if (logfile) fprintf(logfile, "Root pointer 0x%08lx is "
			"not valid, clearing.\n", self->header.trackdir);
		self->header.trackdir = LDBLOCKID_NULL;
	}

	return ldbs_write_header(self);
}







/* Free a block.
 *
 * Enter with: blockid is the block's identity
 *
 * Returns DSK_ERR_OK on success, DSK_ERR_BADPARM if block ID is LDBLOCKID_NULL
 * 
 */
dsk_err_t ldbs_delblock(PLDBS self, LDBLOCKID blockid)
{
	LDBS_BLOCKHEAD blockhead, bh2;
	dsk_err_t err;	
	LDBLOCKID pos = LDBLOCKID_NULL;

	if (!self) return DSK_ERR_BADPTR;	
	if (blockid == LDBLOCKID_NULL) return DSK_ERR_BADPARM;

	/* Load the requested block header */
	err = ldbs_read_blockhead(self, &blockhead, blockid);
	if (err) return err;

	/* Walk the data chain removing any references to the selected 
	 * block. */
	pos = self->header.used;
	while (0 != pos)
	{
		err = ldbs_read_blockhead(self, &bh2, pos);
		if (err) return err;

		if (bh2.next == blockid)
		{
			bh2.next = blockhead.next;
			err = ldbs_write_blockhead(self, &bh2, pos);
			if (err) return err;
		}
		pos = bh2.next;
	}
	/* If this block is at the head of the chain, remove it */
	if (self->header.used == blockid)
	{
		self->header.used = blockhead.next;
		self->header.dirty = 1;
	}
	/* Now blank this block... */
	memcpy(blockhead.type, FREEBLOCK, 4);
	blockhead.ulen = 0;
	blockhead.next = self->header.free;
	err = ldbs_write_blockhead(self, &blockhead, blockid);
	if (err) return err;
	self->header.free = blockid;	
	self->header.dirty = 1;
	return DSK_ERR_OK;
}

/* Get the block ID of the root directory */
dsk_err_t ldbs_getroot(PLDBS self, LDBLOCKID *blockid)
{
	if (!self || !blockid) return DSK_ERR_BADPTR;

	*blockid = self->header.trackdir;
	return DSK_ERR_OK;
}


/* Designate a block as the track directory */
dsk_err_t ldbs_setroot(PLDBS self, LDBLOCKID blockid)
{
	if (!self) return DSK_ERR_BADPTR;

	if (blockid != self->header.trackdir)
	{
		self->header.trackdir = blockid;
		self->header.dirty = 1;
	}
	return DSK_ERR_OK;
}

/* Delete all blocks in a blockstore */
dsk_err_t ldbs_clear(PLDBS self)
{
	LDBS_BLOCKHEAD blockhead;
	LDBLOCKID blockid = self->header.used;
	dsk_err_t err = DSK_ERR_OK;

	/* For each block... */
	while (0 != blockid)
	{
		/* Load its header */
		err = ldbs_read_blockhead(self, &blockhead, blockid);
		if (err) break;

		/* And erase it */
		err = ldbs_delblock(self, blockid);
		if (err) break;

		blockid = blockhead.next;
	}
	/* If there was a track directory, there isn't now */
	if (self->dir) 
	{
		ldbs_free(self->dir);
		self->dir = NULL;
	}
	self->header.trackdir = 0;	
	self->header.dirty = 1;
	return err;
}

static LDBLOCKID remap(LDBLOCKID *map, unsigned maplen, LDBLOCKID id)
{
	unsigned n;

	if (id == LDBLOCKID_NULL) 
	{
/*		printf("remap(NULL)=NULL\n"); */
		return LDBLOCKID_NULL;
	}

	for (n = 0; n < maplen; n += 2)
	{
/*		printf("{%ld:%ld}", map[n], map[n+1]); */
		if (map[n] == id) 
		{
/*			printf("remap(%ld)=%ld\n", map[n], map[n+1]); */
			return map[n+1];
		}
	}
#ifndef WIN16
	fprintf(stderr, "Internal error: remap(%lx)=NULL\n", id); 
#endif
/* Should never happen: A valid ID is not found in the list. All we 
 * can do is return null. */
	return LDBLOCKID_NULL;
}

dsk_err_t remap_track_header(PLDBS self, unsigned char *th, unsigned thlen,
				LDBLOCKID *mapping, unsigned maplen)
{
	unsigned se_offset, se_len;
	unsigned ns;
	unsigned sectors;

	if (self->version < 2)
	{
		se_offset = 6;
		se_len = 12;
		sectors = ldbs_peek2(th);
	}
	else
	{
		se_offset = ldbs_peek2(th);
		se_len    = ldbs_peek2(th + 2);
		sectors   = ldbs_peek2(th + 4);
	}
	for (ns = 0; ns < sectors; ns++)
	{
		LDBLOCKID id;

		/* Corrupt source file */
		if (se_offset + 12 + ns * se_len > thlen) 
		{ 
			return DSK_ERR_CORRUPT; 
		}	
		id = ldbs_peek4(th + se_offset + 8 + ns * se_len);
		if (id == LDBLOCKID_NULL) continue;
		id = remap(mapping, maplen, id);
		ldbs_poke4(th + se_offset + 8 + ns * se_len, id);
	}
	return DSK_ERR_OK;
}

/* Copy all blocks from one blockstore to another. 
 * The cloning process isn't just a matter of copying blocks from one file
 * to the other, because the track directory and track headers contain 
 * block IDs. So we do two passes: the first to migrate the blocks 
 * themselves, the second to fix up the block IDs. */
dsk_err_t ldbs_clone(PLDBS source, PLDBS dest)
{
	LDBLOCKID *mapping;
	unsigned maplen, mapmax;
	LDBS_BLOCKHEAD blockhead;
	LDBLOCKID blockid = source->header.used;
	LDBLOCKID newid = LDBLOCKID_NULL;
	dsk_err_t err = DSK_ERR_OK;
	void *buf;
	char type[4];
	size_t len;

	/* Build a mapping of block IDs: old, new, old, new... */
	mapmax = 400;	/* So 400 entries can map 200 IDs */
	mapping = ldbs_malloc(mapmax * sizeof(LDBLOCKID));
	if (!mapping) return DSK_ERR_NOMEM;
	maplen = 0;

	/* Delete everything out of the destination file. */
	err = ldbs_clear(dest);
	if (err) 
	{
		ldbs_free(mapping);
		return err;
	}
	/* First, move the data blocks across. This happens to reverse 
	 * their order, just to be fun. */
	while (LDBLOCKID_NULL != blockid)
	{
	/* Load the block header so we know the next entry in the list */
		err = ldbs_read_blockhead(source, &blockhead, blockid);
		if (err) 
		{
			break;
		}
	/* Load the block proper */
		err = ldbs_getblock_a(source, blockid, type, &buf, &len);
		if (err) 
		{
			break;
		}

	/* Write it out to the destination file */	
		newid = LDBLOCKID_NULL;	
		err = ldbs_putblock(dest, &newid, type, buf, len);
		ldbs_free(buf);
		if (err) 
		{
			break;
		}
	/* Add (blockid, newid) to the mapping array */	
		if (maplen == mapmax)
		{
			mapmax *= 2;
			mapping = ldbs_realloc(mapping, 
				mapmax * sizeof(LDBLOCKID));
			if (!mapping) return DSK_ERR_NOMEM;
		}
/*
fprintf(stderr, "%08lx -> %08lx %c%c%c%c maplen=%d\n", blockid, newid, 
			isprint(type[0]) ? type[0] : '.',
			isprint(type[1]) ? type[1] : '.',
			isprint(type[2]) ? type[2] : '.',
			isprint(type[3]) ? type[3] : '.', maplen);
*/
		mapping[maplen++] = blockid;
		mapping[maplen++] = newid;
	/* And go to the next block */
		blockid = blockhead.next;
	}
	if (err) 
	{
		ldbs_free(mapping);
		return err;
	}
/* Remap the block IDs. The first one is the ID of the track directory. */
	if (source->header.trackdir)
	{
		dest->header.trackdir = remap(mapping, maplen, 
						source->header.trackdir);

	}
/* Up to now, ldbs_clone() has been working at blockstore level. If the 
 * file it is processing is a disc image, it now needs to remap the IDs 
 * in its directory and track headers */
	if (!memcmp(source->header.subtype, LDBS_DSK_TYPE, 4) ||
	    !memcmp(source->header.subtype, LDBS_DSK_TYPE_V1, 4))
	{
		dest->version = source->version;
		memcpy(dest->header.subtype, source->header.subtype, 4);
		if (dest->dir)
		{
			ldbs_free(dest->dir);
			dest->dir = NULL;
		}

/* Remap the block IDs in the directory */
		err = ldbs_get_trackdir(dest, &dest->dir, 
				dest->header.trackdir);
		if (!err)
		{
			unsigned n;	
			for (n = 0; n < dest->dir->count; n++)
			{
				dest->dir->entry[n].blockid = 
					remap(mapping, maplen, 
						dest->dir->entry[n].blockid);	
/* If it's a track, we need to redo that track's header too */
				if (dest->dir->entry[n].id[0] == 'T')
				{
					unsigned char *th;
					size_t thlen;
					char type[4];

/* So load the track header (using the newly-remapped block id)  */
					err = ldbs_getblock_a(dest, 
						dest->dir->entry[n].blockid,
						type, (void **)&th, &thlen);
/* Remap the sector references within it... */
					if (!err)
					{
						err = remap_track_header(dest, th, thlen, mapping, maplen);
/* ... and save it */
						if (!err) err = ldbs_rewriteblock(dest, 
						  dest->dir->entry[n].blockid,
						  th, thlen); 
					}
					ldbs_free(th);
				}
				if (err) break;
			}
			dest->dir->dirty = 1;
			
		}
	}
	if (!err)
	{
		err = ldbs_sync(dest);
	}
	memcpy(dest->header.subtype, source->header.subtype, 4);
	ldbs_free(mapping);
	return err;
}





/***************************************************************************/
/************************* DISK IMAGE FUNCTIONS ****************************/
/***************************************************************************/
/* Functions in this section use the blockstore to hold a disk image file; */
/* they are not, and should not be, aware of whether the blockstore is     */
/* implemented as a disk file or in memory                                 */
/***************************************************************************/

/* Encode a track ID as a 4-byte block type */
void ldbs_encode_trackid(char *trackid, dsk_pcyl_t cylinder, dsk_phead_t head)
{
	trackid[0] = 'T';
	ldbs_poke2((unsigned char *)trackid + 1, (unsigned short)cylinder);
	trackid[3] = head;
}

/* Decode a track ID: returns 1 if valid, 0 if not */
int ldbs_decode_trackid(const char *trackid, dsk_pcyl_t *cylinder, dsk_phead_t *head)
{
	if (trackid[0] != 'T') return 0;
	*cylinder = ldbs_peek2((unsigned char *)trackid + 1);
	*head = trackid[3];
	return 1;
}

/* Encode a sector ID */
void ldbs_encode_secid(char *secid, dsk_pcyl_t cylinder, dsk_phead_t head,
		dsk_psect_t sector)
{
	secid[0] = 'S';
	secid[1] = cylinder;
	secid[2] = head;
	secid[3] = sector;
}
/* Decode a sector ID: returns 1 if valid, 0 if not */
int ldbs_decode_secid(const char *secid, dsk_pcyl_t *cylinder, dsk_phead_t *head, dsk_psect_t *sector)
{
	if (secid[0] != 'S') return 0;
	*cylinder = secid[1];
	*head     = secid[2];
	*sector   = secid[3];
	return 1;
}


LDBS_TRACKDIR  *ldbs_trackdir_alloc(unsigned entries)
{
	size_t size = sizeof(LDBS_TRACKDIR) + entries * sizeof(LDBS_TRACKDIR_ENTRY);
	LDBS_TRACKDIR *result = ldbs_malloc(size);
	if (!result) return NULL;

	memset(result, 0, size);
	result->count = entries;
	
	return result;
}

dsk_err_t ldbs_trackdir_copy(PLDBS self, LDBS_TRACKDIR **result)
{
	LDBS_TRACKDIR *copy;

	if (!self || !result) return DSK_ERR_BADPTR;

	if (!self->dir)
	{
		return DSK_ERR_NOTME;
	}

	copy = ldbs_trackdir_alloc(self->dir->count);
	if (!copy) return DSK_ERR_NOMEM;

	memcpy(copy->entry, self->dir->entry, 
			self->dir->count * sizeof(LDBS_TRACKDIR_ENTRY));

	*result = copy;
	return DSK_ERR_OK;
}


static dsk_err_t ldbs_get_trackdir(PLDBS self, LDBS_TRACKDIR **pdir, LDBLOCKID blockid)
{
	char dirtype[4];
	size_t len = 0;
	unsigned char *buf, *ptr;
	LDBS_TRACKDIR *dir;
	short ec, n;
	dsk_err_t err;

	/* Get size of root dir */
	err = ldbs_getblock_a(self, blockid, dirtype, (void **)&buf, &len);
	if (err) return err;
	/* Root dir not of type DIR1? Then bail */
	if (memcmp(dirtype, LDBS_DIR_TYPE, 4)) 
	{
		ldbs_free(buf);
		return DSK_ERR_NOTME;
	}
	/* Length of disk structure = 2 + 8 * count of entries */
	dir = ldbs_trackdir_alloc( (len - 2) / 8);	
	if (!dir) 
	{
		ldbs_free(buf);
		return DSK_ERR_NOMEM;
	}
	/* Get entry count */
	ec = ldbs_peek2(buf);

	/* Read the lesser of stated count and maximum actual size */
	if (ec > dir->count) ec = dir->count;	

	for (n = 0; n < ec; n++)
	{
		ptr = buf + 2 + 8*n;
		memcpy(dir->entry[n].id, ptr, 4);
		dir->entry[n].blockid = ldbs_peek4(ptr + 4);
	}
	ldbs_free(buf);
	*pdir = dir;
	return DSK_ERR_OK;
}





static dsk_err_t ldbs_put_trackdir(PLDBS self, LDBS_TRACKDIR *dir, LDBLOCKID *blkid)
{
	size_t n, count = 0;
	unsigned char *buf;
	size_t bufsize;
	dsk_err_t err;
	
	/* Work out how big the on-disk directory needs to be */
	for (n = 0; n < dir->count; n++)
	{
		if (memcmp(dir->entry[n].id, FREEBLOCK, 4))
		{
			++count;
		}
	}	
	bufsize = count * 8 + 2;
	buf = ldbs_malloc(bufsize);
	if (!buf) return DSK_ERR_NOMEM;
	memset(buf, 0, bufsize);
	ldbs_poke2(buf, (unsigned short)count);
	for (n = count = 0; n < dir->count; n++)
	{
		if (memcmp(dir->entry[n].id, FREEBLOCK, 4))
		{
			memcpy(buf + 2 + 8 * count, dir->entry[n].id, 4);
			ldbs_poke4(buf + 6 + 8 * count, dir->entry[n].blockid);
			++count;
		}
	}
	err = ldbs_putblock(self, blkid, LDBS_DIR_TYPE, buf, bufsize);

	ldbs_free(buf);
	return err;
}


dsk_err_t ldbs_get_trackhead(PLDBS self, LDBS_TRACKHEAD **trkh, 
	dsk_pcyl_t cylinder, dsk_phead_t head)
{
	size_t n;
	unsigned char *buf;
	size_t bufsize;
	LDBLOCKID blkid;
	dsk_err_t err;
	char tbuf[4];
	LDBS_TRACKHEAD *result;
	char type[4];
	unsigned se_offset;	/* Offset to sector entries */
	unsigned se_size;	/* Length of a sector entry */

	ldbs_encode_trackid(type, cylinder, head);
	/* See if the requested track exists */
	if (!self->dir)
	{
		return DSK_ERR_NOTME;
	}
	err = ldbs_trackdir_find(self->dir, type, &blkid);
	if (err) return err;

	/* Not found */
	if (0 == blkid)
	{
		*trkh = 0;
		return DSK_ERR_OK;
	}
	bufsize = 0;
	err = ldbs_getblock_a(self, blkid, tbuf, (void **)&buf, &bufsize);
	if (err) return err;

	/* Track header must be at least 6 bytes */
	if (bufsize < 6)
	{
		ldbs_free(buf);
		return DSK_ERR_CORRUPT;
	}

	/* Disk structure is loaded in buf */
	if (self->version < 2)	/* Sector count is in different places */
	{			/* in v1 and v2 files */
		result = ldbs_trackhead_alloc(ldbs_peek2(buf));
	}
	else
	{
		result = ldbs_trackhead_alloc(ldbs_peek2(buf + 4));
	}
	if (!result) 
	{
		ldbs_free(buf);
		return DSK_ERR_NOMEM;
	}
	if (self->version < 2)	/* V1 has fixed size track & sector headers */
	{
		se_offset = 6;
		se_size = 12;
		result->count    = ldbs_peek2(buf);
		result->datarate = buf[2];
		result->recmode  = buf[3];
		result->gap3     = buf[4];
		result->filler   = buf[5];
	}
	else	/* In V2 their size is specified in the file */
	{
		se_offset        = ldbs_peek2(buf);
		se_size          = ldbs_peek2(buf + 2);
		result->count    = ldbs_peek2(buf + 4);
		result->datarate = buf[6];
		result->recmode  = buf[7];
		result->gap3     = buf[8];
		result->filler   = buf[9];
		if (se_offset >= 12)
		{
			result->total_len = ldbs_peek2(buf + 10);
		}
	}
	/* Indicated size exceeds actual size */
	if (se_offset + result->count * se_size > bufsize)
	{
		ldbs_free(buf);
		return DSK_ERR_CORRUPT;
	}

	for (n = 0; n < result->count; n++)
	{
		result->sector[n].id_cyl  = buf[n * se_size + se_offset];
		result->sector[n].id_head = buf[n * se_size + se_offset + 1];
		result->sector[n].id_sec  = buf[n * se_size + se_offset + 2];
		result->sector[n].id_psh  = buf[n * se_size + se_offset + 3];
		result->sector[n].st1     = buf[n * se_size + se_offset + 4];
		result->sector[n].st2     = buf[n * se_size + se_offset + 5];
		result->sector[n].copies  = buf[n * se_size + se_offset + 6];
		result->sector[n].filler  = buf[n * se_size + se_offset + 7];
		result->sector[n].blockid = ldbs_peek4(buf + n * se_size + se_offset + 8);
		if (se_size >= 16)
		{
			result->sector[n].trail  = ldbs_peek2(buf + n * se_size + se_offset + 12);
			result->sector[n].offset = ldbs_peek2(buf + n * se_size + se_offset + 14);
		}
	}
	ldbs_free(buf);
	*trkh = result;
	return DSK_ERR_OK;
}


dsk_err_t ldbs_put_trackhead(PLDBS self, const LDBS_TRACKHEAD *trkh, 
				dsk_pcyl_t cylinder, dsk_phead_t head)
{
	size_t n;
	unsigned char *buf;
	size_t bufsize;
	dsk_err_t err;
	char type[4];
	unsigned se_offset;	/* Offset to sector entries */
	unsigned se_size;	/* Length of a sector entry */

	ldbs_encode_trackid(type, cylinder, head);

	if (trkh == NULL) return ldbs_putblock_d(self, type, NULL, 0);

	if (self->version < 2)
	{
		se_offset = 6;
		se_size = 12;
	}
	else
	{
		se_offset = 12;
		se_size = 16;
	}
	bufsize = (trkh->count * se_size) + se_offset;	
	buf = ldbs_malloc(bufsize);
	if (!buf) return DSK_ERR_NOMEM;

	if (self->version < 2)
	{
		ldbs_poke2(buf, trkh->count);
		buf[2] = trkh->datarate;
		buf[3] = trkh->recmode;
		buf[4] = trkh->gap3;
		buf[5] = trkh->filler;
	}
	else
	{
		ldbs_poke2(buf,     se_offset);
		ldbs_poke2(buf + 2, se_size);
		ldbs_poke2(buf + 4, trkh->count);
		buf[6] = trkh->datarate;
		buf[7] = trkh->recmode;
		buf[8] = trkh->gap3;
		buf[9] = trkh->filler;
		ldbs_poke2(buf + 10, trkh->total_len);
	}
	for (n = 0; n < trkh->count; n++)
	{
		buf[n * se_size + se_offset] = trkh->sector[n].id_cyl;
		buf[n * se_size + se_offset +  1] = trkh->sector[n].id_head;
		buf[n * se_size + se_offset +  2] = trkh->sector[n].id_sec;
		buf[n * se_size + se_offset +  3] = trkh->sector[n].id_psh;
		buf[n * se_size + se_offset +  4] = trkh->sector[n].st1;
		buf[n * se_size + se_offset +  5] = trkh->sector[n].st2;
		buf[n * se_size + se_offset +  6] = trkh->sector[n].copies;
		buf[n * se_size + se_offset +  7] = trkh->sector[n].filler;
		ldbs_poke4(buf + n * se_size + se_offset + 8, trkh->sector[n].blockid);
		if (se_size >= 16)
		{
			ldbs_poke2(buf + n * se_size + se_offset + 12, trkh->sector[n].trail);
			ldbs_poke2(buf + n * se_size + se_offset + 14, trkh->sector[n].offset);
		}
	}
	err = ldbs_putblock_d(self, type, buf, bufsize);

	ldbs_free(buf);
	return err;
}




dsk_err_t ldbs_getblock_d(PLDBS self, const char *type,
				void *data, size_t *len)
{
	/* Look up that block in the directory */
	LDBLOCKID blkid = LDBLOCKID_NULL;
	dsk_err_t err;

	if (!self || !type) return DSK_ERR_BADPTR;
	/* See if the object already exists in the directory 
	 * (there must be a directory) */
	if (!self->dir) return DSK_ERR_NOTME;

	err = ldbs_trackdir_find(self->dir, type, &blkid);
	if (err) return err;

	return ldbs_getblock(self, blkid, NULL, data, len);
}


dsk_err_t ldbs_getblock_da(PLDBS self, const char *type,
				void **data, size_t *len)
{
	/* Look up that block in the directory */
	LDBLOCKID blkid = LDBLOCKID_NULL;
	dsk_err_t err;

	if (!self || !type) return DSK_ERR_BADPTR;
	/* See if the object already exists in the directory 
	 * (there must be a directory) */
	if (!self->dir) return DSK_ERR_NOTME;

	err = ldbs_trackdir_find(self->dir, type, &blkid);
	if (err) return err;

	return ldbs_getblock_a(self, blkid, NULL, data, len);
}







dsk_err_t ldbs_putblock_d(PLDBS self, const char *type,
				const void *data, size_t len)
{
	/* Firstly, store it in the file */
	LDBLOCKID blkid = LDBLOCKID_NULL;
	dsk_err_t err;
	int n;

	if (!self || !type) return DSK_ERR_BADPTR;
	/* See if the object already exists in the directory 
	 * (there must be a directory) */
	if (!self->dir) return DSK_ERR_NOTME;

	err = ldbs_trackdir_find(self->dir, type, &blkid);
	if (err) return err;

	if (data == NULL)
	{
		err = ldbs_delblock(self, blkid);
		blkid = LDBLOCKID_NULL;
	}
	else
	{
		err = ldbs_putblock(self, &blkid, type, data, len);
	}
	if (err) return err;

	/* Object updated. Keep the directory in sync. */
	for (n = 0; n < self->dir->count; n++)
	{
		/* Is there an existing entry? */
		if (!memcmp(self->dir->entry[n].id, type, 4))
		{
			if (self->dir->entry[n].blockid != blkid)
			{
				self->dir->entry[n].blockid = blkid;
	/* If blockid has become 0 (block deleted) remove that directory 
	 * entry */
				if (blkid == LDBLOCKID_NULL)
				{
					memcpy(self->dir->entry[n].id, 
							FREEBLOCK, 4);
				}
				self->dir->dirty = 1;	
			}
			return DSK_ERR_OK;
		} 
	}
	/* No existing entry found. Add to directory */
	return ldbs_trackdir_add(&self->dir, type, blkid);
}











dsk_err_t ldbs_trackdir_add(LDBS_TRACKDIR **dir, const char type[4], LDBLOCKID blockid)
{
	unsigned n;
	size_t newsize;
	LDBS_TRACKDIR *d2;
	
	if (!dir || !type) return DSK_ERR_BADPTR;

	for (n = 0; n < dir[0]->count; n++)
	{
		if (!memcmp(dir[0]->entry[n].id, FREEBLOCK, 4) ||
		    !memcmp(dir[0]->entry[n].id, type, 4))
		{
			memcpy(dir[0]->entry[n].id, type, 4);
			dir[0]->entry[n].blockid = blockid;
			dir[0]->dirty = 1;	
			return DSK_ERR_OK;
		}
	}
	/* Directory full. Add 20 more entries. */
	newsize = sizeof(LDBS_TRACKDIR) + 
			(dir[0]->count + 20) * sizeof(LDBS_TRACKDIR_ENTRY);
	d2 = ldbs_realloc(dir[0], newsize);
	if (!d2) return DSK_ERR_NOMEM;

	for (n = d2->count; n < (unsigned)d2->count + 20; n++)
	{
		memset(&d2->entry[n], 0, sizeof(LDBS_TRACKDIR_ENTRY));
	}
	memcpy(d2->entry[d2->count].id, type, 4);
	d2->entry[d2->count].blockid = blockid;
	d2->count += 20;
	dir[0] = d2;
	dir[0]->dirty = 1;	
	return DSK_ERR_OK;
}


dsk_err_t ldbs_trackdir_find(LDBS_TRACKDIR *dir, const char *id, LDBLOCKID *result)
{
	unsigned n;

	if (!dir || !id || !result) return DSK_ERR_BADPTR;

	*result = LDBLOCKID_NULL;
	for (n = 0; n < dir->count; n++)
	{
		if (!memcmp(dir->entry[n].id, id, 4)) 
		{
			*result = dir->entry[n].blockid;
			return DSK_ERR_OK;
		}
	}
	return DSK_ERR_OK;
}




LDBS_TRACKHEAD *ldbs_trackhead_alloc(unsigned entries)
{
	size_t size = sizeof(LDBS_TRACKHEAD) + entries * sizeof(LDBS_SECTOR_ENTRY);
	LDBS_TRACKHEAD *result = ldbs_malloc(size);
	if (!result) return NULL;

	memset(result, 0, size);
	result->count = entries;
	
	return result;
}


LDBS_TRACKHEAD *ldbs_trackhead_realloc(LDBS_TRACKHEAD *t, unsigned short entries)
{
	size_t newsize = sizeof(LDBS_TRACKHEAD) + entries * sizeof(LDBS_SECTOR_ENTRY);
	unsigned oldcount = t->count;
	unsigned n;

	LDBS_TRACKHEAD *result = ldbs_realloc(t, newsize);
	if (!result) return NULL;

	/* If structure extended, blank the new bit */
	if (entries > oldcount)
	{
		for (n = oldcount; n < entries; n++)
			memset(&result->sector[n], 0, sizeof(LDBS_SECTOR_ENTRY));
	}	
	result->count = entries;
	
	return result;
}






dsk_err_t ldbs_get_asciiz(PLDBS self, const char type[4], char **buffer)
{
	dsk_err_t err;
	LDBLOCKID blkid;
	char tbuf[4];
	void *ptmp;
	size_t len = 0;

	if (!self || !buffer) return DSK_ERR_BADPTR;

	*buffer = NULL;
	if (!self->dir)
	{
		return DSK_ERR_NOTME;
	}
	err = ldbs_trackdir_find(self->dir, type, &blkid);
	if (err) return err;

	if (0 == blkid)
	{
		return DSK_ERR_OK;
	}
	err = ldbs_getblock_a(self, blkid, tbuf, &ptmp, &len);
	if (err) return err;

	/* Realloc the buffer to make space for a terminating 0 */
	*buffer = ldbs_realloc(ptmp, 1 + len);
	if (!*buffer)
	{
		ldbs_free(ptmp);
		return DSK_ERR_NOMEM;
	}
	(*buffer)[len] = 0;
	return DSK_ERR_OK;
}

dsk_err_t ldbs_get_creator(PLDBS self, char **buffer)
{
	return ldbs_get_asciiz(self, LDBS_CREATOR_TYPE, buffer);
}

dsk_err_t ldbs_put_creator(PLDBS self, const char *creator)
{
	if (!creator)
	{
		return ldbs_putblock_d(self, LDBS_CREATOR_TYPE, NULL, 0);
	}
	return ldbs_putblock_d(self, LDBS_CREATOR_TYPE, creator, 
		strlen(creator));
}

dsk_err_t ldbs_get_comment(PLDBS self, char **buffer)
{
	return ldbs_get_asciiz(self, LDBS_INFO_TYPE, buffer);
}


dsk_err_t ldbs_put_comment(PLDBS self, const char *comment)
{
	if (!comment)
	{
		return ldbs_putblock_d(self, LDBS_INFO_TYPE, NULL, 0);
	}
	return ldbs_putblock_d(self, LDBS_INFO_TYPE, comment, 
		strlen(comment));
}



dsk_err_t ldbs_get_geometry(PLDBS self, DSK_GEOMETRY *dg)
{
	dsk_err_t err;
	unsigned char buf[15];
	LDBLOCKID blkid;
	char tbuf[4];
	size_t len;

	if (!self || !dg) return DSK_ERR_BADPTR;

	if (!self->dir)
	{
		return DSK_ERR_NOTME;
	}
	err = ldbs_trackdir_find(self->dir, LDBS_GEOM_TYPE, &blkid);
	if (err) return err;

	if (0 == blkid)
	{
		memset(dg, 0, sizeof(*dg));
		return DSK_ERR_OK;
	}
	len = sizeof(buf);
	err = ldbs_getblock(self, blkid, tbuf, buf, &len);
	if (err) return err;

	dg->dg_sidedness = buf[0];
	dg->dg_cylinders = ldbs_peek2(buf + 1);
	dg->dg_heads     = buf[3];
	dg->dg_sectors   = buf[4];
	dg->dg_secbase   = buf[5];
	dg->dg_secsize   = ldbs_peek2(buf + 6);
	dg->dg_datarate  = buf[8];
	dg->dg_rwgap     = buf[9];
	dg->dg_fmtgap    = buf[10];
	dg->dg_fm        = ldbs_peek2(buf + 11);
	dg->dg_nomulti   = buf[13];
	dg->dg_noskip    = buf[14];
	return DSK_ERR_OK;
}




dsk_err_t ldbs_put_geometry(PLDBS self, const DSK_GEOMETRY *dg)
{
	unsigned char buf[15];

	if (!self) return DSK_ERR_BADPTR;

	if (!dg) return ldbs_putblock_d(self, LDBS_GEOM_TYPE, NULL, 0);

	buf[0] = dg->dg_sidedness;
	ldbs_poke2(buf +1, (unsigned short)dg->dg_cylinders);
	buf[3] = dg->dg_heads;
	buf[4] = dg->dg_sectors;
	buf[5] = dg->dg_secbase;
	ldbs_poke2(buf + 6, (unsigned short)dg->dg_secsize);
	buf[8] = dg->dg_datarate;
	buf[9] = dg->dg_rwgap;
	buf[10] = dg->dg_fmtgap;
	ldbs_poke2(buf + 11, (unsigned short)dg->dg_fm);
	buf[13] = dg->dg_nomulti;
	buf[14] = dg->dg_noskip;

	return ldbs_putblock_d(self, LDBS_GEOM_TYPE, buf, sizeof(buf));
}
	

dsk_err_t ldbs_get_dpb(PLDBS self, LDBS_DPB *dpb)
{
	dsk_err_t err;
	unsigned char buf[17];
	LDBLOCKID blkid;
	char tbuf[4];
	size_t len;

	if (!self || !dpb) return DSK_ERR_BADPTR;

	if (!self->dir)
	{
		return DSK_ERR_NOTME;
	}
	err = ldbs_trackdir_find(self->dir, LDBS_DPB_TYPE, &blkid);
	if (err) return err;

	if (0 == blkid)
	{
		memset(dpb, 0, sizeof(*dpb));
		return DSK_ERR_OK;
	}
	len = sizeof(buf);
	err = ldbs_getblock(self, blkid, tbuf, buf, &len);
	if (err) return err;

	dpb->spt = ldbs_peek2(buf);
	dpb->bsh = buf[2];
	dpb->blm = buf[3];
	dpb->exm = buf[4];
	dpb->dsm = ldbs_peek2(buf+5);
	dpb->drm = ldbs_peek2(buf+7);
	dpb->al[0] = buf[9];
	dpb->al[1] = buf[10];
	dpb->cks = ldbs_peek2(buf + 11);
	dpb->off = ldbs_peek2(buf + 13);
	dpb->psh = buf[15];
	dpb->phm = buf[16];
	return DSK_ERR_OK;
}




dsk_err_t ldbs_put_dpb(PLDBS self, const LDBS_DPB *dpb)
{
	unsigned char buf[17];

	if (!self) return DSK_ERR_BADPTR;

	if (!dpb) return ldbs_putblock_d(self, LDBS_DPB_TYPE, NULL, 0);

	ldbs_poke2(buf, dpb->spt);
	buf[2] = dpb->bsh;
	buf[3] = dpb->blm;
	buf[4] = dpb->exm;
	ldbs_poke2(buf+5, dpb->dsm);
	ldbs_poke2(buf+7, dpb->drm);
	buf[9]  = dpb->al[0];
	buf[10] = dpb->al[1];
	ldbs_poke2(buf + 11, dpb->cks);
	ldbs_poke2(buf + 13, dpb->off);
	buf[15] = dpb->psh;
	buf[16] = dpb->phm;

	return ldbs_putblock_d(self, LDBS_DPB_TYPE, buf, sizeof(buf));
}

dsk_err_t ldbs_max_cyl_head(PLDBS self, dsk_pcyl_t *cyls, dsk_phead_t *heads)
{
	int n;
	int maxcyl = -1;
	int maxhead = -1;
	dsk_pcyl_t c;
	dsk_phead_t h;

	if (!self)
	{
		return DSK_ERR_BADPTR;
	}
	if (!self->dir)
	{
		return DSK_ERR_NOTME;
	}
	for (n = 0; n < self->dir->count; n++)
	{
		if (ldbs_decode_trackid(self->dir->entry[n].id, &c, &h))
		{
			if ((int)c > maxcyl) maxcyl = c;
			if ((int)h > maxhead) maxhead = h;
		}
	}
	if (cyls)  *cyls  = maxcyl + 1;
	if (heads) *heads = maxhead + 1;
	return DSK_ERR_OK;
}

static dsk_err_t ldbs_all_tracks_body(PLDBS self, dsk_pcyl_t cyl, 
	dsk_phead_t head, LDBS_TRACK_CALLBACK cbk, void *param)
{
	LDBS_TRACKHEAD *trackhead;
	dsk_err_t err = DSK_ERR_OK;

	err = ldbs_get_trackhead(self, &trackhead, cyl, head);
	if (err) return err;

	if (trackhead != NULL)
	{
		err = (*cbk)(self, cyl, head, trackhead, param);
		ldbs_free(trackhead);
	}
	return err;
}


/* Iterate over all tracks in specified order */
dsk_err_t ldbs_all_tracks(PLDBS self, LDBS_TRACK_CALLBACK cbk, 
	dsk_sides_t sides, void *param)
{
	dsk_pcyl_t  cyl, maxc;
	dsk_phead_t head, maxh;
	dsk_err_t err;

	err = ldbs_max_cyl_head(self, &maxc, &maxh);
	if (err) return err;

	switch (sides)
	{
		default:
		case SIDES_EXTSURFACE:
		case SIDES_ALT:
		for (cyl = 0; cyl < maxc; cyl++) 
		{
			for (head = 0; head < maxh; head++)
			{
				err = ldbs_all_tracks_body(self, cyl, head, 
					cbk, param);
				if (err) break;
			}
		}
		break;
		case SIDES_OUTOUT:
		for (head = 0; head < maxh; head++)
		{
			for (cyl = 0; cyl < maxc; cyl++) 
			{
				err = ldbs_all_tracks_body(self, cyl, head, 
					cbk, param);
				if (err) break;
			}
		}
		break;
		case SIDES_OUTBACK:
		for (head = 0; head < maxh; head += 2)
		{
			for (cyl = 0; cyl < maxc; cyl++) 
			{
				err = ldbs_all_tracks_body(self, cyl, head, 
					cbk, param);
				if (err) break;
			}
			for (cyl = maxc; cyl > 0; cyl--) 
			{
				err = ldbs_all_tracks_body(self, cyl-1, head+1, 
					cbk, param);
				if (err) break;
			}
		}
		break;
	}	
	return err;	
}

static dsk_err_t sector_callback_wrap(PLDBS self, dsk_pcyl_t cyl,
					dsk_phead_t head, LDBS_TRACKHEAD *th,
					void *param)
{
	void **p = (void **)param;
	LDBS_SECTOR_CALLBACK cbk = p[0];
	int n;
	dsk_err_t err = DSK_ERR_OK;

	for (n = 0; n < th->count; n++)
	{
		err = (*cbk)(self, cyl, head, &th->sector[n], th, p[1]);
		if (err) break;
	}
	return err;	
}

/* Iterate over all sectors in the file, in cylinder / head order */
dsk_err_t ldbs_all_sectors(PLDBS self, LDBS_SECTOR_CALLBACK cbk, 
	dsk_sides_t sides, void *param)
{
	void *p[2];	

	p[0] = cbk;
	p[1] = param;

	return ldbs_all_tracks(self, sector_callback_wrap, sides, &p[0]);
}

static dsk_err_t track_stat_callback(PLDBS self, 
		dsk_pcyl_t cyl, dsk_phead_t head, LDBS_TRACKHEAD *th,
		void *param)
{
	LDBS_STATS *stats = param;
	unsigned n;

	if (stats->drive_empty && th->count)
	{
		stats->drive_empty = 0;
		stats->min_cylinder = stats->max_cylinder = cyl;
		stats->min_head     = stats->max_head     = head;
		stats->min_spt      = stats->max_spt      = th->count;
		stats->min_secid    = stats->max_secid = th->sector[0].id_sec;
		stats->min_sector_size = stats->max_sector_size = 
			(128 << th->sector[0].id_psh);
	}
	if (cyl < stats->min_cylinder) stats->min_cylinder = cyl;
	if (cyl > stats->max_cylinder) stats->max_cylinder = cyl;
	if (head < stats->min_head) stats->min_head = head;
	if (head > stats->max_head) stats->max_head = head;
	if (th->count < stats->min_spt) stats->min_spt = th->count;
	if (th->count > stats->max_spt) stats->max_spt = th->count;

	for (n = 0; n < th->count; n++)
	{
		LDBS_SECTOR_ENTRY *se = &th->sector[n];
		size_t secsize = (128 << se->id_psh);

		if (se->id_sec < stats->min_secid) stats->min_secid = se->id_sec;
		if (se->id_sec > stats->max_secid) stats->max_secid = se->id_sec;
		if (secsize < stats->min_sector_size) stats->min_sector_size = secsize;	
		if (secsize > stats->max_sector_size) stats->max_sector_size = secsize;	
	}


	return DSK_ERR_OK;
}


dsk_err_t ldbs_get_stats(PLDBS self, LDBS_STATS *stats)
{
	if (!self || !stats) return DSK_ERR_BADPTR;

	memset(stats, 0, sizeof(*stats));
	stats->drive_empty = 1;

	return ldbs_all_tracks(self, track_stat_callback, SIDES_ALT, stats);
}





dsk_err_t ldbs_load_track(PLDBS self, const LDBS_TRACKHEAD *trkh, void **buf, 
			size_t *buflen, size_t sector_size)
{
	size_t tracklen;
	unsigned n;
	int last_id = -1;
	unsigned sectors_done = 0;
	unsigned char *offset;
	dsk_err_t err;

	if (self == NULL || buf == NULL || buflen == NULL) 
		return DSK_ERR_BADPTR;

	/* Empty track, return empty buffer */
	if (trkh->count == 0)
	{
		*buflen = 0;
		*buf = NULL;
		return DSK_ERR_OK;
	}
	/* Calculate buffer size */
	if (sector_size)
	{
		tracklen = trkh->count * sector_size;
	}
	else
	{
		for (n = 0, tracklen = 0; n < trkh->count; n++)
		{
			tracklen += (128 << trkh->sector[n].id_psh);	
		}
	}
	*buf    = ldbs_malloc(tracklen);
	*buflen = tracklen;
	if (!*buf) return DSK_ERR_NOMEM;
	memset(*buf, trkh->filler, tracklen);

	/* Populate buffer */
	offset = (*buf);
	while (sectors_done < trkh->count)
	{
		int candidate = -1;
		size_t secsize;

		/* Find the lowest-numbered sector that hasn't been processed */
		for (n = 0; n < trkh->count; n++)
		{
			if (trkh->sector[n].id_sec <= last_id) continue;
			if (candidate < 0 || 
			    trkh->sector[candidate].id_sec > trkh->sector[n].id_sec)
				candidate = n;
		}
		if (candidate < 0) break;	/* Should never happen! */

		if (sector_size) secsize = sector_size;
		else	 secsize = 128 << trkh->sector[candidate].id_psh;
		if (trkh->sector[candidate].copies)
		{
			size_t ssiz = secsize;

			err = ldbs_getblock(self, 
				trkh->sector[candidate].blockid, 
				NULL, offset, &ssiz);
/* DSK_ERR_OVERRUN will be returned if there are multiple copies of the 
 * sector. We just use the first one in such cases. */
			if (err && err != DSK_ERR_OVERRUN) return err;
		}
		else
		{
			memset(offset, trkh->sector[candidate].filler, secsize);
		}
		offset += secsize;	
		last_id = trkh->sector[candidate].id_sec;
		++sectors_done;
	}
	return DSK_ERR_OK;	
}

