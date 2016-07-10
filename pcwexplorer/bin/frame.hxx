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

// UI components
#define SPLITTER_ID 101
#define TREE_ID     102
#define LIST_ID     103
#define TOOLBAR_ID  104

// nb: Not "coolbar" style
#ifdef __WXMSW__
#define TOOLBAR_STYLE (wxNO_BORDER | /*wxTB_DOCKABLE | */wxTB_HORIZONTAL)
#else
#define TOOLBAR_STYLE (wxTB_HORIZONTAL | wxTB_3DBUTTONS)
#endif

#define AMSTRAD_FORMAT "amstrad"

#define IMAGE_DRIVE 0
#define IMAGE_FOLDER 1
#define IMAGE_FILE 2

#define MAXGROUP 16

#include "wx/treectrl.h"
#include "wx/listctrl.h"
#include "wx/imaglist.h"

class MainList;
class PcwFileFactory;

class PcwFrame : public wxFrame
{
public:
    // ctor(s)
    PcwFrame(const wxString& title, const wxPoint& pos, const wxSize& size);
    ~PcwFrame();
    // event handlers (these functions should _not_ be virtual)

    // File menu
    void OnParentFolder(wxCommandEvent &event);
    void OnQuit(wxCommandEvent& event);

    // View menu
    void OnViewLargeIcons(wxCommandEvent &event);
    void OnViewSmallIcons(wxCommandEvent &event);
    void OnViewList     (wxCommandEvent &event);
    void OnViewReport   (wxCommandEvent &event);

    // Help menu
    void OnAbout(wxCommandEvent& event);

    // Context menus
    void OnCtx(wxCommandEvent& event);
    void OnCtxOpen(wxCommandEvent& event);
    void OnCtxTreeOpen(wxCommandEvent& event);
    void OnCtxDiskProps(wxCommandEvent& event);

    // Tree control
    void OnChooseGroup(wxTreeEvent& event);
    void OnRightClickTree(wxTreeEvent& event);

    // List control
    void OnRightClickList(wxListEvent& event);

    // Initialisation
    //int  LoadDir(const char *drive);
    int  LoadDir();

    void Remove(const char *filename);

private:
    void UpdateChecks(void);

    // The UI
    wxSplitterWindow *m_splitter;
    wxTreeCtrl	     *m_leftPane;
    MainList        *m_rightPane;
    wxToolBar	     *m_toolBar;
    wxMenuBar        *m_menuBar;

    wxImageList      *m_imgBig, *m_imgSmall;

    wxTreeItemId m_idRoot, m_idGroup[16];
    int          m_group;
	 bool		  m_columns_set_up;

    struct cpmSuperBlock m_drive;
    struct cpmInode m_root;
    char **m_gargv;
    int m_gargc;
    PcwFileFactory **m_gfile;

    char *group_name(int group); 
    char *disk_name(void);
    void list_group(int group);
    int in_list_group;	// Avoid recursion

public:
    void OnDClickList(wxMouseEvent &e);

    DSK_PDRIVER m_driver;
    DSK_GEOMETRY m_geom;

    // any class wishing to process wxWindows events must use this macro
    DECLARE_EVENT_TABLE()
};

// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

// IDs for the controls and the menu commands
enum
{
    // menu items
    FILE_Parent,
    FILE_Quit,

    VIEW_LargeIcons,
    VIEW_SmallIcons,
    VIEW_List,
    VIEW_Report,

    HELP_About,

    // Popup context menu
    CTX_Open,
    CTX_TreeOpen,
    CTX_DiskProps
};

