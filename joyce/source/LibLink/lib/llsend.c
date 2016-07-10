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

/* Write a new value. 
 * If we are in LapLink mode, then data output sets the other end's 
 * control lines. 
 * Note that we set the data lines before the control lines. This
 * is for the benefit of ll_rctl_wait() at the other end, which reads the
 * control lines before the data lines. */

ll_error_t ll_send(LL_PDEV self, unsigned char value)
{
	unsigned ctrl = 0;
	ll_error_t err = LL_E_OK;

	if (!self) return LL_E_BADPTR;
	err = ll_isend(self, value);
	if (err) return err;

	switch(self->lld_cable)
	{
		case LL_CABLE_LAPLINK:	
/* LapLink: Echo data bits to the control lines */
			if (value & 1 ) ctrl |= LL_CTL_ERROR;
			if (value & 2 ) ctrl |= LL_CTL_ISELECT;
			if (value & 4 ) ctrl |= LL_CTL_NOPAPER;
			if (value & 8 ) ctrl |= LL_CTL_ACK;
			if (value & 16) ctrl |= LL_CTL_BUSY;
			return ll_isctl(self, value);
		case LL_CABLE_NORMAL:
			break;
	}
	return LL_E_OK;
}


/* Set control lines. */
ll_error_t ll_sctl(LL_PDEV self, unsigned value)
{
	return ll_isctl(self, value);
}



/* Send a byte to a device */
ll_error_t ll_send_byte(LL_PDEV self, unsigned char value)
{
	ll_error_t err;
        if (!self || !self->lld_clazz) return LL_E_BADPTR;

        if (self->lld_clazz->llv_send_byte)
        {
                return (*self->lld_clazz->llv_send_byte)(self, value);
        }
	/* Here is a default implementation. It sets the data, then
	 * strobes the port, with delays that should work for most printers. */

	err = ll_send(self, value); if (err) return err;
	microsleep(1);	// sleep 1uS 
	err = ll_sctl(self, LL_CTL_STROBE); if (err) return err;
	microsleep(1);	// sleep 1uS 
	err = ll_sctl(self, 0); if (err) return err;
	microsleep(1);  // sleep 1uS
	return LL_E_OK;
}


void microsleep(int ms)
{
#ifdef HAVE_WINDOWS_H
	int XXX_how_is_this_done_in_windows;
	Sleep((ms + 999) / 1000);
#else
	usleep(ms);
#endif
}

