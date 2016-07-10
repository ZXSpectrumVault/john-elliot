

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
    #pragma implementation "app.cxx"
    #pragma interface "app.hxx"
#endif

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWindows headers)
#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif
#include "wx/splitter.h"

// ----------------------------------------------------------------------------
// private classes
// ----------------------------------------------------------------------------

#include "app.hxx"
#include "filetype.hxx"
#include "frame.hxx"
//#include "drvdlg.hxx"
#include "drivechooser.hxx"

// ----------------------------------------------------------------------------
// event tables and other macros for wxWindows
// ----------------------------------------------------------------------------

// Create a new application object: this macro will allow wxWindows to create
// the application object during program execution (it's better than using a
// static object for many reasons) and also declares the accessor function
// wxGetApp() which will return the reference of the right type (i.e. MyApp and
// not wxApp)
IMPLEMENT_APP(PcwApp)


#if 0

/* Workaround */
unsigned long wxDialog::OnCtlColor(unsigned long a, unsigned long b, 
				   unsigned int c, unsigned int d,
				   unsigned int e, long f)
{
	return wxPanel::OnCtlColor(a,b,c,d,e,f);

}
#endif

// ============================================================================
// implementation
// ============================================================================

// These are just here so I can see if they're getting called.
PcwApp::PcwApp()
{
//	::MessageBox(NULL, "Test 1", "Test", MB_OK);
//	DebugBreak();
}

bool PcwApp::OnInitGui()
{
//	::MessageBox(NULL, "Test 2", "Test", MB_OK);
	return wxApp::OnInitGui();
}

// ----------------------------------------------------------------------------
// the application class
// ----------------------------------------------------------------------------

// `Main program' equivalent: the program execution "starts" here
bool PcwApp::OnInit()
{
    bool done = FALSE;

//  ::MessageBox(NULL, "Test 3", "Test", MB_OK);
    wxImage::InitStandardHandlers();

    // Enable PNGs
#if defined(__WXGTK__) || defined(__WXMSW__)
  wxImage::AddHandler(new wxPNGHandler);
  wxImage::AddHandler(new wxJPEGHandler);
#endif

    // Create the main application window
    PcwFrame *frame = new PcwFrame("PCW Explorer",
                                 wxPoint(50, 50), wxSize(450, 340));

    // Show it and tell the application that it's our main window
    frame->Show(TRUE);
    SetTopWindow(frame);

    // If a drive filename was passed, use that.
    if (argc >= 2)
    {
		dsk_err_t err;
		err = dsk_open(&frame->m_driver, argv[1], NULL, NULL);
		if (!err)
		{
			err = dsk_getgeom(frame->m_driver, &frame->m_geom);
		}
		if (err)
		{
	                wxString s = "Error on file ";
		       	s += argv[1];
			s += ":\n";
			s += dsk_strerror(err);
			wxMessageBox(s, "Failed to open", wxICON_STOP | wxOK);
		}
		else if (!frame->LoadDir()) done = TRUE;
    }
    if (!done) 
    {
	DriveSelectDialog dc(frame, 101);
	if (dc.ShowModal() != wxID_OK) return FALSE; 
	// Ask for a CP/M-formatted floppy
//	PcwDriveDialog drv(frame,  101);
//	if (drv.ShowModal() != wxID_OK) return FALSE; 

	// Load the directory from the floppy
	dsk_err_t err = dc.OpenDisc(&frame->m_driver);
	if (!err)
	{
        	if (dc.GetFormat() != FMT_UNKNOWN)
			err = dg_stdformat(&frame->m_geom, dc.GetFormat(), 
					NULL, NULL);
		else	err = dsk_getgeom(frame->m_driver, &frame->m_geom);

	}
	if (err)
	{
		wxString s = "Error on file " + dc.GetFilename() + ":\n";
			                        s += dsk_strerror(err);
                wxMessageBox(s, "Failed to open", wxICON_STOP | wxOK);
		return FALSE;
	}

    	if (frame->LoadDir()) return FALSE;
    } 
    // success: wxApp::OnRun() will be called which will enter the main message
    // loop and the application will run. If we returned FALSE here, the
    // application would exit immediately.
    return TRUE;
}

