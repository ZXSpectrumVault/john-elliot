/* liblink: Simulate parallel / serial hardware for emulators

    Copyright (C) 2002  John Elliott <seasip.webmaster@gmail.com>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public
    License along with this library; if not, write to the Free
    Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include "ll.h"
#include "lldrivers.h"

#define SHAREDMEMSIZE 4096

static LL_VTBL *drivers[] =
{
#include "lldrivers.inc"
	NULL
};

/* Allocate driver data */
static ll_error_t ll_alloc(LL_VTBL *clazz, LL_PDEV *self, const char *filename)
{
	LL_PDEV self2;

	if (!clazz || !self || !filename) return LL_E_BADPTR;

	*self = NULL;
	self2 = malloc(clazz->llv_datasize);
	if (!self2) return LL_E_NOMEM;	
	memset(self2, 0, clazz->llv_datasize);

	self2->lld_filename = malloc(1 + strlen(filename));
	if (!self2->lld_filename)
	{
		free(self2);
		return LL_E_NOMEM;
	}
	strcpy(self2->lld_filename, filename);
	self2->lld_clazz  = clazz;
	*self = self2;
	return LL_E_OK;
}


/* Free driver data */
static ll_error_t ll_free(LL_PDEV *self)
{
	if (self == NULL || *self == NULL) return LL_E_BADPTR;

	if ((*self)->lld_filename) free( (*self)->lld_filename);

	free(*self);	
	*self = NULL;
	return LL_E_OK;
}




/* Given a string, find a vtable with that driver name */
static ll_error_t ll_clazz(LL_VTBL **clazz, const char *drvname)
{
	LL_VTBL *v;
	int n;

	if (!drvname) 	/* Default: use first entry */
	{
		*clazz = drivers[0];
		return LL_E_OK;
	}	
	for (n = 0; drivers[n]; n++)
	{
		v = drivers[n];
		if (!strcmp(drvname, v->llv_drvname)) 
		{
			*clazz = v;
			return LL_E_OK;
		}
	}
	return LL_E_NODRIVER;
}


/* Open a connection */
ll_error_t	ll_open(LL_PDEV *pself, const char *filename, const char *drvname)
{
	ll_error_t err;
	LL_PDEV self;
	LL_VTBL *clazz;

	if (!pself) return LL_E_BADPTR;
	*pself = NULL;

/* Create an object */
	err = ll_clazz(&clazz, drvname);
	if (!err) err = ll_alloc(clazz, &self, filename);
	if (err) return err;

	if (clazz->llv_open) err = (*clazz->llv_open)(self);
	if (err)
	{
		ll_free(&self);
		return err;
	}	
	*pself = self;
	return LL_E_OK;
}



int	ll_close(LL_PDEV *pself)
{
	LL_PDEV self;
	LL_VTBL  *clazz;
	ll_error_t err;

	if (!pself) return LL_E_BADPTR;
	self = *pself;			
	if (!self) return LL_E_BADPTR;
	clazz = self->lld_clazz;
	if (!clazz) return LL_E_BADPTR;	

	if (clazz->llv_close)
	{
		err = (*clazz->llv_close)(self);
		if (err != LL_E_OK) return err;		
	}
	ll_free(pself);
	return LL_E_OK;
}


