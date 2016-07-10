

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
    #pragma implementation "prop_bas.cpp"
    #pragma interface "prop_bas.h"
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
// Our own header
#include "prop_bas.hxx"

BEGIN_EVENT_TABLE(PropPageBas, wxWindow)
END_EVENT_TABLE()



PropPageBas::PropPageBas(const char *filename, const struct cpmInode *root,
                       wxWindow *parent, wxWindowID id,
                       const wxPoint& pos, 
                       const wxSize& size, 
                       long style, 
                       const wxString& name)
	: PcwPropPage(parent, id, pos, size, style, name)
{
    struct cpmInode ino;
    struct cpmFile  file;
    unsigned char   bashdr[128];

    memset(&bashdr, 0, sizeof(bashdr));
    if (cpmNamei(root, filename, &ino) != -1)
    {
        if (!cpmOpen(&ino, &file, O_RDONLY))
        {
            cpmRead(&file, (char *)bashdr, sizeof(bashdr));
            cpmClose(&file);
        }
    }
    wxString basdesc, basadv;

    if (IsSpectrumHeader(bashdr))
    {
	basdesc = "Spectrum +3 file";	
	if (bashdr[15] == 0)
	{
		basdesc = "Spectrum +3 BASIC program";
		basadv  = "This file may be usable in a Spectrum emulator.\n"
                          "It is unlikely to convert easily to any other\n"
                          "version of BASIC.";
	}
	else
	{
                basdesc = "Spectrum +3 file (not a BASIC program)";
                basadv  = "This is not a BASIC program, but it may\nbe "
                          "usable in a Spectrum emulator.";
 
	}
    }
    else if (IsAmsdosHeader(bashdr))
    {
        basdesc = "Locomotive BASIC (CPC)";
        basadv  = "You will need a CPC emulator to use \n"
                  "this program.";
    }

    else if (!memcmp(bashdr, "\374\004\001", 3))
    {
	basdesc = "Mallard BASIC (protected)";
	basadv  = "You will need Mallard BASIC on\n"
                  "the PC (or a PCW emulator) to use \n"
                  "this program.";
    }
    else if (!memcmp(bashdr, "\374\004\000", 3))
    {
        basdesc = "Mallard BASIC (not protected)";
        basadv  = "You will need Mallard BASIC on the PC\n"
                  "(or a PCW emulator) to use this program.\n"
                  "If you want to use it in a different \n"
                  "version of BASIC, you will need to save it\n"
                  "as ASCII from Mallard BASIC, either on the\n"
                  "PC or the PCW.";
    }
    else if (bashdr[0] == 0xFE)
    {
        basdesc = "Microsoft BASIC (protected)";
        basadv  = "";
    }
    else if (bashdr[0] == 0xFF)
    {
        basdesc = "Microsoft BASIC (not protected)";
        basadv  = "";
    }
    else if (IsBinaryHeader(bashdr))
    {
        basdesc = "Unknown BASIC type";
        basadv  = "";
    }
    else
    {
        basdesc = "ASCII file";
        basadv  = "This file is stored as ASCII; versions of BASIC\n"
                  "such as QBASIC, GWBASIC and BASIC2 may be able\n"
                  "to import this file. ";
    }

    int y = m_ruler_xy.y + m_rstop;
    (void)new wxStaticText  (this, -1, "BASIC type:",   wxPoint(m_ruler_xy.x,  y));
    (void)new wxStaticText  (this, -1, basdesc,         wxPoint(100,  y));

    y += m_tspacing;
    (void)new wxStaticText  (this, -1, basadv,          wxPoint(m_ruler_xy.x, y),
                             wxSize(330, 150));


}




