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

#if !defined(AFX_MAINWINDOW_H__C7C373A0_E8DF_4072_8796_F3FD7A3095C5__INCLUDED_)
#define AFX_MAINWINDOW_H__C7C373A0_E8DF_4072_8796_F3FD7A3095C5__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class MainWindow  : public wxFrame 
{
public:
	MainWindow(const wxString& title, const wxPoint& pos, const wxSize& size,
            long style = wxDEFAULT_FRAME_STYLE);
	virtual ~MainWindow();

    // event handlers (these functions should _not_ be virtual)
    void OnQuit(wxCommandEvent& event);
    void OnAbout(wxCommandEvent& event);
    void OnIdentify(wxCommandEvent& event);
    void OnFormat(wxCommandEvent& event);
    void OnTransform(wxCommandEvent& event);
private:
    // any class wishing to process wxWindows events must use this macro
    DECLARE_EVENT_TABLE()

};


#endif // !defined(AFX_MAINWINDOW_H__C7C373A0_E8DF_4072_8796_F3FD7A3095C5__INCLUDED_)
