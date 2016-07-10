

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
    #pragma implementation "frame.cpp"
    #pragma interface "frame.h"
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
#include "wx/splitter.h"
#define INSTANTIATE
#include "pcwplore.hxx"
#include "filetype.hxx"
#include "frame.hxx"
#include "discfile.hxx"
#include "aboutdlg.hxx"
#include "list_main.hxx"
#include "driveprop.hxx"
#include "textmetric.hxx"

int metric_x, metric_y, metric_d;

const char cmd[] = "pcwplore";

typedef PcwFileFactory *pPcwFileFactory;

// ----------------------------------------------------------------------------
// ressources
// ----------------------------------------------------------------------------
// the application icon & other bitmaps
#if defined(__WXGTK__) || defined(__WXMOTIF__) || defined(__WXMSW__)
    #include "Xpm/pcw.xpm"
    #include "Xpm/folder.xpm"
    #include "Xpm/folder_small.xpm"
    #include "Xpm/drive.xpm"
    #include "Xpm/drive_small.xpm"

    #include "Xpm/toolbar/up.xpm"
    #include "Xpm/toolbar/largeicons.xpm"
    #include "Xpm/toolbar/smallicons.xpm"
    #include "Xpm/toolbar/list.xpm"
    #include "Xpm/toolbar/report.xpm"
    #include "Xpm/toolbar/about.xpm"
#endif

#define RIGHT_PANE_STYLE (wxLC_SINGLE_SEL)

// ----------------------------------------------------------------------------
// event tables and other macros for wxWindows
// ----------------------------------------------------------------------------

// the event tables connect the wxWindows events with the functions (event
// handlers) which process them. It can be also done at run-time, but for the
// simple menu events like this the static method is much simpler.
BEGIN_EVENT_TABLE(PcwFrame, wxFrame)
    EVT_MENU(FILE_Parent,       PcwFrame::OnParentFolder)
    EVT_MENU(FILE_Quit,         PcwFrame::OnQuit)
    EVT_MENU(VIEW_LargeIcons,   PcwFrame::OnViewLargeIcons)
    EVT_MENU(VIEW_SmallIcons,   PcwFrame::OnViewSmallIcons)
    EVT_MENU(VIEW_List,         PcwFrame::OnViewList)
    EVT_MENU(VIEW_Report,       PcwFrame::OnViewReport)
    EVT_MENU(HELP_About,        PcwFrame::OnAbout)
    EVT_MENU(CTX_Open,          PcwFrame::OnCtxOpen)
    EVT_MENU(CTX_TreeOpen,      PcwFrame::OnCtxTreeOpen)
    EVT_MENU(CTX_DiskProps,     PcwFrame::OnCtxDiskProps)
    EVT_MENU_RANGE(CTX_FIRST, CTX_LAST, PcwFrame::OnCtx)
    EVT_MENU_RANGE(CTX_IMGFIRST, CTX_IMGLAST, PcwFrame::OnCtx)
    EVT_TREE_SEL_CHANGED     (TREE_ID, PcwFrame::OnChooseGroup)
    EVT_TREE_ITEM_RIGHT_CLICK(TREE_ID, PcwFrame::OnRightClickTree)
    EVT_LIST_ITEM_RIGHT_CLICK(LIST_ID, PcwFrame::OnRightClickList)
END_EVENT_TABLE()

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// main frame
// ----------------------------------------------------------------------------

// frame constructor
PcwFrame::PcwFrame(const wxString& title, const wxPoint& pos, const wxSize& size)
       : wxFrame((wxFrame *)NULL, -1, title, pos, size)
{
    int n, c;
    m_gfile = NULL;
    in_list_group = 0;
    m_columns_set_up = false;

// Compute font metrics

    wxFont font( GetFont() );
    if (!font.Ok()) font = *wxSWISS_FONT;

    GetTextExtent(_T("H"), &metric_x, &metric_y, &metric_d, NULL, &font);
// MSW (small fonts) font_x =  8 font_y = 13  descent = 2 
// MSW (large fonts) font_x = 10 font_y = 16  descent = 3
// GTK (small fonts) font_x =  9 font_y = 14  descent = 3 
//
    // set the frame icon
#if defined(__WXGTK__) || defined(__WXMOTIF__)
    SetIcon(wxICON(pcw));
#endif

#if defined(__WXMSW__)
    SetIcon(wxICON(APP_ICON));
#endif

    m_toolBar = CreateToolBar(TOOLBAR_STYLE, TOOLBAR_ID);
//    m_toolBar->SetToolBitmapSize(wxSize(16,16));

    m_splitter = new wxSplitterWindow(this, SPLITTER_ID, wxDefaultPosition, wxDefaultSize, wxSP_3D | wxSP_LIVE_UPDATE);

    m_imgBig    = new wxImageList(32, 32, TRUE);
    m_imgSmall  = new wxImageList(16, 16, TRUE);

// Add fixed icons for drive & folder 

#if defined(__WXGTK__) || defined(__WXMOTIF__) || defined(__WXMSW__)
    m_imgBig  ->Add ( wxIcon (drive_xpm));
    m_imgBig  ->Add ( wxIcon (folder_xpm));
    m_imgSmall->Add ( wxIcon (drive_small_xpm));
    m_imgSmall->Add ( wxIcon (folder_small_xpm));
#else
    #error To be done.
#endif
//
// Instantiate the classes for different file types
//
   for (n = c = 0; FileClassList[n]; n++)
   {
       ++c;
   }
   FileClasses = new pPcwFileFactory[c];

//
// Add icons for filetypes. The first one in the list MUST be the generic 
// filetype.
//
    for (n = 0; FileClassList[n]; n++)
    {
        FileClasses[n] = (*(FileClassList[n]))();

        FileClasses[n]->SetIconIndex(n + IMAGE_FILE);
        m_imgBig  ->Add(FileClasses[n]->GetBigIcon());
        m_imgSmall->Add(FileClasses[n]->GetSmallIcon());
    }
    m_leftPane  = new wxTreeCtrl(m_splitter, TREE_ID, wxDefaultPosition, wxDefaultSize, wxTR_HAS_BUTTONS);
    m_rightPane = new MainList(m_splitter, LIST_ID, wxDefaultPosition, wxDefaultSize, RIGHT_PANE_STYLE | wxLC_ICON);

    m_rightPane->SetImageList(m_imgBig,   wxIMAGE_LIST_NORMAL);
    m_rightPane->SetImageList(m_imgSmall, wxIMAGE_LIST_SMALL);
    m_leftPane->SetImageList(m_imgSmall);

    m_leftPane->Show(TRUE);
    m_rightPane->Show(TRUE);
    m_splitter->SplitVertically(m_leftPane, m_rightPane);
    m_splitter->SetSashPosition(150);

    // Add tools to the toolbar
    m_toolBar->AddTool(FILE_Parent, wxBitmap(up_xpm),
                        wxNullBitmap, FALSE,-1,-1,NULL,"Groups list",
                               "Go to groups list");

    m_toolBar->AddSeparator();
    m_toolBar->AddTool(VIEW_LargeIcons, wxBitmap(largeicons_xpm), 
                       wxNullBitmap, TRUE, -1, -1, NULL, "Large icons");
    m_toolBar->AddTool(VIEW_SmallIcons, wxBitmap(smallicons_xpm), 
                       wxNullBitmap, TRUE, -1, -1, NULL, "Small icons");
    m_toolBar->AddTool(VIEW_List,       wxBitmap(list_xpm),
                       wxNullBitmap, TRUE, -1, -1, NULL, "List");
    m_toolBar->AddTool(VIEW_Report,     wxBitmap(report_xpm),     
                       wxNullBitmap, TRUE, -1, -1, NULL, "Details");

    m_toolBar->AddSeparator();
    m_toolBar->AddTool(HELP_About, wxBitmap(about_xpm),
                       wxNullBitmap, FALSE,-1,-1,NULL,"About",
                               "About this program");

    m_toolBar->Realize();


    // create a menu bar
    wxMenu *menuFile = new wxMenu("", wxMENU_TEAROFF);
    wxMenu *menuHelp = new wxMenu("", wxMENU_TEAROFF);
    wxMenu *menuView = new wxMenu("", wxMENU_TEAROFF);

    menuHelp->Append(HELP_About, "&About...\tCtrl-A", "Show about dialog");
    menuFile->Append(FILE_Parent, "&Groups list", 
                              "Move to groups list");
    menuFile->AppendSeparator();
    menuFile->Append(FILE_Quit, "E&xit\tAlt-X", "Quit this program");

    menuView->Append(VIEW_LargeIcons, "&Large icons", "", TRUE);
    menuView->Append(VIEW_SmallIcons, "&Small icons", "", TRUE);
    menuView->Append(VIEW_List,       "L&ist",        "", TRUE);
    menuView->Append(VIEW_Report,     "&Details",     "", TRUE);

    // now append the freshly created menu to the menu bar...
    m_menuBar = new wxMenuBar();
    m_menuBar->Append(menuFile, "&File");
    m_menuBar->Append(menuView, "&View");
    m_menuBar->Append(menuHelp, "&Help");

    // ... and attach this menu bar to the frame
    SetMenuBar(m_menuBar);

#if wxUSE_STATUSBAR
    // create a status bar just for fun (by default with 1 pane only)
    CreateStatusBar(2);
    SetStatusText("Ready");
#endif // wxUSE_STATUSBAR
    m_group    = -1;
    m_drive.dev.fd = -1;	// No drive open
}

PcwFrame::~PcwFrame()
{
    for (int n = 0; FileClassList[n]; n++)
    {
        delete FileClasses[n];
    }
    if (m_gfile) delete m_gfile;
}



// event handlers

void PcwFrame::OnQuit(wxCommandEvent& WXUNUSED(event))
{
    // TRUE is to force the frame to close
    Close(TRUE);
}

void PcwFrame::OnAbout(wxCommandEvent& WXUNUSED(event))
{
    AboutBox abt(GetIcon(), this,  101);
    abt.ShowModal();
}


int PcwFrame::LoadDir(void)
{
	const char *boo;
	SetStatusText("Examining disc");

	if (m_drive.dev.opened)
	{
		Device_close(&m_drive.dev);	
	}
// Since we have already opened the LibDsk device, don't call Device_open().
// Instead, manually assign the members to achieve the same effect.
	m_drive.dev.dev  = m_driver;
	m_drive.dev.geom = m_geom;
	m_drive.dev.opened = 1;	

	cpmReadSuper(&m_drive, &m_root, AMSTRAD_FORMAT); 

        for (int ftype = 0; FileClassList[ftype]; ftype++)
        {
		FileClasses[ftype]->SetRoot(&m_root);
	}

	m_leftPane->DeleteAllItems();
	m_rightPane->DeleteAllItems();

	static char *starlit = "*";
	char **star = &starlit;
	int n;

	/* Generate a list of filenames on the disc */
	//glob(0,1,star,&m_root,&m_gargc,&m_gargv);
	cpmglob(0,1,star,&m_root,&m_gargc,&m_gargv);


        m_idRoot = m_leftPane->AddRoot(disk_name(), IMAGE_DRIVE);

	for (n = 0; n < MAXGROUP; n++)
	{
		m_idGroup[n] = m_leftPane->AppendItem(m_idRoot,
				       group_name(n), IMAGE_FOLDER);	
	}
	m_leftPane->SelectItem(m_idRoot);

	wxCommandEvent e;
	OnViewLargeIcons(e);	

	list_group(-1);

   SetStatusText("Ready");

	return 0;
}

void PcwFrame::Remove(const char *filename)
{
	long id = m_rightPane->FindItem(-1, filename + 2);

	if (id >= 0) m_rightPane->DeleteItem(id);	

	for (int n = 0; n < m_gargc; n++) 
	{
		if (!strcmp(filename, m_gargv[n]))
		{
			// Write user number E5 into the filename 
			m_gargv[n][0] = 'E';
			m_gargv[n][1] = '5';
		}
	}
}

char *PcwFrame::group_name(int group)
{
	char gn[3], *s; 
	static char title[20];

	sprintf(gn, "%02d", group);
        for (int n = 0; n < m_gargc; n++)
	{
		if (strstr(m_gargv[n], ".grp") && !strncmp(m_gargv[n], gn, 2)) 
		{
			sprintf(title, "%s", m_gargv[n] + 2);
			s = strchr(title, '.');
			if (s) *s = 0;
			return title;
		}
	}
	sprintf(title, "Group %d", group);
	return title;
}

char *PcwFrame::disk_name(void)
{
	return "Disc";
}

void PcwFrame::OnChooseGroup(wxTreeEvent &ev)
{
	wxTreeItemId i = ev.GetItem();

	if (i == m_idRoot) list_group(-1);
	for (int n = 0; n < 16; n++)
	{
		if (i == m_idGroup[n]) list_group(n);
	}
}
void PcwFrame::OnCtxTreeOpen(wxCommandEvent& event)
{
	wxTreeItemId sel = m_leftPane->GetSelection();

	if (sel == m_idRoot) list_group(-1);
	else
	{
		for (int n = 0; n < 16; n++)
		{
			if (sel == m_idGroup[n])
			{
				list_group(n);
				break;
			}
		}
	}
}


void PcwFrame::OnCtxOpen(wxCommandEvent& event)
{
	if (m_group == -1)
	{
		int max = m_rightPane->GetItemCount();

		for (int n = 0; n < max; n++)
		{
			if (m_rightPane->GetItemState(n, wxLIST_STATE_SELECTED))
			{
				list_group(n);
				break;
			}
		}
	}
}


void PcwFrame::OnParentFolder(wxCommandEvent &event)
{
	if (m_group != -1) list_group(-1);
}

void PcwFrame::list_group(int g)
{
	char gno[3];
	int n, itemid, ftype;
	struct cpmStatFS statfsbuf;
	bool report;

	if (in_list_group) return;
	in_list_group = 1;

	m_rightPane->DeleteAllItems();
	report = ((m_rightPane->GetWindowStyleFlag() & wxLC_MASK_TYPE) == wxLC_REPORT);

	m_toolBar->EnableTool(FILE_Parent, (g != -1));
	m_menuBar->Enable(FILE_Parent, (g != -1));

	m_group = g;
	if (g == -1)
	{
		m_leftPane->SelectItem(m_idRoot);
		m_leftPane->EnsureVisible(m_idRoot);

		for (n = 0; n < 16; n++)
		{
			m_rightPane->InsertItem(n, group_name(n), IMAGE_FOLDER);
			if (report)
			{
				m_rightPane->SetItem(n, 1, "");
				m_rightPane->SetItem(n, 2, "File group");
				m_rightPane->SetItem(n, 3, "");
			}
		}
		in_list_group = 0;
		return;
	}
	m_leftPane->SelectItem(m_idGroup[g]);
	m_leftPane->EnsureVisible(m_idGroup[g]);
	sprintf(gno, "%02d", g);

	if (m_gfile) delete m_gfile;
	m_gfile = new pPcwFileFactory[m_gargc];
	for (n = 0; n < m_gargc; n++) m_gfile[n] = NULL;

	cpmStatFS(&m_root, &statfsbuf);
	for (itemid = n = 0; n < m_gargc; n++)
	{
		int img = IMAGE_FILE;
		char *type = "File";
		struct cpmStat statbuf;
		struct cpmInode inode;
		cpm_attr_t attrib;
		wxString strsize;
		wxString strattrib;

		if (strncmp(m_gargv[n], gno, 2)) continue;

		cpmNamei(&m_root,m_gargv[n],&inode);
	 	cpmStat(&inode,&statbuf);
	 	strsize.Printf( "%ldk", (statbuf.size+statfsbuf.f_bsize-1)/
				 statfsbuf.f_bsize*(statfsbuf.f_bsize/1024));
		cpmAttrGet(&inode, &attrib);

		m_gfile[itemid] = FileClasses[0];

		for (ftype = 1; FileClassList[ftype]; ftype++)
		{
			if (FileClasses[ftype]->Identify(m_gargv[n]))
			{
				img  = FileClasses[ftype]->GetIconIndex();
				type = FileClasses[ftype]->GetTypeName();
				m_gfile[itemid] = FileClasses[ftype];
				break;
			}
		}
		m_rightPane->InsertItem (itemid, m_gargv[n]+2, img);
		if (report)
		{
			strattrib.Printf("%c%c%c %c%c%c%c",
				(attrib & CPM_ATTR_RO)	? 'R' : '-',
				(attrib & CPM_ATTR_SYS)  ? 'S' : '-',
				(attrib & CPM_ATTR_ARCV) ? 'A' : '-',
				(attrib & CPM_ATTR_F1)	? '1' : '-',
				(attrib & CPM_ATTR_F2)	? '2' : '-',
				(attrib & CPM_ATTR_F3)	? '3' : '-',
				(attrib & CPM_ATTR_F4)	? '4' : '-');

			m_rightPane->SetItem(itemid, 1, strsize);
			m_rightPane->SetItem(itemid, 2, type);
			m_rightPane->SetItem(itemid, 3, strattrib);
		}
		m_rightPane->SetItemData(itemid++, n);
	}
	in_list_group = 0;
}


void PcwFrame::OnDClickList(wxMouseEvent &event)
{
	int x,y;
	wxGetMousePosition(&x, &y);
	ScreenToClient(&x, &y);

	if (m_group == -1)
	{
		
	}
}

void PcwFrame::OnRightClickList(wxListEvent& event)
{
	int x,y;
	int idx = event.GetIndex();
	int max = m_rightPane->GetItemCount();

#define LIST_STATE (wxLIST_STATE_SELECTED)

	for (int n = 0; n < max; n++)
	{
		int st;

		if (n == idx) st = LIST_STATE;
		else	      st = 0;
		if (m_rightPane->GetItemState(n, LIST_STATE) != st) 
			m_rightPane->SetItemState(n, st, LIST_STATE);
	}

#undef LIST_STATE

	wxMenu mnuPopup;

	if (m_group == -1)
	{
		mnuPopup.Append(CTX_Open, "Open", "Open this folder");
	}

	else
	{
		m_gfile[idx]->BuildContextMenu(mnuPopup);
	}
	wxGetMousePosition(&x, &y);
	ScreenToClient(&x, &y);

	PopupMenu(&mnuPopup, x - 2, y - 2);
}




void PcwFrame::OnRightClickTree(wxTreeEvent& event)
{
	int x,y;
	wxTreeItemId id = event.GetItem();
	wxMenu mnuPopup;

	m_leftPane->SelectItem(id);

	if (id == m_idRoot)
	{
		mnuPopup.Append(CTX_DiskProps, "Properties", "Drive properties");
	}
	else
	{
		mnuPopup.Append(CTX_TreeOpen, "Open", "Open this folder");
	}
	wxGetMousePosition(&x, &y);
	ScreenToClient(&x, &y);

	PopupMenu(&mnuPopup, x - 2, y - 2);
}


void PcwFrame::OnViewLargeIcons(wxCommandEvent &)
{
	long sf = m_rightPane->GetWindowStyleFlag() & (~wxLC_MASK_TYPE);
	m_rightPane->SetWindowStyleFlag(sf | wxLC_ICON);
	UpdateChecks();
	list_group(m_group);
}

void PcwFrame::OnViewSmallIcons(wxCommandEvent &)
{
	long sf = m_rightPane->GetWindowStyleFlag() & (~wxLC_MASK_TYPE);
	m_rightPane->SetWindowStyleFlag(sf | wxLC_SMALL_ICON);
	UpdateChecks();
	list_group(m_group);
}

void PcwFrame::OnViewList(wxCommandEvent &)
{
	long sf = m_rightPane->GetWindowStyleFlag() & (~wxLC_MASK_TYPE);
	m_rightPane->SetWindowStyleFlag(sf | wxLC_LIST);
	UpdateChecks();
	list_group(m_group);
}

void PcwFrame::OnViewReport(wxCommandEvent &)
{
	long sf = m_rightPane->GetWindowStyleFlag() & (~wxLC_MASK_TYPE);
	m_rightPane->SetWindowStyleFlag(sf | wxLC_REPORT);
	m_rightPane->InsertColumn(0, "Name");
	m_rightPane->InsertColumn(1, "Size", wxLIST_FORMAT_RIGHT);
	m_rightPane->InsertColumn(2, "Type");
	m_rightPane->InsertColumn(3, "Attributes");

	UpdateChecks();
	list_group(m_group);
}

void PcwFrame::UpdateChecks(void)
{
	long sf = (m_rightPane->GetWindowStyleFlag() & wxLC_MASK_TYPE);

	m_menuBar->Check(VIEW_LargeIcons,(sf==wxLC_ICON)		? TRUE : FALSE);
	m_menuBar->Check(VIEW_SmallIcons,(sf==wxLC_SMALL_ICON)? TRUE : FALSE);
	m_menuBar->Check(VIEW_List,	 (sf==wxLC_LIST)		? TRUE : FALSE);
	m_menuBar->Check(VIEW_Report,	 (sf==wxLC_REPORT)	 ? TRUE : FALSE);

	m_toolBar->ToggleTool(VIEW_LargeIcons, (sf==wxLC_ICON)		? TRUE : FALSE);
	m_toolBar->ToggleTool(VIEW_SmallIcons, (sf==wxLC_SMALL_ICON)? TRUE : FALSE);
	m_toolBar->ToggleTool(VIEW_List,       (sf==wxLC_LIST)		? TRUE : FALSE);
	m_toolBar->ToggleTool(VIEW_Report,     (sf==wxLC_REPORT)    ? TRUE : FALSE);

}

void PcwFrame::OnCtxDiskProps(wxCommandEvent &event)
{
	struct cpmStatFS buf;
	wxIcon ico(drive_xpm);

	cpmStatFS(&m_root, &buf);	

	DrivePropSheet *page = new DrivePropSheet(&buf, ico,
		this, 101, wxDefaultPosition,
		 wxSize(metric_x * 58 - 97,
			metric_y * 30 + metric_d * 2 + 25));

	page->ShowModal();
	delete page;
}


void PcwFrame::OnCtx(wxCommandEvent &event)
{
	if (!m_gfile) return;

	int menuid = event.GetId();
	int max = m_rightPane->GetItemCount();

	for (int n = 0; n < max; n++)
	{
		if (m_rightPane->GetItemState(n, wxLIST_STATE_SELECTED))
		{
			int fileno = m_rightPane->GetItemData(n);
			m_gfile[n]->OnCtx(this, menuid, m_gargv[fileno]);
			break;
		}
	}
}
