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

#include "wx/wxprec.h"
#ifdef __BORLANDC__
#pragma hdrstop
#endif

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include "dsktool.h"
#include "SerialForm.h"

#ifdef __WXMSW__
static const wxString ports[] = {wxT("COM1:"), wxT("COM2:"), 
				 wxT("COM3:"), wxT("COM4:") };
#else
static const wxString ports[] = {wxT("/dev/ttyS0"), wxT("/dev/ttyS1"), 
				 wxT("/dev/ttyS2"), wxT("/dev/ttyS3") };
#endif

static const wxString bauds[] = {wxT("50"), wxT("110"), wxT("134"), 
				wxT("150"), wxT("300"), wxT("600"),
				wxT("1200"), wxT("1800"), wxT("2400"), 
				wxT("4800"), wxT("9600"), wxT("19200"), 
				wxT("38400"), wxT("57600"), wxT("115200") };

#define BAUDCOUNT (sizeof(bauds) / sizeof(bauds[0]))

BEGIN_EVENT_TABLE(SerialForm, wxDialog)
	EVT_BUTTON(wxID_OK, SerialForm::OnOK)
END_EVENT_TABLE()


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

SerialForm::SerialForm(wxWindow* parent, wxString filename) :
	wxDialog(parent, -1, wxT("Serial connection settings"), 
			wxDefaultPosition, wxSize(410, 190))
{
	m_string = filename;
	(void) new wxStaticText(this, -1, wxT("Port to use:"), wxPoint(5,5));
	(void) new wxStaticText(this, -1, wxT("Data rate:"), wxPoint(5,35));
	(void) new wxStaticText(this, -1, wxT("Remote file/device name:"), wxPoint(5,65));
	(void) new wxStaticText(this, -1, wxT("Remote drive type:"), wxPoint(5,95));

	m_portbox = new wxComboBox(this, COMBO_PORT, wxT(""), wxPoint(170, 5),
			wxSize(200, 20), 4, ports, wxCB_DROPDOWN);
	m_baudbox = new wxComboBox(this, COMBO_BAUD, wxT(""), wxPoint(170,35),
			wxSize(70, 20), BAUDCOUNT, bauds, wxCB_DROPDOWN);
	m_crtscts = new wxCheckBox(this, BUTTON_CRTSCTS, 
		wxT("RTS/CTS handshaking"), wxPoint(240, 35), wxSize(160,20));
	m_namebox = new wxTextCtrl(this, EDIT_NAME, wxT(""), wxPoint(170, 65),
			wxSize(200, 20));
	m_typebox = new wxComboBox(this, COMBO_TYPE, wxT(""), wxPoint(170, 95),
			wxSize(200, 20), 0, NULL, wxCB_DROPDOWN);

	(void)new wxButton(this, wxID_CANCEL, wxT("Cancel"), wxPoint(230, 125), wxSize(80, 30));
	(void)new wxButton(this, wxID_OK, wxT("OK"), wxPoint(320, 125), wxSize(80, 30));

	int idx = 0;
	char *drvname;

	m_typebox->Append(wxT("Auto detect"), (void *)NULL);
	do
	{
		dsk_type_enum(idx++, &drvname);
		if (drvname)
		{
			m_typebox->Append(wxString(drvname, wxConvUTF8), drvname);
		}
	} while (drvname);

	if (m_string.Left(7) == wxT("serial:"))
	{
		wxString details, port, baud, baud1, type;

		details = m_string.AfterFirst(':');
		port = details.BeforeFirst(',');
		details = details.AfterFirst(',');
		m_portbox->SetValue(port);
		baud = details.BeforeFirst(',');
		baud1 = baud.BeforeFirst('+'); if (baud1 != wxT("")) baud = baud1;	
		baud1 = baud.BeforeFirst('-'); if (baud1 != wxT("")) baud = baud1;	
		m_baudbox->SetValue(baud);
		baud1 = details.BeforeFirst(',');
		if      (baud1.Find(wxT("+crtscts")) >= 0) m_crtscts->SetValue(1);
		else if (baud1.Find(wxT("-crtscts")) >= 0) m_crtscts->SetValue(0);
		else				      m_crtscts->SetValue(1); 
		details = details.AfterFirst(',');
		m_namebox->SetValue(details.BeforeFirst(','));
		type = details.AfterFirst(',');
		if (type == wxT(""))
		{
			m_typebox->SetSelection(0);
		}
		else
		{
			int sel = m_typebox->FindString(type);
			if (sel > -1)
			{
				m_typebox->SetSelection(sel);
			}
			else
			{
				m_typebox->SetSelection(-1);
				m_typebox->SetValue(type);
			}
		}
	}
	else
	{
		m_portbox->SetSelection(0);
		m_baudbox->SetSelection(m_baudbox->FindString(wxT("9600")));
		m_crtscts->SetValue(1);
		m_namebox->SetValue(wxT(""));
		m_typebox->SetSelection(0);
	}
}

SerialForm::~SerialForm()
{

}


void SerialForm::OnOK(wxCommandEvent &ev)
{
	char *type;

	m_string = wxT("serial:");
	m_string += m_portbox->GetValue();
	m_string += wxT(",");
	m_string += m_baudbox->GetValue();
	if (m_crtscts->GetValue()) m_string += wxT("+crtscts");
	else			   m_string += wxT("-crtscts");
	m_string += wxT(",");
	m_string += m_namebox->GetValue();

	int sel = m_typebox->FindString(m_typebox->GetValue());
	if (sel > -1)
	{
		type = (char *)m_typebox->GetClientData(sel);
		if (type)
		{
			m_string += wxT(",");
			m_string += wxString(type, wxConvUTF8);
		}
	}
	else
	{
		m_string += wxT(",");
		m_string += m_typebox->GetValue();
	}
// [1.0.3] Seems to be needed to build against wxWidgets 2.8 
#if (wxMAJOR_VERSION > 2) || (wxMAJOR_VERSION == 2 && wxMINOR_VERSION > 6) 
	EndDialog(wxID_OK); 
#else
	wxDialog::OnOK(ev);
#endif
}
