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


class AnneKeyboard: public PcKeyboard
{
public:
	AnneKeyboard(AnneSystem *s);
	virtual ~AnneKeyboard();
//
// Reset device
//
	virtual void reset(void);
	virtual int handleEvent(SDL_Event &e);
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
// Implement the JOYCE "Assign key" calls
//
	void assignKey(Uint16 key, Uint8 l, Uint8 h);
	virtual void edfe(Z80 *R);
//
// nb: Ability to dump state is not yet provided, but would go here.
//
	void clearKeys(void);
	byte in(byte addr);
	void out(byte addr, byte val);
	virtual void poll(Z80 *R);
protected:
        virtual bool parseNode(xmlDocPtr doc, xmlNsPtr ns, xmlNodePtr cur);
	virtual bool storeNode(xmlDocPtr doc, xmlNsPtr ns, xmlNodePtr cur);

// Redefine a group of keys
	void setRange(int *R, byte a, byte b, byte c, byte d);

// Apply latest settings
	void keySet(void);
	virtual void pushKey(byte k);

	byte	m_shift;	// Shift register
	byte	m_state;	// PCW16 kbd controller state
	short	m_keysWaiting;	// Keyboard has keys waiting?
};

