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

/* Read data lines. */
ll_error_t ll_recv_poll(LL_PDEV self, unsigned char *value)
{
	return ll_irecv_poll(self,value);
}

ll_error_t ll_recv_wait(LL_PDEV self, unsigned char *value)
{
	return ll_irecv_wait(self,value);
}

static void ll_merge(unsigned *control, unsigned char data)
{
	if (!control) return;
	
	if (data &  1) *control |= LL_CTL_ERROR;
	if (data &  2) *control |= LL_CTL_ISELECT;
	if (data &  4) *control |= LL_CTL_NOPAPER;
	if (data &  8) *control |= LL_CTL_ACK;
	if (data & 16) *control |= LL_CTL_BUSY;
}


/* Read control lines. In LapLink mode, OR the control lines with the 
 * data lines to which they are connected. Not all drivers implement
 * reading the data lines, so be prepared for the ll_irecv() to fail. */
ll_error_t ll_rctl_poll(LL_PDEV self, unsigned *value)
{
	unsigned char data = 0;
	ll_error_t err;

	err = ll_irctl_poll(self,value);
	if (err) return err;
	switch(self->lld_cable)
	{
		case LL_CABLE_LAPLINK:
		err = ll_irecv_poll(self, &data);

		if (err != LL_E_OK && err != LL_E_NOTIMPL) return err;	
		if (err == LL_E_OK) ll_merge(value, data);
		break;
	
		case LL_CABLE_NORMAL:
		break;
	}
	return LL_E_OK;
}


/* Read control lines. In LapLink mode, OR the control lines with the 
 * data lines to which they are connected. Not all drivers implement
 * reading the data lines, so be prepared for the ll_irecv() to fail. */
ll_error_t ll_rctl_wait(LL_PDEV self, unsigned *value)
{
	unsigned char data = 0;
	ll_error_t err;

	err = ll_irctl_wait(self,value);
	if (err) return err;
	switch(self->lld_cable)
	{
		case LL_CABLE_LAPLINK:
		err = ll_irecv_poll(self, &data);

		if (err != LL_E_OK && err != LL_E_NOTIMPL) return err;	
		if (err == LL_E_OK) ll_merge(value, data);
		break;
	
		case LL_CABLE_NORMAL:
		break;
	}
	return LL_E_OK;
}





