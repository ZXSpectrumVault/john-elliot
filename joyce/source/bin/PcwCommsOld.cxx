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

#include "Pcw.hxx"

void com_ems(Z80 *R);

void fid_com(Z80 *R)
{
//
// Stubs for the JOYCE 1.x comms API. This has now been replaced with the 
// emulated CPS8256.
//
	switch(R->c)
	{
		case 3: R->a = 0;    break;
		case 4: R->a = 0x1A; break;
		case 5: R->a = 0;    break;
		case 7: com_ems(R);  break;
		case 8: break;
		case 9: R->a = 0x80; break;
	}
	return;
}
