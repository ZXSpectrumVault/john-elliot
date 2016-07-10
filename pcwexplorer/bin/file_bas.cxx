

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
    #pragma implementation "file_bas.cxx"
    #pragma interface "file_bas.hxx"
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
#include "prop_bas.hxx"

#if defined(__WXGTK__) || defined(__WXMOTIF__) || defined(__WXMSW__)
    #include "Xpm/file_basic.xpm"
    #include "Xpm/file_basic_small.xpm"
#endif



BasFileFactory::BasFileFactory()
{
}

BasFileFactory::~BasFileFactory()
{
}


wxIcon BasFileFactory::GetBigIcon()
{
#if defined(__WXGTK__) || defined(__WXMOTIF__) || defined(__WXMSW__)
        return wxIcon(file_basic_xpm);
#endif
}

wxIcon BasFileFactory::GetSmallIcon()
{
#if defined(__WXGTK__) || defined(__WXMOTIF__) || defined(__WXMSW__)
        return wxIcon(file_basic_small_xpm);
#endif
}

bool BasFileFactory::Identify(const char *filename, const unsigned char *first128)
{
	UNUSED(first128)
  	if (strstr(filename, ".bas")) return true;
        if (strstr(filename, ".seb")) return true;	/* +3DOS self-extractor */
	return false;
}

char *BasFileFactory::GetTypeName()
{
	return "BASIC program";
}

void BasFileFactory::AddPages(wxNotebook *notebook, char const *filename)
{
        PcwFileFactory::AddPages(notebook, filename);

        PropPageBas *pbas = new PropPageBas(filename, GetRoot(),
                              notebook, PROP_PROGRAM);

        notebook->AddPage(pbas, "Program");
}



PcwFileFactory *GetBasFileClass()
{
	return new BasFileFactory;
}

