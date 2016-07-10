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

//
// An abstraction of a path within the filesystem. 
//

class Path
{
protected:
	string m_pathname;

	virtual bool isSep(char c);
public:
	Path();	
	Path(const string s);
	virtual ~Path();

	static bool isDir(const string s);

	inline bool isDir(void)  { return isDir(m_pathname); }
	inline bool isRoot(void) { return isRoot(m_pathname); }
	virtual bool isRoot(const string s);
	virtual bool up(void);
	virtual bool chdir(const string s);
	virtual bool chdirAbs(const string s);
	virtual void listFiles(vector<string> &v);
	virtual void listFolders(vector<string> &v);
	inline string &getPathName() { return m_pathname; }
};


