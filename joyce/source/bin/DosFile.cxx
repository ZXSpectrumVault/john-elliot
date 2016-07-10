/************************************************************************

    JOYCE v2.1.0 - Amstrad PCW emulator

    Copyright (C) 1996, 2001-2  John Elliott <seasip.webmaster@gmail.com>

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

#include "Anne.hxx"
#include "DosFile.hxx"

DosFile::DosFile()
{
	m_data = NULL;
	m_len  = 0;
}


DosFile::DosFile(int l) 
{
	if (l) m_data = new byte[l];
	else m_data = NULL;
	m_len = l;
}

void DosFile::load(byte *d, int l)
{
	if (m_data) delete m_data;
	m_data = new byte[l];
	memcpy(m_data, d, l);
	m_len = l;
}

byte *DosFile::lookup(const char *name, bool isdir)
{
	int n;
	char ns[12];
	
	sprintf(ns, "%-11.11s", name);
	for (n = 0; n < m_len; n += 32)
	{
		if (memcmp(ns, m_data + n, 11)) continue;
		if (((m_data[n+11] & 0x10) != 0) != isdir) continue;
		return m_data + n;
	}
	return NULL;
}

byte *DosFile::lookup16(const char *name)
{
	int n;
	char ns[33];

	sprintf(ns, "%-32.32s", name);	
	for (n = 0; n < m_len; n += 64)
	{
		if (memcmp(ns, m_data + n + 30, 32)) continue;
		return m_data + n;
	}
	return NULL;
}

DosFile::~DosFile() 
{	
	if (m_data) delete m_data;
}
