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
#include "IdentityForm.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

IdentityForm::IdentityForm(wxWindow* parent, const wxString &title, DSK_PDRIVER dsk, DSK_GEOMETRY *dg) :
	wxDialog(parent, -1, title, wxDefaultPosition, wxSize(400, 375))
{
   	const char *comp;
	char *comment;
	dsk_err_t err;
	unsigned char drv_status;

	memcpy(&m_dg, dg, sizeof(m_dg));

	comp = dsk_compname(dsk);
	(void)new wxStaticText(this, -1, wxT("Driver:"),        wxPoint(5,   5), wxSize(95,  25));
	(void)new wxStaticText(this, -1, wxString(dsk_drvdesc(dsk), wxConvUTF8), wxPoint(105, 5), wxSize(300, 25));
	(void)new wxStaticText(this, -1, wxT("Compression:"),   wxPoint(5,  25), wxSize(95,  25));
	(void)new wxStaticText(this, -1, 
		comp ? wxString(comp, wxConvUTF8) : wxT("None"),
		wxPoint(105,25), wxSize(300, 25));

	(void)new wxStaticText(this, -1, wxT("Sidedness:"),     wxPoint(5,  45), wxSize(95,  25));
	(void)new wxStaticText(this, -1, wxString::Format(wxT("%d"), m_dg.dg_sidedness), wxPoint(105,45), wxSize(300, 25));
	(void)new wxStaticText(this, -1, wxT("Cylinders:"),     wxPoint(5,  65), wxSize(95,  25));
	(void)new wxStaticText(this, -1, wxString::Format(wxT("%d"), m_dg.dg_cylinders), wxPoint(105,65), wxSize(300, 25));
	(void)new wxStaticText(this, -1, wxT("Heads:"),         wxPoint(5,  85), wxSize(95,  25));
	(void)new wxStaticText(this, -1, wxString::Format(wxT("%d"), m_dg.dg_heads), wxPoint(105,85), wxSize(300, 25));
	(void)new wxStaticText(this, -1, wxT("Sectors:"),       wxPoint(5, 105), wxSize(95,  25));
	(void)new wxStaticText(this, -1, wxString::Format(wxT("%d"), m_dg.dg_sectors), wxPoint(105,105), wxSize(300, 25));
	(void)new wxStaticText(this, -1, wxT("Sec. size:"),     wxPoint(5, 125), wxSize(95,  25));
	(void)new wxStaticText(this, -1, wxString::Format(wxT("%d"), m_dg.dg_secsize), wxPoint(105,125), wxSize(300, 25));
	(void)new wxStaticText(this, -1, wxT("1st sector:"),    wxPoint(5, 145), wxSize(95,  25));
	(void)new wxStaticText(this, -1, wxString::Format(wxT("%d"), m_dg.dg_secbase), wxPoint(105,145), wxSize(300, 25));
	(void)new wxStaticText(this, -1, wxT("Data rate:"),     wxPoint(5, 165), wxSize(95,  25));
	(void)new wxStaticText(this, -1, wxString::Format(wxT("%d"), m_dg.dg_datarate), wxPoint(105,165), wxSize(300, 25));
	(void)new wxStaticText(this, -1, wxT("Record mode:"),   wxPoint(5, 185), wxSize(95,  25));
	(void)new wxStaticText(this, -1, m_dg.dg_fm ? wxT("FM") : wxT("MFM"), wxPoint(105,185), wxSize(300, 25));

	(void)new wxStaticText(this, -1, wxT("R/W gap:"),       wxPoint(5, 205), wxSize(95,  25));
	(void)new wxStaticText(this, -1, wxString::Format(wxT("0x%02x"), m_dg.dg_rwgap), wxPoint(105,205), wxSize(300, 25));
	(void)new wxStaticText(this, -1, wxT("Format gap:"),    wxPoint(5, 225), wxSize(95,  25));
	(void)new wxStaticText(this, -1, wxString::Format(wxT("0x%02x"), m_dg.dg_fmtgap), wxPoint(105,225), wxSize(300, 25));
	err = dsk_drive_status(dsk, &m_dg, 0, &drv_status);
	(void)new wxStaticText(this, -1, wxT("Drive status:"),  wxPoint(5, 245), wxSize(95,  25));
	if (!err)
	{
		(void)new wxStaticText(this, -1, wxString::Format(wxT("0x%02x"), drv_status), wxPoint(105,245), wxSize(300, 25));
	}
	else
	{
		(void)new wxStaticText(this, -1, wxString(dsk_strerror(err), wxConvUTF8), wxPoint(105,245), wxSize(300, 25));
	}
	err = dsk_get_comment(dsk, &comment);
	if (err == DSK_ERR_OK && comment != NULL)
	{
		(void)new wxStaticText(this, -1, wxT("Comment:"),    wxPoint(5, 265), wxSize(95,  25));
		(void)new wxStaticText(this, -1, wxString(comment, wxConvUTF8), wxPoint(105,265), wxSize(300, 100));
	}
	(void)new wxButton(this, wxID_OK, wxT("OK"), wxPoint(160, 305), wxSize(80, 30));
}

IdentityForm::~IdentityForm()
{

}
