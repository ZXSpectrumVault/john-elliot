
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pe.h"


#ifndef SEEK_SET
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2
#endif



int new_PEFILE(PEFILE *self, FILE *fp)
{
        int r = 0;
	size_t optsz;
        long rec = ftell(fp);
	long m_off;

	if (!self) return -1;
	new_MZFILE(&self->super, fp);
	self->super.open_resdir = pe_open_resdir;
	m_off = mz_check_ne(&self->super);

	self->m_hdr_offs = 0;
        if (rec >= 0 && m_off >= 0 && fseek(fp, m_off, SEEK_CUR) >= 0)
	{
        	/* Load PE header */
        	if (fread(self->m_magic, 1, 4, fp) == 4 &&
                          self->m_magic[0] == 'P' &&
                          self->m_magic[1] == 'E')
                {
                        r = 0;
                        self->m_hdr_offs = ftell(fp) - 4;
			if (fread(self->m_ihdr, 1, 20, fp) < 20) r = -1;
			optsz = PEEK16(self->m_ihdr, 16); 
			if (optsz > 240) optsz = 240;
			if (fread(self->m_ohdr, 1, optsz, fp) < optsz) r = -1;
			self->m_hdr_end = ftell(fp);
                }
        }
        if (rec >= 0) fseek(fp, rec, SEEK_SET);
	return r;
}


RESDIR *pe_load_resdir(PEFILE *self, long offs)
{
	byte data[16];
	RESDIR32 *rdir;

	if (fseek(self->super.m_fp, offs, SEEK_SET)) return NULL;
	if (fread(data, 1, 16, self->super.m_fp) < 16) return NULL;

	rdir = malloc(sizeof(RESDIR32));
	if (!rdir) return NULL;
	new_RESDIR32(rdir, self, data, offs);
	return (RESDIR *)rdir;
}


RESDIR *pe_open_resdir(MZFILE *s)
{
	int count;
	byte sechdr[40];
	long pos = -1; 
	RESDIR32 *rdir;
	PEFILE *self = (PEFILE *)s;

	if (fseek(s->m_fp, self->m_hdr_end, SEEK_SET)) return NULL;

	count = PEEK16(self->m_ihdr, 2);
				/* No. of sections in section dir */

	while (count)
	{
		if (fread(sechdr, 1, 40, s->m_fp) < 40) return NULL; 
		
		if (!strcmp((char *)sechdr, ".rsrc"))
		{
			pos = PEEK32(sechdr, 20);
			if (fseek(s->m_fp, pos, SEEK_SET)) return NULL;

			rdir = (RESDIR32 *)pe_load_resdir(self, pos);
			
			rdir->rsc_org  = pos;
			rdir->rva_base = PEEK32(sechdr, 12) - pos;
			rdir->m_parent = NULL;
			return (RESDIR *)rdir;
		}
		--count;
	}
	return NULL;
}




