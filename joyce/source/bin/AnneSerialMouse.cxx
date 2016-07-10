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

#include "Anne.hxx"
#include "UiContainer.hxx"
#include "UiMenu.hxx"
#include "UiLabel.hxx"
#include "UiSeparator.hxx"
#include "UiSetting.hxx"
#include "UiCommand.hxx"

AnneSerialMouse::AnneSerialMouse(AnneSystem *s) :
        PcwDevice("mouse", "serial"), PcwInput(s)
{
	XLOG("AnneSerialMouse::AnneSerialMouse()");
	reset();
	m_basePort = 0x20;
	m_direct = true;	// false
}


AnneSerialMouse::~AnneSerialMouse()
{
}


//
// Reset device
//
void AnneSerialMouse::reset(void)
{
	SDL_ShowCursor(1);
	m_tracking = false;
	m_cursorShown = true;
	m_packetPos = 0;

// Set up for the PCW's 640x480 screen
	
	m_xmin = 80;
	m_xmax = 720;
	m_ymin = 60;
	m_ymax = 540;	/* v2.1.4: 60 pixels border at the bottom, not 20! */
	m_xscale = 4;
	m_yscale = 3;
	getMousePos(&m_ox, &m_oy);

	updateSettings();
}


void AnneSerialMouse::updateSettings(void)
{

}

//
// Implement PCW mouse
//
void AnneSerialMouse::poll(Z80 *R)
{

}


Uint8 AnneSerialMouse::getMousePos(int *rx, int *ry)
{
	int x, y;
	Uint8 b;

	b = SDL_GetMouseState(&x, &y);

	if (x <  m_xmin) x = m_xmin;
	if (x >= m_xmax) x = m_xmax - 1;
	if (y <  m_ymin) y = m_ymin;
	if (y >= m_ymax) y = m_ymax - 1;

	x -= m_xmin; x = (x*2) / m_xscale;
	y -= m_ymin; y = (y*2) / m_yscale;

	*rx = x;
	*ry = y;
	return b;
}


int AnneSerialMouse::dx(int l)
{
	int x, y;

	getMousePos(&x, &y);

	y = x - m_ox;

	if (y > l)  y = l;
	if (y < -l) y = -l;
	
	m_ox += y;
	return y;	
}


int AnneSerialMouse::dy(int l)
{
	int x, y;

	getMousePos(&x, &y);
	
	x = y - m_oy;
	if (x > l)  x = l;
	if (x < -l) x = -l;

	m_oy += x;

	return x;	
}

int AnneSerialMouse::buttons(void)
{
	int x, y;
	return SDL_GetMouseState(&x, &y);
}





byte AnneSerialMouse::in(word port)
{

	if (!isEnabled()) return 0xFF;
	if (port != m_basePort) return 0xFF;

/* If already sending a packet, continue to do so */
	if (m_packetPos)
	{
		byte b = m_packet[m_packetPos - 1];
		++m_packetPos;
		if (m_packetPos == 6) 
		{
			m_packetPos = 0;
			((AnneAsic *)m_sys->m_asic)->sioIsr(true);
		}
		return b;
	}
/* Generate a Mouse Systems mouse packet. */

	createPacket();

	m_packetPos = 2;
	return m_packet[0];
}


//
// Create a mouse packet & prepare to fire it off.
//
void AnneSerialMouse::createPacket()
{
	int y, x, b, c;
	if (m_direct)
	{
		b = SDL_GetMouseState(&x, &y);
// (v2.1.4) Yow. That limiting code had Serious Issues.
		if (x >= m_xmin && x < m_xmax && y >= m_ymin && y < m_ymax)
		{
			x -= m_xmin;
			y -= m_ymin;
			c = 0;	/* Rearrange buttons */
			if (PCWRAM[0x0D46])	// Swap buttons?
			{	
				c = b & 7;	/* L,M,R just happen to be
						 * in the right order */
			}
			else
			{
				if (b & 1) c |= 4;	/* L */
				if (b & 2) c |= 2;	/* M */
				if (b & 4) c |= 1;	/* R */
			}
			PCWRAM[0x43E] = x & 0xFF;
			PCWRAM[0x43F] = x >> 8;
			PCWRAM[0x440] = y & 0xFF;
			PCWRAM[0x441] = y >> 8;
			PCWRAM[0x446] = c;
		}	
	}
	else	
	{
		m_packetPos = 1;
	
		y = dy(127);
		x = dx(127);
		b = buttons();
	
		m_packet[0] = 0x80;
		if (b & 1) m_packet[0] |= 4;	/* L */
		if (b & 2) m_packet[0] |= 2;	/* M */
		if (b & 4) m_packet[0] |= 1;	/* R */

		m_packet[1] = ((signed char)(x));
		m_packet[2] = ((signed char)(-y));
		m_packet[3] = 0;
		m_packet[4] = 0;

		((AnneAsic *)m_sys->m_asic)->sioIsr(true);
	}
}

//
// Settings functions.
// See if this device has user-settable options. If it does, populates
// "key" and "caption" and returns true; else, returns false.
//
bool AnneSerialMouse::hasSettings(SDLKey *key, string &caption)
{
	*key     = SDLK_m;
	caption = "  Mouse"; 
	return true;
}
//
// Display settings screen
// Return 0 if OK, -1 if quit message received.
// 
UiEvent AnneSerialMouse::settings(UiDrawer *d)
{	
	bool direct = m_direct;
	int x,y, sel;
	UiEvent uie;
	x = y = 0;
	sel = -1;
	do
	{
		bool ena  = isEnabled();
		bool com3 = (m_basePort == 0x20);
		bool com1 = (m_basePort == 0x28);
		if (direct) { com1 = false; com3 = false; }
		if (!ena)   { com1 = false; com3 = false; direct = false; }
		UiMenu uim(d);

		uim.add(new UiLabel("    Mouse settings    ", d))
		   .add(new UiSeparator(d))
                   .add(new UiSetting(SDLK_n, !ena,  "  No emulated mouse  ",d))
	           .add(new UiSetting(SDLK_1, com1,  "  Serial mouse on COM1  ",d))
	           .add(new UiSetting(SDLK_3, com3,  "  Serial mouse on COM3  ",d))
	           .add(new UiSetting(SDLK_u, direct,"  Update ROSANNE directly  ",d));

		uim.add(new UiSeparator(d))
                   .add(new UiCommand(SDLK_ESCAPE, "  EXIT  ", d));
	
		// Only centre the menu once; we don't want it jumping about
		// the screen as options appear and disappear.
		if (!x) 
		{
			d->centreContainer(uim);
			x = uim.getX();
			y = uim.getY();
		}
		else
		{
			uim.setX(x);
			uim.setY(y);
		}

		if (sel >= (int)uim.size()) 
			uim.setSelected(-1);
		else	uim.setSelected(sel);
		uie = uim.eventLoop();
		if (uie == UIE_QUIT) return uie;
		
		sel = uim.getSelected();	
		switch(uim[sel].getKey())
		{
			case SDLK_ESCAPE: m_direct = direct; updateSettings(); return UIE_OK; 
			case SDLK_n:   enable(false); direct = false; m_basePort = -1; break;
			case SDLK_1:   enable(true); direct = false; m_basePort = 0x28; break;
			case SDLK_3:   enable(true); direct = false; m_basePort = 0x20; break;
			case SDLK_u:   enable(true); direct = true;  m_basePort = 0x20; break;
			default: break;
		}
	} while(1);
	m_direct = direct;
	return UIE_OK;
}
//
// nb: Ability to dump state is not yet provided, but would go here.
//

bool AnneSerialMouse::parseNode(xmlDocPtr doc, xmlNsPtr ns, xmlNodePtr cur)
{
        char *s = (char *)xmlGetProp(cur, (xmlChar *)"base");

        if (s && atoi(s)) m_basePort = atoi(s);
       	else if (s) joyce_dprintf("Mouse base port '%s' is not valid - ignored\n",s);
        if (s) xmlFree(s);
        
	s = (char *)xmlGetProp(cur, (xmlChar *)"direct");
	if (s && !strcmp(s, "1"))  m_direct = true;
	if (s && !strcmp(s, "0")) m_direct = false;
	if (s) xmlFree(s);
	return parseEnabled(doc, ns, cur);	
}


bool AnneSerialMouse::storeNode(xmlDocPtr doc, xmlNsPtr ns, xmlNodePtr cur)
{
	char bp[20];

	sprintf(bp, "%d", m_basePort);
	xmlSetProp(cur, (xmlChar *)"base",   (xmlChar *)bp);
	xmlSetProp(cur, (xmlChar *)"direct", (xmlChar *)(m_direct ? "1" : "0"));
	return storeEnabled(doc, ns, cur);
}


void AnneSerialMouse::out(word address, byte value)
{
}


int AnneSerialMouse::handleEvent(SDL_Event &event)
{
	if (event.type == SDL_MOUSEMOTION)
	{
		if (!m_packetPos) createPacket();
		return 1;
	}
	if (event.type == SDL_MOUSEBUTTONDOWN)
	{
// XXX Check for 640x480 terminal
		if (event.button.x < 60 && event.button.y >= 557) 
			((AnneSystem *)m_sys)->powerPress();
		else	if (!m_packetPos) createPacket();
		return 1;
	}
	if (event.type == SDL_MOUSEBUTTONUP)
	{
// XXX Check for 640x480 terminal
		if (event.button.x < 60 && event.button.y >= 557) 
			((AnneSystem *)m_sys)->powerRelease();
		else	if (!m_packetPos) createPacket();
		return 1;
	}
	return 0;
}



