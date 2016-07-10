

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
    #pragma implementation "file_mda.cxx"
    #pragma interface "file_mda.hxx"
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
#include "prop_mda.hxx"

#if defined(__WXGTK__) || defined(__WXMOTIF__) || defined(__WXMSW__)
    #include "Xpm/file_mda.xpm"
    #include "Xpm/file_mda_small.xpm"
#endif

PcwMdaFactory::PcwMdaFactory() : PcwGraphicFactory()
{
	m_handler = new MdaImageHandler();
	wxImage::AddHandler(m_handler);
}

PcwMdaFactory::~PcwMdaFactory()
{
}


wxIcon PcwMdaFactory::GetBigIcon()
{
#if defined(__WXGTK__) || defined(__WXMOTIF__) || defined(__WXMSW__)
	return wxIcon(file_mda_xpm);
#endif
}


wxIcon PcwMdaFactory::GetSmallIcon()
{
#if defined(__WXGTK__) || defined(__WXMOTIF__) || defined(__WXMSW__)
        return wxIcon(file_mda_small_xpm);
#endif
}


bool PcwMdaFactory::Identify(const char *filename, const unsigned char *first128)
{
	UNUSED(first128);
	if (strstr(filename, ".mda") ||
            strstr(filename, ".mdp")) return true;

	return false;
}

void PcwMdaFactory::AddPages(wxNotebook *notebook, char const *filename)
{
        PcwFileFactory::AddPages(notebook, filename);

        PropPageMda *pimg = new PropPageMda(GetTypeName(), filename, GetRoot(),
                              notebook, PROP_IMAGE);

        notebook->AddPage(pimg, "Image");
}

wxSize PcwMdaFactory::GetDimensions(const char *filename)
{
        struct cpmInode ino;
        struct cpmFile  file;
        unsigned char   mdahdr[132];
        int w,h;

        memset(&mdahdr, 0, sizeof(mdahdr));
        if (cpmNamei(GetRoot(), filename, &ino) == -1) return wxDefaultSize;

        if (cpmOpen(&ino, &file, O_RDONLY)) return wxDefaultSize;
        cpmRead(&file, (char *)mdahdr, 132);
        cpmClose(&file);

	if (memcmp(mdahdr, ".MDA", 4)) return wxDefaultSize;

        h = mdahdr[128] + 256 * mdahdr[129];
        w = 8 * mdahdr[130] + 256 * mdahdr[131];

        return wxSize(w,h);
}


int PcwMdaFactory::GetImageType()
{
	return wxBITMAP_TYPE_MDA;
}

char *PcwMdaFactory::GetTypeName()
{
	return "MicroDesign Area";
}


PcwFileFactory *GetMdaFileClass(void)
{
	return new PcwMdaFactory();
}
