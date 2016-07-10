/************************************************************************

    JOYCE v2.1.0 - Amstrad PCW emulator

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

#include "Pcw.hxx"
#include "UiContainer.hxx"
#include "UiLabel.hxx"
#include "UiSeparator.hxx"
#include "UiCommand.hxx"
#include "UiSetting.hxx"
#include "UiMenu.hxx"
#include "UiTextInput.hxx"

byte *PCWRAM;	/* The PCW's memory */

PcwMemory::PcwMemory() : PcwDevice("memory")
{
	m_nBanks = 0;
}

PcwMemory::~PcwMemory()
{
	if (PCWRAM) free(PCWRAM);
	PCWRAM = NULL;
}


void PcwMemory::reset(void)
{
	if (m_memSizeActual != m_memSizeRequested) re_alloc();

	memset(PCWRAM, 0, m_memSizeActual);	
}


bool PcwMemory::parseNode(xmlDocPtr doc, xmlNsPtr ns, xmlNodePtr cur)
{
	char *t = (char *)xmlGetProp(cur, (xmlChar *)"kbytes");
	if (t) { m_memSizeRequested = 1024L * atol(t); xmlFree(t); }
	return true;
}


bool PcwMemory::storeNode(xmlDocPtr doc, xmlNsPtr ns, xmlNodePtr cur)
{
        char buf[30];

        sprintf(buf, "%ld", m_memSizeRequested / 1024);
        xmlSetProp(cur, (xmlChar *)"kbytes", (xmlChar *)buf);
	return true;
}

//
// Try to allocate the requested amount of memory. If that fails, ask for 
// smaller amounts in 256k decrements.
//
int PcwMemory::alloc(PcwArgs *args)
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
int PcwMemory::re_alloc(void)
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

