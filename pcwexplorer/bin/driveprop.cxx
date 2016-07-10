

/* PCW Explorer - access Amstrad PCW discs on Linux or Windows
    Copyright (C) 2000, 2006  John Elliott

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
// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------
#ifdef __GNUG__
    #pragma implementation "driveprop.cxx"
    #pragma interface "driveprop.hxx"
#endif

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWindows headers
#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif
#include "filetype.hxx"
#include "proppage.hxx"
#include "prop_drive.hxx"
// Our own header
#include "driveprop.hxx"
#include "textmetric.hxx"

BEGIN_EVENT_TABLE(DrivePropSheet, wxDialog)
	EVT_BUTTON(wxID_APPLY, DrivePropSheet::OnApply)
	EVT_BUTTON(wxID_OK, DrivePropSheet::OnOK)
END_EVENT_TABLE()





DrivePropSheet::DrivePropSheet(struct cpmStatFS *buf, wxIcon &icon,
                       PcwFrame *parent, wxWindowID id, 
                       const wxPoint& pos, 
                       const wxSize& size, 
                       long style, 
                       const wxString& name)
	: wxDialog((wxWindow *)parent, (int)id, wxString("Drive properties"), pos, size, style, name)
{
    wxButton *btnOK /*, *btnCancel, *btnApply*/ ;

    wxPoint notebook_xy((metric_x * 3)/ 2 - 7, metric_d + 5);
    wxSize  notebook_size(size.GetWidth() - (5 * metric_x)/2 + 3,
                          size.GetHeight() - (metric_y * 4 + 2 * metric_d + 12));
    wxSize  button_size( ((metric_x - 2) * 25)/2 , metric_y + 2 * metric_d + 6);

    int right_x = notebook_size.x + notebook_xy.x + (metric_x - 2);

    m_notebook = new wxNotebook(this, DRVPROP_ID, notebook_xy,
                                                    notebook_size);
    int spacing = button_size.GetWidth() + (metric_x - 2);
#ifdef __WXMSW__
    int bspace  = (metric_y * 3 + 2 * metric_d + 12);
#else
    int bspace  = (metric_y * 3 + 2 * metric_d);
#endif

/* XXX nothing for "Apply" or "Cancel" to do!
    btnOK     = new wxButton(this, wxID_OK, "OK",
                 wxPoint(right_x - 3*spacing, size.GetHeight()-bspace), sizeButton);
    btnCancel = new wxButton(this, wxID_CANCEL, "Cancel",
                 wxPoint(right_x - 2*spacing, size.GetHeight()-bspace), sizeButton);
    btnApply = new wxButton(this, wxID_APPLY, "Apply",
                 wxPoint(right_x - spacing, size.GetHeight()-bspace), sizeButton);
*/
    btnOK     = new wxButton(this, wxID_OK, "OK",
                 wxPoint(right_x - spacing, size.GetHeight()-bspace), button_size);

    btnOK->SetDefault();

    m_notebook->AddPage(new PropPageDrive(buf, icon, m_notebook,
                        DRVPAGE_ID), "Free space");

    Centre(wxBOTH);

}


void DrivePropSheet::OnOK(wxCommandEvent &e)
{
	//if (!OnApply(e)) return;
	OnApply(e);
	wxDialog::OnOK(e);
}


//bool DrivePropSheet::OnApply(wxCommandEvent &)
void DrivePropSheet::OnApply(wxCommandEvent &)
{
	int c = m_notebook->GetPageCount();
	bool b;

	for (int n = 0; n < c; n++)
	{
		b = ((PropPageDrive *)(m_notebook->GetPage(n)))->OnApply();
		//if (!b) return false;	
	}	
	//return true;
}


