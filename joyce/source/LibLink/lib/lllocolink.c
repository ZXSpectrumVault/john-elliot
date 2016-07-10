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

/* Transmit bytes through a LocoLink link. The link can either be a genuine
 * LocoLink system, or a LapLink cable. */


static unsigned char unscramble(unsigned ctl, int master)
{
	unsigned char data = 0;

	if (master)
	{
		if ((ctl & LL_CTL_BUSY))  data |= 1;
		if (!(ctl & LL_CTL_ACK))  data |= 2; 
	}	
	else
	{
		if (ctl & LL_CTL_ERROR)   data |= 1;
		if (ctl & LL_CTL_ISELECT) data |= 2;
	}
	return data;
}


/* Read LocoLink control lines*/
ll_error_t ll_lrecv_poll(LL_PDEV self, int master, unsigned char *v)
{
	unsigned ctl;
	ll_error_t err;

	/* This assumes that the two ends of the LocoLink connection are 
	 * connected LapLink-style; so that data lines being set at one
	 * end affect control lines at the other. Because of this, the 
	 * master and slave ends aren't symmetrical (on a LapLink cable,
	 * the master uses Data 0, Data 1, Busy and Ack; the slave uses
	 * Data 3, Data 4, Select and Error.
	 *
	 * I bypass the laplink mode setting here, and act as if it's 
	 * always off. If there's a liblink at the other end, that 
	 * will do the job of setting the correct bits. */
	err = ll_irctl_poll(self, &ctl);
	if (err) return err;
	if (v) *v = unscramble(ctl, master);
	return err;
}


/* Wait for control lines to change */
ll_error_t ll_lrecv_wait(LL_PDEV self, int master, unsigned char *v)
{
	unsigned ctl;
	ll_error_t err;

	/* This assumes that the two ends of the LocoLink connection are 
	 * connected LapLink-style; so that data lines being set at one
	 * end affect control lines at the other. Because of this, the 
	 * master and slave ends aren't symmetrical (on a LapLink cable,
	 * the master uses Data 0, Data 1, Busy and Ack; the slave uses
	 * Data 3, Data 4, Select and Error.
	 *
	 * I bypass the laplink mode setting here, and act as if it's 
	 * always off. If there's a liblink at the other end, that 
	 * will do the job of setting the correct bits. */
	err = ll_irctl_wait(self, &ctl);
	if (err) return err;
	if (v) *v = unscramble(ctl, master);
	return err;
}


/* Set LocoLink control lines */
ll_error_t ll_lsend(LL_PDEV self, int master, unsigned char v)
{
	ll_error_t err = LL_E_OK;
	unsigned ctl = 0;
	unsigned char data = 0;

	/* This assumes that the two ends of the LocoLink connection are 
	 * connected LapLink-style; so that data lines being set at one
	 * end affect control lines at the other. Because of this, the 
	 * master and slave ends aren't symmetrical (on a LapLink cable,
	 * the master uses Data 0, Data 1, Busy and Ack; the slave uses
	 * Data 3, Data 4, Select and Error.
	 *
	 * I bypass the laplink mode setting here, and act as if it's 
	 * always on. */

	if (master) 
	{
/* Real LocoLink hardware only registers I/O if STROBE is active */
		ctl |= LL_CTL_STROBE;
		if (v & 1) ctl |= LL_CTL_ERROR;
		if (v & 2) ctl |= LL_CTL_ISELECT;
		err = ll_isend(self, v & 3);
		if (!err) err = ll_isctl(self, ctl);
		if (!err) err = ll_isctl(self, ctl & ~LL_CTL_STROBE);
	}
	else	/* Slave. */
	{
	/* Transmit both to an original LocoLink cable (data on BUSY and ACK)
	 * and to a LapLink cable (data on D4 and D3). Note that the sense of
	 * BUSY is inverted - when BUSY is high, D4 is low, and vice versa.
	 */
		if (v & 1) ctl |= LL_CTL_BUSY; else data |= 16; 
		if (!(v & 2)) { ctl |= LL_CTL_ACK;  data |= 8;  }
		err = ll_isend(self, data);
		if (!err) err = ll_isctl(self, ctl);
	}
	return err;
}






