/************************************************************************

    JOYCE v2.1.0 - Amstrad PCW emulator

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

#include "Pcw.hxx"
#include "inline.hxx"
#include <errno.h>

#define TMPDIR "/tmp"
#ifdef WIN32 
# ifndef S_IRUSR
#  define S_IRUSR 0000400
#  define S_IWUSR 0000200
# endif
#endif

string gl_homeRoot;

char *name_from_path(char *s)
{
	char *title = s;

	s = strrchr(title, '/');
	if (s) title = s + 1;
#ifdef WIN32
	s = strrchr(title, '\\');
	if (s) title = s + 1;
	s = strrchr(title, ':');
	if (s) title = s + 1;
#endif
	return title;
}


string getHomeRoot(void)
{
	return gl_homeRoot;
}

#ifdef UNIX

#include <pwd.h>

string getHomeDir(void)
{
	const char *s; 
	static char cwdbuf[PATH_MAX];

	// Firstly, try $HOME. If that fails, parse /etc/passwd.
	s = getenv("HOME");	

	if (s == NULL)
	{
		int uid = getuid();
		struct passwd * pw;
		setpwent();
		while (pw = getpwent())
		{
			if (pw->pw_uid == uid)
			{
				s = pw->pw_dir;
				endpwent();
				break;
			}
		}	
		endpwent();
	}
	// Try to get the current directory as an absolute path. If 
	// that fails, try a relative path.
	if (s == NULL) s = getcwd(cwdbuf, PATH_MAX);
	if (s == NULL) s = "."; 

	return s;
}

// irrelevant in Unix
void setHomeDir(string s)  
{

}

string getLibDir(void)
{
	return string(LIBROOT) + "/";
}


void checkExistsDir(const string path)
{
	DIR *dir = opendir(path.c_str());
        if (dir == NULL && errno == ENOENT) mkdir(path.c_str(), 0755);
        if (dir != NULL) closedir(dir);
}

#elif defined(WIN32)

#include <windows.h>
#include <shlobj.h>


//
// Find the path to a Windows special folder
//
string getShellFolder(int csidl)
{
	// Pointer to the shell's IMalloc interface. 
	LPMALLOC pMalloc; 
	LPITEMIDLIST pidl; 
//	LPSHELLFOLDER pFolder; 
	char sBuffer[MAX_PATH];
	string result;

	// Try to get the current directory as an absolute path. If 
	// that fails, try a relative path. This will be used if the
	// "My Documents" folder doesn't exist. 

	char *s = getcwd(sBuffer, MAX_PATH);
	if (s == NULL) strcpy(sBuffer, ".");
	result = sBuffer;

	// Get the shell's allocator. 
	if (!SUCCEEDED(SHGetMalloc(&pMalloc))) return result;
 
	// Get the PIDL for the folder. 
	if (SUCCEEDED(SHGetSpecialFolderLocation(NULL, csidl, &pidl))) 
	{ 
		if (SHGetPathFromIDList(pidl, sBuffer)) result = sBuffer;
		// Free the PIDL for the folder. 
       		pMalloc->Free(pidl); 
	} 
	// Release the shell's allocator. 
	pMalloc->Release(); 
	return result; 
} 

string getHomeDir(void) 
{ 
	// [v1.9.2] See if the user would prefer the JOYCE directory
	// to be elsewhere
	HKEY hk;
	long l = RegOpenKeyEx(HKEY_CURRENT_USER,
				"Software\\jce@seasip\\JOYCE", 0, KEY_READ, &hk);
	if (l == ERROR_SUCCESS)
	{
		char buf[MAX_PATH];
		DWORD dws;

		dws = MAX_PATH;
		l = RegQueryValueEx(hk, "HomeDir", NULL, NULL, (BYTE *)buf, &dws);
		RegCloseKey(hk);
		if (l == ERROR_SUCCESS) return buf;
	}
	return getShellFolder(CSIDL_PERSONAL); 
}


void setHomeDir(string s) 
{ 
	// [v1.9.2] See if the user would prefer the JOYCE directory
	// to be elsewhere
        HKEY hKey;
        DWORD disp;

        long r = RegCreateKeyEx(HKEY_CURRENT_USER, "Software\\jce@seasip\\JOYCE",
                        0, "REG_SZ", REG_OPTION_NON_VOLATILE,
                        KEY_ALL_ACCESS, NULL, &hKey, &disp);
        if (r != ERROR_SUCCESS) return;
        RegSetValueEx(hKey, "HomeDir", 0, REG_SZ, (BYTE *)(s.c_str()),
                        1 + s.size());
        RegCloseKey(hKey);
}


string getLibDir(void)
{
	char modFile[MAX_PATH];
	string s;
	int n;

	GetModuleFileName(NULL, modFile, MAX_PATH);
	s = modFile;

	for (n = s.size() - 1; n >= 0; n--)
	{
		if (s[n] == '\\' || s[n] == '/' || s[n] == ':') 
		{
			s = s.substr(0, n + 1); //s.erase(n + 1);
			s += "lib/";
			return s;
		}
	}
	return "./lib/";
}


void checkExistsDir(const string path)
{
	WIN32_FIND_DATA wfd;
	HANDLE hFind;

	if (path.size() == 1 && path[0] == '/') return;
	if (path.size() == 3 && path[1] == ':' && 
	    (path[2] == '/' || path[2] == '\\')) return;
	if (path.size() == 2 && path[1] == ':')  return;

	hFind = FindFirstFile(path.c_str(), &wfd);
	if (hFind == INVALID_HANDLE_VALUE)
	{
		mkdir(path.c_str());
	}
	else 
	{
		if (!(wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
		{
			joyce_dprintf("%s is a file where Joyce was expecting "
				"a directory. No good will come of it.\n",
				path.c_str());
		}
		FindClose(hFind);
	}
}

#else	

string getHomeDir(void)
{
	return ".";
}


void checkExistsDir(const string path)
{
	DIR *dir = opendir(path.c_str());
        if (dir == NULL && errno == ENOENT) mkdir(path.c_str(), 0755);
        if (dir != NULL) closedir(dir);
}

#endif

string findPcwDir(PcwFileType t, bool global)
{
	string s, dirc;
	
	if (global) s = getLibDir();
	else	    s = getHomeDir() + "/" + getHomeRoot() + "/";

        switch(t)
        {
                case FT_NORMAL:		dirc = ""; 		break;
                case FT_BOOT:		dirc = "Boot";		break;
                case FT_DISC:		dirc = "Disks";		break;
                case FT_PNGS:		dirc = "Pngs";		break;
                case FT_PS:		dirc = "PS";		break;
		case FT_ANNEFSROOT:	dirc = "PcW16FS";	break;
                case FT_OLDBOOT:	dirc = "boot";		break;
                case FT_OLDDISC:	dirc = "disks";		break;
        }
	return s + dirc; 
}



static void findPcwFile(PcwFileType t, const string fname, const string mode,
		string &oname, FILE *&pfp)
{
	string path, homepath, libpath, dirc;
	FILE *fp; 

	switch(t)
	{
                case FT_NORMAL:		dirc = ""; 		break;
                case FT_BOOT:		dirc = "Boot/";		break;
                case FT_DISC:		dirc = "Disks/";	break;
                case FT_PNGS:		dirc = "Pngs/";		break;
                case FT_PS:		dirc = "PS/";		break;
		case FT_ANNEFSROOT:	dirc = "PcW16FS/";	break;
                case FT_OLDBOOT:	dirc = "boot/";		break;
                case FT_OLDDISC:	dirc = "disks/";	break;
	}
	homepath = getHomeDir();
	homepath += "/";
	checkExistsDir(homepath);
	homepath += getHomeRoot();
	homepath += "/";
	checkExistsDir(homepath);

	path = homepath;
	path += dirc;
	checkExistsDir(path);
	path += fname;
	fp = fopen(path.c_str(), mode.c_str());
	if (fp) 
	{ 
		pfp = fp; 
		oname = path; 
		return;
	}

	libpath = getLibDir();
	checkExistsDir(libpath);
	libpath += dirc;
	checkExistsDir(libpath);
	libpath += fname;
	fp = fopen(libpath.c_str(), mode.c_str());
	if (fp) 
	{ 
		pfp = fp; 
		oname = libpath; 
		return;
	}
	pfp = NULL;
	oname = path;
}

string findPcwFile(PcwFileType ft, const string name, const string mode)
{
	FILE *fp;
	string str;

	findPcwFile(ft,name,mode,str,fp);	
	if (fp) fclose(fp);
	return str;
}

FILE  *openPcwFile(PcwFileType ft, const string name, const string mode)
{
        FILE *fp;
        string str;

        findPcwFile(ft,name,mode,str,fp);
	return fp;
}



string displayName (string filename, int len)
{
	int pod = (len - 3) / 2;
	if ((int)filename.size() < len) return filename;

	return filename.substr(0, pod) + "..." + 
		filename.substr(filename.size() - pod);	
}




FILE *openTemp(char *name, const string type, const string mode)
{
	FILE *fp = NULL;

#ifdef _WIN32
	do
	{
		char tname[PATH_MAX];
		GetTempPath(PATH_MAX, tname);
		if (!GetTempFileName(tname, "joy", 0, name)) return NULL;
		remove(name);

		char *s = strrchr(name, '.');
		if (s) strcpy(s, type.c_str());
		else   strcat(name, type.c_str());
                // The O_EXCL prevents the possibility of overwriting
                // existing files.
                int fd = open(name, O_RDWR | O_CREAT | O_EXCL | O_BINARY, 
					S_IRUSR | S_IWUSR);
		fp = NULL;
 		if (fd >= 0) fp = fdopen(fd, mode.c_str()); //
	} while (!fp); 
#else 
	char *tdir = getenv("TMPDIR");
	do
	{
		// I'm using tmpnam() here. By itself this is not 
		// sufficient to guarantee a unique temporary filename;
		// but mkstemp() doesn't allow me to specify the file 
		// extension (probably not necessary here anyway).
		if (tdir) sprintf(name, "%s/joyceXXXXXXXX", tdir);
		else	  sprintf(name, TMPDIR "/joyceXXXXXXXX");

		if (!tmpnam(name)) continue;
		strcat(name, type.c_str());

		// The O_EXCL | O_NOFOLLOW prevents symlink attacks on the 
		// possibly insecure temp filename I got. 
		int fd = open(name, O_RDWR | O_CREAT | O_EXCL | O_NOFOLLOW, 
				S_IRUSR | S_IWUSR);
		if (fd < 0) continue;

		fp = fdopen(fd, mode.c_str());
		if (!fp) perror(name);
		return fp;
	} while (!fp);
#endif
	return fp;
}



