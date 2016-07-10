/************************************************************************

    JOYCE v2.1.5 - Amstrad PCW emulator

    Copyright (C) 1996, 2001-2,2004  John Elliott <seasip.webmaster@gmail.com>

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

class UiDrawCallback;
class UiDrawer;

class PcwJoystick : public PcwDevice
{
public:
	PcwJoystick(string sName);
	virtual ~PcwJoystick();

//
// Settings functions.
// See if this device has user-settable options. If it does, populates
// "key" and "caption" and returns true; else, returns false.
//
	virtual bool hasSettings(SDLKey *key, std::string &caption);
//
// Display settings screen
// Return 0 if OK, -1 if quit message received.
// 
	virtual UiEvent settings(UiDrawer *d);
//
// nb: Ability to dump state is not yet provided, but would go here.
//

// Get (digital) joystick position. Returns a bitwise OR of such of 
// l/r/u/d/f1/f2 that are true (eg: if in top left corner, returns l | u)
//
	unsigned getPosition(unsigned l, unsigned r, unsigned u, unsigned d,
			unsigned f1, unsigned f2);

//
// Made these public so PcwJoystick can be slaved to PcwKeyboard
//
	virtual void reset(void);
        virtual bool parseNode(xmlDocPtr doc, xmlNsPtr ns, xmlNodePtr cur);
	virtual bool storeNode(xmlDocPtr doc, xmlNsPtr ns, xmlNodePtr cur);
protected:

	SDL_Joystick *m_stick;
	int	m_stickno;	
	int	m_sensitivity;
};

