
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mz.h"
#include "ne.h"

#ifndef SEEK_SET
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2
#endif

/********************** NE functions *******************************/

void new_NEFILE(NEFILE *self, FILE *fp)
{
	long t, ne_off;

	if (!self) return;
	new_MZFILE(&self->super, fp);
	self->super.open_resdir = ne_open_resdir;	

	memset(self->m_nehdr, 0, sizeof(self->m_nehdr));
	t      = ftell(fp);
	ne_off = mz_check_ne(&self->super);

	self->m_hdr_offs = 0;
	if (t >= 0 && ne_off >= 0 && fseek(fp, ne_off, SEEK_CUR) >= 0)
	{
		if (fread(self->m_nehdr, 1, 0x40, fp) == 0x40 &&
		    self->m_nehdr[0] == 'N' &&
		    self->m_nehdr[1] == 'E')
		{
			// Offset from start of file = offset from MZ + 
			//                             file pointer
			self->m_hdr_offs = ne_off + t;
		}
	} 
	if (t >= 0) fseek(fp, t, SEEK_SET);
}


int ne_seek_resdir(NEFILE *self)
{
	long offs;

	if (!self->m_hdr_offs) return -1;

	offs = PEEK16(self->m_nehdr, 0x24);

	if (fseek(self->super.m_fp, self->m_hdr_offs + offs, SEEK_SET) < 0) return -1;

	return 0;	
}

RESDIR *ne_open_resdir(MZFILE *s)
{	
	RESROOTDIR16 *rdir;
	NEFILE *self = (NEFILE *)s;
	if (!self->m_hdr_offs) return NULL;	// Not an NE file

	if (ne_seek_resdir(self)) return NULL;

	rdir = malloc(sizeof(RESROOTDIR16));
	if (!rdir) return NULL;
	new_RESROOTDIR16(rdir, self);
	if (rdir->super.m_offset >= 0) return &rdir->super;
	free(rdir);
	return NULL;
}

