
#ifndef NE_H_INCLUDED

#define NE_H_INCLUDED 1

#include "rstypes.h"
#include "rsmacro.h"
#include "resdir.h"
#include "res16.h"

#include "mz.h"

typedef struct nefile
{
	MZFILE super;
	byte m_nehdr[0x40];	/* The NE header, as a byte array */
	long m_hdr_offs;	/* Offset of NE header from start of file */

} NEFILE; 

void new_NEFILE(NEFILE *self, FILE *fp);
RESDIR *ne_open_resdir(MZFILE *self);
int ne_seek_resdir(NEFILE *self);	/* Seek to the resource directory */

/* Resource directory within the NE file */

typedef struct resdir16
{
	RESDIR super;
	NEFILE *m_nefp;		/* Underlying file */
	long m_names_offs;	/* Offset of resource names table */
	int  m_align;		/* Alignment of resources */
} RESROOTDIR16;

void new_RESROOTDIR16(RESROOTDIR16 *self, NEFILE *fp);	/* NE file containing the resources */
RESDIRENTRY *resrootdir16_get_subdir(RESROOTDIR16 *self, long offset);
RESDIRENTRY *resrootdir16_find_first(RESDIR *self);
RESDIRENTRY *resrootdir16_find_next(RESDIR *self, RESDIRENTRY *e);
RESDIR      *resrootdir16_get_parent(RESDIR *self);
int          resrootdir16_get_type(RESDIR *self);

/* An entry that refers to a subdirectory */

typedef struct resrootentry16
{
	RESDIRENTRY super;

	TYPEINFO16 m_ti;
} RESROOTENTRY16;

void new_RESROOTENTRY16(RESROOTENTRY16 *self, RESROOTDIR16 *d, byte *tibuf, long offset);
int resrootentry16_is_subdir(RESDIRENTRY *self);
RESDIR *resrootentry16_open_subdir(RESDIRENTRY *self);
int resrootentry16_get_type(RESDIRENTRY *self);
long resrootentry16_get_id(RESDIRENTRY *self);
void resrootentry16_get_name(RESDIRENTRY *self, char *buf, int len);
long resrootentry16_get_address(RESDIRENTRY *self);
long resrootentry16_get_size(RESDIRENTRY *self);

/* A resource "subdirectory" - a list of resources of one type */

typedef struct ressubdir16
{
	RESDIR        super;
        NEFILE       *m_nefp;     /* Underlying file */
	RESROOTDIR16 *m_root;
        TYPEINFO16    m_ti;	/* Resource type & count */
	int	      m_count;	/* Count of entries in this subdir */
} RESSUBDIR16; 

void new_RESSUBDIR16(RESSUBDIR16 *self, RESROOTENTRY16 *entry);
RESDIRENTRY *ressubdir16_get_subentry(RESDIR *self, int index, long offset);
RESDIRENTRY *ressubdir16_find_first(RESDIR *self);
RESDIRENTRY *ressubdir16_find_next(RESDIR *self, RESDIRENTRY *e);
RESDIR      *ressubdir16_get_parent(RESDIR *self);
int          ressubdir16_get_type(RESDIR *self);


/* An entry in a subdirectory for a particular resource */
typedef struct ressubentry16
{
	RESDIRENTRY super;
        int        m_index;  /* Index of this entry in the subdirectory */
        NAMEINFO16 m_ni;     /* Resource name & address */
} RESSUBENTRY16;


void new_RESSUBENTRY16(RESSUBENTRY16 *self, RESDIR *dir, long offset, byte *nibuf, int index);
int  ressubentry16_is_subdir(RESDIRENTRY *self);
RESDIR *ressubentry16_open_subdir(RESDIRENTRY *self);
int  ressubentry16_get_type(RESDIRENTRY *self);
long ressubentry16_get_id(RESDIRENTRY *self);
void ressubentry16_get_name(RESDIRENTRY *self, char *buf, int len);
long ressubentry16_get_address(RESDIRENTRY *self);
long ressubentry16_get_size(RESDIRENTRY *self);


typedef struct  
{
        byte    bWidth;
        byte    bHeight;
        byte    bClrCount;
        byte    bReserved;
        wyde    wPlanes;
        wyde    wBitCount;
        dwyde   dwBytesInRes;
	wyde	wIconID;
} ICONENTRY16;

typedef struct 
{
	RESSUBENTRY16 re;
	long	   idir_offs;
	int 	   rsc_align;
	wyde	   idReserved;
	wyde	   idType;
	wyde	   idCount;
	int	   index;
	ICONENTRY16 ie;
} ICONDIR16;
  
#endif /* def NE_H_INCLUDED */








