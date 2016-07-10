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
#include "UiScrollingMenu.hxx"
#include "PcwFileList.hxx"
#include "PcwFileChooser.hxx"

string displaySocket (const string filename, int len);

PcwPrinter::PcwPrinter(const string portname) : PcwLibLink("printer", portname)
{
	long l[1];

	string s = "PcwPrinter::PcwPrinter {";
	s += portname;
	s += "} [0]";
	XLOG(s.c_str());

	time(&m_tick);
	m_timeout = 30;
	XLOG("Leaving PcwPrinter::PcwPrinter()");
}

PcwPrinter::~PcwPrinter()
{
}


void PcwPrinter::writeChar(char c)
{
	ll_error_t e;

	if (!isEnabled()) return;

	if (!m_lldev) open();
	if (m_lldev) e = ll_send_byte(m_lldev, c);
	showError(e);
	time(&m_tick);
}


bool PcwPrinter::isBusy(void)
{
	unsigned busyline;
	ll_error_t e;

	if (!isEnabled()) return true;

	if (!m_lldev) open();
	if (!m_lldev) return true;

	e = ll_rctl_poll(m_lldev, &busyline);
	showError(e);
	if (e != LL_E_OK) return true;	
	if (busyline & LL_CTL_BUSY) return true;
	return false;
}


void PcwPrinter::printerTick(void)
{
	time_t t2;
	ll_error_t e = LL_E_OK;

	time(&t2);
	if ((t2 - m_tick) > m_timeout)  close();
	else if (m_lldev) e = ll_flush(m_lldev);
	showError(e);
}

bool PcwPrinter::parseNode(xmlDocPtr doc, xmlNsPtr ns, xmlNodePtr cur)
{
	if (!PcwLibLink::parseNode(doc, ns, cur)) return false;

	char *t = (char *)xmlGetProp(cur, (xmlChar *)"timeout");
	if (t) 
	{
		m_timeout = atoi(t);
		xmlFree(t);
	}
	return true;	
}


bool PcwPrinter::storeNode(xmlDocPtr doc, xmlNsPtr ns, xmlNodePtr cur)
{
	char buf[20];
	
	if (!PcwLibLink::storeNode(doc, ns, cur)) return false;

	sprintf(buf, "%d", m_timeout);
	xmlSetProp(cur, (xmlChar *)"timeout", (xmlChar *)buf);	
	return true;
}

string PcwPrinter::getTitle(void)
{
	return "Printer";
}









