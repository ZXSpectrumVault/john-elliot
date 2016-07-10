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

UiContainer::UiContainer(UiDrawer *d) : UiControl(d) 
{
}

UiContainer::~UiContainer()
{
	clear();
}

void UiContainer::clear(void)
{
	for (unsigned n = 0; n < m_entries.size(); n++)
	{
		delete m_entries[n];	
	}
	m_entries.clear();
}




UiEvent UiContainer::onMouse(Uint16 x, Uint16 y)
{
        unsigned n;
	UiEvent uie = UIE_CONTINUE;

        for (n = 0; n < size(); n++)
        {
		if (m_entries[n]->hitTest(x,y)) uie = m_entries[n]->onMouse(x,y);
		if (uie != UIE_CONTINUE) break;
        }
        return uie;
}




