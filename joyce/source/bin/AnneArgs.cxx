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

AnneArgs::AnneArgs(int argc, char **argv) : PcwArgs(argc, argv)
{
        m_bootRom         = ""; //findPcwFile(FT_BOOT, BOOT_SYS, "rb");
	m_warmStart 	  = false;
}

const char *AnneArgs::appname()
{
	return "ANNE";
}


bool AnneArgs::processArg(const char *s)
{
	if (!stricmp(s, "-ws")) 
	{
		m_warmStart = true; return true;
	}
	return false;
	
}
