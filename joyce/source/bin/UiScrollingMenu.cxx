/************************************************************************

    JOYCE v2.1.6 - Amstrad PCW emulator

    Copyright (C) 1996, 2001, 2004  John Elliott <seasip.webmaster@gmail.com>

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
#include "UiLabel.hxx"
#include "UiContainer.hxx"
#include "UiMenu.hxx"
#include "UiScrollingMenu.hxx"


UiScrollingMenu::UiScrollingMenu(int itemH, unsigned visCount, UiDrawer *d) : 
	UiMenu(d)
{
	m_visCount = visCount;
	m_itemH    = itemH;
	m_top      = 0;
	m_onscreen = false;
}

UiScrollingMenu::~UiScrollingMenu()
{

}


void UiScrollingMenu::draw(int selected)
{
	unsigned int n;
	SDL_Rect rc;
        UiLabel   dummy("", m_drawer);

	rc = getRect();	
        bool b = m_drawer->getManualUpdate();
        m_drawer->setManualUpdate(true);

	m_drawer->drawMenuBorder(rc, m_rcInner);

	for (n = 0; n < m_visCount; n++) 
	{
		drawElement(selected, n + m_top);
	}
	// XXX draw a scrollbar 
        m_drawer->setManualUpdate(b);
        m_drawer->updateRect(rc);
}


void UiScrollingMenu::drawElement(int selected, int e)
{
	UiControl *c;
        UiLabel   dummy("", m_drawer);
	SDL_Rect  rcInner = m_rcInner;
	SDL_Rect  rcBounds;

	if (e < (int)m_top || e >= (int)(m_top + m_visCount) ) return;

	if (e < (int)m_entries.size()) c = m_entries[e];
	else                           c = &dummy;

	rcInner.y += (e - m_top) * m_itemH;
	rcBounds = c->getBounds();
	c->setX(rcInner.x);
	c->setY(rcInner.y);
	rcBounds.x = rcInner.x;	
	rcBounds.y = rcInner.y;
	c->setBounds(rcBounds);

	c->draw(selected && (e == m_selected));
}


void UiScrollingMenu::setSelected(int ns)
{
	int s = m_selected;
	
	if (ns != s)
	{
		m_selected = ns;
		if (!m_onscreen) return;
		showSelected();
	
		drawElement(0, s);
		drawElement(1, ns);
	}
}



void UiScrollingMenu::showSelected(void)
{
	int t = m_top;

	if (m_selected < m_top) m_top = m_selected;
	if (m_selected >= (m_top + (int)m_visCount)) 
		m_top = m_selected - (int)m_visCount + 1;  

	if (m_top != t) draw(1);
	
}





void UiScrollingMenu::pack(void)
{
	unsigned n, w, w1;
	SDL_Rect rc, rcb;

	for (n = w = 0; n < size(); n++)
	{
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
	rc.h = m_visCount * m_itemH;
	m_drawer->measureMenuBorder(rc, rc);
	m_w = rc.w;
	m_h = rc.h;
}


UiEvent UiScrollingMenu::onMouse(Uint16 x, Uint16 y)
{
	if (y >= getY() && y < m_rcInner.y) return onKey(SDLK_UP);
	if (y >= m_rcInner.y + m_rcInner.h && 
	    y < (getY() + getHeight()) )    return onKey(SDLK_DOWN);
	return UiMenu::onMouse(x,y);
}




UiEvent UiScrollingMenu::onKey(SDLKey k)
{
	int step = 1;
	int m;
	UiEvent uie;
	
	if (!m_entries[m_selected]->wantAllKeys()) for (m = 0; m < (int)size(); m++) 
	{
		if (getKey(m) == k)
		{
			uie = m_entries[m]->onKey(k);
		
			if (uie == UIE_SELECT)
			{
                       		setSelected(m);
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
                case SDLK_RETURN:       return UIE_OK;

		case SDLK_PAGEUP:	m = m_selected - (int)m_visCount + 1;
					if (m < 0) m = 0;
					while (m < m_selected &&
					      !(m_entries[m]->canFocus())) ++m;
					
					setSelected(m);
					return UIE_CONTINUE;	

		case SDLK_PAGEDOWN:	m = m_selected + m_visCount - 1;
					if (m >= (int)size()) m = size() - 1;
					while (m > m_selected && 
					     !(m_entries[m]->canFocus())) --m;
					setSelected(m);
					return UIE_CONTINUE;	

                case SDLK_UP:           step = -1;
                case SDLK_DOWN:         m = m_selected;
					do
                                        {
                                                m += step;
                                        }       
                                        while (m >= 0 && m < (int)size() &&
                                          !(m_entries[m]->canFocus()));
                           
                                        if (m < 0 || m >= (int)size())
						return UIE_CONTINUE;
					setSelected(m);
                                        return UIE_CONTINUE;


		default: return m_entries[m_selected]->onKey(k);
	}	
	return UIE_CONTINUE;
}



UiEvent UiScrollingMenu::eventLoop(void)
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
	m_onscreen = true;	
        draw(1); 
	showSelected();
        UiEvent e = UiContainer::eventLoop();
	m_onscreen = false;

	m_drawer->restoreSurface(surface, rc);
	return e;
}






