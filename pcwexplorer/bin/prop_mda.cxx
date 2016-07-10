

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
    #pragma implementation "prop_mda.cxx"
    #pragma interface "prop_mda.hxx"
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
#include "filetype.hxx"
// Our own header
#include "prop_mda.hxx"

BEGIN_EVENT_TABLE(PropPageMda, wxWindow)
END_EVENT_TABLE()



PropPageMda::PropPageMda(const char *typedesc, const char *filename,
                       const struct cpmInode *root,
                       wxWindow *parent, wxWindowID id,
                       const wxPoint& pos,
                       const wxSize& size,
                       long style,
                       const wxString& name)
	: PcwPropPage(parent, id, pos, size, style, name)
{
    wxString strw, strh, strv;
    int w,h;

    strw = "";
    strh = "";

    struct cpmInode ino;
    struct cpmFile  file;
    unsigned char   mdahdr[132];

    memset(&mdahdr, 0, sizeof(mdahdr));
    if (cpmNamei(root, filename, &ino) != -1)
    {
        if (!cpmOpen(&ino, &file, O_RDONLY))
        {
            cpmRead(&file, (char *)mdahdr, 132);
            cpmClose(&file);
        }

        if (!memcmp(mdahdr, ".MDA", 4))
        {
            h =     mdahdr[128] + 256 * mdahdr[129];
            w = 8 * mdahdr[130] + 256 * mdahdr[131];

	         strw.Printf("%d", w);
	         strh.Printf("%d", h);
        }
    }

    int y = m_ruler_xy.y + m_rstop;
	 int x = 120;
    (void)new wxStaticText  (this, -1, "Image type:", wxPoint(m_ruler_xy.x,   y));
    (void)new wxStaticText  (this, -1, typedesc,      wxPoint(x,  y));
	 y += m_tspacing;
    (void)new wxStaticText  (this, -1, "Width:",      wxPoint(m_ruler_xy.x,   y));
    (void)new wxStaticText  (this, -1, strw,          wxPoint(x,  y));
	 y += m_tspacing;
    (void)new wxStaticText  (this, -1, "Height:",     wxPoint(m_ruler_xy.x,   y));
    (void)new wxStaticText  (this, -1, strh,          wxPoint(x,  y));
	 y += m_tspacing + m_rsbot;
    (void)new wxStaticLine  (this, -1, wxPoint(m_ruler_xy.x, y), m_ruler_size);
    y += m_rstop;


    if (!memcmp(mdahdr, ".MDA", 4))
    {
        char buf[50];

        sprintf(buf, "%-14.14s", mdahdr+4);
	     (void)new wxStaticText(this, -1, "Created by:", wxPoint(m_ruler_xy.x, y));
        teCreator = new wxTextCtrl(this, -1, buf, wxPoint(x, y), wxSize(150, -1));

        y += m_tspacing;
	     sprintf(buf, "%-7.7s", mdahdr+25);
        (void)new wxStaticText(this, -1, "Serial number:", wxPoint(m_ruler_xy.x, y));
        (void)new wxStaticText(this, -1, buf, wxPoint(x, y));
	     sprintf(buf, "%-5.5s", mdahdr+18);
	  	  if      (!strcmp(buf, "v1.00")) strv = "v1.00 (MicroDesign 2)";
	     else if (!strcmp(buf, "v1.30")) strv = "v1.30 (MicroDesign 3)";
	     else    strv = buf;

        y += m_tspacing;
	     (void)new wxStaticText(this, -1, "File version:", wxPoint(m_ruler_xy.x, y));
	     (void)new wxStaticText(this, -1, strv, wxPoint(x, y));

        /* XXX For initial release, disable this edit box as it does nothing */

        teCreator->SetEditable(false);
	}
}

bool PropPageMda::OnApply(void)
{
	char buf[15];
	wxString strVer = teCreator->GetValue();

	sprintf(buf, "%-14.14s", (const char *)strVer);
	wxMessageBox(buf);

	/* XXX Does not write the creator back to the file. Is this even
         *     possible with current cpmtools? */

	return true;
}



