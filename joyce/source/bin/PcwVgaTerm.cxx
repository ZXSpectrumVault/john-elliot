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

PcwVgaTerm::PcwVgaTerm(PcwSystem *s) : PcwSdlTerm("vga", s)
{
	XLOG("PcwVgaTerm::PcwVgaTerm()");
	m_menuTerm = NULL;
	m_framebuf = NULL;
	
	m_clrBG.r = 0x00; m_clrBG.g = 0x00; m_clrBG.b = 0xAA;
	m_clrFG.r = 0xFF; m_clrFG.g = 0xFF; m_clrFG.b = 0xFF;
        m_menuBg.r   = 0x00; m_menuBg.g   = 0x00; m_menuBg.b   = 0xAA;
        m_menuFg.r   = 0x55; m_menuFg.g   = 0xFF; m_menuBg.b   = 0xFF;
        m_shadowBg.r = 0x00; m_shadowBg.g = 0x00; m_shadowBg.b = 0x55;
        m_shadowFg.r = 0x55; m_shadowFg.g = 0x55; m_shadowBg.b = 0xAA;
}


PcwVgaTerm::~PcwVgaTerm()
{	
	XLOG("PcwVgaTerm::~PcwVgaTerm()");
	if (m_term)	 delete m_term;
	if (m_menuTerm)  delete m_menuTerm;
}


void PcwVgaTerm::onGainFocus()     // Terminal is gaining focus
{
	if (!m_term) m_term = new SdlVt52(m_sysVideo->getSurface()); 
	PcwTerminal::onGainFocus();
}


////////////////////////////////////////////////////////////////////////////
//
// Code to load/store device settings from XML file
//

bool PcwVgaTerm::parseNode(xmlDocPtr doc, xmlNsPtr ns, xmlNodePtr cur)
{
	parseColour(doc, ns, cur, "menu",   m_menuFg,   m_menuBg);
	parseColour(doc, ns, cur, "shadow", m_shadowFg, m_shadowBg);
	return true;
}


bool PcwVgaTerm::storeNode(xmlDocPtr doc, xmlNsPtr ns, xmlNodePtr cur)
{
	storeColour(doc, ns, cur, "menu",   m_menuFg,   m_menuBg);
	storeColour(doc, ns, cur, "shadow", m_shadowFg, m_shadowBg);
	return true;
}







