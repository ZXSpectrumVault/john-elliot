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

class DriveSelectDialog : public wxDialog
{
public:
	DriveSelectDialog(wxWindow *parent, wxWindowID id,
			 const wxPoint& pos = wxDefaultPosition,
                         long style = wxDEFAULT_DIALOG_STYLE | wxCAPTION,
                         const wxString& name = "dialogBox");
	virtual ~DriveSelectDialog();

	inline wxString &GetFilename()   { return m_filename; }
	inline const char *GetType()     { return (m_type.IsEmpty()) ? NULL : m_type.c_str(); }
	inline const char *GetCompress() { return (m_compress.IsEmpty()) ? NULL : m_compress.c_str(); }
	inline dsk_format_t GetFormat()  { return m_format; }
	inline bool GetDoubleStep()      { return m_doubleStep; }
	inline int GetForceHead()        { return m_forceHead; }
	inline int GetRetries()          { return atoi(m_retries); }

	dsk_err_t OpenDisc(DSK_PDRIVER *pdsk);
	dsk_err_t CreateDisc(DSK_PDRIVER *pdsk);
private:
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

	void OnBrowse(wxCommandEvent &event);
	void OnSerial(wxCommandEvent &event);
	void OnOK(wxCommandEvent &event);
	void OnComboName(wxCommandEvent &event);

	DECLARE_EVENT_TABLE()
};

#define BTN_BROWSE 1
#define COMBO_NAME 2
#define BTN_SERIAL 3

