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

#include "Pcw.hxx"
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


PcwJoystick::PcwJoystick(string sName) 
	: PcwDevice("joystick", sName)
{
	m_stickno = -1;
	m_stick   = NULL;
	m_sensitivity = SENSITIVITY;
}


PcwJoystick::~PcwJoystick()
{
	reset();
}

bool PcwJoystick::hasSettings(SDLKey *key, std::string &caption)
{
	*key = SDLK_j;
	caption = "  Joystick  ";
	return true;
}
//
// Display settings screen
// Return 0 if OK, -1 if quit message received.
// 
UiEvent PcwJoystick::settings(UiDrawer *d)
{
	UiEvent uie;
	int n;
	int sel = -1;
	SDLKey k;

	while (1)
	{
		UiMenu uim(d);
		uim.add(new UiLabel("  Choose system joystick  ", d));
		uim.add(new UiSeparator(d));
		uim.add(new UiSetting(SDLK_0, (m_stickno == -1), 
				"  0. No joystick  ", d));
		for (n = 0; n < SDL_NumJoysticks(); n++)
		{
			char buf[80];
			const char *str = SDL_JoystickName(n);
			if (!str) str = "Unknown joystick";
			sprintf(buf, "  %d. %s  ", n + 1, str);
			k = (SDLKey)(SDLK_1 + n);
			uim.add(new UiSetting(k, (n == m_stickno), buf, d));
		}
		uim.add(new UiSeparator(d))
		   .add(new UiCommand(SDLK_ESCAPE, "  EXIT  ",d));
		d->centreContainer(uim);
		uim.setSelected(sel);
		uie = uim.eventLoop();
		if (uie == UIE_QUIT) return uie;
		sel = uim.getSelected();
		switch(k = uim[sel].getKey())
		{
			case SDLK_ESCAPE: return UIE_OK;
			case SDLK_0:
				m_stickno = -1;
				reset();
				break;
			default: 
				n = (k - SDLK_1);
				if (n >= 0 && n < SDL_NumJoysticks())
				{
					m_stickno = n;
					reset();
				}
				break;
		}
	}
	return UIE_OK;
}

bool PcwJoystick::parseNode(xmlDocPtr doc, xmlNsPtr ns, xmlNodePtr cur)
{
	char *s = (char *)xmlGetProp(cur, (xmlChar *)"stick");
	if (s)
	{
		int n = atoi(s);
		if (n >= -1)
		{
			m_stickno = n;
			reset();
		}
		xmlFree(s);
	}
	s = (char *)xmlGetProp(cur, (xmlChar *)"sensitivity");
	if (s)
	{
		m_sensitivity = atoi(s);
		xmlFree(s);
	}
	return parseEnabled(doc, ns, cur);
}


bool PcwJoystick::storeNode(xmlDocPtr doc, xmlNsPtr ns, xmlNodePtr cur)
{
	char buf[20];

	sprintf(buf, "%d", m_stickno);
	xmlSetProp(cur, (xmlChar *)"stick", (xmlChar *)buf);
	sprintf(buf, "%d", m_sensitivity);
	xmlSetProp(cur, (xmlChar *)"sensitivity", (xmlChar *)buf);
	return storeEnabled(doc, ns, cur);
}

void PcwJoystick::reset(void)
{
	if (m_stick) SDL_JoystickClose(m_stick);
	m_stick = NULL;
	enable(m_stickno >= 0);
}


unsigned PcwJoystick::getPosition(unsigned l, unsigned r, unsigned u, 
			unsigned d, unsigned f1, unsigned f2)
{
	Sint16 xpos, ypos;
	int button1, button2;
	unsigned res = 0;

	if (!m_stick)
	{
		if (m_stickno < 0) return 0;
		m_stick = SDL_JoystickOpen(m_stickno);
		if (!m_stick)
		{
			joyce_dprintf("Joystick %d open failed: %s\n", 
					m_stickno, SDL_GetError());
			m_stickno = -1;	// Don't try again
			return 0;
		}
	}
	xpos = SDL_JoystickGetAxis(m_stick, 0);
	ypos = SDL_JoystickGetAxis(m_stick, 1);
	button1 = SDL_JoystickGetButton(m_stick, 0);
	button2 = SDL_JoystickGetButton(m_stick, 1);

	if (xpos < -m_sensitivity) res |= l;
	if (xpos >  m_sensitivity) res |= r;
	if (ypos < -m_sensitivity) res |= u;
	if (ypos >  m_sensitivity) res |= d;
	if (button1) res |= f1;
	if (button2) res |= f2;

	return res;
}



