

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
// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------
#ifdef __GNUG__
    #pragma implementation "proppage.cpp"
    #pragma interface "proppage.h"
#endif

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWindows headers
#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif

#include "textmetric.hxx"
#include "proppage.hxx"

#define BORDER_OFF (wxDOUBLE_BORDER | wxSUNKEN_BORDER | \
                    wxRAISED_BORDER | wxSIMPLE_BORDER | \
					wxSTATIC_BORDER)


PcwPropPage::PcwPropPage(wxWindow *parent, wxWindowID id,
                       const wxPoint& pos,
                       const wxSize& size,
                       long style,
                       const wxString& name)

 : wxWindow(parent, id, pos, size, 0, name)
{
    // Geometry. To match Windows exactly (with large and small fonts) we
    // need to set these values dynamically. What's odd is that no two
    // scaling factors seem to come out the same, so I'm obviously not
    // replicating Windows' dialog-unit calculation correctly.
    int x    =  3 * metric_x - 13;
    int xsp  = 12 * metric_x - 36;
    // small ruler size = 319
    // large ruler size = 430

    m_ruler_size  = wxSize ( (metric_x * 50) - 81 , 2);
    m_ruler_xy    = wxPoint( x,    (metric_y * 4));
    m_icon_xy     = wxPoint( x,    (metric_y - metric_d + 1));
    m_filename_xy = wxPoint( x+xsp,(metric_y + metric_d + 3));

    m_tspacing    = metric_y + metric_d + 7;	// Text vertical spacing
    m_rstop       = metric_y;         // Space between ruler and text below
    m_rsbot       = metric_d + 7;     // Space between text and ruler below
    }


bool PcwPropPage::OnApply(void)
{
	return true;
}

