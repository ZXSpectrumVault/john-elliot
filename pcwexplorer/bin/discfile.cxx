

/* PCW Explorer - access Amstrad PCW discs on Linux or Windows
    Copyright (C) 2000, 2006  John Elliott

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
#include "wx/wxprec.h"
#ifdef __BORLANDC__
    #pragma hdrstop
#endif
#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif

#include "pcwplore.hxx"
#include "discfile.hxx"


int PcwDiscFile::Open(const cpmInode *root, int group, const char *name, int mode)
{
	char cpmname[20], *dot;

	dot = strrchr((char *)name, '.');

	if (dot) sprintf(cpmname,"%02d%-8.8s%-3.3s", group, name, dot+1);
	else	 sprintf(cpmname,"%02d%-8.8s%3s",    group, name, " ");

	printf("Opening %s\n", name);
	
	if (cpmNamei(root, cpmname, &m_ino) == -1) return -1;

	return EWrap(cpmOpen(&m_ino, this, mode));
}

int PcwDiscFile::Open(struct cpmInode *ino, int mode)
{
	memcpy(&m_ino, ino, sizeof(m_ino));
	return EWrap(cpmOpen(&m_ino, this, mode));
} 

int PcwDiscFile::Read(void *buf, int len)
{
	return cpmRead(this, (char *)buf, len);
}

int PcwDiscFile::Write(void *buf, int len)
{
        return cpmWrite(this, (char *)buf, len);
}

int PcwDiscFile::Close(void)
{
	return EWrap(cpmClose(this));
}

int PcwDiscFile::EWrap(int error)
{
	if (!error) return error;

	wxMessageBox(boo, "CP/M disc error", wxOK | wxICON_ERROR);
	return error;
}
