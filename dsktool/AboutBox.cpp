/************************************************************************

    Dsktool v1.0.5 - LibDsk front end

    Copyright (C) 2005, 2007, 2010  John Elliott <jce@seasip.demon.co.uk>

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
#include "AboutBox.h"

static const char gl_stipple[8] = { ~0x11, ~0x00, ~0x44, ~0x00, ~0x11, ~0x00, ~0x44, ~0x00 };


/////////////////////////////////////////////////////////////////////////////
// AboutBox dialog used for App About
static const wxString copying[] = 
{
    wxT(""),
    wxT("This program is free software; you can redistribute it and/or modify"),
    wxT("it under the terms of the GNU General Public License as published by"),
    wxT("the Free Software Foundation; either version 2 of the License, or"),
    wxT("(at your option) any later version."),
    wxT(""),
    wxT("This program is distributed in the hope that it will be useful, "),
    wxT("but WITHOUT ANY WARRANTY; without even the implied warranty of "),
    wxT("MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the "),
    wxT("GNU General Public License for more details."),
    wxT(""),
    wxT("You should have received a copy of the GNU General Public License"),
    wxT("along with this program; if not, write to the Free Software"),
    wxT("Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA"),
};

#include "res/dsktool48.xpm"

AboutBox::AboutBox(wxWindow *pFrame) : wxDialog(pFrame, -1, wxT("About Dsktool"),
	wxDefaultPosition, wxSize(692, 422), 
    wxDEFAULT_DIALOG_STYLE),
	m_bmpPattern(gl_stipple, 8, 8),
	m_brGrey (wxSystemSettings::GetColour(wxSYS_COLOUR_3DFACE), wxSOLID),
	m_brWhite(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW), wxSOLID),
	m_brBlack(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT), wxSOLID),
	m_rect1(16, 12, 664, 19),
	m_rect2(40, 40, 620, 301)
{                                                 
	m_brStipple.SetStipple(m_bmpPattern);
	SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW));

	m_lbLicence = new wxListBox(this, -1, wxPoint(41,110), wxSize(618, 230), 
			sizeof(copying) / sizeof(copying[0]), copying, wxBORDER_NONE);

	(void) new wxStaticBitmap(this, -1, wxBitmap(dsktool48_xpm), wxPoint(58, 50)); 
	(void) new wxStaticText(this, -1, wxT("About Dsktool"), wxPoint(286,14), wxSize(120,16), wxALIGN_CENTRE | wxST_NO_AUTORESIZE );
	(void) new wxStaticText(this, -1, wxT("Diskette Tool v1.0.5"), wxPoint(118,50), wxSize(342,16));
	(void) new wxStaticText(this, -1, wxT("Copyright (c) 2005, 2007, 2010 John Elliott <jce@seasip.demon.co.uk>"), wxPoint(118,70), wxSize(532,16));
	(void) new wxStaticText(this, -1, wxT("Built with LibDsk v" LIBDSK_VERSION), wxPoint(118,90), wxSize(532,16));
	m_bnOK = new wxButton(this, wxID_OK, wxT("OK"), wxPoint(590, 350), wxSize(70, 30));
	m_bnOK->SetDefault();
}

AboutBox::~AboutBox()
{
}


BEGIN_EVENT_TABLE(AboutBox, wxDialog)
	EVT_ERASE_BACKGROUND(AboutBox::OnEraseBackground)
	EVT_SYS_COLOUR_CHANGED(AboutBox::OnSysColourChanged)
END_EVENT_TABLE()



void AboutBox::OnSysColourChanged(wxSysColourChangedEvent& event)
{
	m_brGrey .SetColour(wxSystemSettings::GetColour(wxSYS_COLOUR_3DFACE));
	m_brWhite.SetColour(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW));
	m_brBlack.SetColour(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT));
	SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW));
	Refresh();

	wxWindow::OnSysColourChanged(event);
}


void AboutBox::OnEraseBackground(wxEraseEvent& event)
{
	wxPoint ptWnd;
	wxSize 	seWnd;

	ptWnd   = wxPoint(-1,-1);//GetPosition();
	seWnd   = GetClientSize();
	wxDC *pDC = event.GetDC();
	if (pDC)
	{
		pDC->SetBrush(m_brGrey);
		pDC->DrawRectangle(ptWnd.x, ptWnd.y, 2+seWnd.GetWidth(), 2+seWnd.GetHeight());
		pDC->SetBrush(m_brStipple);
		pDC->DrawRectangle(m_rect1.GetX(), m_rect1.GetY(), m_rect1.GetWidth(), m_rect1.GetHeight());
		pDC->SetBrush(m_brWhite);
		pDC->DrawRectangle(m_rect2.GetX(), m_rect2.GetY(), m_rect2.GetWidth(), m_rect2.GetHeight());
		pDC->SetBrush(wxNullBrush);
	}
	else
	{
		wxClientDC dc(this);
		dc.SetBrush(m_brGrey);
		dc.DrawRectangle(ptWnd.x, ptWnd.y, 2+seWnd.GetWidth(), 2+seWnd.GetHeight());
		dc.SetBrush(m_brStipple);
		dc.DrawRectangle(m_rect1.GetX(), m_rect1.GetY(), m_rect1.GetWidth(), m_rect1.GetHeight());
		dc.SetBrush(m_brWhite);
		dc.DrawRectangle(m_rect2.GetX(), m_rect2.GetY(), m_rect2.GetWidth(), m_rect2.GetHeight());
		dc.SetBrush(wxNullBrush);
	}	                            
}
