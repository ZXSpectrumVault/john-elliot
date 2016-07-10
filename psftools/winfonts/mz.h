 
#ifndef MZ_H_INCLUDED
#define MZ_H_INCLUDED 1

#include "rstypes.h"
#include "rsmacro.h"

/** Skip over an MZ (DOS) header 
 *
 * If the file has an extended header (eg: NE or PE) return with the file
 * pointer at the extended header. Else return with the file pointer 
 * where it was.
 * 
 * Returns: -1 if an fseek() or ftell() failed.
 *           0 if extended header found.
 *           1 if no extended header found.
 */

/* Check the next 2 bytes of the file (not moving the file pointer)
 *
 * returns -1 if seek error
 *          0 if type unrecognised
 *          1 for MZ
 *          2 for NE
 *          3 for PE 
 */

#include "resdir.h"

typedef struct mzfile
{
	FILE *m_fp;
	byte m_mzhead[0x40];
	RESDIR *(*open_resdir)(struct mzfile *self);
} MZFILE;


void   new_MZFILE(MZFILE *self, FILE *fp);
int    mz_type(MZFILE *self);
long   mz_check_ne(MZFILE *self);	
RESDIR *mz_open_resdir(MZFILE *self);

MZFILE *get_mzfile(FILE *fp);



#endif /* ndef MZ_H_INCLUDED */

