

/* PCW Explorer - access Amstrad PCW discs on Linux or Windows
    Copyright (C) 2000  John Elliott

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
*/

#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#include "wx/wx.h"
#include "wx/listctrl.h"
#include "wx/splitter.h"
#include "pcwplore.hxx"
#include "frame.hxx"
#include "list_main.hxx"

IMPLEMENT_DYNAMIC_CLASS(MainList, wxListCtrl);

BEGIN_EVENT_TABLE(MainList, wxListCtrl)
//    EVT_MOUSE_EVENTS  (MainList::OnMouse)
END_EVENT_TABLE()

void MainList::OnMouse(wxMouseEvent &event)
{
	if (!m_frame) return;
	if (!event.ButtonDClick()) return;

	m_frame->OnDClickList(event);
}

