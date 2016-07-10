
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

/************** NE resource sub-directory functions **************************/

void new_RESSUBDIR16(RESSUBDIR16 *self, RESROOTENTRY16 *entry)
{
	self->super.find_first = ressubdir16_find_first;
	self->super.find_next  = ressubdir16_find_next;
	self->super.get_parent = ressubdir16_get_parent;
	self->super.get_type   = ressubdir16_get_type;

	memcpy(&(self->m_ti), &(entry->m_ti), sizeof(self->m_ti));
	self->m_root   = (RESROOTDIR16 *)(entry->super.m_dir);
	self->m_nefp   = self->m_root->m_nefp;
	self->super.m_offset = entry->super.m_offset;
	self->m_count  = self->m_ti.rtResourceCount;
}


RESDIRENTRY *ressubdir16_get_subentry(RESDIR *s, int index, long offset)
{
	RESSUBDIR16 *self = (RESSUBDIR16 *)s;
        FILE *fp = self->m_nefp->super.m_fp;
        byte nibuf[12];
	RESSUBENTRY16 *re;

        if (fseek(fp, offset, SEEK_SET) < 0) return NULL;

        if (fread(nibuf, 1, 12, fp) < 12) return NULL;

	re = malloc(sizeof(RESSUBENTRY16));
	if (!re) return NULL;
	new_RESSUBENTRY16(re, &self->super, offset, nibuf, index);
	return (RESDIRENTRY *)re;
}

RESDIRENTRY *ressubdir16_find_first(RESDIR *s)
{
	return ressubdir16_get_subentry(s, 0, s->m_offset + 8);
}	

RESDIRENTRY *ressubdir16_find_next(RESDIR *s, RESDIRENTRY *e)
{
	int index;
	long offset;
	RESSUBENTRY16 *entry = (RESSUBENTRY16 *)e;
	RESSUBDIR16   *self  = (RESSUBDIR16 *)s;

	offset = 12 + e->m_offset;
	index = 1 + entry->m_index;

	if (index >= self->m_count) return NULL;

	return ressubdir16_get_subentry(s, index, offset);
}

RESDIR *ressubdir16_get_parent(RESDIR *self)
{
	return &((RESSUBDIR16 *)self)->m_root->super;
}

int ressubdir16_get_type(RESDIR *s)
{
	return (((RESSUBDIR16 *)s)->m_ti.rtTypeID) ^ 0x8000; 
}


