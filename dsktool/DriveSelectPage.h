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

#if !defined(AFX_DRIVESELECTPAGE_H__F4E26D0F_A38A_40AA_9DA8_BB1F09D03172__INCLUDED_)
#define AFX_DRIVESELECTPAGE_H__F4E26D0F_A38A_40AA_9DA8_BB1F09D03172__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

typedef enum
{
	PT_IDENTIFY,
	PT_FORMAT,
	PT_READ,
	PT_WRITE,
} PageType;

class DriveSelectPage : public wxWizardPageSimple  
{
public:
	DriveSelectPage(PageType pt, wxWizard* parent = NULL, wxWizardPage* prev = NULL, wxWizardPage* next = NULL, const wxBitmap&bitmap = wxNullBitmap);
	virtual ~DriveSelectPage();

	inline wxString &GetFilename()   { return m_filename; }
	const char *GetType();
	const char *GetCompress();
	inline dsk_format_t GetFormat()  { return m_format; }
	inline bool GetDoubleStep()      { return m_doubleStep; }
	inline int GetForceHead()        { return m_forceHead; }
	int GetRetries();

	dsk_err_t OpenDisc(DSK_PDRIVER *pdsk);
	dsk_err_t CreateDisc(DSK_PDRIVER *pdsk);
private:
	PageType m_pageType;
	wxString m_filename;
	wxString m_type;
	wxString m_compress;
	dsk_format_t m_format;
	bool m_doubleStep;
	int  m_forceHead;
	wxString m_retries;

	wxComboBox *m_drivebox;
	wxComboBox *m_compbox;
	wxComboBox *m_typebox;
	wxComboBox *m_formbox;
	wxCheckBox *m_dstep;
	wxComboBox *m_head;
	wxTextCtrl *m_retrybox;
	wxButton *m_browse;
	wxButton *m_serial;
	char	*m_typebuf;
	char	*m_compbuf;

	void OnBrowse(wxCommandEvent &event);
	void OnSerial(wxCommandEvent &event);
	void OnWizardPageChanging(wxWizardEvent& event);
	void OnComboName(wxCommandEvent &event);

	DECLARE_EVENT_TABLE()
};

#define BTN_BROWSE 1
#define COMBO_NAME 2
#define BTN_SERIAL 3

#endif // !defined(AFX_DRIVESELECTPAGE_H__F4E26D0F_A38A_40AA_9DA8_BB1F09D03172__INCLUDED_)
