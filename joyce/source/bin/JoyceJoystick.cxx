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

#include "Joyce.hxx"
#include "UiContainer.hxx"
#include "UiLabel.hxx"
#include "UiSeparator.hxx"
#include "UiCommand.hxx"
#include "UiSetting.hxx"
#include "UiMenu.hxx"
#include "UiTextInput.hxx"
#include "UiScrollingMenu.hxx"

/* On a scale of 1 - 32767, how far do you have to push the stick before
 * it registers? */
#define SENSITIVITY 16384


JoyceJoystick::JoyceJoystick() 
	: PcwJoystick("external")
{
	m_type = 0;
}


JoyceJoystick::~JoyceJoystick()
{
}

//
// Display settings screen
// Return 0 if OK, -1 if quit message received.
// 
UiEvent JoyceJoystick::settings(UiDrawer *d)
{
	UiEvent uie;
	int n;
	int sel = -1;
	SDLKey k;

	while (1)
	{
		UiMenu uim(d);
		uim.add(new UiLabel("  Set joystick type  ", d));
		uim.add(new UiSeparator(d));
		uim.add(new UiSetting(SDLK_0, (m_type == 0), "  0. No joystick  ", d));
		uim.add(new UiSetting(SDLK_1, (m_type == 1), "  1. Spectravideo  ", d));
		uim.add(new UiSetting(SDLK_2, (m_type == 2), "  2. Kempston  ", d));
		uim.add(new UiSetting(SDLK_3, (m_type == 3), "  3. Cascade  ", d));
/* This type is supported, but not encouraged. A true emulation is not possible
 * unless I emulate the entire DK'Tronics sound board. So the only way to 
 * select this joystick type is by editing joycehw.xml.
 *		uim.add(new UiSetting(SDLK_4, (m_type == 4), "  4. DK'Tronics  ", d));
 */
		uim.add(new UiSeparator(d))
		   .add(new UiCommand(SDLK_s, "  Select host joystick  ", d))
		   .add(new UiSeparator(d))
		   .add(new UiCommand(SDLK_ESCAPE, "  EXIT  ",d));
		d->centreContainer(uim);
		uim.setSelected(sel);
		uie = uim.eventLoop();
		if (uie == UIE_QUIT) return uie;
		sel = uim.getSelected();
		switch(k = uim[sel].getKey())
		{
			case SDLK_ESCAPE: return UIE_OK;
			case SDLK_s:
				uie = PcwJoystick::settings(d);
				if (uie == UIE_QUIT) return uie;
				break;
			case SDLK_0:
				m_type = 0;
				break;
			case SDLK_1:
				m_type = 1;
				break;
			case SDLK_2:
				m_type = 2;
				break;
			case SDLK_3:
				m_type = 3;
				break;
			case SDLK_4:
				m_type = 4;
				break;
		}
	}
	return UIE_OK;
}



bool JoyceJoystick::parseNode(xmlDocPtr doc, xmlNsPtr ns, xmlNodePtr cur)
{
	char *s = (char *)xmlGetProp(cur, (xmlChar *)"type");
	if (s)
	{
		int n = atoi(s);
		if (n >= 0) m_type = n; 
		xmlFree(s);
	}
	return PcwJoystick::parseNode(doc, ns, cur);
}


bool JoyceJoystick::storeNode(xmlDocPtr doc, xmlNsPtr ns, xmlNodePtr cur)
{
	char buf[20];

	sprintf(buf, "%d", m_type);
	xmlSetProp(cur, (xmlChar *)"type", (xmlChar *)buf);
	return PcwJoystick::storeNode(doc, ns, cur);
}


bool JoyceJoystick::readPort(word address, byte &value)
{
	switch (m_type)
	{
//
//  Head Over Heels joystick directions:
//  		L    U
//
//  		D    R   fire=jump
//
//
// Spectravideo: 
// Bit 2 = Left, Bit 4 = Right, Bit 3 = Up, Bit 0 = Down, Bit 1 = Fire 
		case 1: if (address != 0xE0) break;
			value = getPosition(4, 16, 8, 1, 2, 2);
			return true;
// Kempston (Assuming it's the same as the Spectrum version: 
// Bit 1 = left, Bit 0 = right, Bit 2 = Up, Bit 3 = Down, Bit 4 = Fire 
		case 2: if (address != 0x9F) break;
			value = getPosition(2, 1, 8, 4, 16, 16);
			return true;
//
// Cascade: Active low.
// Bit 0 = Left, Bit 1 = right, Bit 4 = Up, Bit 2 = Down, Bit 7 = Fire.
		case 3: if (address != 0xE0) break;
			value = ~getPosition(1, 2, 16, 4, 128, 128);
			return true;
//
// DK'Tronics: Active low.
// XXX This should be part of a proper driver for the DK'Tronics sound
// board, the same way that the keyboard joysticks are part of the keyboard
// driver.
// Bit 2 = Left, Bit 3 = Right, Bit 5 = Up, Bit 4 = Down, Bit 6 = Fire.
		case 4: if (address != 0xA9) break;
			value = ~getPosition(4, 8, 32, 16, 64, 64);
			return true;


	}	
	return false;
}


