

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
    #pragma implementation "file_cut.cxx"
    #pragma interface "file_cut.hxx"
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
    #include "Xpm/file_cut.xpm"
    #include "Xpm/file_cut_small.xpm"
#endif

PcwCutFactory::PcwCutFactory() : PcwGraphicFactory()
{
        m_handler = new CutImageHandler();
        wxImage::AddHandler(m_handler);
}

PcwCutFactory::~PcwCutFactory()
{
}


wxIcon PcwCutFactory::GetBigIcon()
{
#if defined(__WXGTK__) || defined(__WXMOTIF__) || defined(__WXMSW__)
	return wxIcon(file_cut_xpm);
#endif
}


wxIcon PcwCutFactory::GetSmallIcon()
{
#if defined(__WXGTK__) || defined(__WXMOTIF__) || defined(__WXMSW__)
        return wxIcon(file_cut_small_xpm);
#endif
}


bool PcwCutFactory::Identify(const char *filename, const unsigned char *first128)
{
	UNUSED(first128)
	if (strstr(filename, ".cut")) return true;

	return false;
}


wxSize PcwCutFactory::GetDimensions(const char *filename)
{
	struct cpmInode ino;
        struct cpmFile  file;
        unsigned char   cuthdr[4];
	int w,h;

	memset(&cuthdr, 0, sizeof(cuthdr));
        if (cpmNamei(GetRoot(), filename, &ino) == -1) return wxDefaultSize;

        if (cpmOpen(&ino, &file, O_RDONLY)) return wxDefaultSize;
	cpmRead(&file, (char *)cuthdr, 4);
	cpmClose(&file);
       
	h = (cuthdr[0] + 256 * cuthdr[1]) / 2;
	w = cuthdr[2] + 256 * cuthdr[3];
 
	return wxSize(w,h);
}

char *PcwCutFactory::GetTypeName()
{
	return "Stop Press CUTout";
}

int PcwCutFactory::GetImageType()
{
        return wxBITMAP_TYPE_CUT;
}



PcwFileFactory *GetCutFileClass(void)
{
	return new PcwCutFactory();
}
