/************************************************************************

    JOYCE v2.1.5 - Amstrad PCW emulator

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
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "UiTypes.hxx"
#include "UiControl.hxx"
#include "UiLabel.hxx"
#include "UiContainer.hxx"
#include "UiCommand.hxx"
#include "UiMenu.hxx"
#include "UiScrollingMenu.hxx"
#ifdef WIN32
#include <windows.h>
#endif
#include "PcwFileEntry.hxx"
#include "PcwFileList.hxx"


PcwFileList::PcwFileList(int itemH, unsigned visCount, UiDrawer *d) : 
	UiScrollingMenu(itemH, visCount, d)
{
	m_onscreen = true;
}

PcwFileList::~PcwFileList()
{

}


void PcwFileList::draw(int selected)
{
	UiScrollingMenu::draw(selected);
}


void PcwFileList::drawElement(int selected, int e)
{
	UiScrollingMenu::drawElement(selected, e);
}


void PcwFileList::fill(vector<string> &v, string dir)
{
	clear();
	for (unsigned n = 0; n < v.size(); n++)
        {
                add(new PcwFileEntry(v[n], dir, m_drawer));
        }
	if (m_selected >= (int)v.size()) m_selected = 0;
	if (m_top      >= (int)v.size()) m_top = 0;
	pack();
}


void PcwFileList::setW(int w, int iw)
{
	unsigned n;
	SDL_Rect rcb;

	m_w = w;

        for (n = 0; n < size(); n++)
        {
                rcb = m_entries[n]->getRect();
                rcb.w = iw;
                m_entries[n]->setBounds(rcb);
        }
	rcb.x = getX();
	rcb.y = getY();
	rcb.w = w;
	rcb.h = getHeight();
	setBounds(rcb);
}

