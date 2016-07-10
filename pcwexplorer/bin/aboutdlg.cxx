

/* PCW Explorer - access Amstrad PCW discs on Linux or Windows
    Copyright (C) 2000,2006  John Elliott

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
    #pragma implementation "aboutdlg.cxx"
    #pragma interface "aboutdlg.hxx"
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

// Our own header
#include "aboutdlg.hxx"

BEGIN_EVENT_TABLE(AboutBox, wxDialog)
END_EVENT_TABLE()



AboutBox::AboutBox(const wxIcon &appIcon, wxWindow *parent, wxWindowID id, 
                       const wxPoint& pos, 
                       const wxSize& size, 
                       long style, 
                       const wxString& name)
	: wxDialog(parent, id, "About PCW Explorer", pos, size, style, name)
{
	wxButton *btnOK;

	(void)new wxStaticBitmap(this, -1, appIcon, wxPoint(10,10), wxSize(32,32));
	
	(void)new wxStaticText(this, -1, 
        	"PCW Explorer v0.3.0", wxPoint(45,10));
        (void)new wxStaticText(this, -1, 
        	"Copyright (C) 2000, 2006, John Elliott\n"
		"CP/M access code based on Michael Haardt's cpmtools\n"
		"GUI framework: wxWidgets\n\n", wxPoint(10,55));

	(void)new wxStaticText(this, -1, 
    "This program is free software; you can redistribute "
    "it and/or modify it under the\n"
    "terms of the GNU General Public License as published by the Free "
    "Software \n"
    "Foundation; either version 2 of the License, or (at your option) "
    "any later version.\n\n"

    "This program is distributed in the hope that it "
    "will be useful, but WITHOUT ANY\n"
    "WARRANTY; without even the implied warranty of "
    "MERCHANTABILITY or\n"
    "FITNESS FOR A PARTICULAR PURPOSE.  See the GNU "
    "General Public License \n"
    "for more details.\n\n"
  
    "You should have received a copy of the GNU General "
   "Public License along with \nthis program; if not, write "
   "to the Free Software Foundation, Inc., 59 Temple Place, \n"
   "Suite 330, Boston, MA  02111-1307  USA" , wxPoint(10, 125));


	btnOK = new wxButton(this, wxID_OK, "OK", wxPoint(200, 350), wxSize(80,25));
	btnOK->SetDefault();

    Centre(wxBOTH);
}


