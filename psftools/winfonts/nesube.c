
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

/*********************** NE resource-entry *******************************/

void new_RESSUBENTRY16(RESSUBENTRY16 *self, RESDIR *dir, long offset, byte *nibuf, int index)
{
	self->m_ni.rnOffset = PEEK16(nibuf, 0);
	self->m_ni.rnLength = PEEK16(nibuf, 2);
	self->m_ni.rnFlags  = PEEK16(nibuf, 4);
	self->m_ni.rnID     = PEEK16(nibuf, 6);
	self->m_ni.rnHandle = PEEK16(nibuf, 8);
	self->m_ni.rnUsage  = PEEK16(nibuf, 10);	
	self->super.m_offset = offset;
	self->m_index        = index;
	self->super.m_dir    = dir;

	self->super.is_subdir   = ressubentry16_is_subdir;
	self->super.open_subdir = ressubentry16_open_subdir;
	self->super.get_type    = ressubentry16_get_type;
	self->super.get_id      = ressubentry16_get_id;
	self->super.get_name    = ressubentry16_get_name;
	self->super.get_address = ressubentry16_get_address;
	self->super.get_size    = ressubentry16_get_size;
}



int ressubentry16_is_subdir(RESDIRENTRY *self)
{
	return 0;
}


RESDIR *ressubentry16_open_subdir(RESDIRENTRY *self)
{
	return NULL;
}

long ressubentry16_get_id(RESDIRENTRY *s)
{
	RESSUBENTRY16 *self = (RESSUBENTRY16 *)s;
	return (self->m_ni.rnID ^ 0x8000);
}

void ressubentry16_get_name(RESDIRENTRY *s, char *buf, int len)
{
	int rlen = 0;
	long t, rnid, offs;
	RESSUBENTRY16 *self = (RESSUBENTRY16 *)s;
	RESDIR *dir;
	FILE *fp;

 	rnid = self->m_ni.rnID;
	if (rnid & 0x8000) 
	{
		if (len < 6) { buf[0] = 0; return; }
		sprintf(buf, "%ld", rnid & 0x7FFF);
		return;
	}
	// Work back up to the root
	dir = s->m_dir;
	while ((dir->get_parent)(dir)) dir = (dir->get_parent)(dir);
	offs = dir->m_offset + rnid;

	fp = ((RESROOTDIR16 *)dir)->m_nefp->super.m_fp;

	t = ftell(fp);
	if (t >= 0 && (!fseek(fp, offs, SEEK_SET)) && (rlen = fgetc(fp)) != EOF)
	{
		if ((rlen + 1) > len) rlen = len - 1;
		rlen = fread(buf, 1, rlen, fp);
		buf[rlen] = 0;
	}
	else buf[0] = 0;
	if (t >= 0) fseek(fp, t, SEEK_SET);
}

long ressubentry16_get_address(RESDIRENTRY *s)
{
        RESDIR *dir = s->m_dir;
	RESSUBENTRY16 *self = (RESSUBENTRY16 *)s;	
       
	while (dir->get_parent(dir)) dir = dir->get_parent(dir);

	return self->m_ni.rnOffset << ((RESROOTDIR16 *)dir)->m_align;
}

long ressubentry16_get_size(RESDIRENTRY *s)
{
        RESDIR *dir = s->m_dir;
	RESSUBENTRY16 *self = (RESSUBENTRY16 *)s;	
        
	while (dir->get_parent(dir)) dir = dir->get_parent(dir);

        return self->m_ni.rnLength << ((RESROOTDIR16 *)dir)->m_align;
}


int ressubentry16_get_type(RESDIRENTRY *self)
{
	return (self->m_dir->get_type)(self->m_dir);
}



