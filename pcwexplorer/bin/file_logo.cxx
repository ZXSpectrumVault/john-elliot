

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
    #pragma implementation "file_logo.cxx"
    #pragma interface "file_logo.hxx"
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

#include "pcwplore.hxx"
#include "filetype.hxx"

#if defined(__WXGTK__) || defined(__WXMOTIF__) || defined(__WXMSW__)
    #include "Xpm/file_logo.xpm"
    #include "Xpm/file_logo_small.xpm"
#endif

PcwLogoFactory::PcwLogoFactory() : PcwGraphicFactory()
{
	m_handler = new LogoImageHandler();
	wxImage::AddHandler(m_handler);
}

PcwLogoFactory::~PcwLogoFactory()
{
}


wxIcon PcwLogoFactory::GetBigIcon()
{
#if defined(__WXGTK__) || defined(__WXMOTIF__) || defined(__WXMSW__)
	return wxIcon(file_logo_xpm);
#endif
}


wxIcon PcwLogoFactory::GetSmallIcon()
{
#if defined(__WXGTK__) || defined(__WXMOTIF__) || defined(__WXMSW__)
        return wxIcon(file_logo_small_xpm);
#endif
}


bool PcwLogoFactory::Identify(const char *filename, const unsigned char *first128)
{
   UNUSED(first128)
	if (strstr(filename, ".pic")) return true;

	return false;
}


wxSize PcwLogoFactory::GetDimensions(const char *filename)
{
   UNUSED(filename)
	return wxSize(720, 256);
}


int PcwLogoFactory::GetImageType()
{
	return wxBITMAP_TYPE_LOGO;
}

char *PcwLogoFactory::GetTypeName()
{
	return "DR LOGO picture";
}


PcwFileFactory *GetLogoFileClass(void)
{
	return new PcwLogoFactory();
}
