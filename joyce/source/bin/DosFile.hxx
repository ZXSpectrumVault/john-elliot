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


class DosFile
{
	byte *m_data;
	int m_len;

public:	
	DosFile();
	DosFile(int l);
	~DosFile(); 
	void load(byte *d, int l);
	/* Look up an entry, treating this file as a DOS directory */
	byte *lookup(const char *name, bool isdir);
	/* Look up an entry, treating this file as the PCW16 catalogue  */
	byte *lookup16(const char *name);
	inline byte *getData() { return m_data; }
	inline int getLength() { return m_len; }
};
