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

/* Read current value, whether changed or not */
ll_error_t ll_irecv_poll(LL_PDEV self, unsigned char *value)
{
	ll_error_t err;
	if (!self || !self->lld_clazz) return LL_E_BADPTR;

	if (self->lld_clazz->llv_recv_poll)
	{
		err = (*self->lld_clazz->llv_recv_poll)(self, value);
		if (!err) self->lld_lastread = *value;
		return err;
	}
	return LL_E_NOTIMPL;
}



/* Wait for value received to change, before handling it */
ll_error_t ll_irecv_wait(LL_PDEV self, unsigned char *value)
{
	ll_error_t err;
	unsigned char m, n;
	int busy_count;

	if (!self || !self->lld_clazz) return LL_E_BADPTR;

	if (self->lld_clazz->llv_recv_wait)
	{
		return (*self->lld_clazz->llv_recv_wait)(self, value);
	}
/* There is no proper "wait". We have to busy-wait. */

	m = self->lld_lastread;
/* usleep() from time to time, to reduce CPU load. */
	busy_count = 0;
	n = m;

	do
	{
		++busy_count;
		if (busy_count == 100)
		{
#ifdef HAVE_WINDOWS_H
			sleep(1);
#else
			usleep(1);
#endif
			busy_count = 0;
		}	
		err = ll_irecv_poll(self, &n);
		if (err) return err;
	} while (m == n);

	*value = n;
	return LL_E_OK;
}

/* Read control lines */
ll_error_t ll_irctl_poll(LL_PDEV self, unsigned *value)
{
	ll_error_t err;
	if (!self || !self->lld_clazz) return LL_E_BADPTR;

	if (self->lld_clazz->llv_rctl_poll)
	{
		err = (*self->lld_clazz->llv_rctl_poll)(self, value);
		if (!err) self->lld_lastrctl = *value;
		return err;
	}
	return LL_E_NOTIMPL;
}



/* Wait for value received to change, before handling it */
ll_error_t ll_irctl_wait(LL_PDEV self, unsigned *value)
{
	ll_error_t err;
	unsigned m, n;
	int busy_count;

	if (!self || !self->lld_clazz) return LL_E_BADPTR;

	if (self->lld_clazz->llv_rctl_wait)
	{
		return (*self->lld_clazz->llv_rctl_wait)(self, value);
	}
/* There is no proper "wait". We have to busy-wait. */

	m = self->lld_lastrctl;
/* usleep() from time to time, to reduce CPU load. */
	busy_count = 0;
	n = m;

	do
	{
		++busy_count;
		if (busy_count == 100)
		{
#ifdef HAVE_WINDOWS_H
			sleep(1);
#else
			usleep(1);
#endif
			busy_count = 0;
		}	
		err = ll_irctl_poll(self, &n);
		if (err) return err;
	} while (m == n);

	*value = n;
	return LL_E_OK;
}


