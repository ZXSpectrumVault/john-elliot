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


class JoycePcwKeyboard: public PcwDevice, public PcwInput
{
public:
	JoycePcwKeyboard(JoyceSystem *s);
	virtual ~JoycePcwKeyboard();
//
// Reset device
//
	virtual void reset(void);
//
// Implement PCW keyboard
//
	virtual void poll(Z80 *R);
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
protected:
        virtual bool parseNode(xmlDocPtr doc, xmlNsPtr ns, xmlNodePtr cur);
	virtual bool storeNode(xmlDocPtr doc, xmlNsPtr ns, xmlNodePtr cur);

// Redefine a group of keys
	void setRange(int *R, byte a, byte b, byte c, byte d);

// Apply latest settings
	void keySet(void);

// Calculate 
	void updateMatrix(void);

// Swap ALT / DELETE settings
	bool	m_swapAlt;
	bool	m_swapDel;
// Keyboard option links
	bool	m_lk1;
	bool	m_lk2;
	bool	m_lk3;

	bool	m_shiftLock;
	bool	m_autoShift;
	bool	m_trueShift;
	bool	m_keyPress;
	Uint8	m_pcwKeyMap[17], m_oldKeyMap[17];

	int	m_ticker;

// Keyboard mappings
	Uint16 m_keybMap[4 * SDLK_LAST];

	PcwJoystick m_stick1, m_stick2;
};

