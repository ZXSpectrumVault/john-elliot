

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
    #pragma implementation "prop_general.cxx"
    #pragma interface "prop_general.hxx"
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
#include "prop_general.hxx"
#include "textmetric.hxx"

BEGIN_EVENT_TABLE(PropPageGeneral, wxWindow)
END_EVENT_TABLE()



PropPageGeneral::PropPageGeneral(wxIcon icon, const char *filename,
                       const char *typedesc, struct cpmInode *root,
                       wxWindow *parent, wxWindowID id, 
                       const wxPoint& pos, 
                       const wxSize& size,
                       long style,
                       const wxString& name)
	: PcwPropPage(parent, id, pos, size, style, name)
{
    wxString group, strsize, create, update, access, protmode;

    m_filename = filename;
    m_icon     = icon;

    group.Printf("%-2.2s", filename);
    group.Printf("Group %d", atoi(group));

    struct cpmStatFS statfsbuf;
    struct cpmStat statbuf;
    cpm_attr_t attrib;

    cpmStatFS(root, &statfsbuf);
    cpmNamei(root,filename,&m_file);
    cpmStat(&m_file,&statbuf);
    strsize.Printf( "%ld (%ld k)", statbuf.size,
      (statbuf.size+statfsbuf.f_bsize-1)/
                   statfsbuf.f_bsize*(statfsbuf.f_bsize/1024));
    cpmAttrGet(&m_file, &attrib);

    struct tm *tmp;

    tmp=gmtime(&statbuf.mtime);
    update.Printf("%02d/%02d/%04d %02d:%02d  ",
        tmp->tm_mon+1,tmp->tm_mday,tmp->tm_year+1900,tmp->tm_hour,tmp->tm_min);
    tmp=gmtime(&statbuf.ctime);
    create.Printf("%02d/%02d/%04d %02d:%02d",
        tmp->tm_mon+1,tmp->tm_mday,tmp->tm_year+1900,tmp->tm_hour,tmp->tm_min);
    tmp=gmtime(&statbuf.atime);
    access.Printf("%02d/%02d/%04d %02d:%02d",
        tmp->tm_mon+1,tmp->tm_mday,tmp->tm_year+1900,tmp->tm_hour,tmp->tm_min);
	 if      (attrib & CPM_ATTR_PWREAD)  protmode = "Password to read";
    else if (attrib & CPM_ATTR_PWWRITE) protmode = "Password to write";
    else if (attrib & CPM_ATTR_PWDEL)   protmode = "Password to delete";
    else                                protmode = "None";

    if (!statbuf.ctime) create="n/a";
    if (!statbuf.mtime) update="n/a";
    if (!statbuf.atime) access="n/a";

    int y;
    (void)new wxStaticBitmap(this, -1, m_icon, m_icon_xy, wxSize(32,32));
    (void)new wxStaticText  (this, -1, m_filename.Mid(2), m_filename_xy);
    (void)new wxStaticLine  (this, -1, m_ruler_xy, m_ruler_size);
    y = m_ruler_xy.y + m_rstop;
    (void)new wxStaticText  (this, -1, "Type:",     wxPoint(m_ruler_xy.x,    y));
    (void)new wxStaticText  (this, -1, typedesc,    wxPoint(m_filename_xy.x, y));
    y += m_tspacing;
    (void)new wxStaticText  (this, -1, "Location:", wxPoint(m_ruler_xy.x,    y));
    (void)new wxStaticText  (this, -1, group,       wxPoint(m_filename_xy.x, y));
    y += m_tspacing;
    (void)new wxStaticText  (this, -1, "Size:",     wxPoint(m_ruler_xy.x,    y));
    (void)new wxStaticText  (this, -1, strsize,     wxPoint(m_filename_xy.x, y));
    y += m_tspacing + m_rsbot;
    (void)new wxStaticLine  (this, -1, wxPoint(m_ruler_xy.x, y), m_ruler_size);
    y += m_rstop;
    (void)new wxStaticText  (this, -1, "Created:",  wxPoint(m_ruler_xy.x,    y));
    (void)new wxStaticText  (this, -1, create,      wxPoint(m_filename_xy.x, y));
    y += m_tspacing;
    (void)new wxStaticText  (this, -1, "Updated:",  wxPoint(m_ruler_xy.x,    y));
    (void)new wxStaticText  (this, -1, update,      wxPoint(m_filename_xy.x, y));
    y += m_tspacing;
    (void)new wxStaticText  (this, -1, "Accessed:", wxPoint(m_ruler_xy.x,    y));
    (void)new wxStaticText  (this, -1, access,      wxPoint(m_filename_xy.x, y));
	 y += m_tspacing;
    (void)new wxStaticText  (this, -1, "Protected:", wxPoint(m_ruler_xy.x,   y));
    (void)new wxStaticText  (this, -1, protmode,     wxPoint(m_filename_xy.x,y));
	 y += m_tspacing + m_rsbot;
    (void)new wxStaticLine  (this, -1, wxPoint(m_ruler_xy.x, y), m_ruler_size);
    y += m_rstop;
    (void)new wxStaticText  (this, -1, "Attributes:", wxPoint(m_ruler_xy.x, y));
    int x = m_filename_xy.x;
    y -= 3;
    int as1 = 12 * metric_x;
    m_attrib[0] = new wxCheckBox    (this, ATTRIB_RO, "Read only", wxPoint(x,       y), wxSize(as1, m_tspacing));
    m_attrib[1] = new wxCheckBox    (this, ATTRIB_SYS, "System",   wxPoint(x+  as1, y), wxSize(as1, m_tspacing));
    m_attrib[2] = new wxCheckBox    (this, ATTRIB_ARCV, "Archive", wxPoint(x+2*as1, y), wxSize(as1, m_tspacing));
    y += m_tspacing;
    as1 = 8 * metric_x;
    m_attrib[3] = new wxCheckBox    (this, ATTRIB_F1, "F1", wxPoint(x,       y), wxSize(as1, m_tspacing));
    m_attrib[4] = new wxCheckBox    (this, ATTRIB_F2, "F2", wxPoint(x+  as1, y), wxSize(as1, m_tspacing));
    m_attrib[5] = new wxCheckBox    (this, ATTRIB_F3, "F3", wxPoint(x+2*as1, y), wxSize(as1, m_tspacing));
    m_attrib[6] = new wxCheckBox    (this, ATTRIB_F4, "F4", wxPoint(x+3*as1, y), wxSize(as1, m_tspacing));

    m_attrib[0]->SetValue(attrib & CPM_ATTR_RO);
    m_attrib[1]->SetValue(attrib & CPM_ATTR_SYS);
    m_attrib[2]->SetValue(attrib & CPM_ATTR_ARCV);
    m_attrib[3]->SetValue(attrib & CPM_ATTR_F1);
    m_attrib[4]->SetValue(attrib & CPM_ATTR_F2);
    m_attrib[5]->SetValue(attrib & CPM_ATTR_F3);
    m_attrib[6]->SetValue(attrib & CPM_ATTR_F4);
}

bool PropPageGeneral::OnApply(void)
{
    int attrib;
    cpmAttrGet(&m_file, &attrib);

    if (m_attrib[0]->GetValue()) attrib |= CPM_ATTR_RO;   else attrib &= ~CPM_ATTR_RO;
    if (m_attrib[1]->GetValue()) attrib |= CPM_ATTR_SYS;  else attrib &= ~CPM_ATTR_SYS;
    if (m_attrib[2]->GetValue()) attrib |= CPM_ATTR_ARCV; else attrib &= ~CPM_ATTR_ARCV;
    if (m_attrib[3]->GetValue()) attrib |= CPM_ATTR_F1;   else attrib &= ~CPM_ATTR_F1;
    if (m_attrib[4]->GetValue()) attrib |= CPM_ATTR_F2;   else attrib &= ~CPM_ATTR_F2;
    if (m_attrib[5]->GetValue()) attrib |= CPM_ATTR_F3;   else attrib &= ~CPM_ATTR_F3;
    if (m_attrib[6]->GetValue()) attrib |= CPM_ATTR_F4;   else attrib &= ~CPM_ATTR_F4;

    if (cpmAttrSet(&m_file, attrib))
    {
       ::wxMessageBox(boo, "Failed to set file attributes",
                                        wxCANCEL | wxICON_HAND | wxCENTRE);
       return false;
    }
    return true;
} 

