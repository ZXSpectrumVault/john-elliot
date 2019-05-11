
#ifndef PE_H_INCLUDED

#define PE_H_INCLUDED 1

#include "rstypes.h"
#include "rsmacro.h"
#include "resdir.h"

#include "mz.h"
/* Structures for accessing a PE-format file's resources 
 */

typedef struct pefile
{
	MZFILE super;
	byte m_magic[4];	/* "PE\0\0" - magic no. */
	byte m_ihdr[20];	/* The PE IMAGE_FILE_HEADER */
	byte m_ohdr[240];	/* The PE IMAGE_OPTIONAL_HEADER */
	long m_hdr_offs;	/* Offset of PE header from start of file */
	long m_hdr_end;		/* End of PE header */	
} PEFILE;

int new_PEFILE(PEFILE *self, FILE *fp);
RESDIR *pe_open_resdir(MZFILE *self);
RESDIR *pe_load_resdir(PEFILE *self, long offs);

/* Resource directory within the PE file */

typedef struct resdir32
{
	RESDIR super;
	PEFILE *m_pefp;		/* Underlying file */
	long rva_base;		/* Difference between RVA & raw address */
	long rsc_org;		/* Offset of resource root from start *
				 * of file */
	long rsc_stamp;		/* Time stamp */
	int  rsc_verh;		/* Version high */
	int  rsc_verl;		/* Version low */
	int  rsc_nnamed; 	/* No. of named entries */
	int  rsc_nummed;	/* No. of numbered entries */
	struct resdir32 *m_parent;
	long m_name;
} RESDIR32;

void new_RESDIR32(RESDIR32 *self, PEFILE *pfp, byte *data, long offset);
RESDIRENTRY *resdir32_load_entry(RESDIR32 *self, int index);
RESDIRENTRY *resdir32_find_first(RESDIR *self);
RESDIRENTRY *resdir32_find_next(RESDIR *self, RESDIRENTRY *e);
RESDIR      *resdir32_get_parent(RESDIR *self);
int 	    resdir32_get_type(RESDIR *self);	

typedef struct resptr32
{
	dwyde offset;		/* Offset to real resource data */
	dwyde size;		/* Size of real resource data */
	dwyde codepage;		/* Resource codepage */
	dwyde reserved;		/* Reserved */
} RESPTR32;

typedef struct resentry32
{
	RESDIRENTRY super;
	int	m_index;	/* Index of this entry in the directory */
	long	m_name;		/* Name */
	RESPTR32 m_rptr;
} RESENTRY32;

void    new_RESENTRY32(RESENTRY32 *self, RESDIR32 *dir, byte *data, int index); 
int     resentry32_is_subdir(RESDIRENTRY *self);
RESDIR *resentry32_open_subdir(RESDIRENTRY *self);
int     resentry32_get_type(RESDIRENTRY *self);
long    resentry32_get_id(RESDIRENTRY *self);
void    resentry32_get_name(RESDIRENTRY *self, char *buf, int len);
int     resentry32_load(RESENTRY32 *self, FILE *fp);
long    resentry32_get_address(RESDIRENTRY *self);
long    resentry32_get_size(RESDIRENTRY *self);

typedef struct
{
        byte    bWidth;
        byte    bHeight;
        byte    bClrCount;
        byte    bReserved;
        wyde    wPlanes;
        wyde    wBitCount;
        dwyde   dwBytesInRes;
        wyde    wIconID;
} ICONENTRY32;


typedef struct
{
	RESDIR32   *rd;
        RESENTRY32 *re;
	RESPTR32   rp;
        wyde       idReserved;
        wyde       idType;
        wyde       idCount;
        int        index;
        ICONENTRY32 ie;
} ICONDIR32;




int rsc32_icondir_findfirst(RESDIR32 *rdir, ICONDIR32 *id, int cursor);
int rsc32_icondir_findnext(ICONDIR32 *id);
int rsc32_icon_findfirst(ICONDIR32 *id);
int rsc32_icon_findnext(ICONDIR32 *id);
int rsc32_icon_load(RESDIR32 *rdir, ICONDIR32 *id, byte **pData, int cursor);

#endif /* def PE_H_INCLUDED */








