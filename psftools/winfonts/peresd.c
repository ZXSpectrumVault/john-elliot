
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pe.h"

#ifndef SEEK_SET
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2
#endif


void new_RESDIR32(RESDIR32 *self, PEFILE *fp, byte *data, long offset)
{
	self->super.find_first = resdir32_find_first;
	self->super.find_next  = resdir32_find_next;
	self->super.get_parent = resdir32_get_parent;
	self->super.get_type   = resdir32_get_type;
	self->rsc_stamp  = PEEK32(data, 4);
	self->rsc_verh   = PEEK16(data, 8);
	self->rsc_verl   = PEEK16(data, 10);
	self->rsc_nnamed = PEEK16(data, 12);
	self->rsc_nummed = PEEK16(data, 14);

	self->super.m_offset = offset;
	self->m_pefp   = fp;
	self->m_parent = NULL;
	self->m_name   = 0;
}


RESDIRENTRY *resdir32_load_entry(RESDIR32 *self, int index)
{
	byte data[8];
	RESENTRY32 *re;
	FILE *fp = self->m_pefp->super.m_fp;

        if (fseek(fp, self->super.m_offset + 16 + 8*index, SEEK_SET)) 
		return NULL;

	if (fread(data, 1, 8, fp) < 8) return NULL;

	re = malloc(sizeof(RESENTRY32));
	if (!re) return NULL;
	new_RESENTRY32(re, self, data, index);

	if ((re->super.is_subdir)(&re->super)) return &re->super;
	if (resentry32_load(re, fp)) { free(re); return NULL; }
	return &re->super;
}


RESDIRENTRY *resdir32_find_first(RESDIR *rd)
{
	RESDIR32 *self = (RESDIR32 *)rd;
	if (self->rsc_nnamed == 0 && self->rsc_nummed == 0) return NULL;

	return resdir32_load_entry(self, 0);
}



RESDIRENTRY *resdir32_find_next(RESDIR *rd, RESDIRENTRY *re)
{
	RESDIR32 *self = (RESDIR32 *)rd;
	RESENTRY32 *e = (RESENTRY32 *)re;

	int index = e->m_index + 1;
	if (index >= (self->rsc_nnamed + self->rsc_nummed)) return NULL; 

	return resdir32_load_entry(self, index);
}

RESDIR *resdir32_get_parent(RESDIR *rd)
{
	RESDIR32 *self = (RESDIR32 *)rd;
	return &self->m_parent->super;
}

int resdir32_get_type(RESDIR *rd)
{
	RESDIR32 *self = (RESDIR32 *)rd;
	RESDIR *dir = resdir32_get_parent(rd);
	if (!dir) return -1;	// Root dir

	if (!(dir->get_parent)(dir)) return self->m_name;	// Toplevel subdir

	return (dir->get_type)(dir);			// Lower subdir
}
