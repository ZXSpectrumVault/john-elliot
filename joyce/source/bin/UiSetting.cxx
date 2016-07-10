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
#include "UiTypes.hxx"
#include "UiControl.hxx"
#include "UiLabel.hxx"
#include "UiSetting.hxx"

UiSetting::UiSetting(SDLKey k, bool checked, string caption, UiDrawer *d) :
	UiLabel(caption, d)
{
	setKey(k);
	setChecked(checked);
}

UiSetting::~UiSetting()
{

}

UiEvent UiSetting::onKey(SDLKey k)
{
	if (k == SDLK_SPACE) return UIE_OK;
	return UiLabel::onKey(k);
}



void UiSetting::draw(int selected)
{
	UiLabel::draw(selected);
	m_drawer->drawGlyph(selected, getX(), getY(), 
			m_checked ? UIG_CHECKED	: UIG_UNCHECKED);
}



bool UiSetting::canFocus(void)
{
        return true;
}


