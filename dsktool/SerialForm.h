/************************************************************************

    Dsktool v1.0.2 - LibDsk front end

    Copyright (C) 2005, 2007  John Elliott <jce@seasip.demon.co.uk>

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


#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class SerialForm : public wxDialog  
{
	wxComboBox *m_portbox;
	wxComboBox *m_typebox;
	wxComboBox *m_baudbox;
	wxTextCtrl *m_namebox;
	wxCheckBox *m_crtscts;

	void	OnOK(wxCommandEvent &ev);

public:
	SerialForm(wxWindow* parent, wxString filename = wxT(""));
	virtual ~SerialForm();

	wxString m_string;

	DECLARE_EVENT_TABLE()
};


#define COMBO_PORT 1
#define COMBO_BAUD 2
#define BUTTON_CRTSCTS 3
#define EDIT_NAME 4
#define COMBO_TYPE 5
