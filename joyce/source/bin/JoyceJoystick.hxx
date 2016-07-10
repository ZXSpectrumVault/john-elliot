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

class JoyceJoystick : public PcwJoystick
{
public:
	JoyceJoystick();
	virtual ~JoyceJoystick();

//
// Display settings screen
// Return 0 if OK, -1 if quit message received.
// 
	virtual UiEvent settings(UiDrawer *d);

	bool readPort(word address, byte &value);

        virtual bool parseNode(xmlDocPtr doc, xmlNsPtr ns, xmlNodePtr cur);
	virtual bool storeNode(xmlDocPtr doc, xmlNsPtr ns, xmlNodePtr cur);

protected:
	int	m_type;	// 0 => none
			// 1 => Spectravideo
			// 2 => Kempston
			// 3 => Cascade
			// 4 => DKTronics
			// (only Spectravideo is implemented!)

};

