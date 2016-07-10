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

#include "Joyce.hxx"

JoyceCenPrinter::JoyceCenPrinter() : PcwPrinter("centronics") 
{
	m_lastch = -1;
}

 
JoyceCenPrinter::~JoyceCenPrinter() 
{}

void JoyceCenPrinter::out(byte port, byte value)
{
	if (!isEnabled()) return;

	switch(port)
	{
		// Toggle STROBE
		case 0x85: writeChar(m_lastch); break;
		// Reset port
		case 0x86: m_lastch = -1;       break;
		// Load the character latch
		case 0x87: m_lastch = value;    break;
	}
}

byte JoyceCenPrinter::in(byte port)
{
	if (!isEnabled()) return 0xFF;

	if (port == 0x84 || port == 0x85)
	{
		return isBusy() ? 0xFF : 0xFE;
	}
	return 0xFF;
}


bool JoyceCenPrinter::hasSettings(SDLKey *key, string &caption)
{
	*key = SDLK_c;
	caption = "  CEN port  ";	
	return true;
}


string JoyceCenPrinter::getTitle(void)
{
        return "Centronics add-on";
}

