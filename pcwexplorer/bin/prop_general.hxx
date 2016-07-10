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
#include "proppage.hxx"

#define ATTRIB_RO   601
#define ATTRIB_SYS  602
#define ATTRIB_ARCV 603
#define ATTRIB_F1   604
#define ATTRIB_F2   605
#define ATTRIB_F3   606
#define ATTRIB_F4   607

class PropPageGeneral : public PcwPropPage
{
public:
	PropPageGeneral(wxIcon icon, const char *filename,
                       const char *typedesc, struct cpmInode *root,
                       wxWindow *parent, wxWindowID id, 
                       const wxPoint& pos = wxDefaultPosition, 
                       const wxSize& size = wxDefaultSize, 
                       long style = wxDEFAULT_DIALOG_STYLE | wxCAPTION, 
                       const wxString& name = "dialog");

	wxCheckBox *m_attrib[7];
	wxString m_filename;
	wxIcon   m_icon;
	struct cpmInode m_file;	

	virtual bool OnApply();
protected:
	DECLARE_EVENT_TABLE()
};

