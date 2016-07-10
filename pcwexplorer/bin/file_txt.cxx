

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
#ifdef __GNUG__
    #pragma implementation "file_txt.cxx"
    #pragma interface "file_txt.hxx"
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

#include "filetype.hxx"

#if defined(__WXGTK__) || defined(__WXMOTIF__) || defined(__WXMSW__)
    #include "Xpm/file_txt.xpm"
    #include "Xpm/file_txt_small.xpm"
#endif



TxtFileFactory::TxtFileFactory()
{
}

TxtFileFactory::~TxtFileFactory()
{
}


wxIcon TxtFileFactory::GetBigIcon()
{
#if defined(__WXGTK__) || defined(__WXMOTIF__) || defined(__WXMSW__)
        return wxIcon(file_txt_xpm);
#endif
}

wxIcon TxtFileFactory::GetSmallIcon()
{
#if defined(__WXGTK__) || defined(__WXMOTIF__) || defined(__WXMSW__)
        return wxIcon(file_txt_small_xpm);
#endif
}

bool TxtFileFactory::Identify(const char *filename, const unsigned char *first128)
{
   UNUSED(first128)

	if (strstr(filename, ".txt") || 
            strstr(filename, ".doc") ||
            strstr(filename, ".me" ) ||
            strstr(filename, "readme")) return true;
	return false;
}

char *TxtFileFactory::GetTypeName()
{
	return "Text document";
}

//	virtual void AddPages(wxNotebook *notebook, const char *filename);

wxWindow *TxtFileFactory::GetViewport(wxDialog *parent, const char *filename)
{
	return PcwFileFactory::GetTextViewport(parent, filename);
}

PcwFileFactory *GetTxtFileClass()
{
	return new TxtFileFactory;
}

