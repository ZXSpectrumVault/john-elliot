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
#include "UiMenu.hxx"

UiMenu::UiMenu(UiDrawer *d) : UiContainer(d)
{
	m_selected = -1;
	m_w = m_h = 0;
}

UiMenu::~UiMenu()
{

}


void UiMenu::draw(int selected)
{
	unsigned int n;
	SDL_Rect rc, rcInner, rcBounds;
	
	bool b = m_drawer->getManualUpdate();
	m_drawer->setManualUpdate(true);
	rc = getRect();	
	m_drawer->drawMenuBorder(rc, rcInner);

	for (n = 0; n < size(); n++)
	{
		rcBounds = m_entries[n]->getBounds();
		m_entries[n]->setX(rcInner.x);
		m_entries[n]->setY(rcInner.y);
		rcBounds.x = rcInner.x;	
		rcBounds.y = rcInner.y;
		m_entries[n]->setBounds(rcBounds);
		m_entries[n]->draw(selected && ((int)n == m_selected));
		rcInner.y += m_entries[n]->getHeight();
	}
	m_drawer->setManualUpdate(b);
	m_drawer->updateRect(rc);
}



void UiMenu::pack(void)
{
	unsigned n, h, w, w1;
	SDL_Rect rc, rcb;

	for (n = h = w = 0; n < size(); n++)
	{
		h += m_entries[n]->getHeight();
		w1 = m_entries[n]->getWidth();	
		if (w1 > w) w = w1;
	}
	for (n = 0; n < size(); n++)
	{
		rcb = m_entries[n]->getRect();
                rcb.w = w;
                m_entries[n]->setBounds(rcb);

	}
	rc.x = 0;
	rc.y = 0;
	rc.w = w;
	rc.h = h;
	m_drawer->measureMenuBorder(rc, rc);
	m_w = rc.w;
	m_h = rc.h;
}


Uint16 UiMenu::getWidth(void)
{
	return m_w;
}

Uint16 UiMenu::getHeight(void)
{
	return m_h;
}


UiEvent UiMenu::onKey(SDLKey k)
{
	int step = 1;
	unsigned m;
	UiEvent uie;
	
	if (!m_entries[m_selected]->wantAllKeys()) for (m = 0; m < size(); m++) 
	{
		if (getKey(m) == k)
		{
			uie = m_entries[m]->onKey(k);
		
			if (uie == UIE_SELECT)
			{
                         	m_entries[m_selected]->draw(0);
                       		m_selected = m;
                        	m_entries[m_selected]->draw(1);
				return UIE_SELECT;
			}
			return uie;
		}
	}

	switch(k)
	{
                case SDLK_END:                  /* ESCAPE */
                case SDLK_ESCAPE:       return UIE_CANCEL;

                case SDLK_KP_ENTER:
                case SDLK_RETURN:       return m_entries[m_selected]->onSelected();

                case SDLK_UP:           step = -1;
                case SDLK_DOWN:         m = m_selected;
                                        do
                                        {
                                                m += step;
                                        }       
                                        while (m >= 0 && m < size() &&
                                          !(m_entries[m]->canFocus()));
                           
                                        if (m < 0 || m >= size())
						return UIE_CONTINUE;
                                        m_entries[m_selected]->draw(0);
					m_selected = m;
					m_entries[m_selected]->draw(1);
                                        return UIE_CONTINUE;


		default: return m_entries[m_selected]->onKey(k);
	}	
	return UIE_CONTINUE;
}



UiEvent UiMenu::eventLoop(void)
{
	unsigned n;
	SDL_Rect rc;

        pack();
	rc.x = getX();
	rc.y = getY();
	rc.w = m_w;
	rc.h = m_h;

        if (m_selected < 0) for (n = 0; n < size(); n++) 
        {
                if (m_entries[n]->canFocus()) { m_selected = n; break; }
        }


        SDL_Surface *surface = m_drawer->saveSurface(rc);
	
        draw(1);

        UiEvent e = UiContainer::eventLoop();

	m_drawer->restoreSurface(surface, rc);
	return e;
}



UiEvent UiMenu::onMouse(Uint16 x, Uint16 y)
{
	UiEvent e = UiContainer::onMouse(x,y);
	unsigned n;

	if (e == UIE_OK || e == UIE_SELECT)
	{
	        for (n = 0; n < size(); n++)
        	{
        	        if (m_entries[n]->hitTest(x,y)) {m_selected = n; break; }
		}
		if (n < size())
		{
			return m_entries[n]->onSelected();	
		}
		return UIE_OK;
        }	
	return e;
}



UiEvent UiMenu::onEvent(SDL_Event &ev)
{
        UiEvent e = UiControl::onEvent(ev);

	if (e == UIE_SELECT)   return UIE_CONTINUE;
        if (e != UIE_CONTINUE) return e;

	return m_entries[m_selected]->onEvent(ev);	
}

