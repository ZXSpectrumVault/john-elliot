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

#include "Joyce.hxx"
#include "UiContainer.hxx"
#include "UiLabel.hxx"
#include "UiSeparator.hxx"
#include "UiCommand.hxx"
#include "UiSetting.hxx"
#include "UiMenu.hxx"
#include "UiTextInput.hxx"
#include "UiScrollingMenu.hxx"
#include "PcwFileList.hxx"

#define DAISY_MAX_H 3300	// Longer than an A3 page

//
// Returned values from IN 0xFD
//
#define DAISY_READ	1	/* Controller has data for CPU */
#define DAISY_NOWRITE	2	/* Controller is busy, don't send data */
#define DAISY_INT	4	/* Controller is interrupting */
#define DAISY_UNK0	0x10	/* Always 1 */
#define DAISY_IDLE	0x20	/* Daisy controller is idle */
#define DAISY_UNK1	0x40
#define DAISY_CANCMD	0x80	/* Daisy can be commanded */


//
// Returned values from IN 0x1FC
//
#define DAISY_CTLFAIL	2	// Controller failed
#define DAISY_BAILIN	4	// Bail bar in
#define DAISY_COVERDN	8	// Cover down
#define DAISY_PAPER	16	// Paper present
#define DAISY_RIBBON	32	// Ribbon present
#define DAISY_NOPTR	64	// No printer
#define DAISY_PTRFAIL	128	// Printer failed.

#define XX 0x00
// The mapping of petal numbers on the printwheel back to ISO-8859-1
// This is for the default "Prestige Pica 10" wheel
// It can be configured by editing joycehw.xml (no UI is provided, as
// it is very unlikely that one will be needed).
static unsigned char gl_wheel[128] = 
{
	'#', '.', '<', '[', '=', '9', '8', '7',	// 00-07
	'6', '5', '0', '4', '3', '2', '1', ',',	// 08-0F
	'-', ']', 'k', 'y', 'm', 'f', 'd', 'g',	// 10-17
	'n', 'c', 't', 'r', 'l', 'u', 'e', 'o',	// 18-1F
	'i', 'a', 's', 'h', 'q', 'p', 'w', 'b',	// 20-27		
	'v', ';', 'x', '¾', 'j', 'z', '½', '>',	// 28-2F
	' ', '.', '²', '!', '+', '(','\'', '&',	// 30-37  -- 0x30 is "2/3"
	'_', '£', ')', '$', '/', '"', '*', ',', // 38-3F  not present in latin-1
	'?', '@', 'K', 'Y', 'M', 'F', 'D', 'G',	// 40-47
	'N', 'C', 'T', 'R', 'L', 'U', 'E', 'O',	// 48-4F
	'I', 'A', 'S', 'H', 'Q', 'P', 'W', 'B',	// 50-57
	'V', ':', 'X', '¼', 'J', 'Z', '%', '³',	// 58-5F
	' ', '|', '÷', '°', '~',  XX,  XX, XX,	// 60-67 -- 0x60 is "1/3" 
	 XX,  XX,  XX,  XX,  XX,  XX,  XX, XX,	// 68-6F  not present in latin-1
	 XX,  XX,  XX,  XX,  XX,  XX,  XX, XX,	// 70-77
	 XX,  XX,  XX,  XX,  XX,  XX,  XX, XX,	// 78-7F
};
#undef XX

JoyceDaisy::JoyceDaisy(JoyceSystem *sys) : PcwPrinter("daisy"), m_sys(sys)
{
	XLOG("JoyceDaisy::JoyceDaisy()");
	reset();
	m_curcmd = 0;
	m_resultw = m_resultr = 0;
	m_daisyEnabled = true;
	m_intTime = 0;
	m_format = PS;

        m_fp = NULL;
        m_w =  999;     // A4 width
        m_h = 2245;     // A4 height
        m_y = 192;      // Start 1 inch down
	m_x = 0;
        m_psdir = findPcwDir(FT_PS, false);
	m_drawing_page = false;
	m_sequence = 0;
	// Initialise printwheel data
	memcpy(m_wheel, gl_wheel, sizeof(m_wheel));
        m_fontname = "Courier";   // Font name
        m_ptsize = 10;            // Font size (points)
	m_bailIn = true;
	m_coverOpen = false;
	m_ribbon = true;
}



JoyceDaisy::~JoyceDaisy()
{
	XLOG("JoyceDaisy::~JoyceDaisy()");
        endPage();
        if (m_fp) closePS();
}

void JoyceDaisy::reset()
{
	m_intEnabled   = false;
	m_interrupting = false;
	m_intWaiting   = false;
}


byte JoyceDaisy::parInp()
{
        unsigned busyline;
        ll_error_t e;
	byte r = 0;

        if (!isEnabled()) return 0x40;

        if (!m_lldev) open();
        if (!m_lldev) return 0x40;

        e = ll_rctl_poll(m_lldev, &busyline);
        showError(e);
        if (e != LL_E_OK) return 0x40;
        if (busyline & LL_CTL_ACK)     r |= 0x80;
        if (busyline & LL_CTL_BUSY)    r |= 0x40;
        if (busyline & LL_CTL_ERROR)   r |= 0x20;
        if (busyline & LL_CTL_ISELECT) r |= 0x10;
        if (busyline & LL_CTL_NOPAPER) r |= 0x08;
        return r;
}
	



void JoyceDaisy::out(word port, byte value)
{
	switch (port & 0x1FF)
	{
		case 0x1FC:
			m_curcmd = 1;
			m_cmd[0] = value;
			break;
		case 0x0FC:
			m_cmd[m_curcmd++] = value;
			break;
	}
	if (m_curcmd == 2)
	{
		//fprintf(stderr, "Daisy command: %02x%02x\n", m_cmd[0], m_cmd[1]);
		// Move the head or the paper.
		if (m_cmd[0] >= 0x80)
		{
			int amount = ((m_cmd[0] & 0x0F) << 8) | m_cmd[1];
			switch(m_cmd[0] & 0xF0)
			{
//
// The purpose of the ones that just break; is unknown.
//
				case 0x80: break;
				case 0x90: m_x -= amount; break;
				case 0xA0: break;
				case 0xB0: m_x += amount; break;
				case 0xC0: m_y += amount; break;
				case 0xD0: break;
				case 0xE0: m_y -= amount; break;
				case 0xF0: break;
			}
			checkEndForm();
			m_intTime = 500;
		}
		// Turn wheel & print letter
		else if ((m_cmd[0] & 0xC0) == 0x40)
		{
			// Letter is m_cmd[1]
			// Impression in low bits of m_cmd[0]; if it's 0
			// then the character is not printed (used when the
			// wheel is being aligned)
			if (m_cmd[0] & 0x3F)
			{
				char c = m_wheel[m_cmd[1] & 0x7F];
			        if (m_drawing_page || startPage())
					putChar(c, m_x, m_y);	
			}
			m_intTime = 1000;
		}
		else switch(m_cmd[0])
		{
			case 0x01:	// while printing, set ribbon / paper
					// parameters. We ignore these for now.
					break;
			// PAR status
			case 0x02:	addResult(parInp());
					break;
			case 0x04:	writeChar(m_cmd[1]);	
					break;
			case 0x05:	// dummy
					break;
			case 0x06:	// dummy
					addResult(0xD8);
					break;
			case 0x07:	// EI
					m_intEnabled = true;
					if (m_intWaiting) interrupt();
					m_intWaiting = false;
					break;
			case 0x08:	// DI
					m_intEnabled = false;
					break;
// Acknowledge interrupt
			case 0x09:      m_sys->m_asic->daisyIsr(false);
					m_interrupting = false;
					break;	
			case 0x0A:	// dummy
				        addResult(0x00); 
					break;
			case 0x0B:	// dummy
				        addResult(0xE0); 
					break;
			case 0x12:	addResult(0);
					reset();
					break;
			default: joyce_dprintf("Unknown DAISY command: %02x %02x\n", m_cmd[0], m_cmd[1]);	
				break;
		}
	}
}


byte JoyceDaisy::in(word port)
{
	byte r;

	port &= 0x1FF;

	if (port == 0x1FC)
	{
		if (!m_daisyEnabled) return DAISY_NOPTR;
		r = DAISY_BAILIN | DAISY_COVERDN | DAISY_PAPER | 
			    DAISY_RIBBON;
		if (!m_bailIn  ) r &= ~DAISY_BAILIN;
		if (m_coverOpen) r &= ~DAISY_COVERDN;
		if (!m_ribbon  ) r &= ~DAISY_RIBBON;	
		return r;
	}
	if (port == 0xFC)
	{
		if (m_resultw == m_resultr) 
		{
			m_resultw = m_resultr = 0;
			return m_resultLatch;
		}
		return m_resultLatch = m_result[m_resultr++];
	}
	r = DAISY_UNK0 | DAISY_CANCMD | DAISY_IDLE;
			// Always seems to be 1
	if (m_intWaiting)
	{
		r &= ~DAISY_CANCMD;	// Cannot take any more command bytes
		r &= ~DAISY_IDLE;	// And working as hard as anything
	}
	if (m_interrupting) r |= DAISY_INT;
	if (m_resultw != m_resultr) r |= DAISY_READ;
	return r;
}

void JoyceDaisy::addResult(byte b)
{
	if (m_resultw == m_resultr)
	{
		m_resultw = m_resultr = 0;
	}
	m_result[m_resultw++] = b;
}


bool JoyceDaisy::hasSettings(SDLKey *key, string &caption)
{
	*key = SDLK_p;
	caption = "  PAR and daisywheel  ";
	return true;
}


UiEvent JoyceDaisy::settings(UiDrawer *d)
{
        UiEvent e;

        while (1)
        {
                UiMenu uim(d);
                bool bEnabled;
                int sel;

                bEnabled = isEnabled();
                uim.add(new UiLabel("  PCW9512 printers  ", d));
                uim.add(new UiSeparator(d));
		uim.add(new UiCommand(SDLK_d, "  Daisywheel  ", d, UIG_SUBMENU));
		uim.add(new UiCommand(SDLK_p, "  PAR port  ", d, UIG_SUBMENU));
                uim.add(new UiSeparator(d));
                uim.add(new UiCommand(SDLK_ESCAPE, "  EXIT  ", d));
                d->centreContainer(uim);
                e = uim.eventLoop();
                if (e == UIE_QUIT) return e;
                sel = uim.getSelected();
                switch(uim.getKey(sel))
		{
                        case SDLK_ESCAPE: return UIE_CONTINUE;
                        case SDLK_d: e = daisySettings(d);
                                     if (e == UIE_QUIT) return e;
                                     break;
                        //case SDLK_p: e = PcwPrinter::settings(d);
                        case SDLK_p: e = PcwPrinter::settings(d);
                                     if (e == UIE_QUIT) return e;
                                     break;
                        default:     break;
                }
        }
        return UIE_CONTINUE;
}



string JoyceDaisy::getTitle(void)
{
	return "  PAR port  ";
}


bool JoyceDaisy::storePrintWheel(xmlDocPtr doc, xmlNsPtr ns, xmlNodePtr cur)
{
        bool found = false;	
	int n;
        xmlNodePtr output, newOutput, pin;
	char buf[30];

        newOutput    = xmlNewNode(ns, (xmlChar *)"printwheel");
 	for (n = 0; n < sizeof(m_wheel); n++) 
	{
		if (m_wheel[n]) 
		{
			pin = xmlNewNode(ns, (xmlChar *)"pin");
			sprintf(buf, "%d", n);
			xmlSetProp(pin, (xmlChar *)"number", (xmlChar *)buf);
			sprintf(buf, "%d", m_wheel[n]);
			xmlSetProp(pin, (xmlChar *)"character", (xmlChar *)buf);
			xmlAddChild(newOutput, pin);
			xmlAddChild(newOutput, xmlNewText((xmlChar *)"\n"));

		}	
	} 

        for (output = cur->xmlChildrenNode; output; output = output->next)
        {
                if (output->ns != ns) continue;
                if (strcmp("printwheel", (char *)output->name)) continue;

                found = true;
                xmlReplaceNode(output, newOutput);
        }
        if (!found) xmlAddChild(cur, newOutput);
        return true;
}

bool JoyceDaisy::parsePrintWheel(xmlDocPtr doc, xmlNsPtr ns, xmlNodePtr cur)
{
	int n;
        xmlNodePtr output, pin;
        
	for (output = cur->xmlChildrenNode; output; output = output->next)
        {
                if (output->ns != ns) continue;
                if (strcmp("printwheel", (char *)output->name)) continue;

		memset(&m_wheel, 0, sizeof(m_wheel));
		for(pin = output->xmlChildrenNode; pin; pin = pin->next)
		{
			char *i, *c;
			int idx, ch;
			if (pin->ns != ns) continue;
                	if (strcmp("pin", (char *)pin->name)) continue;
	
			i = (char *)xmlGetProp(pin, (xmlChar *)"number");	
			c = (char *)xmlGetProp(pin, (xmlChar *)"character");
	
			if (i && c) 
			{ 
				idx = atoi(i); 
				ch  = atoi(c);
				if (idx >= 0 && idx < sizeof(m_wheel)) m_wheel[idx] = ch;
			}
			if (i) xmlFree(i);
			if (c) xmlFree(c);
		}
		return true;
        }
        return true;
}

bool JoyceDaisy::parseNode(xmlDocPtr doc, xmlNsPtr ns, xmlNodePtr cur)
{
	char *t;
	if (!PcwPrinter::parseNode(doc, ns, cur)) return false;
     
	if (!parsePrintWheel(doc, ns, cur)) return false;
  
	parseFilename(doc, ns, cur, "psoutputdir", m_psdir);

        t = (char *)xmlGetProp(cur, (xmlChar *)"width");
        if (t) { m_w = atoi(t); xmlFree(t); }
        t = (char *)xmlGetProp(cur, (xmlChar *)"height");
        if (t) { m_h = atoi(t); xmlFree(t); }
        t = (char *)xmlGetProp(cur, (xmlChar *)"fontsize");
        if (t) { m_ptsize = atoi(t); xmlFree(t); }
        t = (char *)xmlGetProp(cur, (xmlChar *)"fontname");
        if (t) { m_fontname = t; xmlFree(t); }
      
        t = (char *)xmlGetProp(cur, (xmlChar *)"format");
        if (t)
        {
                if (!strcmp(t, "PS"))  m_format = PS;
#ifdef HAVE_WINDOWS_H
                if (!strcmp(t, "GDI")) m_format = GDI;
#endif
                xmlFree(t);
        }
	
 
	char *s = (char *)xmlGetProp(cur, (xmlChar *)"daisyenabled");

        if (!s) return true;    // Default setting used

        if (!strcmp(s, "1")) { m_daisyEnabled = true;  xmlFree(s); return true; }
        if (!strcmp(s, "0")) { m_daisyEnabled = false; xmlFree(s); return true; }
        joyce_dprintf("'enabled=%s' option not recognised.\n", s);
	xmlFree(s);
        return false;
}


bool JoyceDaisy::storeNode(xmlDocPtr doc, xmlNsPtr ns, xmlNodePtr cur)
{
	char buf[30];

        xmlSetProp(cur, (xmlChar *)"daisyenabled", (xmlChar *)(m_daisyEnabled ? "1" : "0"));
        storeFilename(doc, ns, cur, "psoutputdir", m_psdir);
        sprintf(buf, "%d", (int)(m_w));
        xmlSetProp(cur, (xmlChar *)"width", (xmlChar *)buf);
        sprintf(buf, "%d", (int)(m_h));
        xmlSetProp(cur, (xmlChar *)"height", (xmlChar *)buf);
	xmlSetProp(cur, (xmlChar *)"fontname", (xmlChar *)m_fontname.c_str());
        sprintf(buf, "%d", m_ptsize);
        xmlSetProp(cur, (xmlChar *)"fontsize", (xmlChar *)buf);
        switch(m_format)
        {
                case PS:   xmlSetProp(cur, (xmlChar *)"format", (xmlChar *)"PS");
#ifdef HAVE_WINDOWS_H
                case GDI:  xmlSetProp(cur, (xmlChar *)"format", (xmlChar *)"GDI");
#endif
        }

	storePrintWheel(doc, ns, cur);	
        return PcwPrinter::storeNode(doc, ns, cur);
}



UiEvent JoyceDaisy::daisySettings(UiDrawer *d)
{
	int x,y,sel;
	bool a3, a4, a4l, a5, a5l, cont11, cont12, custom;
	bool bEnabled;
	UiEvent uie;
	char cust[80];
	PrinterFormat format = m_format;

	// Should we add settings for PostScript font & point size, or
	// leave them (along with printwheel) for advanced users who 
	// can edit the config file?
	
	x = y = 0;
	sel = -1;
	while (1)
	{
		UiMenu uim(d);
		string prevfn;
		string prevdr;

		a3  = (m_w == 1403  && m_h == 3196);
		a4l = (m_w == 1403  && m_h == 1598);
		a4  = (m_w == 999   && m_h == 2245);
		a5l = (m_w == 999   && m_h == 1122);
		a5  = (m_h == 1598  && m_w == 701);
		cont11 = (m_w == 960 && m_h == 2112);
		cont12 = (m_w == 960 && m_h == 2304);
		custom = false;	
		if (!(a4 || a5l || a5 || cont11 || cont12)) custom = true;
		if (custom) sprintf(cust, "  Custom size (%f\" x %f\")  ",
				(m_w / 120.0), (m_h / 192.0) );
		else sprintf(cust, "  Custom size ... ");

		bEnabled = m_daisyEnabled;
		uim.add(new UiLabel("  Daisywheel  ", d));
		uim.add(new UiSeparator(d));
		uim.add(new UiSetting(SDLK_d, !bEnabled, "  Disconnected  ", d));
		uim.add(new UiSetting(SDLK_c,  bEnabled, "  Connected  ",    d));

		if (bEnabled)
		{
                	uim.add(new UiSeparator(d));
                	uim.add(new UiSetting(SDLK_3,  a3,     "  A3 portrait ",    d));
                	uim.add(new UiSetting(SDLK_4,  a4,     "  A4 portrait ",    d));
                	uim.add(new UiSetting(SDLK_5,  a5,     "  A5 portrait ",    d));
                	uim.add(new UiSetting(SDLK_a,  a4l,    "  A4 landscape ",   d));
                	uim.add(new UiSetting(SDLK_l,  a5l,    "  A5 landscape ",   d));
                	uim.add(new UiSetting(SDLK_1,  cont11, "  11\" continuous ",   d));
                	uim.add(new UiSetting(SDLK_2,  cont12, "  12\" continuous ",   d));
                	uim.add(new UiSetting(SDLK_0,  custom, cust,                   d));

			string ostat = "  Output is to ";
			if (m_psdir[0] == '|') ostat += "pipe: " + displayName(m_psdir.substr(1), 40);
			else		       ostat += "directory: " + displayName(m_psdir, 40);
#ifdef HAVE_WINDOWS_H
			uim.add(new UiSeparator(d));
			uim.add(new UiSetting(SDLK_p, format == PS,  "  Output in PostScript format  ", d));
			uim.add(new UiSetting(SDLK_g, format == GDI, "  Output using Windows' GDI  ", d));
#endif
			if (m_format == PS) 
			{
				uim.add(new UiSeparator(d));
				uim.add(new UiLabel  (ostat, d)); 
				uim.add(new UiCommand(SDLK_r,  "  Output to diRectory  ", d, UIG_SUBMENU));
				uim.add(new UiCommand(SDLK_u,  "  Output to UNIX command... ",d, UIG_SUBMENU));
			}
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
                        case SDLK_3:    m_w = 1403; m_h = 3196; break;
                        case SDLK_a:    m_w = 1403; m_h = 1598; break;
                        case SDLK_4:    m_w =  999; m_h = 2245; break;
                        case SDLK_l:    m_w =  999; m_h = 1122; break;
                        case SDLK_5:    m_w =  701; m_h = 1598; break;
                        case SDLK_1:    m_w =  960; m_h = 2112; break;
                        case SDLK_2:    m_w =  960; m_h = 2304; break;
                        case SDLK_0:    uie = getCustomPaper(d);
                                        if (uie == UIE_QUIT) return uie;
                                        break;
			case SDLK_p: format = PS; break;
#ifdef HAVE_WINDOWS_H
			case SDLK_g: format = GDI; break;
#endif
			case SDLK_d: m_daisyEnabled = false; break;
			case SDLK_c: m_daisyEnabled = true;  break;
                        case SDLK_r:    m_y = DAISY_MAX_H + 1;
                                        checkEndForm();
                                        m_psdir = findPcwDir(FT_PS, false);
                                        break;
                        case SDLK_u:    prevdr = m_lldriver;
					prevfn = m_llfilename;
					m_y = DAISY_MAX_H + 1;
                                        checkEndForm();
                                        uie = getPipe(d);
                                        if (uie == UIE_QUIT) return uie;
                                        m_psdir = "|";
					m_psdir += m_llfilename;
					m_llfilename = prevfn;
					m_lldriver = prevdr;
                                        break;

			case SDLK_ESCAPE: close();  return UIE_OK;
			default: break;
		}
		m_format = format;
	}
	return UIE_OK;
}


void JoyceDaisy::interrupt(void)
{
	m_sys->m_asic->daisyIsr(true);
	m_interrupting = true;
}



void JoyceDaisy::tick()
{
	if (!m_intTime) return;
	--m_intTime;
	if (!m_intTime)
	{
		//fprintf(stderr, "Daisy interrupt [%d]!\n", m_intEnabled);
		if (m_intEnabled) interrupt();
		else m_intWaiting = true;
	}
}


bool JoyceDaisy::startPage(void)
{
        switch(m_format)
        {
                case PS:  return startPagePS();
#ifdef HAVE_WINDOWS_H
                case GDI:
                          if (m_drawing_page) return true;
// [JOYCE 2.1.1] Correct the scaling 
			  m_gdiPrint.m_fontSize = m_ptsize * 24;
			  if (m_fontname == "Courier") 
				m_gdiPrint.m_fontFace = "Courier New";
			  else  m_gdiPrint.m_fontFace = m_fontname;

                          m_drawing_page = m_gdiPrint.startPage();
                          return m_drawing_page;
#endif
        }
        return false;

}

////////////////////////////////////////////////////////////////////////////
//
// Output page as a Postscript file
//
bool JoyceDaisy::startPagePS(void)
{	
	if (m_drawing_page) return true;
	m_drawing_page = true;

	/* Try to open a printer page. For each filename we try, first
	 * attempt to open it for reading. If that succeeds, then 
	 * go to the next file sequence. If it fails, then try to open
	 * it for writing. 
	 *
	 * Note that this is a while() loop rather than a do() loop, 
	 * because PS (unlike PNG) can use the same file for multiple pages.
	 * */
	
	while (!m_fp)	
	{
		if (m_psdir[0] == '|')
		{
			m_fp = popen(m_psdir.c_str() + 1, "w");
		}
		else
		{	
			if (m_psdir == "") m_psdir = ".";	
			if (m_psdir[m_psdir.length() - 1] == '/')
				m_psdir = m_psdir.substr(0, m_psdir.length() -1);
			checkExistsDir(m_psdir);
			sprintf(m_outFilename, "%s/daisy%d.ps", m_psdir.c_str(), m_sequence);
			m_fp = fopen(m_outFilename, "rb");
			if (m_fp)
			{
				fclose(m_fp);
				m_fp = NULL;
				++m_sequence;
			}
			else
			{
				m_fp = fopen(m_outFilename, "w");
				if (!m_fp) ++m_sequence;
			}
		}
	}
	fprintf(m_fp,
		"%%! \n"
		"%% PCW daisy page \n"
		"%% Auto-generated by JOYCE v%d.%d.%d"
		"%% All coordinates in points\n\n", 
                BCDVERS >> 8,
               (BCDVERS >> 4) & 0x0F,
                BCDVERS & 0x0F);
	// The closest I can get to Prestige Pica 10 is Courier 10
	fprintf(m_fp, "/%s findfont \n"
                      "%d scalefont \n"
		      "setfont \n\n", m_fontname.c_str(), m_ptsize);

	return true;
}


void JoyceDaisy::closePS(void)
{
	if (!m_fp) return;
	if (m_psdir[0] == '|') pclose(m_fp);	
	else fclose(m_fp);
	m_fp = NULL;
}


bool JoyceDaisy::endPagePS(void)
{
	if (!m_drawing_page) return true;
	m_drawing_page = false;

	fprintf(m_fp, "\n\nshowpage\n");

	//closePS();	// No point. All printer output can go in one
			// big PS file.
	return true;
}

bool JoyceDaisy::endPage(void)
{
        switch(m_format)
        {
                case PS:  return endPagePS();
#ifdef HAVE_WINDOWS_H
                case GDI: m_drawing_page = false;
                          return m_gdiPrint.endPage();
#endif
        }
        return false;
}


void JoyceDaisy::putChar(char c, float xf, float yf)
{
        switch(m_format)
        {
                case PS:  charPS(c, xf, yf);
#ifdef HAVE_WINDOWS_H
// [JOYCE v2.1.1: Convert from 120/192dpi to 360dpi]
                case GDI: m_gdiPrint.putChar(xf * 3.0, yf * 1.875, c);
#endif
        }
}




void JoyceDaisy::charPS(char c, float xf, float yf)
{
	if (!m_fp) return;
	if (!c || c == ' ') return;
//
// Convert from the 120dpi / 192dpi coords used by JOYCE internally
// to the 72dpi coordinates used by Postscript. The "+96" moves the text up
// by half an inch; I don't know if this is necessary.
//
	double xp = (xf * 0.6);
	double yp = (m_h + 96 - yf) * 0.375;

	//fprintf(stderr, "(%c) ", c);

	fprintf(m_fp, " newpath \n");
	fprintf(m_fp, " %f %f moveto \n", xp, yp);
	if (c == ')' || c == '(' ) fprintf(m_fp, " (\\%c) show\n", c);
	else          fprintf(m_fp, " (%c) show\n", c);
}

void JoyceDaisy::checkEndForm(void)
{
        if (m_y >= DAISY_MAX_H && m_drawing_page)
        {
                endPage();
                m_y = 360;      /* Feed a new sheet; and we can't print
                                 * on the top 6 lines */
        }
}





UiEvent JoyceDaisy::control(UiDrawer *d)
{
	int x,y,sel;
        UiEvent uie;
	char sLine[50];

        x = y = 0;
        sel = -1;

	if (!isEnabled())
	{
                UiMenu uim(d);
                uim.add(new UiLabel("  No printer  ", d));
                uim.add(new UiSeparator(d));
                uim.add(new UiCommand(SDLK_ESCAPE,  "  EXIT  ", d));
                d->centreContainer(uim);
                uie =  uim.eventLoop();
		if (uie != UIE_QUIT) return UIE_CONTINUE;
		return uie;	
	}
        while (1)
        {
                UiMenu uim(d);

		if (m_y <= 208) sprintf(sLine, "  Top of form  ");
		else sprintf(sLine, "  At line: %03d  ", (int)(m_y / 32));

                uim.add(new UiLabel("  Daisywheel printer  ", d));
		uim.add(new UiLabel(sLine, d));
                uim.add(new UiSeparator(d));
                uim.add(new UiSetting(SDLK_o, !m_bailIn,    "  Bail bar Out  ", d));
                uim.add(new UiSetting(SDLK_i,  m_bailIn,    "  Bail bar In  ",  d));
                uim.add(new UiSetting(SDLK_u,  m_coverOpen, "  Cover Up  ", d));
                uim.add(new UiSetting(SDLK_d, !m_coverOpen, "  Cover Down  ",  d));
                uim.add(new UiSetting(SDLK_p,  m_ribbon,    "  Ribbon present  ", d));
                uim.add(new UiSetting(SDLK_a, !m_ribbon,    "  Ribbon absent  ",  d));
                uim.add(new UiSeparator(d));
		uim.add(new UiCommand(SDLK_f,  "  Form feed  ", d));
                uim.add(new UiCommand(SDLK_ESCAPE,  "  EXIT  ", d));
                uim.setSelected(sel);
                d->centreContainer(uim);
                uie =  uim.eventLoop();

                if (uie == UIE_QUIT) return uie;
                sel = uim.getSelected();
                switch(uim.getKey(sel))
                {
                        case SDLK_f:
				m_y = DAISY_MAX_H + 1;
				checkEndForm();
				break;
			case SDLK_u: 
				m_coverOpen = true;
				break;
			case SDLK_d:
				m_coverOpen = false;
				break;
			case SDLK_p: 
				m_ribbon = true;
				break;
			case SDLK_a:
				m_ribbon = false;
				break;
			case SDLK_i: 
				m_bailIn = true;
				break;
			case SDLK_o:
				m_bailIn = false;
				break;
                        case SDLK_ESCAPE: 	
				return UIE_OK;
                        default: break;
                }

        }
	return UIE_CONTINUE;
}


UiEvent JoyceDaisy::getCustomPaper(UiDrawer *d)
{
        int x,y,sel;
        UiEvent uie;
        char sw[50], sh[50];
	float f;

        x = y = 0;
        sel = -1;
	while (1)
	{
		sprintf(sw, "%f", m_w / 120.0);
		sprintf(sh, "%f", m_h / 192.0);
		UiMenu uim(d);
                uim.add(new UiLabel("  Custom paper size  ", d));
                uim.add(new UiSeparator(d));
                uim.add(new UiTextInput(SDLK_w, "  Width in inches: __________  ",d));
                uim.add(new UiTextInput(SDLK_h, "  Height in inches: __________  ",d));
                uim.add(new UiSeparator(d));
                uim.add(new UiCommand(SDLK_ESCAPE,  "  EXIT  ", d));
                d->centreContainer(uim);
                uim.setSelected(sel);
                ((UiTextInput &)uim[2]).setText(sw);
                ((UiTextInput &)uim[3]).setText(sh);

                uie =  uim.eventLoop();
                if (uie == UIE_QUIT) return uie;
                sel = uim.getSelected();
                switch(uim.getKey(sel))
                {
			case SDLK_w:	f = atof(((UiTextInput &)uim[2]).getText().c_str());
					if (f) m_w = (int)(120.0 * f);
					break;
			case SDLK_h:	f = atof(((UiTextInput &)uim[3]).getText().c_str());
					if (f) m_h = (int)(192.0 * f);
					break;
			case SDLK_ESCAPE:
					return UIE_OK;
			default:	break;
		}
	}
	return UIE_CONTINUE;	
}


