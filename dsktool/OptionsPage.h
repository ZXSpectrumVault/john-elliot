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

#if !defined(AFX_OPTIONSPAGE_H__459974D4_FEE4_4027_8CDC_16AD34BE5D9A__INCLUDED_)
#define AFX_OPTIONSPAGE_H__459974D4_FEE4_4027_8CDC_16AD34BE5D9A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class OptionsPage : public wxWizardPageSimple  
{
public:
	OptionsPage(wxWizard* parent, wxWizardPage* prev = NULL, wxWizardPage* next = NULL, const wxBitmap&bitmap = wxNullBitmap);
	virtual ~OptionsPage();

	int GetFirst();
	int GetLast();
	inline bool GetMD3()     { return m_md3; }
	inline bool GetLogical() { return m_logical; }
	inline bool GetApricot() { return m_apricot; }
	inline bool GetFormat()  { return m_format; }
private:
	bool m_md3;
	bool m_logical;
	bool m_apricot;
	bool m_format;
	wxString m_first, m_last;
	wxCheckBox *m_check[5];
	wxTextCtrl *m_text[2];

	void OnWizardPageChanging(wxWizardEvent& event);
	void OnCheckPartCopy(wxCommandEvent& event);	

	DECLARE_EVENT_TABLE()
};

#define CHECK_PARTCOPY 1

#endif // !defined(AFX_OPTIONSPAGE_H__459974D4_FEE4_4027_8CDC_16AD34BE5D9A__INCLUDED_)
