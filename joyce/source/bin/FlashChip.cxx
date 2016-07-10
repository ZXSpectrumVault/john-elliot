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

FlashChip::FlashChip()
{
	m_ram = NULL;
	reset();
}


FlashChip::~FlashChip()
{

}


void FlashChip::setRam(byte *ram)
{
	m_ram = ram;
}

byte FlashChip::fetch(unsigned long addr)
{
	if (!m_ram) return 0xFF;
	
	switch(m_mode)
	{
		case FM_NORMAL:
		case FM_ERASE1:
			return m_ram[addr];
		case FM_WRBYTE2:
		case FM_READSTAT:
		case FM_ERASE2:
		case FM_ERSUSP:
			return m_stat;
		case FM_ERSUSP2:	
			//m_mode = FM_NORMAL;
			return m_stat;
		case FM_READID:
			if (addr & 1) return 0xA2;	// Device code
			else	      return 0x89;
	}
	return 0xFF;
}

void FlashChip::store(unsigned long addr, byte b)
{
	if (!m_ram) return;

	//printf("FlashChip::store(%06x, %02x)\n", addr, b);

	switch (m_mode)	
	{
		default:
			//printf("Unknown Flash command: %02x\n", b);
			reset();
			return;
		case FM_NORMAL:
			if (b == 0xFF) { reset(); return; }
			if (b == 0x90) { m_mode = FM_READID;   return; }
			if (b == 0x70) { m_mode = FM_READSTAT; return; }	
			if (b == 0x50) { m_stat = 0; return; }
			if (b == 0x20) { m_eraddr = addr; m_mode = FM_ERASE1; return; }
			if (b == 0x40) { m_wraddr = addr; m_mode = FM_WRBYTE1; return; }
			if (b == 0x10) { m_wraddr = addr; m_mode = FM_WRBYTE1; return; }
			return;
		case FM_WRBYTE2:	/* After a write */
			if (b == 0x40) { m_wraddr = addr; m_mode = FM_WRBYTE1; return; }
			if (b == 0x10) { m_wraddr = addr; m_mode = FM_WRBYTE1; return; }
			/* FALL THROUGH */
		case FM_READID:
		case FM_READSTAT:
		case FM_ERSUSP2:
			if (b == 0xFF) m_mode = FM_NORMAL;
			return;
		case FM_ERASE1:
			if (addr == m_eraddr && b == 0xD0)
			{
				m_mode = FM_ERASE2;
				addr &= ~(0xFFFF);	
//printf("Erasing Flash from %06x\n", addr);
				//memset(m_ram + addr, 0, 0x10000);
				memset(m_ram + addr, 0xFF, 0x10000);
				m_stat |= 0x80;	/* Done! */
				return;
			}
			else m_mode = FM_NORMAL;
			break;
		case FM_ERASE2:
			if (b == 0xB0) m_mode = FM_ERSUSP;
			if (b == 0xFF) m_mode = FM_NORMAL;
			break;
		case FM_ERSUSP:
			if (b == 0xD0) m_mode = FM_ERASE2;
			if (b == 0xFF) m_mode = FM_NORMAL;
			if (b == 0x70) 
			{
				m_mode = FM_ERSUSP2;
				m_stat |= 0x80;		// OK
				m_stat &= ~0x40;	// Erase complete 
			}
			break;
		case FM_WRBYTE1:
			if (addr == m_wraddr) m_ram[addr] = b;
			m_stat |= 0x80;	/* Done! */
			m_mode = FM_WRBYTE2;
			break;
	}
		
}

void FlashChip::reset(void)
{
	m_mode = FM_NORMAL;
	m_stat = 0;
}

