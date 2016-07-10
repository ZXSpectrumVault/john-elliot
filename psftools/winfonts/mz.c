
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mz.h"
#include "ne.h"
#include "pe.h"

#ifndef SEEK_SET
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2
#endif

/*************************** MZ functions *********************************/

void new_MZFILE(MZFILE *self, FILE *fp)
{
	long tel;

	if (!self) return;
	self->m_fp = fp;
	memset(self->m_mzhead, 0, sizeof(self->m_mzhead));
	self->open_resdir = mz_open_resdir;
	tel = ftell(fp);
	if (tel != -1)
	{
		fread(self->m_mzhead, 1, sizeof(self->m_mzhead), fp);
		fseek(fp, tel, SEEK_SET);
	}
}

/** Check if this file has an NE or PE header.
 *
 * If the file has an extended header (eg: NE or PE) return the 
 * offset of the header from the MZ header. If not, return -1.
 * 
 */

long mz_check_ne(MZFILE *self)
{
	if (self->m_mzhead[0] == 'M' && self->m_mzhead[1] == 'Z')
	{
		long offset = PEEK16(self->m_mzhead, 0x18);      /* Length of MZ */
		if (offset >= 0x40)
		{
			offset = PEEK16(self->m_mzhead, 0x3C);  /* Offset to NE header */
			return offset;	/* -> start of NE header */
		}
	}
	return -1;
}

/* Check the MZ header (not moving the file pointer)
 *
 * returns  0 if type unrecognised
 *          1 for MZ
 *          2 for NE
 *          3 for PE 
 */

int mz_type(MZFILE *self)
{
	char hdr[3];
	long off, t;
	int res;
	
	hdr[0] = self->m_mzhead[0];
	hdr[1] = self->m_mzhead[1];
	hdr[2] = 0;

	if (strcmp(hdr, "MZ")) return 0;

	off = mz_check_ne(self);
	if (off < 0) return 1;

	t = ftell(self->m_fp);
	if (t >= 0)
	{
		res = 1;
		if (fseek(self->m_fp, off, SEEK_SET) >= 0 &&
		    fread(hdr, 1, 2, self->m_fp) == 2)
		{
		        if (!strcmp(hdr, "NE")) res = 2;
			if (!strcmp(hdr, "PE")) res = 3;
			if (!strcmp(hdr, "LE")) res = 4;
		}
		fseek(self->m_fp, t, SEEK_SET);
	}
	else return -1;

	return res;
}


RESDIR *mz_open_resdir(MZFILE *self)
{
	return NULL;
}


MZFILE *get_mzfile(FILE *fp)
{
	MZFILE *mztmp = malloc(sizeof(MZFILE));
	NEFILE *netmp;
	PEFILE *petmp;

	if (!mztmp) return NULL;	
	new_MZFILE(mztmp, fp);
	switch(mz_type(mztmp))
	{
		case 1: return mztmp; 
		case 2: free(mztmp);
			netmp = malloc(sizeof(NEFILE));
			if (!netmp) return NULL;
			new_NEFILE(netmp, fp);	
			return (MZFILE *)netmp;
		case 3: free(mztmp);
			petmp = malloc(sizeof(PEFILE));
			if (!petmp) return NULL;
			new_PEFILE(petmp, fp);	
			return (MZFILE *)petmp;
	}
	free(mztmp);
	return NULL;
}
