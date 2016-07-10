/************************************************************************

    Dsktool v1.0.2 - LibDsk front end

    Copyright (C) 2005, 2007  John Elliott <jce@seasip.demon.co.uk>

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

*************************************************************************/


#include "wx/wxprec.h"
#ifdef __BORLANDC__
#pragma hdrstop
#endif

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include "wx/wizard.h"
#include "wx/progdlg.h"

#include "dsktool.h"
#include "MainWindow.h"
#include "AboutBox.h"
#include "DriveSelectPage.h"
#include "OptionsPage.h"
#include "IdentityForm.h"

#include "res/dsktool2.xpm"
#include "res/dsktool16.xpm"
#include "res/dsktool32.xpm"
#include "res/dsktool48.xpm"

#ifndef __WXMSW__
#include "res/identify.xpm"
#include "res/format.xpm"
#include "res/copy0.xpm"
#include "res/copy1.xpm"
#include "res/copy2.xpm"
#endif

//IMPLEMENT_CLASS(MainWindow, wxFrame)

BEGIN_EVENT_TABLE(MainWindow, wxFrame)
	EVT_MENU(wxID_EXIT, MainWindow::OnQuit)
	EVT_MENU(ID_APP_ABOUT, MainWindow::OnAbout)
	EVT_MENU(ID_IDENTIFY, MainWindow::OnIdentify)
	EVT_MENU(ID_FORMAT, MainWindow::OnFormat)
	EVT_MENU(ID_TRANSFORM, MainWindow::OnTransform)
END_EVENT_TABLE()

static MainWindow *st_mw;

static void report(const char *message)
{
	st_mw->SetStatusText(wxString(message, wxConvUTF8));
}

static void report_end()
{
	st_mw->SetStatusText(wxT(""));
}


// frame constructor
MainWindow::MainWindow(const wxString& title, const wxPoint& pos, const wxSize& size, long style)
       : wxFrame(NULL, -1, title, pos, size, style)
{
    // set the frame icon
	wxIconBundle appIcon;
	appIcon.AddIcon(wxIcon(dsktool48_xpm));
	appIcon.AddIcon(wxIcon(dsktool32_xpm));
	if (wxDisplayDepth() < 16)	// Only add the mono icon if on a mono display; 
	{							// otherwise it can get used in preference to the colour one.
		appIcon.AddIcon(wxIcon(dsktool2_xpm));
	}
	appIcon.AddIcon(wxIcon(dsktool16_xpm));
	SetIcons(appIcon);

	// Create a menu.
	wxMenu *file_menu = new wxMenu;

	file_menu->Append(ID_IDENTIFY, wxT("Identify..."));
	file_menu->Append(ID_FORMAT, wxT("Format..."));
	file_menu->Append(ID_TRANSFORM, wxT("Copy..."));
	file_menu->AppendSeparator();
	file_menu->Append(wxID_EXIT, wxT("E&xit\tCtrl-Q"));

	wxMenu *help_menu = new wxMenu;
	help_menu->Append(ID_APP_ABOUT, wxT("&About Dsktool\tF1"));

	wxMenuBar *menu_bar = new wxMenuBar;

	menu_bar->Append(file_menu, wxT("&Disc"));
	menu_bar->Append(help_menu, wxT("&Help"));

	//// Associate the menu bar with the frame
	SetMenuBar(menu_bar);
	st_mw = this;

    // create a status bar.
    CreateStatusBar(2);
    SetStatusText(_T("Ready"));
}

MainWindow::~MainWindow()
{

}


// event handlers

void MainWindow::OnQuit(wxCommandEvent& WXUNUSED(event))
{
    // TRUE is to force the frame to close
    Close(TRUE);
}

void MainWindow::OnAbout(wxCommandEvent& WXUNUSED(event))
{
	AboutBox aboutDlg(this);
	aboutDlg.ShowModal();
}


void MainWindow::OnIdentify(wxCommandEvent &WXUNUSED(event))
{
    wxWizard *wizard = new wxWizard(this, -1,
                    wxT("Identify disc or image"),
                    wxBITMAP(identify));

    // a wizard page may be either an object of predefined class
    wxWizardPageSimple *page1 = new wxWizardPageSimple(wizard);
    wxStaticText *text = new wxStaticText(page1, -1,
             wxT("This wizard will attempt to identify a disc or disc image file, and display\nthe information determined.\n")
             wxT("\n")
        );
    wxSize size = text->GetBestSize();

	DriveSelectPage *page2 = new DriveSelectPage(PT_IDENTIFY, wizard, page1);
	page1->SetNext(page2);

	dsk_reportfunc_set(report, report_end);
	wizard->SetPageSize(size);
	if ( wizard->RunWizard(page1) )
	{
		DSK_PDRIVER dsk = NULL;
		DSK_GEOMETRY dg;
		dsk_err_t err;

		err = page2->OpenDisc(&dsk);
		if (!err) 
		{
			if (page2->GetFormat() != FMT_UNKNOWN) 
				 err = dg_stdformat(&dg, page2->GetFormat(), NULL, NULL);
			else err = dsk_getgeom(dsk, &dg);
		}
		if (!err)
		{
			IdentityForm form(this, page2->GetFilename(), dsk, &dg);
			form.ShowModal();
		}

		dsk_close(&dsk);

		if (err)
		{
			wxString s = wxT("Error on file ") + page2->GetFilename() + wxT(":\n");
			s += wxString(dsk_strerror(err), wxConvUTF8);
			wxMessageBox(s, wxT("Failed to identify"), wxICON_STOP | wxOK);
		}
    }

    wizard->Destroy();
}


static unsigned char spec169 [10] = { 0,    0, 40, 9, 2, 2, 3, 2, 0x2A, 0x52 };
static unsigned char spec180 [10] = { 0,    0, 40, 9, 2, 1, 3, 2, 0x2A, 0x52 };
static unsigned char spec200 [10] = { 0,    0, 40,10, 2, 1, 3, 3, 0x0C, 0x17 };
static unsigned char spec720 [10] = { 3, 0x81, 80, 9, 2, 1, 4, 4, 0x2A, 0x52 };
static unsigned char spec800 [10] = { 3, 0x81, 80,10, 2, 1, 4, 4, 0x0C, 0x17 };

void MainWindow::OnFormat(wxCommandEvent &WXUNUSED(event))
{
    wxWizard *wizard = new wxWizard(this, -1,
                    wxT("Format disc or image"),
                    wxBITMAP(format));

    // a wizard page may be either an object of predefined class
    wxWizardPageSimple *page1 = new wxWizardPageSimple(wizard);
    wxStaticText *text = new wxStaticText(page1, -1,
             wxT("This wizard will format a disc or a new disc image file.\n")
             wxT("\n")
             wxT("WARNING: Formatting a disc will delete any existing data on it!\n")
        );
    wxSize size = text->GetBestSize();

	DriveSelectPage *page2 = new DriveSelectPage(PT_FORMAT, wizard, page1);
	page1->SetNext(page2);

    wizard->SetPageSize(size);
    if ( wizard->RunWizard(page1) )
    {
		DSK_PDRIVER dsk = NULL;
		DSK_GEOMETRY dg;
		dsk_err_t err = DSK_ERR_NOTIMPL;
		dsk_pcyl_t cyl;
		dsk_phead_t head;
		dsk_cchar_t fdesc;
		bool stopped = false;

		err = page2->CreateDisc(&dsk);
		if (!err) err = dg_stdformat(&dg, page2->GetFormat(), NULL, &fdesc);
		if (!err)
		{
			unsigned max = dg.dg_heads * dg.dg_cylinders;
			wxProgressDialog *progress = new wxProgressDialog
				(wxT("Formatting ") + page2->GetFilename() + wxT(" as ") + wxString(fdesc, wxConvUTF8), 
				wxT(""), max, this, wxPD_APP_MODAL | wxPD_CAN_ABORT);

			dsk_reportfunc_set(report, report_end);
			for (cyl = 0; cyl < dg.dg_cylinders; cyl++)
			{
				for (head = 0; head < dg.dg_heads; head++)
				{
					wxString msg = wxString::Format(wxT("Formatting cylinder %02d/%02d Head %d/%d"), cyl +1, dg.dg_cylinders, head+1, dg.dg_heads);
					SetStatusText(msg);
					stopped = !progress->Update(cyl * dg.dg_heads + head, msg);
					if (stopped) break;
					err = dsk_apform(dsk, &dg, cyl, head, 0xE5);
					if (err) break;	
				}
				if (stopped || err) break;
			}
			// Create a disc spec on the first sector, if the format's recognised 
			if (!stopped && !err)
			{
				unsigned char bootsec[512];
				unsigned int do_bootsec = 0;

				progress->Update(max-1, wxT("Writing first sector"));
				SetStatusText(wxT("Writing first sector"));
				memset(bootsec, 0xE5, sizeof(bootsec));
				switch(page2->GetFormat())
				{
					case FMT_180K:
						memcpy(bootsec, spec180, 10);
						do_bootsec = 1;
						break;
					case FMT_CPCSYS:
						memcpy(bootsec, spec169, 10);
						do_bootsec = 1;
						break;
					case FMT_CPCDATA:
						break;
					case FMT_200K:
						memcpy(bootsec, spec200, 10);
						do_bootsec = 1;
						break;
					case FMT_720K:
						memcpy(bootsec, spec720, 10);
						do_bootsec = 1;
						break;
					case FMT_800K:
						memcpy(bootsec, spec800, 10);
						do_bootsec = 1;
						break;
					default:
						break;
				}
				if (do_bootsec) err = dsk_lwrite(dsk, &dg, bootsec, 0);
			}
			delete progress;
		}
		SetStatusText(wxT(""));
		if (dsk) dsk_close(&dsk);

		if (err)
		{
			wxString s = wxT("Error on file ") + page2->GetFilename() + wxT(":\n");
			s += wxString(dsk_strerror(err), wxConvUTF8);
			wxMessageBox(s, wxT("Failed to format"), wxICON_STOP | wxOK);
		}
		else if (stopped)
		{
			wxMessageBox(wxT("Format cancelled by user. The disc is not completely formatted - do not use it!"), 
				wxT("User cancelled"), wxICON_STOP | wxOK);
		}
		else
		{
			wxMessageBox(wxT("Format complete."), 
				wxT("Format disc"), wxICON_INFORMATION | wxOK);

		}
    }

    wizard->Destroy();
}


/* A quick-and-dirty converter which replaces an Apricot MS-DOS 
 * boot block with a standard PCDOS one. Intended use is for 
 * loopback mounting:
 *
 * dsktrans -otype raw -apricot /dev/fd0 filename.ufi
 * mount -o loop -t msdos filename.ufi /mnt/floppy
 */

static void apricot_bootsect(unsigned char *bootsect) 
{
	int n;
	int heads     = bootsect[0x16];
	int sectors   = bootsect[0x10] + 256 * bootsect[0x11];
	char label[8];

/* Check that the first 8 bytes are ASCII or all zeroes. */
	for (n = 0; n < 8; n++)
	{
		if (bootsect[n] != 0 && 
			(bootsect[n] < 0x20 || bootsect[n] > 0x7E)) return;
	}
	memcpy(label, bootsect, 8);
	bootsect[0] = 0xE9;	/* 80x86 jump */
	bootsect[1] = 0x40;
	bootsect[2] = 0x90;
	memcpy(bootsect + 3, label, 8);	/* OEM ID */
	memcpy(bootsect + 11, bootsect + 80, 13);	/* BPB */
	bootsect[24] = sectors & 0xFF;
	bootsect[25] = sectors >> 8;
	bootsect[26] = heads & 0xFF;
	bootsect[27] = heads >> 8;
	memset(bootsect + 28, 0, 512 - 28);
	bootsect[0x40] = 0x90;	/* Minimal boot code: INT 18, diskless boot */
	bootsect[0x41] = 0x90;  /* or ROM BASIC (delete as applicable) */
	bootsect[0x42] = 0xcd;
	bootsect[0x43] = 0x18;
}



void MainWindow::OnTransform(wxCommandEvent &WXUNUSED(event))
{
	char *buf = NULL;
	char *cmt = NULL;
	bool md3     = false;
	bool logical = false;
	bool apricot = false;
	bool noformat = false;
	int last = -1, first = -1;
	wxString errorFile;

    wxWizard *wizard = new wxWizard(this, -1,
                    wxT("Copy disc or image file"),
                    wxBITMAP(copy0));

    // a wizard page may be either an object of predefined class
    wxWizardPageSimple *page1 = new wxWizardPageSimple(wizard);
    wxStaticText *text = new wxStaticText(page1, -1,
             wxT("This wizard will copy from one disc or image file to another.\n")
             wxT("\n")
             wxT("WARNING: The disc you are copying to will be completely erased\n")
			 wxT("and rewritten.")
        );
    wxSize size = text->GetBestSize();

	DriveSelectPage *page2 = new DriveSelectPage(PT_READ,  wizard, page1, NULL, wxBITMAP(copy1));
	DriveSelectPage *page3 = new DriveSelectPage(PT_WRITE, wizard, page2, NULL, wxBITMAP(copy2));
	OptionsPage     *page4 = new OptionsPage(wizard, page3);
	page1->SetNext(page2);
	page2->SetNext(page3);
	page3->SetNext(page4);

	dsk_reportfunc_set(report, report_end);
    wizard->SetPageSize(size);
    if ( wizard->RunWizard(page1) )
    {
		DSK_PDRIVER dskfrom = NULL, dskto = NULL;
		DSK_GEOMETRY dg;
		dsk_err_t err = DSK_ERR_NOTIMPL;
		dsk_pcyl_t cyl;
		dsk_phead_t head;
		dsk_psect_t sec;
		dsk_cchar_t fdesc;
		bool stopped = false;

		errorFile = page2->GetFilename();
		err = page2->OpenDisc(&dskfrom);
		if (!err) 
		{
			errorFile = page3->GetFilename();
			err = page3->CreateDisc(&dskto);
		}
		if (!err) 
		{
			errorFile = page2->GetFilename();
			if (page2->GetFormat() == FMT_UNKNOWN) err = dsk_getgeom(dskfrom, &dg);
			else err = dg_stdformat(&dg, page2->GetFormat(), NULL, &fdesc);
		}
		if (!err)
		{
			logical = page4->GetLogical();
			apricot = page4->GetApricot();
			md3     = page4->GetMD3();
			last    = page4->GetLast();
			first   = page4->GetFirst();
			noformat= !page4->GetFormat();
			if (last == -1)  last  = dg.dg_cylinders - 1;
			if (first == -1) first = 0;
			buf = (char *)dsk_malloc(dg.dg_secsize);
			if (!buf) err = DSK_ERR_NOMEM;
		}
		if (!err)
		{
			unsigned max = dg.dg_heads * (last - first + 1) * dg.dg_sectors;
			wxProgressDialog *progress = new wxProgressDialog
				(wxT("Copying from ") + page2->GetFilename() + wxT(" to ") + page3->GetFilename(), 
				wxT(""), max, this, wxPD_APP_MODAL | wxPD_CAN_ABORT);

			// Copy comment, if any 
			dsk_get_comment(dskfrom, &cmt);
			dsk_set_comment(dskto, cmt);

			for (cyl = first; (int)cyl <= last; ++cyl)
			{
				for (head = 0; head < dg.dg_heads; ++head)
				{
					if (md3)
					{
						// MD3 discs have 256-byte sectors in cyl 1 head 1
						if (cyl == 1 && head == 1) dg.dg_secsize = 256;
						else					   dg.dg_secsize = 512;
					}
					// Format track!
					if (err == DSK_ERR_OK && (!noformat)) 
					{
						errorFile = page3->GetFilename();
						err = dsk_apform(dskto, &dg, cyl, head, 0xE5);
					}
					if (!err) for (sec = 0; sec < dg.dg_sectors; ++sec)
					{
						wxString msg = wxString::Format(wxT("Cyl %02d/%02d Head %d/%d Sector %03d/%03d"), 
							cyl +1, dg.dg_cylinders,
				 			head+1, dg.dg_heads,
							sec+dg.dg_secbase, dg.dg_sectors + dg.dg_secbase - 1); 
					
						SetStatusText(msg);
						stopped = !progress->Update(dg.dg_sectors * (cyl * dg.dg_heads + head) + sec, msg);
						if (stopped) break;

						errorFile = page2->GetFilename();
						if (logical)
						{
							dsk_lsect_t ls;
							dsk_sides_t si;

			/* Convert sector to logical using SIDES_ALT sidedness. Raw DSKs 
			 * are always created so that the tracks are stored in SIDES_ALT 
			 * order. */
							si = dg.dg_sidedness;
							dg.dg_sidedness = SIDES_ALT;
							dg_ps2ls(&dg, cyl, head, sec, &ls);
							dg.dg_sidedness = si;
							err = dsk_lread(dskfrom, &dg, buf, ls);
						}
						else err = dsk_pread(dskfrom, &dg, buf, cyl,head,sec + dg.dg_secbase);
						// MD3 discs have deliberate bad sectors in cyl 1 head 1
						if (md3 && err == DSK_ERR_DATAERR && dg.dg_secsize == 256) err = DSK_ERR_OK;
						if (err) break;
						if (apricot && cyl == 0 && head == 0 && sec == 0)
							apricot_bootsect((unsigned char *)buf);
						errorFile = page3->GetFilename();
						err = dsk_pwrite(dskto,&dg,buf,cyl,head, sec + dg.dg_secbase);
						if (err) break;
					} // end sector loop
					if (err || stopped) break;
				} // end head loop
				if (err || stopped) break;
			}
			delete progress;
		}
		SetStatusText(wxT(""));
		if (dskto) dsk_close(&dskto);
		if (dskfrom) dsk_close(&dskfrom);

		if (err)
		{
			wxString s = wxT("Error on file ") + errorFile + wxT(":\n");
			s += wxString(dsk_strerror(err), wxConvUTF8);
			wxMessageBox(s, wxT("Failed to copy"), wxICON_STOP | wxOK);
		}
		else if (stopped)
		{
			wxMessageBox(wxT("Copy cancelled by user. The destination disc / file is not complete - do not use it!"), 
				wxT("User cancelled"), wxICON_STOP | wxOK);
		}
		else
		{
			wxMessageBox(wxT("Copy complete."), 
				wxT("Copy disc / image file"), wxICON_INFORMATION | wxOK);

		}
    }
	if (buf) dsk_free(buf);
	SetStatusText(wxT(""));
    wizard->Destroy();
}
