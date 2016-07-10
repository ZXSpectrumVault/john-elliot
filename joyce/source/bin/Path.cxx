/************************************************************************

    JOYCE v2.1.8 - Amstrad PCW emulator

    Copyright (C) 1996, 2001, 2004-5  John Elliott <seasip.webmaster@gmail.com>

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

#include <string>
#include <vector>
#include <algorithm>

using namespace std;

#include <stdio.h>
#include <limits.h>
#include <unistd.h>
#include <dirent.h>
#include "Path.hxx"
#include "config.h"
#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif
#ifdef HAVE_WINDOWS_H
#include <windows.h>
#endif

//
// An abstraction of a path within the filesystem. 
//

Path::Path() 
{
	char *s = getcwd(NULL, 0);
	m_pathname = s;
	free(s);
}

Path::Path(string s)
{
	int l;

	m_pathname = s;
	l = m_pathname.size() - 1;
	// mini-stl doesn't have erase()
	// if (l >= 0 && isSep(m_pathname[l])) m_pathname.erase(l); 
	if (l >= 0 && isSep(m_pathname[l])) m_pathname = m_pathname.substr(0, l);
}


Path::~Path()
{
}


bool Path::isSep(char c)
{
#ifdef WIN32
	if (c == '\\') return true;
#endif
	return (c == '/');
}


bool Path::isRoot(string pathname)
{
#ifdef WIN32
	if (pathname.size() == 3 && pathname[1] == ':' && isSep(pathname[2])) return true;
	if (pathname.size() == 2 && pathname[1] == ':') return true;
#endif
	if (pathname.size() != 1) return false;
	return isSep(pathname[0]);
}


bool Path::up(void)
{
	int n;

	if (isRoot()) return false;

	for (n = m_pathname.size() - 1; n >= 0; n--) if (isSep(m_pathname[n]))
	{
		m_pathname = m_pathname.substr(0, n);
		// If at the root, use "/".
		if (m_pathname == "") m_pathname = "/";
#ifdef WIN32
		// Convert "C:" path to "C:/".
		if (m_pathname.size() == 2 && m_pathname[1] == ':')
			m_pathname += "/";
#endif  
		return true;
	}
#ifdef WIN32
	// Convert "C:" path to "C:/".
	if (m_pathname.size() == 2 && m_pathname[1] == ':')
		m_pathname += "/";
#endif  
	return false;
}


bool Path::chdir(const string newdir)
{
	if (newdir ==  "." ) return true;
	if (newdir ==  "..") return up();

	if (!isRoot(m_pathname)) m_pathname += "/";
	m_pathname += newdir;
	return true;
}

bool Path::chdirAbs(const string newdir)
{
        m_pathname = newdir;

#ifdef WIN32
	// Convert "C:" path to "C:/".
	if (m_pathname.size() == 2 && m_pathname[1] == ':')
		m_pathname += "/";
#endif
        return true;
}

// mini-STL doesn't have sort(). We have to fake it, using qsort().
#ifdef __MBSTRING_H	// not using mini-STL

static vector<string> *st_vect;

int compare(const void *p1, const void *p2)
{
	int n1 = *(int *)p1;
	int n2 = *(int *)p2;

#ifdef WIN32	// case-insensitive sort if possible under Win32

	#if defined(HAVE_STRCASECMP)
		return (strcasecmp((*st_vect)[n1].c_str(), (*st_vect)[n2].c_str()));
	#elif defined(HAVE_STRICMP)
		return (stricmp((*st_vect)[n1].c_str(), (*st_vect)[n2].c_str()));
	#elif defined(HAVE_STRCMPI)
		return (strcmpi((*st_vect)[n1].c_str(), (*st_vect)[n2].c_str()));
	#else
		return (strcmp((*st_vect)[n1].c_str(), (*st_vect)[n2].c_str()));
	#endif
#else
	return (strcmp((*st_vect)[n1].c_str(), (*st_vect)[n2].c_str()));
#endif
}

vector<string> do_sort( vector<string> v)
{
	int *idxs;
	int n;

	if (v.size() < 2) return v;
	idxs = new int[v.size()];
	for (n = 0; n < v.size(); n++) idxs[n] = n;

	st_vect = &v;
	qsort(idxs, v.size(), sizeof(int), compare); 

	vector<string> v2;
	for (n = 0; n < v.size(); n++) 
	{
		v2.push_back(v[idxs[n]]);
	}
	delete idxs;
	return v2;
}

#endif



	
void Path::listFiles(vector<string> &v)
{
	DIR *dir = opendir(m_pathname.c_str());
	struct dirent *de;

	if (!dir)
	{
		v.push_back(".");
		v.push_back("..");
		return;	
	}
	while ((de = readdir(dir)))
	{
		string s = de->d_name;
		v.push_back(s);
	}
	closedir(dir);
#ifndef __MBSTRING_H	// not using mini-STL
	sort(v.begin(), v.end());
#else
	// mini-STL doesn't have sort(). We have to fake it.
	v = do_sort(v);	
#endif
}

void Path::listFolders(vector<string> &v)
{
	DIR *dir = opendir(m_pathname.c_str());
	struct dirent *de;
	struct stat st;

	if (!dir)
	{
		v.push_back(".");
		v.push_back("..");
		return;	
	}
	while ((de = readdir(dir)))
	{
		string s = m_pathname;
		string t = de->d_name;
		s += "/";
		s += t;
		if (isDir(s)) v.push_back(t);
	}
	closedir(dir);
#ifndef __MBSTRING_H	// not using mini-STL
	sort(v.begin(), v.end());
#else
	// mini-STL doesn't have sort(). We have to fake it.
	v = do_sort(v);	
#endif
}


bool Path::isDir(const string s)
{
#ifdef WIN32
	WIN32_FIND_DATA wfd;
	HANDLE hFind;

	hFind = FindFirstFile(s.c_str(), &wfd);
	if (hFind == INVALID_HANDLE_VALUE) return false;
	FindClose(&hFind);
	return ((wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0);
#else
	struct stat st;
	if (stat(s.c_str(), &st)) return false;
	return 	S_ISDIR(st.st_mode);
#endif
}
