/************************************************************************

    JOYCE v1.90 - Amstrad PCW emulator

    Copyright (C) 1996, 2001  John Elliott <seasip.webmaster@gmail.com>

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

#include <SDL.h>
#include <string>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifdef WIN32
#include <windows.h>
#endif
#include "UiTypes.hxx"
#include "UiControl.hxx"
#include "UiLabel.hxx"
#include "UiCommand.hxx"
#include "PcwFileEntry.hxx"

extern string displayName (string filename, int len);


PcwFileEntry::PcwFileEntry(string filename, string dirname, UiDrawer *d)
		: UiCommand(SDLK_UNKNOWN, string("  ")+filename+string("  "), d)
{
	m_filename = filename;
	m_dirname  = dirname;
	m_statted = false;
}

PcwFileEntry::~PcwFileEntry()
{

}

void PcwFileEntry::draw(int selected)
{
	stat();
	m_caption = "  " + displayName(m_filename, 36) + "  ";
	UiCommand::draw(selected);
}

#ifdef WIN32
WIN32_FIND_DATA &PcwFileEntry::stat(void)
{
	if (!m_statted)
	{
		// Special case: These two are always directories
		if (m_filename == "." || m_filename == "..")
		{
			m_statted = true;
			m_st.dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY;
			setGlyph(UIG_SUBMENU);
		}
		HANDLE hFind = ::FindFirstFile((m_dirname + m_filename).c_str(), &m_st);

		if (hFind != INVALID_HANDLE_VALUE)
		{	
			m_statted = true;
			if (m_st.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) setGlyph(UIG_SUBMENU);
			::FindClose(hFind);
		}
	}
	return m_st;
}
#else


struct stat &PcwFileEntry::stat(void)
{
	if (!m_statted)
	{
		if (!::stat((m_dirname + m_filename).c_str(), &m_st)) 
		{	
			m_statted = true;
			if (S_ISDIR(m_st.st_mode)) setGlyph(UIG_SUBMENU);
		}
	}
	return m_st;
}

#endif
