

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
    #pragma implementation "filetype.cxx"
    #pragma interface "filetype.hxx"
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
#include "wx/tab.h"
#include "propsheet.hxx"
#include "filetype.hxx"
#include "prop_general.hxx"
#include "preview.hxx"
#include "frame.hxx"
#include "textmetric.hxx"

#if defined(__WXGTK__) || defined(__WXMOTIF__) || defined(__WXMSW__)
    #include "Xpm/file.xpm"
    #include "Xpm/file_small.xpm"
#endif

PcwFileFactory::PcwFileFactory()
{
	m_iconIndex = -1;
}

PcwFileFactory::~PcwFileFactory()
{

}


wxIcon PcwFileFactory::GetBigIcon()
{
#if defined(__WXGTK__) || defined(__WXMOTIF__) || defined(__WXMSW__)
	return wxIcon(file_xpm);
#endif
}


wxIcon PcwFileFactory::GetSmallIcon()
{
#if defined(__WXGTK__) || defined(__WXMOTIF__) || defined(__WXMSW__)
        return wxIcon(file_small_xpm);
#endif
}

void PcwFileFactory::SetIconIndex(int idx)
{
	m_iconIndex = idx;
}


int PcwFileFactory::GetIconIndex(void)
{
	return m_iconIndex;
}



bool PcwFileFactory::Identify(const char *filename, const unsigned char *first128)
{
	/* This is a generic file, so it always identifies. */
	UNUSED(filename);
   UNUSED(first128);
	return true;
}


void PcwFileFactory::BuildContextMenu(wxMenu &menu)
{
	menu.Append(CTX_Export,     "Export", "Export this file");
	menu.Append(CTX_Delete,     "Delete", "Delete this file");
	menu.Append(CTX_Properties, "Properties");
        menu.Append(CTX_View,       "Preview", "View this file");
}


void PcwFileFactory::OnCtx(wxFrame *frame, int event, const char *filename)
{
	switch(event)
	{
		case CTX_Export:
			OnExport(frame, filename);
			break;

		case CTX_Properties:
			OnProperties(frame, filename);
			break;

                case CTX_View:
			OnView(frame, filename);
			break;

		case CTX_Delete:
			if (cpmUnlink(GetRoot(),filename)==-1)
			{
				::wxMessageBox(boo, "Failed to delete file",
					wxCANCEL | wxICON_HAND | wxCENTRE);
			}
			else
			{
				((PcwFrame *)frame)->Remove(filename);
			}
			break;
	}	
}

void PcwFileFactory::OnExport(wxFrame *frame, const char *filename)
{
        wxString f = wxFileSelector( "Save Image", (const char *)NULL,
                                    filename+2, NULL,
                                    "All files (*.*)|*.*" );

        if (f == "")  return;

        struct cpmInode ino;
        struct cpmFile  file;
	unsigned int rl;
	char buf[1024];

	FILE *fp = fopen(f, "wb");
	if (!fp)
	{
                ::wxMessageBox("Could not open destination file. ",
                               "Export file", 
                               wxOK | wxICON_ERROR);
		return;
	}
        if (cpmNamei(GetRoot(), filename, &ino) == -1) 
	{
		fclose(fp);
		remove(f);
                ::wxMessageBox("Could not open source file. ", 
                               "Export file", 
                               wxOK | wxICON_ERROR);

		return;
	}

        if (cpmOpen(&ino, &file, O_RDONLY)) 
	{
		fclose(fp);
		remove(f);
                ::wxMessageBox("Could not open source file. ",
                               "Export file",   
                               wxOK | wxICON_ERROR);
		return;
	}
	while ((rl = cpmRead(&file, buf, 1024)))
	{
		if (fwrite(buf, 1, rl, fp) < rl)
		{
	                ::wxMessageBox("Write error; output file truncated. ",
       		                        "Export file",   
               		                wxOK | wxICON_ERROR);
			break;
		}
	}
	fclose(fp);	
        cpmClose(&file);
}

void PcwFileFactory::OnProperties(wxFrame *frame, const char *filename)
{
	PcwPropSheet *page = new PcwPropSheet(this, filename, frame, 101,
		wxDefaultPosition, wxSize(metric_x * 58 - 97,
                                metric_y * 30 + metric_d * 2 + 25));

	page->ShowModal();
	delete page;

}


void PcwFileFactory::AddPages(wxNotebook *notebook, const char *filename)
{
	PropPageGeneral *pgen = new PropPageGeneral(GetBigIcon(), filename, 
                     GetTypeName(), GetRoot(), notebook, PROP_GENERAL);
                                 
	notebook->AddPage(pgen, "General"); 
}


static void addhex(int base, wxListBox *lb, unsigned char *data, int len)
{
	int n;
	wxString strLine, strDgt;

	strLine.Printf("%04x: ", base);
	for (n = 0; n < len; n++)
	{
		strDgt.Printf("%02x ", data[n]);
		strLine += strDgt;
	}
	for (n = len; n < 16; n++) strLine += "   ";
	strLine += "  ";
        for (n = 0; n < len; n++)
        {
		if (data[n] >= 0x20 && data[n] <= 0x7E)
                {
			strDgt.Printf("%c", data[n]);
                	strLine += strDgt;
		}
		else
		{
			strLine += ".";
		}
        }
        for (n = len; n < 16; n++) strLine += " ";
	lb->Append(strLine);
}


wxWindow *PcwFileFactory::GetTextViewport(wxDialog *parent,
                                                         const char *filename)
{
        wxListBox *lb = new wxListBox(parent, -1);
        struct cpmInode ino;
        struct cpmFile  file;
        unsigned char buffer[1024];
        int l, n, llen, base;
	wxString strLine;	

        if (cpmNamei(GetRoot(), filename, &ino) == -1)
        {
                lb->Append("(Could not access file)");
                return lb;
        }
        if (cpmOpen(&ino, &file, O_RDONLY))
        {
                lb->Append("(Could not access file)");
                return lb;
        }
	
	lb->SetFont(wxFont(lb->GetFont().GetPointSize(), wxMODERN, wxNORMAL, wxNORMAL));

        base = 0;
        llen = 0;
	strLine = "";
        while ((l = cpmRead(&file, (char *)buffer, 1024)))
        {
                for (n = 0; n < l; n++)
                {
			if (buffer[n] == '\033') break;
			if (buffer[n] == '\r') continue;
			if (buffer[n] == '\n')
			{
				lb->Append(strLine);
				strLine = "";
				continue;
			}
			strLine += buffer[n];
                }
        }
	lb->Append(strLine);
        cpmClose(&file);

        return lb;
}





wxWindow *PcwFileFactory::GetBinaryViewport(wxDialog *parent, 
                                                         const char *filename)
{
	wxListBox *lb = new wxListBox(parent, -1);
        struct cpmInode ino;
        struct cpmFile  file;
	unsigned char buffer[1024];
	unsigned char line[16];	
	int l, n, llen, base;

        if (cpmNamei(GetRoot(), filename, &ino) == -1) 
	{
		lb->Append("(Could not access file)");
		return lb;
	}
        if (cpmOpen(&ino, &file, O_RDONLY)) 
	{
                lb->Append("(Could not access file)");
                return lb;
	}

	/* Use a fixed-pitch font */
	lb->SetFont(wxFont(lb->GetFont().GetPointSize(), wxMODERN, wxNORMAL, wxNORMAL));

	base = 0;	
	llen = 0;
	while ((l = cpmRead(&file, (char *)buffer, 1024)))
	{
		for (n = 0; n < l; n++)
		{
			line[llen] = buffer[n];
			++llen;
			if (llen == 16)
			{
				addhex(base, lb, line, llen);
				llen = 0;
				base += 16;
			}
		}
	}
	if (llen) addhex(base, lb, line, llen);
        cpmClose(&file);

	return lb;
}


wxWindow *PcwFileFactory::GetViewport(wxDialog *parent, const char *filename)
{
	char *error = NULL;
	bool binary = IsBinaryFileType(filename, &error);

	if (error) 
	{
                wxListBox *lb = new wxListBox(parent, -1);
                lb->Append(error);
                return lb;
	}
	if (binary) return GetBinaryViewport(parent, filename);
        return GetTextViewport(parent, filename);
}


bool PcwFileFactory::IsBinaryFileType(const char *filename, char **error)
{
        /* Get record 0 and guess whether ASCII or binary */
        struct cpmInode ino;
        struct cpmFile  file;
        unsigned char buffer[128];
	int n;

        if (cpmNamei(GetRoot(), filename, &ino) == -1)
        {
		*error = "(Could not access file)";
		return true;
        }
        if (cpmOpen(&ino, &file, O_RDONLY))
        {
                *error = "(Could not access file)";
		return true;
        }
	n = cpmRead(&file, (char *)buffer, 128);
	if (n < 128) memset(buffer + n, 26, 128 - n);
	cpmClose(&file);
	
	return IsBinaryHeader(buffer);
}

bool IsBinaryHeader(unsigned char *buffer)
{
	for (int n = 0; n < 128; n++)
	{
		if (buffer[n] == 26) return false;
		if ((buffer[n] < 32 || buffer[n] > 126) && buffer[n] != '\r'
                    && buffer[n] != '\n' && buffer[n] != '\t') return true;
	}
	return false;
}

bool IsAmsdosHeader(unsigned char *buffer)
{
	unsigned short sum = 0;

	/* Prevent triggering on all-zeroes header */
	if (buffer[1] == 0) return false;

        for (int n = 0; n < 67; n++)
        {
		sum += buffer[n];
        }
	if ((sum & 0xFF) != buffer[67]) return false;
	if ((sum >> 8)   != buffer[68]) return false;
        return true;
}

bool IsSpectrumHeader(unsigned char *buffer)
{
	unsigned char sum = 0;
	int n;

	if (memcmp(buffer, "PLUS3DOS\032", 9)) return false;

        for (n = 0; n < 127; n++)
        {
		sum += buffer[n];
        }
	if (sum == buffer[n]) return true;
        return false;
}




void PcwFileFactory::OnView(wxFrame *frame, const char *filename)
{
	wxString str;

	str.Printf("Preview of %s", filename + 2);

	wxDialog *wDlg = new PreviewDialog(this, filename, frame, -1, str, 
                                           wxDefaultPosition, wxSize(620, 400),
                         wxRESIZE_BORDER | wxDEFAULT_DIALOG_STYLE | wxCAPTION); 

	wDlg->Centre(wxBOTH);
	wDlg->ShowModal();
}


char *PcwFileFactory::GetTypeName()
{
	return "File";
}
