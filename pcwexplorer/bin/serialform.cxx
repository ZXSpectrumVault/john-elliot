/************************************************************************

    PCW Explorer - access Amstrad PCW discs on Linux or Windows
        
    Copyright (C) 2000, 2006  John Elliott <jce@seasip.demon.co.uk>

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

#include "pcwplore.hxx"
#include "serialform.hxx"

#ifdef __WXMSW__
static const wxString ports[] = {"COM1:", "COM2:", "COM3:", "COM4:" };
#else
static const wxString ports[] = {"/dev/ttyS0", "/dev/ttyS1", "/dev/ttyS2", 
				"/dev/ttyS3" };
#endif

static const wxString bauds[] = {"50", "110", "134", "150", "300", "600",
				"1200", "1800", "2400", "4800", "9600",
				"19200", "38400", "57600", "115200" };

#define BAUDCOUNT (sizeof(bauds) / sizeof(bauds[0]))

BEGIN_EVENT_TABLE(SerialForm, wxDialog)
	EVT_BUTTON(wxID_OK, SerialForm::OnOK)
END_EVENT_TABLE()


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

SerialForm::SerialForm(wxWindow* parent, wxString filename) :
	wxDialog(parent, -1, "Serial connection settings", 
			wxDefaultPosition, wxSize(400, 190))
{
	m_string = filename;
	(void) new wxStaticText(this, -1, "Port to use:", wxPoint(5,5));
	(void) new wxStaticText(this, -1, "Data rate:", wxPoint(5,35));
	(void) new wxStaticText(this, -1, "Remote file/device name:", wxPoint(5,65));
	(void) new wxStaticText(this, -1, "Remote drive type:", wxPoint(5,95));

	m_portbox = new wxComboBox(this, COMBO_PORT, "", wxPoint(170, 5),
			wxSize(200, 20), 4, ports, wxCB_DROPDOWN);
	m_baudbox = new wxComboBox(this, COMBO_BAUD, "", wxPoint(170,35),
			wxSize(70, 20), BAUDCOUNT, bauds, wxCB_DROPDOWN);
	m_crtscts = new wxCheckBox(this, BUTTON_CRTSCTS, 
		"RTS/CTS handshaking", wxPoint(240, 35), wxSize(150,20));
	m_namebox = new wxTextCtrl(this, EDIT_NAME, "", wxPoint(170, 65),
			wxSize(200, 20));
	m_typebox = new wxComboBox(this, COMBO_TYPE, "", wxPoint(170, 95),
			wxSize(200, 20), 0, NULL, wxCB_DROPDOWN);

	(void)new wxButton(this, wxID_CANCEL, "Cancel", wxPoint(220, 125), wxSize(80, 30));
	(void)new wxButton(this, wxID_OK, "OK", wxPoint(310, 125), wxSize(80, 30));

	int idx = 0;
	char *drvname;

	m_typebox->Append("Auto detect", (void *)NULL);
	do
	{
		dsk_type_enum(idx++, &drvname);
		if (drvname)
		{
			m_typebox->Append(drvname, drvname);
		}
	} while (drvname);

	if (m_string.Left(7) == "serial:")
	{
		wxString details, port, baud, baud1, type;

		details = m_string.AfterFirst(':');
		port = details.BeforeFirst(',');
		details = details.AfterFirst(',');
		m_portbox->SetValue(port);
		baud = details.BeforeFirst(',');
		baud1 = baud.BeforeFirst('+'); if (baud1 != "") baud = baud1;	
		baud1 = baud.BeforeFirst('-'); if (baud1 != "") baud = baud1;	
		m_baudbox->SetValue(baud);
		baud1 = details.BeforeFirst(',');
		if      (baud1.Find("+crtscts") >= 0) m_crtscts->SetValue(1);
		else if (baud1.Find("-crtscts") >= 0) m_crtscts->SetValue(0);
		else				      m_crtscts->SetValue(1); 
		details = details.AfterFirst(',');
		m_namebox->SetValue(details.BeforeFirst(','));
		type = details.AfterFirst(',');
		if (type == "")
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
		m_baudbox->SetSelection(m_baudbox->FindString("9600"));
		m_crtscts->SetValue(1);
		m_namebox->SetValue("");
		m_typebox->SetSelection(0);
	}
}

SerialForm::~SerialForm()
{

}


void SerialForm::OnOK(wxCommandEvent &ev)
{
	char *type;

	m_string = "serial:";
	m_string += m_portbox->GetValue();
	m_string += ",";
	m_string += m_baudbox->GetValue();
	if (m_crtscts->GetValue()) m_string += "+crtscts";
	else			   m_string += "-crtscts";
	m_string += ",";
	m_string += m_namebox->GetValue();

	int sel = m_typebox->FindString(m_typebox->GetValue());
	if (sel > -1)
	{
		type = (char *)m_typebox->GetClientData(sel);
		if (type)
		{
			m_string += ",";
			m_string += type;
		}
	}
	else
	{
		m_string += ",";
		m_string += m_typebox->GetValue();
	}

	wxDialog::OnOK(ev);
}
