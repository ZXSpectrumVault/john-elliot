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
#include "wx/notebook.h"

#define PROPSHEET_ID 101

class PcwFileFactory;

class PcwPropSheet : public wxDialog
{
public:
	PcwPropSheet(PcwFileFactory *factory, const char *filename,
                       wxWindow *parent, wxWindowID id, 
                       const wxPoint& pos = wxDefaultPosition, 
                       const wxSize& size = wxSize(350, 390), 
                       long style = wxDEFAULT_DIALOG_STYLE | wxCAPTION, 
                       const wxString& name = "dialog");

	wxNotebook *m_notebook;
	wxButton *m_btnApply, *m_btnOK, *m_btnCancel;

	// events
	void OnOK(wxCommandEvent &);
	//bool OnApply(wxCommandEvent &);
	void OnApply(wxCommandEvent &);

protected:
	DECLARE_EVENT_TABLE()
};

