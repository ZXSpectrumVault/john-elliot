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
#include "PcwMenuFont.hxx"
#include "inline.hxx"

PcwSdlTerm::PcwSdlTerm(const char *id, PcwSystem *s) : PcwTerminal(id)
{
	m_sys = s;
	m_term = NULL;
	m_oldTerm = NULL;
	m_tick = 0;
        m_clrFG.r = m_clrFG.g = m_clrFG.b = 0xFF;
	m_clrBG.r = m_clrBG.g = m_clrBG.b = 0x00;
	XLOG("PcwSdlTerm::PcwSdlTerm()");
}


PcwSdlTerm::~PcwSdlTerm()
{
	char buf[50];
	sprintf(buf, "PcwSdlTerm::~PcwSdlTerm() %p", m_term);
	XLOG(buf);
	if (m_term) delete m_term;
	XLOG("PcwSdlTerm::~PcwSdlTerm() 2");
}


void PcwSdlTerm::reset(void)
{
	if (m_term) m_term->reset();
}

bool PcwSdlTerm::storeNode(xmlDocPtr doc, xmlNsPtr ns, xmlNodePtr cur)
{
	return storeColour(doc, ns, cur, "colour", m_clrFG, m_clrBG);
}

bool PcwSdlTerm::parseNode(xmlDocPtr doc, xmlNsPtr ns, xmlNodePtr cur)
{
	return parseColour(doc, ns, cur, "colour", m_clrFG, m_clrBG);
}


void PcwSdlTerm::setForeground(byte r, byte g, byte b)
{
	m_clrFG.r = r;
	m_clrFG.g = g;
	m_clrFG.b = b;

	if (m_term) m_term->setForeground(r,g,b);
}


void PcwSdlTerm::setBackground(byte r, byte g, byte b)
{
	m_clrBG.r = r;
	m_clrBG.g = g;
	m_clrBG.b = b;
	if (m_term) m_term->setBackground(r,g,b);
}


void PcwSdlTerm::onScreenTimer(void)
{
	++m_tick;
	if (m_tick == 18) 
	{
		if (m_term) m_term->onTimer50();
		m_tick = 0;
	}
}

void PcwSdlTerm::onDebugSwitch(bool b)
{
	if (m_oldTerm) m_oldTerm->onDebugSwitch(b);
	PcwTerminal::onDebugSwitch(b);
}


PcwTerminal& PcwSdlTerm::operator <<(unsigned char c)
{
	if (m_term)  (*m_term) << c; 
	return (*this);
}


void PcwSdlTerm::beepOn(void)
{
	if (m_term && !m_beepStat) (*m_term) << '\7';
	PcwTerminal::beepOn();
}


int ui_char_w() { return 8; }
int ui_char_h() { return 16; }
