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


class AnneSerialMouse: public PcwDevice, public PcwInput
{
	friend class AnneZ80;
public:
	AnneSerialMouse(AnneSystem *s);
	virtual ~AnneSerialMouse();
//
// Reset device
//
	virtual void reset(void);
//
// Implement PCW mouse
//
	virtual void poll(Z80 *R);
	virtual int handleEvent(SDL_Event &e);
	virtual byte in(word port);
	virtual void out(word address, byte value);

//
// Settings functions.
// See if this device has user-settable options. If it does, populates
// "key" and "caption" and returns true; else, returns false.
//
	virtual bool hasSettings(SDLKey *key, string &caption);
//
// Display settings screen
// 
	virtual UiEvent settings(UiDrawer *d);
//
// nb: Ability to dump state is not yet provided, but would go here.
//

protected:
        virtual bool parseNode(xmlDocPtr doc, xmlNsPtr ns, xmlNodePtr cur);
	virtual bool storeNode(xmlDocPtr doc, xmlNsPtr ns, xmlNodePtr cur);

	void updateSettings(void);
	void createPacket(void);

	// Get mouse position in PCW screen coordinates
	Uint8 getMousePos(int *, int *);
	int dy(int l);
	int dx(int l);
	int buttons(void);

	bool	  m_tracking;
	bool	  m_cursorShown;
	bool	  m_direct;

	int	  m_ox, m_oy;
	Uint8	  m_packet[5];
	int	  m_packetPos;
	int	  m_basePort;
// Screen scaling things
//
// XXX These should be autodetected from the active terminal.
//
	int	m_xmin, m_xmax, m_ymin, m_ymax, m_xscale, m_yscale;
};

