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

class PcwFileEntry : public UiCommand 
{
private:
#ifdef WIN32
	WIN32_FIND_DATA m_st;
#else
	struct stat m_st;
#endif
	bool	m_statted;
	string  m_filename;
	string	m_dirname;
public:
	PcwFileEntry(string filename, string dirname, UiDrawer *d);
	virtual ~PcwFileEntry();

	virtual void draw(int selected);
	inline string getFilename() { return m_filename; }
#ifdef WIN32
	WIN32_FIND_DATA &stat(void);
#else
	struct stat &stat(void);
#endif
};




