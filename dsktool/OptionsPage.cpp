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
#include "OptionsPage.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////


BEGIN_EVENT_TABLE(OptionsPage, wxWizardPageSimple)
    EVT_WIZARD_PAGE_CHANGING(-1, OptionsPage::OnWizardPageChanging)
	EVT_CHECKBOX(CHECK_PARTCOPY, OptionsPage::OnCheckPartCopy)
END_EVENT_TABLE()

OptionsPage::OptionsPage(wxWizard* parent, wxWizardPage* prev, wxWizardPage* next, const wxBitmap&bitmap) :
	wxWizardPageSimple(parent, prev, next, bitmap)
{
	m_md3     = false;
	m_logical = false;
	m_apricot = false;
	m_format  = true;

	m_check[0] = new wxCheckBox(this, -1, wxT("Format while copying"),          wxPoint(5, 5), wxSize(250, 20));
	m_check[1] = new wxCheckBox(this, -1, wxT("MicroDesign 3 copy protection"), wxPoint(5,25), wxSize(250, 20));
	m_check[2] = new wxCheckBox(this, -1, wxT("Reorder logical sectors"),       wxPoint(5,45), wxSize(250, 20));
	m_check[3] = new wxCheckBox(this, -1, wxT("Rewrite Apricot boot sector"),   wxPoint(5,65), wxSize(250, 20));
	m_check[4] = new wxCheckBox(this, CHECK_PARTCOPY, wxT("Partial copy"),      wxPoint(5,85), wxSize(250, 20));	

	m_check[0]->SetValue(m_format);

	m_first = wxT("0");
	m_last  = wxT("79");
	(void)new wxStaticText(this, -1, wxT("First cylinder:"), wxPoint(25,107), wxSize(100, 20));
	m_text[0] = new wxTextCtrl(this, -1, m_first, wxPoint(125,105), wxSize(40, 24), 0, wxTextValidator(wxFILTER_NUMERIC, &m_first));
	(void)new wxStaticText(this, -1, wxT("Last cylinder:"), wxPoint(25, 132), wxSize(100, 20));
	m_text[1] = new wxTextCtrl(this, -1, m_last, wxPoint(125, 135), wxSize(40, 24), 0, wxTextValidator(wxFILTER_NUMERIC, &m_last));
	
	m_text[0]->Enable(m_check[3]->GetValue());
	m_text[1]->Enable(m_check[3]->GetValue());
}

OptionsPage::~OptionsPage()
{

}


void OptionsPage::OnWizardPageChanging(wxWizardEvent& event)
{
	m_format  = m_check[0]->GetValue();
	m_md3     = m_check[1]->GetValue();
	m_logical = m_check[2]->GetValue();
	m_apricot = m_check[3]->GetValue();
	if (!m_check[4]->GetValue())
	{
		m_first = wxT("");
		m_last  = wxT("");
	}

}


void OptionsPage::OnCheckPartCopy(wxCommandEvent &event)
{
	m_text[0]->Enable(m_check[4]->GetValue());
	m_text[1]->Enable(m_check[4]->GetValue());
}


int OptionsPage::GetFirst()
{
	long val;

	if (m_first.IsEmpty()) return -1;
	if (!m_first.ToLong(&val)) return -1;

	return (int)val;	
}

int OptionsPage::GetLast()
{
	long val;

	if (m_last.IsEmpty()) return -1;
	if (!m_last.ToLong(&val)) return -1;

	return (int)val;	
}


