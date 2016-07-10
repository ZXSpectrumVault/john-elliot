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


AnneRTC::AnneRTC() : PcwDevice()
{
	reset();
}

AnneRTC::~AnneRTC()
{

}


//
// Reset device
//
void AnneRTC::reset(void)
{
	m_ticks = 0;
	m_stopped = false;
}

byte AnneRTC::in(byte port)
{
        time_t t;
        struct tm *ptm;

        if (port > 0xF9)
        {
                time(&t);
                ptm = localtime(&t);
        }
        switch(port)
        {
				// Step down 1000Hz to 256Hz
                case 0xF9: return (SDL_GetTicks() * 256) / 1000;
                case 0xFA: return ptm->tm_sec;		
                case 0xFB: return ptm->tm_min;
                case 0xFC: return ptm->tm_hour;
                case 0xFD: return ptm->tm_mday;
                case 0xFE: return ptm->tm_mon + 1;
                case 0xFF: if (m_stopped) return ((ptm->tm_year - 80) % 100) | 0x80;
                           else return ((ptm->tm_year - 80) % 100);
        }
        return 0xFF;
}


void AnneRTC::out(byte port, byte value)
{
        /* Note: Clock setting is not supported. You get the host system's
         * clock. */

        if (port == 0xF9) m_stopped = !(value & 1);
}



void AnneRTC::tick(void)
{
	if (!m_stopped) ++m_ticks;
}



bool AnneRTC::parseNode(xmlDoc *doc, xmlNs *ns, xmlNode *ptr)
{
	return true;
}

bool AnneRTC::storeNode(xmlDoc *doc, xmlNs *ns, xmlNode *ptr)
{
	return true;
}

