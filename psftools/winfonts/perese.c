
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pe.h"

#ifndef SEEK_SET
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2
#endif


void new_RESENTRY32(RESENTRY32 *self, RESDIR32 *dir, byte *data, int index)
{
	self->super.is_subdir   = resentry32_is_subdir;
	self->super.open_subdir = resentry32_open_subdir;
	self->super.get_type    = resentry32_get_type;
	self->super.get_id      = resentry32_get_id;
	self->super.get_name    = resentry32_get_name;
	self->super.get_address = resentry32_get_address;
	self->super.get_size    = resentry32_get_size;

	self->super.m_dir       = &dir->super;
	self->m_name   = PEEK32(data, 0);
	self->super.m_offset = PEEK32(data, 4);
	self->m_index  = index;
	memset(&self->m_rptr, 0, sizeof(self->m_rptr)); 
}



int resentry32_is_subdir(RESDIRENTRY *s)
{
	if (s->m_offset & 0x80000000L) return 1;
	return 0;
}



RESDIR *resentry32_open_subdir(RESDIRENTRY *s)
{
	RESENTRY32 *self = (RESENTRY32 *)s;
	RESDIR32 *parent = (RESDIR32 *)(s->m_dir);
	RESDIR32 *child;
	PEFILE   *pe = parent->m_pefp;

	if (!(s->m_offset & 0x80000000L)) return NULL;

	child = (RESDIR32 *)pe_load_resdir(pe, (s->m_offset & 0x7FFFFFFFL) + parent->rsc_org);
	if (!child) return NULL;

	child->rsc_org  = parent->rsc_org;
	child->rva_base = parent->rva_base; 
	child->m_name   = self->m_name;
	child->m_parent = parent;

        return (RESDIR *)child;
}

int resentry32_load(RESENTRY32 *self, FILE *fp)
{
        byte data[16];
        RESDIR32 *rdir = (RESDIR32 *)(self->super.m_dir);

        if (fseek(fp, rdir->rsc_org + (self->super.m_offset & 0x7FFFFFFFL), SEEK_SET))
                return -1;
        if (fread(data, 1, 16, fp) < 16) return -1;
        self->m_rptr.offset   = PEEK32(data, 0);
        self->m_rptr.size     = PEEK32(data, 4);
        self->m_rptr.codepage = PEEK32(data, 8);
        self->m_rptr.reserved = PEEK32(data, 12);

	return 0;
}


int resentry32_get_type(RESDIRENTRY *s)
{
	RESDIR *dir = s->m_dir;
	int type;	

	while ( (type = (*dir->get_type)(dir)) == -2) 
	{
		dir = (*dir->get_parent)(dir);
		if (!dir) return -2;
	}	
	return type;
}


long resentry32_get_id(RESDIRENTRY *e)
{
	RESENTRY32 *self = (RESENTRY32 *)e;
	return self->m_name;	
}


long resentry32_get_address(RESDIRENTRY *e)
{
	RESENTRY32 *self = (RESENTRY32 *)e;
	return self->m_rptr.offset - ((RESDIR32 *)(e->m_dir))->rva_base;		
}

long resentry32_get_size(RESDIRENTRY *e)
{
	RESENTRY32 *self = (RESENTRY32 *)e;
	return self->m_rptr.size;
} 

void resentry32_get_name(RESDIRENTRY *e, char *buf, int len)
{
	long t, offs;
	int  rlen=0, rlenh=0;
	RESDIR32 *dir;
	RESENTRY32 *self = (RESENTRY32 *)e;
	FILE *fp;
	if (!(self->m_name & 0x80000000L))
	{
		if (len < 11) { buf[0] = 0; return; }
		sprintf(buf, "%ld", self->m_name);
		return;
	}
	offs = self->m_name & 0x7FFFFFFFL;	

        dir = (RESDIR32 *)(e->m_dir);
	offs += dir->rsc_org;

        fp = dir->m_pefp->super.m_fp;

        t = ftell(fp);
        if (t >= 0 && (!fseek(fp, offs, SEEK_SET)) && 
	    (rlen  = fgetc(fp)) != EOF &&
            (rlenh = fgetc(fp)) != EOF)
        {
		int rpos = 0, b;
		wchar_t *wstr, w;	

		rlen |= (rlenh << 8);	// No. of words in file
		wstr = malloc(sizeof(wchar_t) * (rlen + 1));
		if (wstr)
		{
			for (rpos = 0; rpos < rlen; rpos++)
			{
				b = fgetc(fp);
				if (b == EOF) break;
				w = b;
				b = fgetc(fp);
				if (b == EOF) break;
				w |= (b << 8);
				wstr[rpos] = w;
			}
			wstr[rpos] = 0;
#ifdef __PACIFIC__
/* Pacific doesn't have wcstombs so fake it */
			for (b = 0; b < rlen; b++) 
			{
				if (wstr[b] >= 0x20 && wstr[b] <= 0x7E)
					buf[b] = wstr[b];
				else	buf[b] = '?';
				if (b >= (len - 1)) break;
			}
			buf[b] = 0;
#else
			wcstombs(buf, wstr, len - 1); 
#endif
			buf[len - 1] = 0;
			free(wstr);
		} 
       	       	else buf[0] = 0; 
        }
        else buf[0] = 0;
        if (t >= 0) fseek(fp, t, SEEK_SET);
	
}
