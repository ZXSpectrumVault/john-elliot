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

#include "Joyce.hxx"


JoyceFDC::JoyceFDC(JoyceSystem *s) : PcwFDC(s)
{
}

JoyceFDC::~JoyceFDC()
{
}

PcwModel JoyceFDC::adaptModel(PcwModel j)
{
	bool is8256, is9256;

	is9256 = is8256 = false;
	if (m_driveMap[2] == 10 && m_driveMap[3] == 11) is9256 = true;
	if (m_driveMap[2] == 12 && m_driveMap[3] == 13) is8256 = true; 

	if (is8256 && j == PCW10   ) return PCW8000;
	if (is8256 && j == PCW9000P) return PCW9000;
	if (is9256 && j == PCW8000 ) return PCW10;
	if (is9256 && j == PCW9000 ) return PCW9000P;
	return j;
}	

void JoyceFDC::setModel(PcwModel j)
{
	bool is8256, is9256;
	int n;

	PcwDevice::setModel(j);

	is9256 = is8256 = false;
	if (m_driveMap[2] == 10 && m_driveMap[3] == 11) is9256 = true;
	if (m_driveMap[2] == 12 && m_driveMap[3] == 13) is8256 = true; 
	if (is8256 == 0 && is9256 == 0) return;	// Don't interfere with
						// custom setup.

	//
	// If there are changes, rewire drives 2 and 3 only.
	//
	if (is9256 && (j == PCW8000 || j == PCW9000))
	{
		m_driveMap[2] = 10; 
		m_driveMap[3] = 11;
        	for (n = 2; n < 4; n++) fd_eject(fdc_getdrive(m_fdc, n));
		reset(2);
	}	
	if (is8256 && (j == PCW9000P || j == PCW10))
	{
		m_driveMap[2] = 12; 
		m_driveMap[3] = 13;
        	for (n = 2; n < 4; n++) fd_eject(fdc_getdrive(m_fdc, n));
		reset(2);
	}	
		
}



