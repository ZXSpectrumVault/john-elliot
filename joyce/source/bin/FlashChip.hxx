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

typedef enum {
	FM_NORMAL,
	FM_READID,
	FM_READSTAT,
	FM_ERASE1,
	FM_ERASE2,
	FM_ERSUSP,
	FM_ERSUSP2,
	FM_WRBYTE1,
	FM_WRBYTE2,
} FlashMode;

class FlashChip
{
public:
	FlashChip(void);
	~FlashChip();

	void setRam(byte *ram);
	void store(unsigned long addr, byte b);
	byte fetch(unsigned long addr);
	void reset(void);

private:
	byte *m_ram;	
	byte m_stat;
	FlashMode m_mode;
	unsigned long m_eraddr;
	unsigned long m_wraddr;
};


