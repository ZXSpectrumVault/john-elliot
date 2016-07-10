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
#ifndef PROPPAGE_H_INCLUDED 

#define PROPPAGE_H_INCLUDED 1

class PcwPropPage : public wxWindow
{
public:
        PcwPropPage(wxWindow *parent, wxWindowID id, 
                       const wxPoint& pos = wxDefaultPosition, 
                       const wxSize& size = wxDefaultSize,          
                       long style = wxDEFAULT_DIALOG_STYLE | wxCAPTION, 
                       const wxString& name = "window");

	virtual bool OnApply(void);
protected:
	// Geometry
	int m_ruler_w;		// Width of a horizontal divider
   int m_tspacing;	// Vertical space between text lines
   wxPoint m_filename_xy;
	wxPoint m_ruler_xy;
   wxPoint m_icon_xy;
   wxSize  m_ruler_size;
   int m_rstop;		// Space between ruler and following static
   int m_rsbot;		// Space between static and following ruler
};

#endif
