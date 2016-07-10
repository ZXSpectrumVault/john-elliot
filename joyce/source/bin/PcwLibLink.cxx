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

PcwLibLink::PcwLibLink(const string devname, const string portname) : PcwDevice(devname, portname)
{
	string s = "PcwLibLink::PcwLibLink {";
	s += portname;
	s += "} [0]";
	XLOG(s.c_str());

//	m_filename[0] = 0;	// [v1.9.1] You can't *do* that with STL 
				// strings!

	m_lldev = NULL;
	m_llwarned = false;		
				
#if defined(WIN32)
	m_llfilename = "lpt1:";
	m_lldriver = "parfile";
#elif defined(UNIX)
	m_llfilename = "lpr";
	m_lldriver = "parpipe";
#else
	m_llfilename = "";
	m_lldriver = "";
#endif
}

PcwLibLink::~PcwLibLink()
{
	close();
}


void PcwLibLink::reset(void)
{
	close();
	m_llwarned = false;
}


void PcwLibLink::showError(ll_error_t e)
{
	if (e == LL_E_OK) return;
	if (m_llwarned) return;
	m_llwarned = true;
	string serr(ll_strerror(e));

	// XXX wrong
	string sErr = "Failed to open LibLink connection to " + m_llfilename 
	     + ": " + serr;
	fprintf(stderr, "Error %d: %s\n", e, serr.c_str());
	alert(sErr.c_str());
}




bool PcwLibLink::open(void)
{
	ll_error_t e;
	if (m_llfilename == "") return false;

	e = ll_open(&m_lldev, m_llfilename.c_str(), m_lldriver.c_str());
	showError(e);
	if (e != LL_E_OK) return false;
	return true;
}

void PcwLibLink::close()
{
	ll_error_t e;
	if (!m_lldev) return;

	e = ll_close(&m_lldev);
	showError(e);
}

bool PcwLibLink::parseNode(xmlDocPtr doc, xmlNsPtr ns, xmlNodePtr cur)
{
	char *t;
	string filename;
	close();

	parseEnabled(doc, ns, cur);
	parseFilename(doc, ns, cur, "liblinkOutput", m_llfilename);
	parseFilename(doc, ns, cur, "liblinkDriver", m_lldriver);

	if (m_llfilename == "")	// Legacy support...
	{
		parseFilename(doc, ns, cur, "output", filename);
		if (filename[0] == '|') 
		{
			m_llfilename = filename.substr(1);
			m_lldriver   = "parpipe";
		}
		else if (filename != "")
		{
			m_llfilename = filename;
			m_lldriver   = "parfile";
		}
	}
	return true;	
}

bool PcwLibLink::storeNode(xmlDocPtr doc, xmlNsPtr ns, xmlNodePtr cur)
{
	char buf[20];

	storeEnabled(doc, ns, cur);
	storeFilename(doc, ns, cur, "output", "");
	storeFilename(doc, ns, cur, "liblinkOutput", m_llfilename);
	storeFilename(doc, ns, cur, "liblinkDriver", m_lldriver);
	return true;
}


UiEvent PcwLibLink::getPipe(UiDrawer *d)
{
	UiMenu uim(d);
	UiEvent e;

	uim.add(new UiTextInput(SDLK_u, "  UNIX command: _______________________________  ", d));
	uim.add(new UiSeparator(d));
	uim.add(new UiCommand(SDLK_o, "  OK  ", d));

	d->centreContainer(uim);
	if (m_lldriver == "parpipe") ((UiTextInput &)uim[0]).setText(m_llfilename);	
	else			     ((UiTextInput &)uim[0]).setText("");

	e = uim.eventLoop();

	if (e == UIE_OK)
	{
		m_lldriver = "parpipe";
		m_llfilename = ((UiTextInput &)uim[0]).getText();
	}
	return e;
}


UiEvent PcwLibLink::getSocket(UiDrawer *d)
{
	int x,y,sel;
	UiEvent uie;
	bool tcps, unixs, client, server;
	string sockname;
/* Parse the connection string */
	tcps = unixs = client = server = false;
	if (m_lldriver == "parsocket")
	{
		const char *s = m_llfilename.c_str();
		char cs;

		if      (m_llfilename.substr(0, 5) == "unix:") unixs = true;
		else if (m_llfilename.substr(0, 4) == "tcp:")  tcps = true;
	
		s = strchr(s, ':');
		if (s)
		{
			++s;
			cs = toupper(*s);
			if (cs == 'C') client = true;
			if (cs == 'S') server = true;
			s = strchr(s, ':');
			if (s) sockname = s + 1;
		}	
	}
	else
	{
#ifdef HAVE_WINSOCK_H
		tcps = true; 
		client = true; 
		sockname = "127.0.0.1:8256";
#else
		unixs = true; 
		client = true; 
		sockname = "/tmp/locolink.socket";
#endif	
	}	
	
	x = y = 0;
	sel = -1;
	UiTextInput *te;
	while (1)
	{
		UiMenu uim(d);

		uim.add(new UiLabel("  Set up options for socket connection  ", d));
		uim.add(new UiSeparator(d));
#ifndef HAVE_WINSOCK_H	// WINSOCK doesn't support UNIX domain sockets
		uim.add(new UiSetting(SDLK_u, unixs, "  UNIX domain socket  ", d));
		uim.add(new UiSetting(SDLK_t, tcps,  "  TCP/IP socket ",    d));
		uim.add(new UiSeparator(d));
#endif
		uim.add(new UiSetting(SDLK_c, client,  "  Client (Normal)  ", d));
		uim.add(new UiSetting(SDLK_s, server,  "  Server (LocoLink slave)  ", d));
		uim.add(new UiSeparator(d));
		uim.add(te = new UiTextInput(SDLK_n, "  Socket Name: _______________________________  ", d));
		uim.add(new UiSeparator(d));
		uim.add(new UiCommand(SDLK_o, "  OK  ", d));
		uim.add(new UiCommand(SDLK_ESCAPE, "  EXIT  ", d));

		d->centreContainer(uim);
		te->setText(sockname);	
	
		uim.setSelected(sel);
		uie = uim.eventLoop();
		if (uie == UIE_QUIT) return uie;
		sel = uim.getSelected();	
		switch(uim.getKey(sel))
		{
			case SDLK_u: tcps = false; unixs = true; uie = UIE_CONTINUE; break;
			case SDLK_t: tcps = true; unixs = false; uie = UIE_CONTINUE; break;
			case SDLK_s: server = true; client = false; uie = UIE_CONTINUE; break;	
			case SDLK_c: server = false; client = true; uie = UIE_CONTINUE; break;	
			case SDLK_n: sockname = te->getText(); uie = UIE_CONTINUE; break;
			case SDLK_ESCAPE: return UIE_CANCEL;
			case SDLK_o:
				    m_lldriver = "parsocket";
				    if (unixs) m_llfilename = "unix:";
				    else       m_llfilename = "tcp:";
				    if (server) m_llfilename += "S:";
				    else        m_llfilename += "C:";
				    m_llfilename += sockname;
				    return UIE_OK;				
		}
	}
	return uie;
}


string displaySocket (const string filename, int len)
{
	string p1;
	const char *s = filename.c_str();
	char cs;

	s = strchr(s, ':'); if (!s) return displayName(filename, len);
	++s;
	cs = toupper(*s);

	if      (filename.substr(0, 5) == "unix:") p1 = "UNIX domain";
	else if (filename.substr(0, 4) == "tcp:")  p1 = "TCP/IP";
	else return displayName(filename, len);

	if      (cs == 'S') p1 += " server";
	else if (cs == 'C') p1 += " client"; 
	s = strchr(s, ':'); if (!s) return displayName(filename, len);
	p1 += s;
	
	return displayName(p1, len);
}

UiEvent PcwLibLink::getPort(UiDrawer *d)
{
	UiEvent e;
	PcwFileChooser f("  OK  ", d);
	if (m_lldriver == "parport") f.initDir(m_llfilename);
	e = f.eventLoop();
	if (e == UIE_QUIT) return e;
	if (e == UIE_OK) 
	{ 
		m_lldriver = "parport"; 
		m_llfilename = f.getChoice(); 
	}
	return e;
}

UiEvent PcwLibLink::getFile(UiDrawer *d)
{
	UiEvent e;
	PcwFileChooser f("  OK  ", d);
	if (m_lldriver == "parfile") f.initDir(m_llfilename);
	e = f.eventLoop();
	if (e == UIE_QUIT) return e;
	if (e == UIE_OK) 
	{ 
		m_lldriver = "parfile"; 
		m_llfilename = f.getChoice(); 
	}
	return e;
}


UiEvent PcwLibLink::settings(UiDrawer *d)
{
	int x,y,sel;
	UiEvent uie;
	
	x = y = 0;
	sel = -1;
	while (1)
	{
		UiMenu uim(d);
		bool bEnabled;

		bEnabled = isEnabled();
		uim.add(new UiLabel(getTitle(), d));
		uim.add(new UiSeparator(d));
		uim.add(new UiSetting(SDLK_d, !bEnabled, "  Disconnected  ", d));
		uim.add(new UiSetting(SDLK_c,  bEnabled, "  Connected  ",    d));

		if (bEnabled)
		{
			string ostat = "  Output is to ";
			if      (m_lldriver == "parpipe"  ) ostat += "pipe: " + displayName(m_llfilename, 40);
			else if (m_lldriver == "parfile"  ) ostat += "file: " + displayName(m_llfilename, 40);
			else if (m_lldriver == "parport"  ) ostat += "port: " + displayName(m_llfilename, 40);
			else if (m_lldriver == "parsocket") ostat += displaySocket(m_llfilename, 46);
			else ostat += m_lldriver + ": " + displayName(m_llfilename, 40);
			uim.add(new UiSeparator(d));
			uim.add(new UiLabel  (ostat, d)); 
			uim.add(new UiCommand(SDLK_f,  "  Output to File...  ", d, UIG_SUBMENU));
			uim.add(new UiCommand(SDLK_u,  "  Output to UNIX command... ",d, UIG_SUBMENU));
#ifdef HAVE_LINUX_PARPORT_H
			uim.add(new UiCommand(SDLK_p,  "  Output to Parallel port... ",d, UIG_SUBMENU));
#endif
			uim.add(new UiCommand(SDLK_s,  "  Output to Socket... ",d, UIG_SUBMENU));
#ifdef WIN32
			uim.add(new UiCommand(SDLK_1,  "  Output to LPT1:  ", d));
			uim.add(new UiCommand(SDLK_2,  "  Output to LPT2:  ", d));
			uim.add(new UiCommand(SDLK_3,  "  Output to LPT3:  ", d));
			uim.add(new UiCommand(SDLK_4,  "  Output to LPT4:  ", d));
#endif
		}

		d->centreContainer(uim);
		uim.add(new UiSeparator(d));
		uim.add(new UiCommand(SDLK_ESCAPE,  "  EXIT  ", d));
		uim.setSelected(sel);

		uie =  uim.eventLoop();
	
		if (uie == UIE_QUIT) return uie;
		sel = uim.getSelected();	
		switch(uim.getKey(sel))
		{
			case SDLK_1: m_llfilename = "LPT1:"; m_lldriver = "parfile"; break;
			case SDLK_2: m_llfilename = "LPT2:"; m_lldriver = "parfile"; break;
			case SDLK_3: m_llfilename = "LPT3:"; m_lldriver = "parfile"; break;
			case SDLK_4: m_llfilename = "LPT4:"; m_lldriver = "parfile"; break;
			case SDLK_d: enable(false); break;
			case SDLK_c: enable();      break;
			case SDLK_f:    uie = getFile(d);
					if (uie == UIE_QUIT) return uie;
					break;
			case SDLK_u:    uie = getPipe(d);
					if (uie == UIE_QUIT) return uie;
					break;
			case SDLK_p:    uie = getPort(d);
					if (uie == UIE_QUIT) return uie;
					break;
			case SDLK_s:    uie = getSocket(d);
					if (uie == UIE_QUIT) return uie;
					break;

			case SDLK_ESCAPE: close();  return UIE_OK;
			default: break;
		}

	}
	return UIE_OK;
}









