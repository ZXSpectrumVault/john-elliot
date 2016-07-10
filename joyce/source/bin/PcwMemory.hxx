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
extern byte *PCWRAM;            /* The PCW's memory, all 2048k */

class PcwMemory : public PcwDevice
{
protected:
	long m_memSizeRequested,
	     m_memSizeActual;
	int  m_nBanks;	

public:
	PcwMemory();
	~PcwMemory();
	virtual void reset(void);
	inline long sizeRequested() { return m_memSizeRequested; }
	inline long sizeAllocated() { return m_memSizeActual; }
// 
// Allocate memory. Returns size in K.
//
	virtual int alloc(PcwArgs *args) = 0;
	virtual int re_alloc(void) = 0;
//
// Page the memory
//
	virtual void out(byte port, byte value) = 0;
	virtual byte in(byte port) = 0;
	virtual byte getPage(byte port) = 0;

protected:
	virtual bool parseNode(xmlDocPtr doc, xmlNsPtr ns, xmlNodePtr cur);
	virtual bool storeNode(xmlDocPtr doc, xmlNsPtr ns, xmlNodePtr cur);
};


