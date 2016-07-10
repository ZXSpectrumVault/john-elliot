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
#include "pcwplore.hxx"
#include <wx/notebook.h>
#include <wx/image.h>

/* File type detection */

bool IsBinaryHeader (unsigned char *data);
bool IsAmsdosHeader (unsigned char *data);
bool IsSpectrumHeader(unsigned char *data);

/* 

  Sensible (?) handling of file types in the PCW Explorer is done by 
 having a separate class for each file type, descended from 
 PcwFileFactory. There is then an array of functions to generate 
 classes of the appropriate type.

  Note that these are "factory" classes and they are singletons - there 
 is not a separate one for each file.

*/

class PcwFileFactory
{
public:
	PcwFileFactory();
	virtual ~PcwFileFactory();

	virtual wxIcon GetBigIcon();	/* 32x32 icon for this class */
	virtual wxIcon GetSmallIcon();	/* 16x16 icon for this class */

	virtual bool Identify(const char *filename, const unsigned char *first128 = NULL);

	void SetIconIndex(int idx);	/* Set when icon is being registered */
	int  GetIconIndex(void);

	// What is this type of file called?
	virtual char *GetTypeName();

	// Build a context menu for this type of file
	virtual void BuildContextMenu(wxMenu &);

	// Build property pages for this file type
	virtual void AddPages(wxNotebook *notebook, const char *filename);

	// Return a window containing a preview of a file
	virtual wxWindow *GetViewport(wxDialog *parent, const char *filename);
	// Helpers for simple file dumps as text or binary
	wxWindow *GetTextViewport(wxDialog *parent, const char *filename);
	wxWindow *GetBinaryViewport(wxDialog *parent, const char *filename);
	bool IsBinaryFileType(const char *filename, char **error);

	// Context menu events
	virtual void OnCtx(wxFrame *frame, int menuid, const char *filename);

        virtual void OnView      (wxFrame *frame, const char *filename);
	virtual void OnProperties(wxFrame *frame, const char *filename);
	virtual void OnExport    (wxFrame *frame, const char *filename);

	inline void SetRoot(struct cpmInode *r) { m_root = r; }
	inline struct cpmInode *GetRoot() { return m_root; }

private:
	int  m_iconIndex;
	struct cpmInode *m_root;
};

// Generic file types
#include "file_graphic.hxx"

typedef PcwFileFactory *(*GET_FILE_CLASS)(void);

/* To add a new class (eg: MDA image)
	1. Create a header file (file_mda.h) containing:

             class MdaFileFactory: public PcwFileFactory

         and a function: 

             MdaFileFactory *GetMdaFileClass(void); 

         to return a new MdaFileFactory object.

	2. #include "file_mda".h here
        3. Add GetMdaFileClass to the end of the array of FileClasses below.
        4. Override methods of PcwFile as appropriate.

*/

/* PCW graphics */
#include "file_cut.hxx"
#include "file_spc.hxx"
#include "file_mvg.hxx"
#include "file_mda.hxx"
#include "file_logo.hxx"
/* Other graphics */
#include "file_png.hxx"
/* PCW text */
#include "file_txt.hxx"
/* PCW programs */
#include "file_sub.hxx"
#include "file_com.hxx"
#include "file_bas.hxx"

#ifdef INSTANTIATE
	PcwFileFactory *GetFileClass(void)
	{
		return new PcwFileFactory;
	}

	GET_FILE_CLASS FileClassList[] = 
	{
		&GetFileClass,	
//
                &GetCutFileClass,
		&GetSpcFileClass,
		&GetMvgFileClass,
		&GetMdaFileClass,
		&GetLogoFileClass,
//
		&GetPngFileClass,
//
		&GetSubFileClass,
		&GetComFileClass,
		&GetBasFileClass,
                &GetTxtFileClass,

		NULL
	};

	PcwFileFactory **FileClasses;
#else

	extern GET_FILE_CLASS FileClassList[];
	extern PcwFileFactory **FileClasses;
#endif


// Context menu item IDs

#define CTX_FIRST	300
#define CTX_Export	301
#define CTX_Delete	302
#define CTX_Properties	303
#define CTX_View        304
#define CTX_ExImage	305
#define CTX_LAST	306

#define CTX_IMGFIRST	500	// IDs for "Export image" submenu
#define CTX_IMGLAST	699

// Property page IDs

#define PROP_GENERAL	401
#define PROP_IMAGE      402
#define PROP_PROGRAM    403

#ifndef UNUSED
#define UNUSED(x) ((void)x);
#endif

