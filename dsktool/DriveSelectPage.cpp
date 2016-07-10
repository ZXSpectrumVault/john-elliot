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

#include "wx/wizard.h"

#include "dsktool.h"
#include "DriveSelectPage.h"
#include "SerialForm.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

BEGIN_EVENT_TABLE(DriveSelectPage, wxWizardPageSimple)
	EVT_BUTTON(BTN_BROWSE, DriveSelectPage::OnBrowse)
	EVT_BUTTON(BTN_SERIAL, DriveSelectPage::OnSerial)
	EVT_COMBOBOX(COMBO_NAME, DriveSelectPage::OnComboName) 
    EVT_WIZARD_PAGE_CHANGING(-1, DriveSelectPage::OnWizardPageChanging)
END_EVENT_TABLE()

#ifdef __WXMSW__
static const wxString choices[] = {wxT("A:"), wxT("B:") };
#else
static const wxString choices[] = {wxT("/dev/fd0"), wxT("/dev/fd1") };
#endif

static const wxString choice2[] = {wxT("Auto Sides"), wxT("Side 1"), wxT("Side 2") };

DriveSelectPage::DriveSelectPage(PageType pt, wxWizard* parent, wxWizardPage* prev, wxWizardPage* next, const wxBitmap&bitmap) :
	m_pageType(pt), wxWizardPageSimple(parent, prev, next, bitmap)
	
{
	intptr_t idx;
	dsk_err_t err;
	char *drvname = NULL;
	char *compname = NULL;
	const char *formname = NULL;
	const char *formdesc = NULL;
	m_typebuf = NULL;
	m_compbuf = NULL;

	m_doubleStep = false;
	m_forceHead  = -1;
	m_retries = wxT("1");

	switch(m_pageType)
	{
		case PT_READ: 
			(void)new wxStaticText(this, -1, wxT("Drive / Disc image to READ:"), wxPoint(5, 5));
			break;
		case PT_WRITE: 
			(void)new wxStaticText(this, -1, wxT("Drive / Disc image to WRITE:"), wxPoint(5, 5));
			break;
		default: 
			(void)new wxStaticText(this, -1, wxT("Select drive / disc image:"), wxPoint(5, 5));
			break;
	}
	(void)new wxStaticText(this, -1, wxT("Drive / file type:"), wxPoint(5, 55));
	(void)new wxStaticText(this, -1, wxT("Compression:"), wxPoint(5, 105));
	if (m_pageType == PT_FORMAT)
	{
		(void)new wxStaticText(this, -1, wxT("Use format:"), wxPoint(5, 155));
	}
	else if (m_pageType != PT_WRITE)
	{
		(void)new wxStaticText(this, -1, wxT("Override format:"), wxPoint(5, 155));
	}
	(void)new wxStaticBox(this, -1, wxT("Floppy drive options"), wxPoint(5, 205), wxSize(330, 50));

	m_drivebox = new wxComboBox(this, COMBO_NAME, wxT(""), wxPoint(10, 25),  wxSize(200, 20), 2, choices, wxCB_DROPDOWN);
	m_browse   = new wxButton  (this, BTN_BROWSE, wxT("Browse"), wxPoint(220, 22), wxSize(70, 30));
	m_serial   = new wxButton  (this, BTN_SERIAL, wxT("Serial"), wxPoint(292, 22), wxSize(70, 30));
	m_typebox  = new wxComboBox(this, -1, wxT(""), wxPoint(10, 75), wxSize(200, 20), 0, NULL, wxCB_READONLY);
 	m_compbox  = new wxComboBox(this, -1, wxT(""), wxPoint(10, 125), wxSize(200, 20), 0, NULL, wxCB_READONLY);
 	m_formbox  = new wxComboBox(this, -1, wxT(""), wxPoint(10, 175), wxSize(200, 20), 0, NULL, wxCB_READONLY);
	m_head     = new wxComboBox(this, -1, wxT(""), wxPoint(10, 225), wxSize(100,20), 3, choice2, wxCB_READONLY);
	m_dstep    = new wxCheckBox(this, -1, wxT("Double-step"), wxPoint(120, 225), wxSize(100,20));
	m_retrybox = new wxTextCtrl(this, -1, m_retries, wxPoint(225, 223), wxSize(40, 24), 0, wxTextValidator(wxFILTER_NUMERIC, &m_retries));
	(void)new wxStaticText(this, -1, wxT("retries"), wxPoint(267,225));

	m_head->SetSelection(0);
	switch (m_pageType)
	{
		case PT_FORMAT: m_format = FMT_720K; break;
		case PT_WRITE:  m_format = FMT_UNKNOWN; 
						m_formbox->Show(false);
						m_formbox->Enable(false);
						break;
		case PT_READ:
		case PT_IDENTIFY:
						m_formbox->Append(wxT("Auto detect"), (void *)(int)FMT_UNKNOWN);
						m_formbox->SetSelection(0);
						m_format = FMT_UNKNOWN;
						break;
	}
	if (m_pageType == PT_READ || m_pageType == PT_IDENTIFY)
	{
		m_typebox->Append(wxT("Auto detect"), (void *)NULL);
		m_typebox->SetSelection(0);
		m_compbox->Append(wxT("Auto detect"), (void *)NULL);
		m_compbox->SetSelection(0);

		m_type     = wxT("");
		m_compress = wxT("");
	}
	else
	{
		m_compbox->Append(wxT("No compression"), (void *)NULL);
		m_compbox->SetSelection(0);
		m_compress = wxT("");
	}
	idx = 0;
	do
	{
		dsk_type_enum(idx++, &drvname);
		if (drvname)
		{
			m_typebox->Append(wxString(drvname, wxConvUTF8), drvname);
		}

	} while (drvname);
	idx = 0;
	do
	{
		dsk_comp_enum(idx++, &compname);
		if (compname)
		{
			m_compbox->Append(wxString(compname, wxConvUTF8), compname);
		}
	} while (compname);
	idx =  0;
	do
	{
		err = dg_stdformat(NULL, (dsk_format_t)idx, &formname, &formdesc);
		if (err == DSK_ERR_OK)
		{
			m_formbox->Append(wxString(formdesc, wxConvUTF8), 
					(void *)idx);			
		}
		++idx;
	}
	while (err == DSK_ERR_OK);

	if (m_pageType == PT_FORMAT)
	{
		m_formbox->SetSelection((int)m_format);
	}
	if (m_pageType == PT_FORMAT || m_pageType == PT_WRITE)
	{
		int pos = m_typebox->FindString(wxT("dsk"));
		if (pos != -1) 
		{
			m_typebox->SetSelection(pos);
		}
		m_type     = wxT("dsk");
	}
}


DriveSelectPage::~DriveSelectPage()
{
	if (m_typebuf) delete [] m_typebuf;
	if (m_compbuf) delete [] m_compbuf;
}

void DriveSelectPage::OnSerial(wxCommandEvent &event)
{
	SerialForm sf(this, m_filename);

	if (sf.ShowModal() == wxID_OK)
	{
		m_filename = sf.m_string;
		m_drivebox->SetValue(sf.m_string);
		int pos = m_typebox->FindString(wxT("remote"));
		if (pos != -1) 
		{
			m_typebox->SetSelection(pos);
			m_type = wxT("remote");
		}
	}
}



void DriveSelectPage::OnBrowse(wxCommandEvent &event)
{
	wxString filename;

	switch (m_pageType)
	{
		case PT_FORMAT: 
			filename = wxFileSelector(wxT("Choose a disc image file to format"), wxT(""), wxT(""), wxT(""), wxT("*.*"), wxFD_SAVE | wxFD_OVERWRITE_PROMPT, this);
			break;
		case PT_WRITE: 
			filename = wxFileSelector(wxT("Choose a disc image file to write to"), wxT(""), wxT(""), wxT(""), wxT("*.*"), wxFD_SAVE | wxFD_OVERWRITE_PROMPT, this);
			break;
		case PT_READ:
			filename = wxFileSelector(wxT("Choose a disc image file to open"), wxT(""), wxT(""), wxT(""), wxT("*.*"), wxFD_OPEN, this);
			break;
		case PT_IDENTIFY:
			filename = wxFileSelector(wxT("Choose a disc image file to open"), wxT(""), wxT(""), wxT(""), wxT("*.*"), wxFD_OPEN, this);
			break;
	}
	if (!filename.empty())
	{
		m_filename = filename;
		m_drivebox->SetValue(filename);
	}
}

void DriveSelectPage::OnWizardPageChanging(wxWizardEvent& event)
{
	m_filename = m_drivebox->GetValue();
	m_type     = wxString((char *)m_typebox->GetClientData(m_typebox->GetSelection()), wxConvUTF8);
	m_compress = wxString((char *)m_compbox->GetClientData(m_compbox->GetSelection()), wxConvUTF8);
	if (m_pageType == PT_FORMAT)
		 m_format   = (dsk_format_t)(m_formbox->GetSelection());
	else m_format   = (dsk_format_t)(m_formbox->GetSelection() - 1);

	m_forceHead  = m_head->GetSelection() - 1;
	m_doubleStep = m_dstep->GetValue();

	if (m_filename.IsEmpty() && event.GetDirection())
	{
		wxMessageBox(_T("You have not entered a filename."), _T("Choose disc"),
             wxICON_WARNING | wxOK, this);

        event.Veto();
	}
}


//
// If they chose a floppy drive from the drop list, switch 
// the 'type' combo over as well.
//
void DriveSelectPage::OnComboName(wxCommandEvent &event)
{
	wxString selflop;
	selflop = wxT("floppy");

	if (m_pageType == PT_FORMAT || m_pageType == PT_WRITE)
	{
#ifdef __WXMSW__
		// Win32 has 2 possible floppy drivers; see if the NTWDM one is installed.
		HANDLE hdriver = CreateFile("\\\\.\\fdrawcmd", GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
		if (hdriver != INVALID_HANDLE_VALUE) 
		{
			selflop = wxT("ntwdm");
		}
		CloseHandle(hdriver);
#endif
		int pos = m_typebox->FindString(selflop);
		if (pos != -1) 
		{
			m_typebox->SetSelection(pos);
		}
	}
}


dsk_err_t DriveSelectPage::OpenDisc(DSK_PDRIVER *pdsk)
{
	wxString &filename = GetFilename();
	wxCharBuffer buf(filename.mb_str(wxConvUTF8));

	dsk_err_t err = dsk_open(pdsk, buf, GetType(), GetCompress());
	if (!err) 
	{
		long retries;
		if (!m_retries.ToLong(&retries)) retries = 1;

		dsk_set_retry(*pdsk, retries);
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



dsk_err_t DriveSelectPage::CreateDisc(DSK_PDRIVER *pdsk)
{
	wxString &filename = GetFilename();
	wxCharBuffer buf(filename.mb_str(wxConvUTF8));

	dsk_err_t err = dsk_creat(pdsk, buf, GetType(), GetCompress());
	if (!err) 
	{
		long retries;
		if (!m_retries.ToLong(&retries)) retries = 1;

		dsk_set_retry(*pdsk, retries);
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

const char *DriveSelectPage::GetType()
{
	if (m_type.IsEmpty()) return NULL;
	if (m_typebuf) delete [] m_typebuf;
	wxCharBuffer buf(m_type.mb_str(wxConvUTF8));
	m_typebuf = new char[1 + strlen(buf)];
	strcpy(m_typebuf, buf);
	return m_typebuf;
}


const char *DriveSelectPage::GetCompress()
{
	if (m_compress.IsEmpty()) return NULL;
	if (m_compbuf) delete [] m_compbuf;
	wxCharBuffer buf(m_compress.mb_str(wxConvUTF8));
	m_compbuf = new char[1 + strlen(buf)];
	strcpy(m_compbuf, buf);
	return m_compbuf;
}

int DriveSelectPage::GetRetries()
{
	long result;

	if (!m_retries.ToLong(&result)) result = 1;
	
	return result;
}
