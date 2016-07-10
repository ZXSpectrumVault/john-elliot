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

#include <SDL.h>
#include <string>
#include <vector>
#include "UiTypes.hxx"
#include "UiControl.hxx"
#include "UiContainer.hxx"
#include "UiMenuBar.hxx"



UiMenuBar::UiMenuBar(UiDrawer *d) : UiContainer(d)
{
	m_selected = -1;
	m_w = 0;
	m_h = 0;
}


UiMenuBar::~UiMenuBar()
{

}



void UiMenuBar::draw(int selected)
{
        unsigned n;
        int rv;
        SDL_Rect mbrect;

        mbrect.x = getX();
        mbrect.y = getY();
        mbrect.w = m_w;
        mbrect.h = m_h;

        m_drawer->drawMenuBar((m_selected > 0), mbrect);

        for (n = 0; n < size(); n++)
        {
                if (m_selected < 0) rv = 0;
                else rv = ((int)n != m_selected);

                m_entries[n]->draw(rv);
        }
}


void UiMenuBar::pack(void)
{
	int x, n, w, gap;
	SDL_Rect rc;
	Uint16 charw, charh;

	m_drawer->measureMenuBar(rc);
	m_drawer->measureString(" ", &charw, &charh);

	setX(rc.x);
	setY(rc.y);	
	m_w = rc.w;
	m_h = rc.h;

        if (size() <= 0) return;

        for (n = w = 0; n < (int)size(); n++) 
        {
                w += m_entries[n]->getWidth();
        }
        /* w = total width of menu texts */

        if (size() == 1) gap = rc.w - w;
        else gap = (rc.w - w) / (size() - 1);

	gap -= (gap % charw);	// XXX Fudge for pixel-positioning text

        x = 0;
        for (n = 0; n < (int)size(); n++)
        {
		SDL_Rect rcb;

                m_entries[n]->setX(x);
                m_entries[n]->setY(rc.y);

		rcb.x = x;
		rcb.y = rc.y;
		rcb.w = m_entries[n]->getWidth() + gap;
		rcb.h = rc.h;
		m_entries[n]->setBounds(rcb);
	
                x += rcb.w;

	}	
}


UiEvent UiMenuBar::onKey(SDLKey k)
{
	unsigned n;
	UiEvent e;

	for (n = 0; n < size(); n++)
	{
		if (m_entries[n]->getKey() == k)
		{
			e = m_entries[n]->onKey(k);
			if (e == UIE_OK || e == UIE_SELECT)
			{
				m_selected = n;
				draw(1);
				return UIE_OK;
			}	
			return e;
		}
	}
	if (m_selected >= 0) return m_entries[m_selected]->onKey(k);
	return UIE_CONTINUE;
}


Uint16 UiMenuBar::getHeight() { return m_h; }
Uint16 UiMenuBar::getWidth()  { return m_w; }


UiEvent UiMenuBar::eventLoop(void)
{
	m_selected = -1;
	pack();
	draw(1);
	return UiContainer::eventLoop();
}


UiEvent UiMenuBar::onMouse(Uint16 x, Uint16 y)
{
        UiEvent e = UiContainer::onMouse(x,y);

        if (e == UIE_OK || e == UIE_SELECT)
        {
                for (unsigned n = 0; n < size(); n++)
                {
                        if (m_entries[n]->hitTest(x,y)) 
			{
				m_selected = n; 
				draw(1);
				break;
			}
                }
        }
        return e;
}

              

