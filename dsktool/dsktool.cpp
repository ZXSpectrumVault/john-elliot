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


// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"
#ifdef __BORLANDC__
#pragma hdrstop
#endif

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include "dsktool.h"
#include "MainWindow.h"
#include "resource.h"

#include "wx/config.h"


IMPLEMENT_APP(DsktoolApp)


BEGIN_EVENT_TABLE(DsktoolApp, wxApp)
END_EVENT_TABLE()


DsktoolApp::DsktoolApp(void)
{
}

                               
bool DsktoolApp::OnInit()
{     
	SetAppName(wxT("Dsktool"));
	SetVendorName(wxT("jce@seasip"));

	m_config     = wxConfigBase::Create();	//new wxConfig("PSFEdit");


	//// Create the main window
	wxFrame *frame = new MainWindow(
                      wxT("Diskette Tools"), wxPoint(0, 0), wxSize(500, 400));

	frame->Centre(wxBOTH);
	frame->Show(TRUE);

	SetTopWindow(frame);

	return TRUE;
}

