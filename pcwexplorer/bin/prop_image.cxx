

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
    #pragma implementation "prop_image.cxx"
    #pragma interface "prop_image.hxx"
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
#include "wx/statline.h"
// Our own header
#include "prop_image.hxx"

BEGIN_EVENT_TABLE(PropPageImage, wxWindow)
END_EVENT_TABLE()



PropPageImage::PropPageImage(const char *typedesc, const wxSize &imageSize,
                       wxWindow *parent, wxWindowID id,
                       const wxPoint& pos, 
                       const wxSize& size, 
                       long style, 
                       const wxString& name)
	: PcwPropPage(parent, id, pos, size, style, name)
{
    wxString strw, strh;

    strw.Printf("%d", imageSize.GetWidth());
    strh.Printf("%d", imageSize.GetHeight());

    int y = m_ruler_xy.y + m_rstop;
    (void)new wxStaticText  (this, -1, "Image type:", wxPoint(m_ruler_xy.x,    y));
    (void)new wxStaticText  (this, -1, typedesc,      wxPoint(m_filename_xy.x, y));
    y += m_tspacing;
    (void)new wxStaticText  (this, -1, "Width:",      wxPoint(m_ruler_xy.x,    y));
    (void)new wxStaticText  (this, -1, strw,          wxPoint(m_filename_xy.x, y));
    y += m_tspacing;
    (void)new wxStaticText  (this, -1, "Height:",     wxPoint(m_ruler_xy.x,    y));
    (void)new wxStaticText  (this, -1, strh,          wxPoint(m_filename_xy.x, y));
    y += m_tspacing + m_rsbot;
    (void)new wxStaticLine  (this, -1, wxPoint(m_ruler_xy.x, y), m_ruler_size);



}

