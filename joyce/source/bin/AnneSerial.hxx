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

class AnneSerial : public PcwDevice
{
public:
	AnneSerial(byte basePort, AnneSystem *s);
	virtual ~AnneSerial();

	virtual void reset(void);
	byte in(byte port);
	void out(byte port, byte value);
	void tick(void);

        virtual bool parseNode(xmlDocPtr doc, xmlNsPtr ns, xmlNodePtr cur);
        virtual bool storeNode(xmlDocPtr doc, xmlNsPtr ns, xmlNodePtr cur);

	virtual string getTitle(void);
	virtual bool hasSettings(SDLKey *key, string &caption);
	UiEvent settings(UiDrawer *d);
// Power switch handling. In a PCW16, the power switch is hooked up to the
// Ring Indicator of the first serial port.
	UiEvent power(UiDrawer *d);
	void setPower(bool b);
	bool getPower();
protected:
	PcwComms *m_comms;
	AnneSystem *m_sys;
	int m_basePort;
	//int m_baudmode, m_txbaud, m_rxbaud, m_baudbuf;
	int m_z80tick;

// 16550 registers
	int  m_dlatch, m_baud;
	byte m_dlab, m_dll, m_dlh;
	byte m_rbr, m_ier, m_iir, m_lcr, m_mcr, m_lsr, m_msr, m_scr,
	     m_thr, m_fcr;

};




