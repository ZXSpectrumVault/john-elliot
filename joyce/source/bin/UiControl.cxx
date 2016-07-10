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
#include <string.h>
#include "UiTypes.hxx"
#include "UiControl.hxx"

UiControl::UiControl(UiDrawer *d)
{
	m_id = SDLK_UNKNOWN;
	memset(&m_rect,   0, sizeof(m_rect));
	memset(&m_bounds, 0, sizeof(m_bounds));
	m_drawer = d;
}

UiControl::~UiControl()
{

}

SDL_Rect UiControl::getRect()
{
	SDL_Rect rc = m_rect;
	rc.w = getWidth();
	rc.h = getHeight();
	return rc;
}

void UiControl::draw(int selected)
{
	SDL_Rect bounds = getBounds();
	SDL_Rect rect   = getRect();

	m_drawer->drawRect(selected, bounds);
	m_drawer->drawRect(selected, rect);
}


bool UiControl::canFocus(void)
{
	return false;
}



/////////////////////////////////////////////////////////////////////////////
//
// Event handling
//

UiEvent UiControl::eventLoop(void)
{
	SDL_Event ev;
	UiEvent uie;	

	SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, 
			SDL_DEFAULT_REPEAT_INTERVAL);
	do
	{
	        SDL_WaitEvent(&ev);

		uie = onEvent(ev);
	}
	while (uie == UIE_CONTINUE);
	SDL_EnableKeyRepeat(0,0);
	return uie;
}


UiEvent UiControl::onEvent(SDL_Event &ev)
{
	switch(ev.type)
	{
		case SDL_KEYDOWN: 
			return onKey(ev.key.keysym.sym);
		case SDL_MOUSEBUTTONDOWN:
			return onMouse(ev.button.x, ev.button.y);
                case SDL_QUIT:
         		return onQuit();
	}
	return UIE_CONTINUE;
}




UiEvent UiControl::onKey(SDLKey k)
{
	if (!wantAllKeys() && k == getKey() && k != SDLK_UNKNOWN && canFocus())     return UIE_SELECT;
	if (k == SDLK_KP_ENTER || k == SDLK_RETURN) return UIE_OK;
	if (k == SDLK_ESCAPE   || k == SDLK_END)    return UIE_CANCEL;
	return UIE_CONTINUE;
}

UiEvent UiControl::onSelected()
{
	return UIE_OK;
}

bool UiControl::hitTest(Uint16 x, Uint16 y)
{
        SDL_Rect rc = getBounds();

        if ((rc.x)        <= x && (rc.y       ) <= y &&
            (rc.x + rc.w) >  x && (rc.y + rc.h) >  y) 
	{
		return true;
	}
	return false;
}



UiEvent UiControl::onMouse(Uint16 x, Uint16 y)
{
	if (hitTest(x, y) && canFocus() ) return UIE_OK;

	return UIE_CONTINUE; 
}


UiEvent UiControl::onQuit(void) 
{
	return UIE_QUIT;
}

//Uint16 UiControl::getHeight() { return 0; }
//Uint16 UiControl::getWidth()  { return 0; }

bool UiControl::wantAllKeys(void)
{
	return false;
}


