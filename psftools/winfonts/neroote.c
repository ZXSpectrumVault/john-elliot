
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

/********* Dummy entries in NE resource root dir *************************/

void new_RESROOTENTRY16(RESROOTENTRY16 *self, RESROOTDIR16 *d, byte *tibuf, long offset)
{
	self->m_ti.rtTypeID 	     = PEEK16(tibuf, 0);
	self->m_ti.rtResourceCount = PEEK16(tibuf, 2);
	self->m_ti.rtReserved      = PEEK32(tibuf, 4);
	self->super.m_dir = &d->super;
	self->super.m_offset = offset;
	self->super.is_subdir   = resrootentry16_is_subdir;
	self->super.open_subdir = resrootentry16_open_subdir;
	self->super.get_name    = resrootentry16_get_name;
	self->super.get_type    = resrootentry16_get_type;
	self->super.get_id      = resrootentry16_get_id;
	self->super.get_address = resrootentry16_get_address;
	self->super.get_size    = resrootentry16_get_size;
}

int resrootentry16_get_type(RESDIRENTRY *e)
{
        return -1;      // Directory
}

long resrootentry16_get_id(RESDIRENTRY *e)
{
	RESROOTENTRY16 *self = (RESROOTENTRY16 *)e;
	return (self->m_ti.rtTypeID ^ 0x8000);
}

void resrootentry16_get_name(RESDIRENTRY *e, char *buf, int len)
{
        int rlen = 0;
        long rnid, t, offs;
	RESROOTENTRY16 *self = (RESROOTENTRY16 *)e;
        RESROOTDIR16 *dir = (RESROOTDIR16 *)(e->m_dir);
	FILE *fp;

 	rnid = self->m_ti.rtTypeID;
        if (rnid & 0x8000)
        {
                if (len < 6) { buf[0] = 0; return; }
                sprintf(buf, "%ld", rnid & 0x7FFF);
                return;
        }
	offs = dir->super.m_offset + rnid;
        fp = dir->m_nefp->super.m_fp;

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





RESDIR *resrootentry16_open_subdir(RESDIRENTRY *e)
{
	RESROOTENTRY16 *self = (RESROOTENTRY16 *)e;
	RESSUBDIR16 *rs = malloc(sizeof(RESSUBDIR16));

	if (!rs) return NULL;
	new_RESSUBDIR16(rs, self);
	return (RESDIR *)rs;
}

int resrootentry16_is_subdir(RESDIRENTRY *e)
{
	return 1;
}


long resrootentry16_get_address(RESDIRENTRY *e)
{
	return e->m_offset;
}


long resrootentry16_get_size(RESDIRENTRY *e)
{
	RESROOTENTRY16 *self = (RESROOTENTRY16 *)e;
	return (self->m_ti.rtResourceCount * 12);
}

