
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

/********************** NE resource directory *******************************/

void new_RESROOTDIR16(RESROOTDIR16 *self, NEFILE *ne)
{
	FILE *fp;
	long direpos;
	byte dirent[8];

        self->super.find_first = resrootdir16_find_first;
        self->super.find_next  = resrootdir16_find_next;
        self->super.get_parent = resrootdir16_get_parent;
        self->super.get_type   = resrootdir16_get_type;

	self->m_nefp = ne;
	fp           = ne->super.m_fp;
	
	self->super.m_offset = ftell(fp);

	if (self->super.m_offset >= 0)
	{
		self->m_align =   (fgetc(fp) & 0xFF);
		self->m_align |=  ((int)fgetc(fp)) << 8;

		direpos = ftell(fp);
		do
		{	
			if (fread(dirent, 1, 8, fp) < 8) 
			{ 
				self->super.m_offset = -1; 
				break;
			}	
			if (PEEK16(dirent, 0)) 
			{
				direpos += 8;
				direpos += 12 * PEEK16(dirent, 2);
			}
			else
			{
				self->m_names_offs = direpos + 2;
			}	
		} while (PEEK16(dirent, 0));
	}
}

RESDIR *resrootdir16_get_parent(RESDIR *self)
{
	return NULL;	
}

int resrootdir16_get_type(RESDIR *self)
{
	return -1;	// Directory
}


RESDIRENTRY *resrootdir16_get_subdir(RESROOTDIR16 *self, long offset)
{
	FILE *fp = self->m_nefp->super.m_fp;
	byte tibuf[8];
	RESROOTENTRY16 *e;

	if (fseek(fp, offset, SEEK_SET) < 0) return NULL;

	if (fread(tibuf, 1, 8, fp) < 8) return NULL;

	e = malloc(sizeof(RESROOTENTRY16));
	if (!e) return NULL;
	new_RESROOTENTRY16(e, self, tibuf, offset);
	return (RESDIRENTRY *)e;
}

RESDIRENTRY *resrootdir16_find_first(RESDIR *self)
{
	return resrootdir16_get_subdir((RESROOTDIR16 *)self, self->m_offset + 2);
}

RESDIRENTRY *resrootdir16_find_next(RESDIR *s, RESDIRENTRY *entry)
{
	long offset;
	RESROOTENTRY16 *e, *e2;
	RESROOTDIR16 *self;

	self = (RESROOTDIR16 *)s;
 	e = (RESROOTENTRY16 *)entry;
	offset = e->super.m_offset;
	offset += 8 + (e->m_ti.rtResourceCount * 12);
	e2 = (RESROOTENTRY16 *)resrootdir16_get_subdir(self, offset);

	if (!e2) return NULL;
	if (e2->m_ti.rtTypeID == 0)	// End of resource directory
	{
		free(e2);
		return NULL;
	}
	return (RESDIRENTRY *)e2;	
}



