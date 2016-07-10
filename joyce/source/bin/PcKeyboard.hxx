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


class PcKeyboard: public PcwDevice, public PcwInput
{
public:
	PcKeyboard(PcwSystem *s);
	virtual ~PcKeyboard();
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
protected:
        virtual bool parseNode(xmlDocPtr doc, xmlNsPtr ns, xmlNodePtr cur);
	virtual bool storeNode(xmlDocPtr doc, xmlNsPtr ns, xmlNodePtr cur);

	virtual void updateLEDs(void);

// Redefine a group of keys
	void setRange(int *R, byte a, byte b, byte c, byte d);

// Apply latest settings
	void keySet(void);
// Keyboard returns a code
	virtual void pushKey(byte k);
	void onCommand(void);

// Swap ALT / DELETE settings
	bool	m_keyPress;
	std::list<byte> m_queue;	// Outgoing keys
	byte	m_command;		// Incoming command
	byte	m_cmdActive;		// Active command
	byte	m_leds;			// LED status
	byte	m_typematic;		// Typematic delay/option byte
	byte	m_scanSet;		// What scancode set to use
	bool	m_enabled;		// Keyboard enabled?
};

