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

#include "wx/wizard.h"

#include "pcwplore.hxx"
#include "drivechooser.hxx"
#include "serialform.hxx"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

BEGIN_EVENT_TABLE(DriveSelectDialog, wxDialog)
	EVT_BUTTON(BTN_BROWSE, DriveSelectDialog::OnBrowse)
	EVT_BUTTON(BTN_SERIAL, DriveSelectDialog::OnSerial)
	EVT_BUTTON(wxID_OK, DriveSelectDialog::OnOK)
	EVT_COMBOBOX(COMBO_NAME, DriveSelectDialog::OnComboName) 
END_EVENT_TABLE()

#ifdef __WXMSW__
static const wxString choices[] = {"A:", "B:" };
#else
static const wxString choices[] = {"/dev/fd0", "/dev/fd1" };
#endif

static const wxString choice2[] = {"Auto Sides", "Side 1", "Side 2" };

DriveSelectDialog::DriveSelectDialog(wxWindow *parent, wxWindowID id,
	const wxPoint& pos, long style,
	const wxString &name) : wxDialog(parent, id, 
		"Choose drive or disc image to open", pos, wxSize(340, 330), 
		style, name)
{
	int idx;
	dsk_err_t err;
	char *drvname = NULL;
	char *compname = NULL;
	const char *formname = NULL;
	const char *formdesc = NULL;

	m_doubleStep = false;
	m_forceHead  = -1;
	m_retries = "1";

	(void)new wxStaticText(this, -1, "Select drive / disc image:", 
			       wxPoint(5, 5));
	(void)new wxStaticText(this, -1, "Drive / file type:", wxPoint(5, 55));
	(void)new wxStaticText(this, -1, "Compression:", wxPoint(5, 105));
	(void)new wxStaticText(this, -1, "Override format:", wxPoint(5, 155));
	(void)new wxStaticBox(this, -1, "Floppy drive options", wxPoint(5, 205), wxSize(330, 50));

	m_drivebox = new wxComboBox(this, COMBO_NAME, "", wxPoint(10, 25),  wxSize(200, 20), 2, choices, wxCB_DROPDOWN);
	m_browse   = new wxButton  (this, BTN_BROWSE, "Browse", wxPoint(220, 22), wxSize(70, 20));
	m_serial   = new wxButton  (this, BTN_SERIAL, "Serial", wxPoint(220, 42), wxSize(70, 20));
	m_typebox  = new wxComboBox(this, -1, "", wxPoint(10, 75), wxSize(200, 20), 0, NULL, wxCB_READONLY);
 	m_compbox  = new wxComboBox(this, -1, "", wxPoint(10, 125), wxSize(200, 20), 0, NULL, wxCB_READONLY);
 	m_formbox  = new wxComboBox(this, -1, "", wxPoint(10, 175), wxSize(200, 20), 0, NULL, wxCB_READONLY);
	m_head     = new wxComboBox(this, -1, "", wxPoint(10, 225), wxSize(100,20), 3, choice2, wxCB_READONLY);
	m_dstep    = new wxCheckBox(this, -1, "Double-step", wxPoint(120, 225), wxSize(100,20));
	m_retrybox = new wxTextCtrl(this, -1, m_retries, wxPoint(225, 223), wxSize(40, 24), 0, wxTextValidator(wxFILTER_NUMERIC, &m_retries));
	(void)new wxStaticText(this, -1, "retries", wxPoint(267,225));

	(void)new wxButton  (this, wxID_CANCEL, "Cancel", wxPoint(10, 270), wxSize(70, 20));
	(void)new wxButton  (this, wxID_OK, "OK", wxPoint(220, 270), wxSize(70, 20));
	m_head->SetSelection(0);
	m_formbox->Append("Auto detect", (void *)(int)FMT_UNKNOWN);
	m_formbox->SetSelection(0);
	m_format = FMT_UNKNOWN;

	m_typebox->Append("Auto detect", (void *)NULL);
	m_typebox->SetSelection(0);
	m_compbox->Append("Auto detect", (void *)NULL);
	m_compbox->SetSelection(0);

	m_type     = "";
	m_compress = "";
	idx = 0;
	do
	{
		dsk_type_enum(idx++, &drvname);
		if (drvname)
		{
			m_typebox->Append(drvname, drvname);
		}

	} while (drvname);
	idx = 0;
	do
	{
		dsk_comp_enum(idx++, &compname);
		if (compname)
		{
			m_compbox->Append(compname, compname);
		}
	} while (compname);
	idx =  0;
	do
	{
		err = dg_stdformat(NULL, (dsk_format_t)idx, &formname, &formdesc);
		if (err == DSK_ERR_OK)
		{
			m_formbox->Append(formdesc, (void *)idx);			
		}
		++idx;
	}
	while (err == DSK_ERR_OK);
}


DriveSelectDialog::~DriveSelectDialog()
{

}

void DriveSelectDialog::OnSerial(wxCommandEvent &event)
{
	SerialForm sf(this, m_filename);

	if (sf.ShowModal() == wxID_OK)
	{
		m_filename = sf.m_string;
		m_drivebox->SetValue(sf.m_string);
		int pos = m_typebox->FindString("remote");
		if (pos != -1) 
		{
			m_typebox->SetSelection(pos);
			m_type = "remote";
		}
	}
}



void DriveSelectDialog::OnBrowse(wxCommandEvent &event)
{
	wxString filename;

	filename = wxFileSelector("Choose a disc image file to open", "", "", "", "*.*", wxOPEN, this);
	if (!filename.empty())
	{
		m_filename = filename;
		m_drivebox->SetValue(filename);
	}
}

void DriveSelectDialog::OnOK(wxCommandEvent &event)
{
	m_filename = m_drivebox->GetValue();
	m_type     = (char *)m_typebox->GetClientData(m_typebox->GetSelection());
	m_compress = (char *)m_compbox->GetClientData(m_compbox->GetSelection());
	m_format   = (dsk_format_t)(m_formbox->GetSelection() - 1);

	m_forceHead  = m_head->GetSelection() - 1;
	m_doubleStep = m_dstep->GetValue();

	if (m_filename.IsEmpty())
	{
		wxMessageBox(_T("You have not entered a filename."), _T("Choose disc"),
             wxICON_WARNING | wxOK, this);

        	return;
	}
	wxDialog::OnOK(event);
}


//
// If they chose a floppy drive from the drop list, switch 
// the 'type' combo over as well.
//
void DriveSelectDialog::OnComboName(wxCommandEvent &event)
{
	wxString selflop;
	selflop = "floppy";
/*
	if (m_pageType == PT_FORMAT || m_pageType == PT_WRITE)
	{
#ifdef __WXMSW__
		// Win32 has 2 possible floppy drivers; see if the NTWDM one is installed.
		HANDLE hdriver = CreateFile("\\\\.\\fdrawcmd", GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
		if (hdriver != INVALID_HANDLE_VALUE) 
		{
			selflop = "ntwdm";
		}
		CloseHandle(hdriver);
#endif
		int pos = m_typebox->FindString(selflop);
		if (pos != -1) 
		{
			m_typebox->SetSelection(pos);
		}
	}
	*/
}


dsk_err_t DriveSelectDialog::OpenDisc(DSK_PDRIVER *pdsk)
{
	dsk_err_t err = dsk_open(pdsk, GetFilename(), GetType(), GetCompress());
	if (!err) 
	{
		dsk_set_retry(*pdsk, atoi(m_retries));
		if (GetForceHead() >= 0)
		{
			dsk_set_option(*pdsk, "HEAD", GetForceHead());
		}
		if (GetDoubleStep())
		{
			dsk_set_option(*pdsk, "DOUBLESTEP", 1);
		}
	}
	return err;
}



dsk_err_t DriveSelectDialog::CreateDisc(DSK_PDRIVER *pdsk)
{
	dsk_err_t err = dsk_creat(pdsk, GetFilename(), GetType(), GetCompress());
	if (!err) 
	{
		dsk_set_retry(*pdsk, atoi(m_retries));
		if (GetForceHead() >= 0)
		{
			dsk_set_option(*pdsk, "HEAD", GetForceHead());
		}
		if (GetDoubleStep())
		{
			dsk_set_option(*pdsk, "DOUBLESTEP", 1);
		}
	}
	return err;
}


