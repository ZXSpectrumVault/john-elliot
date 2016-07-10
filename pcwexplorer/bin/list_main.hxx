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

class MainList : public wxListCtrl
{
  DECLARE_DYNAMIC_CLASS(MainList);

  public:
    MainList() : wxListCtrl() { m_frame = NULL; }

    MainList( wxWindow *parent, wxWindowID id = -1,
      const wxPoint &pos = wxDefaultPosition, 
      const wxSize &size = wxDefaultSize,
#if wxUSE_VALIDATORS
#  if defined(__VISAGECPP__)
      long style = wxLC_ICON, const wxValidator* validator = wxDefaultValidator,
#  else
      long style = wxLC_ICON, const wxValidator& validator = wxDefaultValidator,
#  endif
#endif
       const wxString &name = "listctrl" ) : wxListCtrl(parent, id, pos, size,
#if wxUSE_VALIDATORS
						style, validator,
#endif
						name)
    {
	m_frame = NULL;
    }

  void OnMouse( wxMouseEvent &event );

public: 
	PcwFrame *m_frame;

  DECLARE_EVENT_TABLE()
};

