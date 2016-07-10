

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
    #pragma implementation "prop_drive.cxx"
    #pragma interface "prop_drive.hxx"
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
#include "pcwplore.hxx"
#include "prop_drive.hxx"
#include "wx/statline.h"

BEGIN_EVENT_TABLE(PropPageDrive, wxWindow)
	EVT_PAINT(PropPageDrive::OnPaint)
END_EVENT_TABLE()



PropPageDrive::PropPageDrive(struct cpmStatFS *buf, wxIcon &icon,
                       wxWindow *parent, wxWindowID id, 
                       const wxPoint& pos, 
                       const wxSize& size, 
                       long style, 
                       const wxString& name)
	: PcwPropPage(parent, id, pos, size, style, name)
{
	m_statfs = buf;
	
	wxString bsize, bused, bfree, blocks;

	bsize.Printf("%ld k",   m_statfs->f_bsize / 1024);
	blocks.Printf("%ld k", (m_statfs->f_bsize / 1024) * m_statfs->f_blocks);
	bused.Printf("%ld k",  (m_statfs->f_bsize / 1024) * m_statfs->f_bused);
   bfree.Printf("%ld k",  (m_statfs->f_bsize / 1024) * m_statfs->f_bfree);


    (void)new wxStaticBitmap(this, -1, icon, m_icon_xy, wxSize(32,32));
    (void)new wxStaticText  (this, -1, "Drive", m_filename_xy);
    (void)new wxStaticLine  (this, -1, m_ruler_xy, m_ruler_size);
    int y  = m_ruler_xy.y + m_rstop;
    int rx = m_ruler_xy.x;
    int fx = m_filename_xy.x;
    (void)new wxStaticText  (this, -1, "Block size:",     wxPoint(rx, y));
    (void)new wxStaticText  (this, -1, bsize,             wxPoint(fx, y));
    y += m_tspacing;
    (void)new wxStaticText  (this, -1, "Disk size:",      wxPoint(rx, y));
    (void)new wxStaticText  (this, -1, blocks,            wxPoint(fx, y));
    y += m_tspacing;
    (void)new wxStaticText  (this, -1, "In use:",         wxPoint(rx, y));
    (void)new wxStaticText  (this, -1, bused,             wxPoint(fx, y));
    y += m_tspacing;
    (void)new wxStaticText  (this, -1, "Free space:",     wxPoint(rx, y));
    (void)new wxStaticText  (this, -1, bfree,             wxPoint(fx, y));

    y = m_ruler_xy.y + m_rstop + 5 * m_tspacing;
    (void)new wxStaticText  (this, -1, "Used", wxPoint(fx,       y));
    (void)new wxStaticText  (this, -1, "Free", wxPoint(fx + 200, y));




}

void PropPageDrive::OnPaint(wxPaintEvent &e)
{
	int n;
   int y = m_ruler_xy.y + m_rstop + 5 * m_tspacing;
   int fx = m_filename_xy.x;

	/* Draw a nice pie chart of free space */
	wxPaintDC dc(this);
	wxColour clFree = wxColour(0, 0, 192);
	wxColour clUsed = wxColour(160, 255, 160);

   long prop = (m_statfs->f_bfree * 360) / m_statfs->f_blocks;

	dc.SetPen(*wxBLACK_PEN);
	dc.SetBrush(*wxBLUE_BRUSH);
	dc.DrawEllipse(fx-30,     y + m_tspacing + 10, 250, 100);
	dc.SetBrush(*wxGREEN_BRUSH);
   dc.DrawEllipticArc(fx-30, y + m_tspacing + 10, 250, 100,  0, prop);

/* To make the ellipses go 3D, draw them repeatedly, one pixel higher
  each time. Rather wasteful of CPU, but simple. */

	dc.SetPen(*wxTRANSPARENT_PEN);
	for (n = 9; n > 0; n--)
	{
	    dc.SetBrush(*wxBLUE_BRUSH);
       dc.DrawEllipse(fx-30, y + m_tspacing + n, 250, 100);
       dc.SetBrush(*wxGREEN_BRUSH);
       dc.DrawEllipticArc(fx-30, y + m_tspacing + n, 250, 100, 0, prop);
	}
	dc.SetPen(*wxBLACK_PEN);
	dc.SetBrush(*wxBLUE_BRUSH);

	dc.DrawLine   (fx-30,  y + m_tspacing + 60,  fx-30, y + m_tspacing + 50);
	dc.DrawLine   (fx+219, y + m_tspacing + 60, fx+219, y + m_tspacing + 50);
   dc.DrawEllipse(fx-30,  y + m_tspacing, 250, 100);

	dc.DrawRectangle(fx-30, y-10, 20, 20);

   dc.SetBrush(*wxGREEN_BRUSH);
   dc.DrawEllipticArc(fx-30, y + m_tspacing, 250, 100, 0, prop);

	dc.DrawRectangle(fx+170, y-10, 20, 20);
}
