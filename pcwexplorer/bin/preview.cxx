

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
// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------
#ifdef __GNUG__
    #pragma implementation "preview.cpp"
    #pragma interface "preview.h"
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
#include "filetype.hxx"
#include "preview.hxx"

BEGIN_EVENT_TABLE(PreviewDialog, wxDialog)
END_EVENT_TABLE()


PreviewDialog::PreviewDialog(PcwFileFactory *factory, const char *filename,
                      wxWindow* parent, wxWindowID id, const wxString& title, 
                      const wxPoint& pos, const wxSize& size,
                      long style, const wxString& name) :
		wxDialog(parent, id, title, pos, size, style, name)
{
        // Add controls to the page
        wxBoxSizer *topsizer = new wxBoxSizer( wxVERTICAL );

	wxWindow *viewport = factory->GetViewport(this, filename);

        // Preview pane
        topsizer->Add( viewport, 1, wxEXPAND | wxLEFT|wxRIGHT, 15 );
        // Buttons
        topsizer->Add( CreateButtonSizer( wxOK), 0, wxCENTRE | wxALL, 10 );

        SetAutoLayout( TRUE );
        SetSizer( topsizer );

        topsizer->SetSizeHints( this );
}



