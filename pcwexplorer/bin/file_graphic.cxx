

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
    #pragma implementation "file_graphic.cxx"
    #pragma interface "file_graphic.hxx"
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
#include "wx/mstream.h"
#include "filetype.hxx"
#include "prop_image.hxx"

PcwGraphicFactory::PcwGraphicFactory() : PcwFileFactory()
{
}


void PcwGraphicFactory::AddPages(wxNotebook *notebook, const char *filename)
{
	PcwFileFactory::AddPages(notebook, filename);

        PropPageImage *pimg = new PropPageImage(GetTypeName(), 
                              GetDimensions(filename), notebook, PROP_IMAGE);

        notebook->AddPage(pimg, "Image");
}


wxWindow *PcwGraphicFactory::GetViewport(wxDialog *parent, const char *filename)
{
	wxImage *img = GetImage(filename);

	if (!img)
	{
		return new wxStaticText(parent, -1, "Could not load image");
	}
//	wxBitmap im2 = img->ConvertToBitmap();
	wxBitmap im2(*img);
//	if (!im2)
//	{
//		return new wxStaticText(parent, -1, "Could not load image");
//	}
	wxStaticBitmap *bmp = new wxStaticBitmap(parent, -1, im2, 
                                   wxDefaultPosition);
	delete img;
	return bmp;

}

wxSize PcwGraphicFactory::GetDimensions(const char *filename)
{
	wxImage *img = GetImage(filename);
	
	if (!img) return wxDefaultSize;
	
	wxSize sz(img->GetWidth(), img->GetHeight());

	delete img;
	return sz;
}


wxImage *PcwGraphicFactory::GetImage(const char *filename)
{
        struct cpmInode ino;
        struct cpmFile  file;
	struct cpmStat  statbuf;
	char *data;
//
// Load the whole file into memory
//
        if (cpmNamei(GetRoot(), filename, &ino) == -1) return NULL;
	cpmStat(&ino,&statbuf);
	if (!statbuf.size) return NULL;
        if (cpmOpen(&ino, &file, O_RDONLY)) return NULL;
	data = new char[statbuf.size];
	if (!data)
	{
		cpmClose(&file);
		return NULL;
	}
        cpmRead(&file, data, statbuf.size);
        cpmClose(&file);
//	
// Create a stream
//
	wxMemoryInputStream strm(data, statbuf.size);
	wxImage *img = new wxImage(strm, GetImageType());

	delete data;
	if (!img->Ok())
	{
		delete img;
		return NULL;
	}
	return img;	
}

void PcwGraphicFactory::BuildContextMenu(wxMenu &menu)
{
	int some = 0;
	wxMenu *submenu = new wxMenu("Export as");
	wxList &handlers = wxImage::GetHandlers();

	wxNode *node = handlers.GetFirst();
	while (node)
	{
		wxImageHandler *handler = (wxImageHandler *)node->Data();

		/* Ban BMP saving; it doesn't work properly */
		switch(handler->GetType())	
		{
			case wxBITMAP_TYPE_BMP:
				node = node->Next();
				continue;
		}
		some = 1;	
		submenu->Append(CTX_IMGFIRST + handler->GetType(), 
                                handler->GetExtension(), 
                                handler->GetName());
		node = node->Next();
	}
	if (some) menu.Append(CTX_ExImage, "Export As", submenu, "");
	else delete submenu;
	PcwFileFactory::BuildContextMenu(menu);
}

void PcwGraphicFactory::OnCtx(wxFrame *frame, int menuid, const char *filename)
{
	if (menuid >= CTX_FIRST && menuid <= CTX_LAST)
	{
		PcwFileFactory::OnCtx(frame, menuid, filename);
		return;
	}
	ExportImage(menuid - CTX_IMGFIRST, filename);
}

void PcwGraphicFactory::ExportImage(int format, const char *filename)
{
        char *s, buf[20];

	wxImageHandler *handler = wxImage::FindHandler(format);
	if (!handler)
	{
		::wxMessageBox("Internal error: No image handler registered "
                               "for this file type", "Export image", 
                               wxOK | wxICON_ERROR);
		return;
	}

	wxImage *img = GetImage(filename);
	if (!img) 
	{
                ::wxMessageBox("Failed to read selected image file. ", 
                               "Export image", 
                               wxOK | wxICON_ERROR);
		return;	
	}
	strcpy(buf, filename + 2);
	s = strrchr(buf, '.');
	if (s) *s = 0;
	strcat(buf, ".");
	strcat(buf, handler->GetExtension());
	wxString filter;
	filter.Printf("%s (*.%s)|*.%s", (const char *)handler->GetName(), 
                            (const char *)handler->GetExtension(), 
                            (const char *)handler->GetExtension());

	wxString f = wxFileSelector( "Save Image", (const char *)NULL, 
                                    buf, handler->GetExtension(), 
                                    filter );

	if (f == "")  return;
	if (!img->SaveFile(f, format))
	{
                ::wxMessageBox("Failed to save specified image file. ",
                               "Export image", 
                               wxOK | wxICON_ERROR);
		remove(f);
	}

	delete img;
}



int PcwGraphicFactory::GetImageType(void)
{
	return wxBITMAP_TYPE_PNG;
}

