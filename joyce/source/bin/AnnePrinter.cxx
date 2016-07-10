/************************************************************************

    JOYCE v2.1.0 - Amstrad PCW emulator

    Copyright (C) 1996, 2001-2  John Elliott <seasip.webmaster@gmail.com>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

*************************************************************************/

#include "Anne.hxx"

AnnePrinter::AnnePrinter() : PcwPrinter("parallel")
{
}


AnnePrinter::~AnnePrinter()
{

}

byte	AnnePrinter::in(byte port)
{
        unsigned busyline;
        ll_error_t e;
        byte r = 0;

        if (!isEnabled()) return 0xC0;

        if (!m_lldev) open();
        if (!m_lldev) return 0xC0;

	switch(port)
	{
		// For unidirectional port: Last value written
		case 0x38:	return m_latch;
		case 0x39:	// status
			e = ll_rctl_poll(m_lldev, &busyline);
 			showError(e);
			if (e != LL_E_OK) return 0xC0;
			if (busyline & LL_CTL_BUSY)     r |= 0x80;
			if (!(busyline & LL_CTL_ACK))   r |= 0x40;
			if (busyline & LL_CTL_NOPAPER)  r |= 0x20;
			if (busyline & LL_CTL_ISELECT)  r |= 0x10;
			if (!(busyline & LL_CTL_ERROR)) r |= 0x08;
			return r;
		case 0x3A:	return m_ctrl;
	}
	return 0xFF;
}


void	AnnePrinter::out(byte port, byte value)
{
	switch(port)
	{
		case 0x38:	m_latch = value;
				break;
		case 0x3A:	m_ctrl = value;
				writeCtrl();
				break;
	}
}



bool AnnePrinter::hasSettings(SDLKey *key, string &caption)
{
	*key = SDLK_p;
	caption = "  Printer  ";
	return true;
}



void AnnePrinter::writeCtrl()
{
        unsigned ctrls = 0;
        ll_error_t e;

        if (!isEnabled()) return;

        if (!m_lldev) open();
        if (!m_lldev) return;

	if (m_ctrl & 1)    ctrls |= LL_CTL_STROBE;
	if (m_ctrl & 2)    ctrls |= LL_CTL_AUFEED;
	if (!(m_ctrl & 4)) ctrls |= LL_CTL_INIT;
	if (m_ctrl & 8)    ctrls |= LL_CTL_OSELECT;
// XXX Handle: Bidirectional (if can be bothered)
//             Enable IRQ
//             Bit 7 (if used)
	e = ll_sctl(m_lldev, ctrls);
 	showError(e);
}



