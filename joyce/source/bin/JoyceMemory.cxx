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
#include "UiContainer.hxx"
#include "UiLabel.hxx"
#include "UiSeparator.hxx"
#include "UiCommand.hxx"
#include "UiSetting.hxx"
#include "UiMenu.hxx"
#include "UiTextInput.hxx"

byte *PCWRD[4];          /* Pointers to the current memory banks - reading */
byte *PCWWR[4];          /* Pointers to the current memory banks - writing */


JoyceMemory::JoyceMemory() : PcwMemory()
{
	m_memSizeRequested = 2097152L;	// 2Mb
	m_memSizeActual    = 0L;
	m_nBanks = 0;

	for (int n = 0; n < 4; n++)
	{
		m_lastPage[n] = 0x80 + n;
		m_bankLock[n] = 0;
	}
	PCWRAM = NULL;
}

JoyceMemory::~JoyceMemory()
{
}


void JoyceMemory::reset(void)
{
	PcwMemory::reset();
        out(0xF0,0x80);
        out(0xF1,0x81);
        out(0xF2,0x82);
        out(0xF3,0x83);
        setLock(0xF1);
}


bool JoyceMemory::hasSettings(SDLKey *key, string &caption)
{
	*key = SDLK_y;
	caption = "  memorY  ";
	return true;	
}


UiEvent JoyceMemory::settings(UiDrawer *d)
{
        int x,y,sel;
        UiEvent uie;
        bool m256, m512, m768, m1024, m1280, m1536, m1792, m2048;
	bool custom;
	char cust[50];
        x = y = 0;
        sel = -1;

        while (1)
        {
                UiMenu uim(d);

		m256  = (m_memSizeRequested ==   262144L);
		m512  = (m_memSizeRequested == 2*262144L);
		m768  = (m_memSizeRequested == 3*262144L);
		m1024 = (m_memSizeRequested == 4*262144L);
		m1280 = (m_memSizeRequested == 5*262144L);
		m1536 = (m_memSizeRequested == 6*262144L);
		m1792 = (m_memSizeRequested == 7*262144L);
		m2048 = (m_memSizeRequested == 8*262144L);

                custom = false;
                if (!(m256  || m512  || m768  || m1024 || 
                      m1280 || m1536 || m1792 || m2048 )) custom = true;
                uim.add(new UiLabel("  Memory options  ", d));
                uim.add(new UiSeparator(d));
		uim.add(new UiLabel("  Memory size requested  ", d));
		uim.add(new UiSetting(SDLK_2,  m256, "    256K  ", d));
		uim.add(new UiSetting(SDLK_5,  m512, "    512K  ", d));
		uim.add(new UiSetting(SDLK_7,  m768, "    768K  ", d));
		uim.add(new UiSetting(SDLK_1, m1024, "   1024K  ", d));
		uim.add(new UiSetting(SDLK_8, m1280, "   1280K  ", d));
		uim.add(new UiSetting(SDLK_3, m1536, "   1536K  ", d));
		uim.add(new UiSetting(SDLK_9, m1792, "   1792K  ", d));
		uim.add(new UiSetting(SDLK_4, m2048, "   2048K  ", d));
                uim.add(new UiSeparator(d));
		sprintf(cust, "Current memory size: %ldK", m_memSizeActual / 1024L);
		uim.add(new UiLabel(cust, d));
                uim.add(new UiSeparator(d));
		uim.add(new UiLabel("Memory size changes will take effect", d));
		uim.add(new UiLabel("when the emulated PCW is rebooted.  ", d));
                uim.add(new UiSeparator(d));
                uim.add(new UiCommand(SDLK_ESCAPE,  "  EXIT  ", d));
                uim.setSelected(sel);
		d->centreContainer(uim);
		uie = uim.eventLoop();
		if (uie == UIE_QUIT) return uie;
                sel = uim.getSelected();
                switch(uim.getKey(sel))
		{
			case SDLK_2:	m_memSizeRequested =   262144L; break;
			case SDLK_5:	m_memSizeRequested = 2*262144L; break;
			case SDLK_7:	m_memSizeRequested = 3*262144L; break;
			case SDLK_1:	m_memSizeRequested = 4*262144L; break;
			case SDLK_8:	m_memSizeRequested = 5*262144L; break;
			case SDLK_3:	m_memSizeRequested = 6*262144L; break;
			case SDLK_9:	m_memSizeRequested = 7*262144L; break;
			case SDLK_4:	m_memSizeRequested = 8*262144L; break;
                        case SDLK_ESCAPE: return UIE_OK;
			default: break;
		}
	}
	return UIE_CONTINUE;
}


//
// Try to allocate the requested amount of memory. If that fails, ask for 
// smaller amounts in 256k decrements.
//
int JoyceMemory::alloc(PcwArgs *args)
{
	// Memory size overridden at boot?
	if (args->m_memSize) 
	{ 
		long s = args->m_memSize;
		args->m_memSize = 0;
		s = ((s + 262143L)  & (~262143L));
		m_memSizeRequested = s;
	}

	long siz = m_memSizeRequested;
	if (PCWRAM && m_memSizeRequested == m_memSizeActual) return 16 * m_nBanks;

	if (PCWRAM) free(PCWRAM); 
	PCWRAM = NULL; 
	m_memSizeActual = 0L;
	m_nBanks = 0;

	while (siz >= 262144L)  /* ie 256k */
	{
		PCWRAM=(byte *)malloc(siz);
		if (PCWRAM)
		{
			m_nBanks     = siz / 16384;
			m_memSizeActual = siz;
			return(siz / 1024);
		}
		siz -= 262144L;
	}
	return(0);
}

// This works like alloc() but doesn't free PCWRAM until it's sure that
// it has something to replace it with.
int JoyceMemory::re_alloc(void)
{
	long siz = m_memSizeRequested;
	byte *nram;

	while (siz >= 262144L)  /* ie 256k */
	{
		nram=(byte *)malloc(siz);
		if (nram)
		{
			m_nBanks     = siz / 16384;
			m_memSizeActual = siz;
			if (PCWRAM) free(PCWRAM);
			PCWRAM = nram;
			return(siz / 1024);
		}
		siz -= 262144L;
	}
	// OK, we got nowhere. But at least PCWRAM is untouched.
	return(0);
}


// Process an OUT 0xF4. If one of the top 4 bits is set, memory reads and
// writes will be to the same bank. This is only ever a concern if the 
// CPC-like paging system is used; according to Richard Clayton CP/M and
// LocoScript do not use this system.


void JoyceMemory::setLock(unsigned char value)
{
	int np;

	m_bankLock[0] = value & 0x40;
	m_bankLock[1] = value & 0x10;
	m_bankLock[2] = value & 0x20;
	m_bankLock[3] = value & 0x80;

	/* If memory was paged the CPC way, then paging may have to be
          recalculated */

	for(np = 0; np < 4; np++)
	{
		if (!(m_lastPage[np] & 0x80)) out(0xF0 | np, m_lastPage[np]);
	}
}

		
void JoyceMemory::out(byte port, byte value)
{
	int bankoff;

	port &=3;               /* This handles the memory paging. */
	m_lastPage[port] = value;
	if (value & 0x80)	/* `Extended' (PCW-like) paging */
	{
		value = (value & 0x7F) % m_nBanks;      /* The idea is that PCWRD[addr>>14] */
		bankoff=(value-port)*16384;  /* +addr gives the correct address */
		PCWWR[port] = PCWRD[port] = PCWRAM+bankoff;
	}
	else
	{
		int rv, wv;
		rv = (value >> 4) & 7;
		wv = value & 7;
		bankoff=(rv-port)*16384;  /* +addr gives the correct address */
		PCWRD[port] = PCWRAM+bankoff;
		bankoff=(wv-port)*16384;  /* +addr gives the correct address */
		PCWWR[port] = PCWRAM+bankoff;
		if (m_bankLock[port]) PCWRD[port] = PCWWR[port];
	}
}


byte JoyceMemory::in(byte port)
{
	return 0xFF;
}


byte JoyceMemory::getPage(byte port)
{
	return m_lastPage[port & 3];
}
