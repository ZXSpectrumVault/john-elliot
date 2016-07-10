/************************************************************************
 
    JOYCE v1.90 - Amstrad PCW emulator
 
    Copyright (C) 1996, 2000, 2001  John Elliott <seasip.webmaster@gmail.com>
 
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

#include "Joyce.hxx"
#include <SDL_thread.h>

Executor::Executor(const char *filename, bool removeAfterwards)
{
	strcpy(m_filename, filename);
	m_removeAfterwards = removeAfterwards;
}

static int threadRun(void *p)
{
	Executor *e = (Executor *)p;

	return e->workThread();
}


int Executor::exec(void)
{
	SDL_CreateThread(threadRun, this);
	return 0;
}

int Executor::workThread()
{
	// 2.0.2: Copy our data onto the stack. The "this" pointer can't
	//       be trusted to hang around until the launched program
	//       finishes (at least, it can't in win32; Linux doesn't seem to
	//       mind).
	char filename[PATH_MAX + 1];
	int  removeAfterwards = m_removeAfterwards;
#ifdef _WIN32	
	STARTUPINFO sinfo;
	memset(&sinfo, 0, sizeof(sinfo));
	sinfo.cb = sizeof(sinfo);
	PROCESS_INFORMATION pinfo;	
	char cmdbuf[PATH_MAX * 2 + 4];

	strcpy(filename, m_filename);

	strncpy(cmdbuf, m_filename, PATH_MAX); cmdbuf[PATH_MAX] = 0;

	if (CreateProcess(NULL, cmdbuf, NULL, NULL, 0, NORMAL_PRIORITY_CLASS,
			NULL, NULL, &sinfo, &pinfo))
	{
		WaitForSingleObject(pinfo.hProcess, INFINITE);
	}
	else
	{
		int n = GetLastError();
		LPBYTE buffer;
		FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | 
			      FORMAT_MESSAGE_FROM_SYSTEM, NULL, n, 0, 
				(LPTSTR)&buffer, 20, NULL);
		MessageBox(NULL, (char *)buffer, "Error", MB_OK | MB_ICONSTOP);
		LocalFree(buffer);
	}
	
#else
	strcpy(filename, m_filename);
//
// Use system() to launch the new process.
//
	int result = system(filename);
#endif
	if (removeAfterwards) 
	{
		char *s = strrchr(filename, ' ');
		if (s) remove(s+1);
		else remove(filename);
	}
	return 0;
}


