#ifndef RESDIR_H_INCLUDED
#define RESDIR_H_INCLUDED

#include "rstypes.h"
#include "rsmacro.h"

/* Resource directory interface. RESDIR covers:
 * 
 * Win16 resource root dir
 * Win16 resource subdir
 * Win32 resource dir
 */

typedef struct resdir
{
	long	m_offset;
	struct resdirentry * (*find_first)(struct resdir *self);
	struct resdir * (*get_parent)(struct resdir *self);
	struct resdirentry * (*find_next)(struct resdir *self, struct resdirentry *e);
	int (*get_type)(struct resdir *self);
} RESDIR;

void new_RESDIR(RESDIR *self);
char *resdir_get_typestring(RESDIR *self);
struct resdirentry *resdir_find_name(RESDIR *self, const char *name);
struct resdirentry *resdir_find_id  (RESDIR *self, const int id);

typedef struct resdirentry
{
	long m_offset;
	RESDIR *m_dir;

	int (*is_subdir)(struct resdirentry *self);
	RESDIR * (*open_subdir)(struct resdirentry *self);

	int  (*get_type)(struct resdirentry *self);
	long (*get_id)(struct resdirentry *self);
	void (*get_name)(struct resdirentry *self, char *buf, int len);
	long (*get_address)(struct resdirentry *self);
	long (*get_size)(struct resdirentry *self);
} RESDIRENTRY;

char *resdirentry_get_typestring(RESDIRENTRY *self);

#endif // ndef RESDIR_H_INCLUDED 
