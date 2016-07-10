/************************************************************************

    JOYCE v1.9.5 - Amstrad PCW emulator

    Copyright (C) 2002  John Elliott <seasip.webmaster@gmail.com>

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

#include <SDL.h>
#include <string>
#include <vector>
#include "UiTypes.hxx"
#include "UiControl.hxx"
#include "UiContainer.hxx"
#include "UiLabel.hxx"
#include "UiColourEntry.hxx"
#include "UiMenu.hxx"
#include "UiScrollingMenu.hxx"
#include "colours.hxx"

UiColourEntry::UiColourEntry(SDLKey id, string prompt, SDL_Color &cr, UiDrawer *d) :
	UiLabel(prompt, d), m_cr(cr), m_prompt(prompt)	
{
	setKey(id);
	m_length = prompt.find('_');
	if (m_length < 0) m_length = 0;
}

UiEvent UiColourEntry::onEvent(SDL_Event &e)
{
	return UiLabel::onEvent(e);
}




bool UiColourEntry::canFocus(void)
{
	return true;
}


bool UiColourEntry::wantAllKeys(void)
{
	return UiLabel::wantAllKeys();
}


void UiColourEntry::draw(int selected)
{
	SDL_Rect rc;

	rc.x = getX();
	rc.y = getY() + 1;
//	assert(rc.y > 8 && rc.x > 8);
	m_drawer->measureString("_", &rc.w, &rc.h);
	rc.x += m_length * rc.w;
	rc.w *= 2;
	rc.h -= 2;

	UiLabel::draw(selected);
	m_drawer->drawRect(selected, rc, &m_cr);
}


UiEvent UiColourEntry::onSelected()
{
	int r, g, b;
	SDL_Color cr[64];
	char crstr[40];
	int sel = -1;
	int n = 0;

	if (getKey() == SDLK_UNKNOWN) return UIE_OK;
	UiEvent uie;
	while (1)
	{
		UiScrollingMenu uism(16, 20, m_drawer);
		for (r = 0; r <= 0xFF; r += 0x55)
		  for (g = 0; g <= 0xFF; g += 0x55)
		    for (b = 0; b <= 0xFF; b += 0x55)
		    {
			cr[n].r = r;
			cr[n].g = g;
			cr[n].b = b;
			if (r == m_cr.r && g == m_cr.g && b == m_cr.b) sel = n;
			sprintf(crstr, "  %-22.22s __  ", colourNames[n]);
			uism.add(new UiColourEntry(SDLK_UNKNOWN, crstr, cr[n], m_drawer));
			++n;
		    }
		m_drawer->centreContainer(uism);
		uism.setSelected(sel);
		uie = uism.eventLoop();
		if (uie == UIE_QUIT)   return UIE_QUIT;
		if (uie == UIE_CANCEL) return UIE_CONTINUE;
		sel = uism.getSelected();
		if (uie == UIE_OK)
		{
			m_cr.r = cr[sel].r;
			m_cr.g = cr[sel].g;
			m_cr.b = cr[sel].b;
			return UIE_CONTINUE;
		}
	}
	return UIE_CONTINUE;
}
