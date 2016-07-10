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

#if !defined(AFX_ABOUTBOX_H__2425A810_9B23_43AA_BF0D_3D4949C17822__INCLUDED_)
#define AFX_ABOUTBOX_H__2425A810_9B23_43AA_BF0D_3D4949C17822__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

/////////////////////////////////////////////////////////////////////////////
// AboutBox dialog used for App About


class AboutBox : public wxDialog
{
public:
	AboutBox(wxWindow *pFrame);
	~AboutBox();

	wxListBox	    *m_lbLicence;
	wxRect		m_rect2;
	wxRect		m_rect1;
	wxBrush m_brGrey, m_brBlack, m_brWhite, m_brStipple;
    wxBitmap m_bmpPattern;
	wxButton *m_bnOK;
// Implementation
protected:
	void OnSysColourChanged(wxSysColourChangedEvent& event);
	void OnEraseBackground(wxEraseEvent& event);
	DECLARE_EVENT_TABLE()
};



#endif // !defined(AFX_ABOUTBOX_H__2425A810_9B23_43AA_BF0D_3D4949C17822__INCLUDED_)
