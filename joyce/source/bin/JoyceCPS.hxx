/************************************************************************

    JOYCE v1.90 - Amstrad PCW emulator

    Copyright (C) 1996, 2001  John Elliott <seasip.webmaster@gmail.com>

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

#include "Z80Dart.hxx"

class JoyceCPS : public PcwPrinter
{
public:
	JoyceCPS(JoyceSystem *s);
	virtual ~JoyceCPS();

	virtual void reset(void);
	byte in(byte port);
	void out(byte port, byte value);
	void dropStrobe(void);
	void tick(void);

        virtual bool parseNode(xmlDocPtr doc, xmlNsPtr ns, xmlNodePtr cur);
        virtual bool storeNode(xmlDocPtr doc, xmlNsPtr ns, xmlNodePtr cur);

	virtual string getTitle(void);
	virtual bool hasSettings(SDLKey *key, string &caption);
	UiEvent settings(UiDrawer *d);
	UiEvent serialSettings(UiDrawer *d);
protected:
	PcwComms *m_comms;
	JoyceSystem *m_sys;
	Z80DartA m_dartA;
	Z80DartB m_dartB;
	int m_cench;	// Character in Centronics I/O latch
	int m_z80tick;	// How many Z80 cycles between checking for 
			// "interrupt-driven" I/O?
	int m_baudmode, m_txbaud, m_rxbaud, m_baudbuf;
};




