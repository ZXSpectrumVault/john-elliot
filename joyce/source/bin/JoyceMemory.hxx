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
//
// Memory definitions are global, for speed.
//
// The idea of the paging is that PCWRD[addr>>14][addr] will give the 
//   address of the byte at addr. Separate read and write pointers are 
//     maintained because of the CPC-like paging mode (which AFAIK is not 
//       used by any PCW software). 
//       
extern byte *PCWRD[4];          /* Pointers to the current memory banks - reading */
extern byte *PCWWR[4];          /* Pointers to the current memory banks - writing */

class JoyceMemory : public PcwMemory
{
protected:
	byte m_bankLock[4];       /* Read & write to the same bank? */
	byte m_lastPage[4];

public:
	JoyceMemory();
	~JoyceMemory();
	virtual void reset(void);
        virtual bool hasSettings(SDLKey *key, string &caption);
        virtual UiEvent settings(UiDrawer *d);
        virtual int alloc(PcwArgs *args);
        virtual int re_alloc(void);

//
// Set the memory locking bits for CPC paging mode.
//
	void setLock(byte lock);
//
// Page the memory
//
	void out(byte port, byte value);
	byte in(byte port);
	virtual byte getPage(byte port);
};


