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
#include "UiContainer.hxx"
#include "UiLabel.hxx"
#include "UiSeparator.hxx"
#include "UiCommand.hxx"
#include "UiSetting.hxx"
#include "UiMenu.hxx"
#include "UiTextInput.hxx"

byte anneMemIO[4];
byte *anneMem[4];
byte *PCWFLASH;
FlashChip gl_chip[2];

AnneMemory::AnneMemory() : PcwMemory()
{
	m_flashRequested   = 2097152L;	// 2Mb
	m_memSizeRequested = 2097152L;	// 2Mb
	m_memSizeActual    = 0L;
	m_flashActual      = 0L;
	m_flashBanks       = 0;
	m_nBanks           = 0;

	for (int n = 0; n < 4; n++)
	{
		anneMemIO[n] = n;
	}
	PCWRAM   = NULL;
	PCWFLASH = NULL;
}

AnneMemory::~AnneMemory()
{
	if (PCWFLASH)   free(PCWFLASH);
	PCWFLASH = NULL;
}


void AnneMemory::reset(void)
{
	PcwMemory::reset();
	gl_chip[0].reset();
	gl_chip[1].reset();
        out(0xF0,0x0);
        out(0xF1,0x0);
        out(0xF2,0x0);
        out(0xF3,0x0);
}


bool AnneMemory::hasSettings(SDLKey *key, string &caption)
{
	*key = SDLK_y;
	caption = "  memorY  ";
	return true;	
}


UiEvent AnneMemory::settings(UiDrawer *d)
{
        int x,y,sel;
        UiEvent uie;
        bool m1024, m2048, f1024, f2048;
	bool custom;
	char cust[50];
        x = y = 0;
        sel = -1;

        while (1)
        {
                UiMenu uim(d);

		m1024 = (m_memSizeRequested == 4*262144L);
		m2048 = (m_memSizeRequested == 8*262144L);
		f1024 = (m_flashRequested   == 4*262144L);
		f2048 = (m_flashRequested   == 8*262144L);

                uim.add(new UiLabel("  Memory options  ", d));
                uim.add(new UiSeparator(d));
		uim.add(new UiLabel("  Memory size requested  ", d));
		uim.add(new UiSetting(SDLK_1, m1024, "   1024K  ", d));
		uim.add(new UiSetting(SDLK_2, m2048, "   2048K  ", d));
                uim.add(new UiSeparator(d));
		uim.add(new UiLabel("   Flash size requested  ", d));
		uim.add(new UiSetting(SDLK_0, f1024, "   1024K  ", d));
		uim.add(new UiSetting(SDLK_4, f2048, "   2048K  ", d));
                uim.add(new UiSeparator(d));
		sprintf(cust, "Current memory size: %ldK", m_memSizeActual / 1024L);
		uim.add(new UiLabel(cust, d));
		sprintf(cust, "Current flash size:  %ldK", m_flashActual / 1024L);
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
			case SDLK_1:	m_memSizeRequested = 4*262144L; break;
			case SDLK_2:	m_memSizeRequested = 8*262144L; break;
			case SDLK_0:	m_flashRequested   = 4*262144L; break;
			case SDLK_4:	m_flashRequested   = 8*262144L; break;
                        case SDLK_ESCAPE: return UIE_OK;
			default: break;
		}
	}
	return UIE_CONTINUE;
}


//
// Try to allocate the requested amount of memory. If that fails, ask for 
// smaller amounts in 1Mb decrements.
//
int AnneMemory::alloc(PcwArgs *args)
{
	// Memory size overridden at boot?
	if (args->m_memSize) 
	{ 
		long s = args->m_memSize;
		args->m_memSize = 0;
		s = ((s + 1048576L)  & (~1048576L));
		m_memSizeRequested = s;
	}

	if (PCWRAM   && m_memSizeRequested == m_memSizeActual) return 16 * m_nBanks;
	if (PCWFLASH && m_flashRequested   == m_flashActual)   return 16 * m_flashBanks;

	if (PCWRAM)   free(PCWRAM);   PCWRAM   = NULL; 
	if (PCWFLASH) free(PCWFLASH); PCWFLASH = NULL; 
	m_memSizeActual = 0L;
	m_flashActual = 0L;
	m_nBanks = 0;
	m_flashBanks = 0;

	re_alloc(m_flashRequested, m_flashActual, m_flashBanks, &PCWFLASH);

	if (m_flashActual < 1048576L) gl_chip[0].setRam(NULL); 
	else			      gl_chip[0].setRam(PCWFLASH);
	if (m_flashActual < 2097152L) gl_chip[1].setRam(NULL);
	else			      gl_chip[1].setRam(PCWFLASH + 1048576L);

	return re_alloc(m_memSizeRequested, m_memSizeActual, m_nBanks, &PCWRAM);
}

// This works like alloc() but doesn't free PCWRAM until it's sure that
// it has something to replace it with.
int AnneMemory::re_alloc(void)
{

	re_alloc(m_flashRequested, m_flashActual, m_flashBanks, &PCWFLASH);
	return re_alloc(m_memSizeRequested, m_memSizeActual, m_nBanks, &PCWRAM);
}


int AnneMemory::re_alloc(long &requested, long &actual, int &nbanks, byte **ram)
{
	long siz  = requested;

	byte *nram;

	while (siz >= 1048576L)
	{
		nram=(byte *)malloc(siz);
		if (nram)
		{
			nbanks     = siz / 16384;
			actual     = siz;
			if (*ram) free(*ram);
			*ram = nram;	
//
// Fill the memory with 1 bytes. 0 bytes crash the checksum algorithm if
// the PCW16 has fully-populated flash & RAM. 1 bytes crash the 
// flash compaction algorithm.
			memset(nram, 1, siz);
//			memset(nram, 0x80, siz);
			return(siz / 1024);
		}
		siz -= 1048576L;
	}
	// OK, we got nowhere. But the existing array should still be OK.
	return(0);
}



/* The PCW16 has two discontinous memory ranges: Flash starting at 0,
 * and RAM starting at 2Mb. */
void AnneMemory::out(byte port, byte value)
{
	int bankoff;
	byte *mem  = (value & 0x80) ? PCWRAM   : PCWFLASH;
	int nbanks = (value & 0x80) ? m_nBanks : m_flashBanks;

	port &=3;               /* This handles the memory paging. */
	anneMemIO[port] = value;
//	printf("Select RAM: port=%d value=%02x nbanks=%d\n", port, value, nbanks);

	value = (value & 0x7F) % nbanks;     
//	printf("mod value=%d\n", value);
	bankoff=(value-port)*16384;
	anneMem[port] = mem+bankoff;
}


byte AnneMemory::in(byte port)
{
	return anneMemIO[port % 4];
}

byte AnneMemory::getPage(byte port)
{
	return in(port);
}



bool AnneMemory::parseNode(xmlDocPtr doc, xmlNsPtr ns, xmlNodePtr cur)
{
	char *t = (char *)xmlGetProp(cur, (xmlChar *)"flash");
	if (t) { m_flashRequested = 1024L * atol(t); xmlFree(t); }
	return PcwMemory::parseNode(doc, ns, cur);
}


bool AnneMemory::storeNode(xmlDocPtr doc, xmlNsPtr ns, xmlNodePtr cur)
{
        char buf[30];

        sprintf(buf, "%ld", m_flashRequested / 1024);
        xmlSetProp(cur, (xmlChar *)"flash", (xmlChar *)buf);
	return PcwMemory::storeNode(doc, ns, cur);
}


void flashStore(byte *addr, byte value)
{
	if (addr - PCWFLASH >= 1048576L)
	{
		gl_chip[1].store((addr - PCWFLASH) - 1048576L, value);
	}
	else	gl_chip[0].store(addr - PCWFLASH, value);
}


byte flashFetch(byte *addr)
{
	if (addr - PCWFLASH >= 1048576L)
	{
		return gl_chip[1].fetch((addr - PCWFLASH) - 1048576L);
	}
	return gl_chip[0].fetch(addr - PCWFLASH);
}


