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


JoyceAsic::JoyceAsic(JoyceSystem *s) : PcwAsic(s)
{
	XLOG("JoyceAsic::JoyceAsic()");
	m_sys = s;
	reset();
}

JoyceAsic::~JoyceAsic()
{

}

byte JoyceAsic::inF8(void)
{
	return m_inf8;
}

byte JoyceAsic::inF4(void)
{
	byte v = m_inf8;
	m_inf8 &= 0xF0;	
	return v;	
}


void JoyceAsic::outF8(byte b)
{
	switch(b)
	{
		case 1:	m_sys->setResetFlag(); break;	// Reset
		case 2: case 3: case 4: m_fdcIntMode = b; break; // FDC mode
		case 5: 
		case 6: m_sys->m_fdc->setTerminalCount((b == 5) ? 1 : 0); break;
		case 7: 
		case 8: ((JoycePcwTerm*)m_sys->m_defaultTerminal)->enable(b == 7); break;
		case 9: 
		case 10: m_sys->m_fdc->setMotor( (b == 9) ? 0x0F : 0); break;
		case 11: m_sys->m_activeTerminal->beepOn(); break;
		case 12: m_sys->m_activeTerminal->beepOff(); break;
			break;
	}
}
	
void JoyceAsic::reset(void)
{
	m_fdcIntMode   = 4;
	m_timerCount   = 0;
	m_retraceCount = 0;
	m_timerInt     = false;
        m_inf8         = 0x30;     // 0x20 for the 200-line screen
	m_daisyInt     = false;
}

//
// The FDC is interrupting. Pass this to the CPU (maybe)
//
void JoyceAsic::fdcIsr(int status)
{
	if (status) m_inf8 |= 0x20;
        else        m_inf8 &= (~0x20);
}


void JoyceAsic::daisyIsr(int status)
{
	m_daisyInt = (status != 0);
}


void JoyceAsic::onAsicTimer(void)
{
	++m_timerCount;
	++m_retraceCount;

        if (m_retraceCount == 18) m_retraceCount = 0;
        if (m_retraceCount < 3)                // In the vertical retrace 
                m_inf8 |=   0x40; 
        else    m_inf8 &= (~0x40);

        if (m_timerCount >= 3) /* 300Hz signal */
        {
                m_timerCount = 0;
                if ((m_inf8 & 0x0F) < 0x0F) m_inf8++; /* Increase the 300Hz counter */
//                if (m_timerInt)
//                {
                        m_timerInt = 0;
//
// The timer interrupt is a one-shot thing; if interrupts are disabled, it 
// is lost. To emulate this, only raise the interrupt line if we know the
// interrupt will be serviced. Compare the FDC interrupt, in which the FDC
// keeps holding the interrupt line high until told to stop.
//
			if (m_sys->m_cpu->iff1) m_sys->m_cpu->Int(1);
//		}
	}
	else	// Here we check if the daisywheel printer, FDC or 
	{ 	// CPS8256 interface have interrupted. These 
		// peripherals interrupt by holding the interrupt line
		// active until acknowledged; which we simulate by 
		// interrupting every 900Hz or so until the device stops
		// bothering us. I'm only going to let one device at a
		// time interrupt; hence the elses.
        	if (m_inf8 & 0x20) switch (m_fdcIntMode)
        	{
               		case 2: m_sys->m_cpu->Nmi(true); break;
      		        case 3: m_sys->m_cpu->Int(true); break;
        	}
		else if (m_daisyInt) m_sys->m_cpu->Int(true); 
	}
}



bool JoyceAsic::parseNode(xmlDoc *doc, xmlNs *ns, xmlNode *cur)
{
        int r = 0;

        parseInteger(doc, ns, cur, "cpuspeed", &r);
        if (r) m_sys->m_cpu->setSpeed(r);

       parseInteger(doc, ns, cur, "jdos", &r);
	if (r == JM_PROMPT || r == JM_ALWAYS || r == JM_NEVER)
	{
		m_sys->setJDOSMode((JDOSMode)r);
	}

	xmlChar *s = xmlGetProp(cur, (xmlChar *)"model");
	if (s)
	{
		if (!strcmp((char *)s, "8000" )) m_sys->setModel(PCW8000);
		if (!strcmp((char *)s, "8256" )) m_sys->setModel(PCW8000);
		if (!strcmp((char *)s, "8512" )) m_sys->setModel(PCW8000);
		if (!strcmp((char *)s, "9256" )) m_sys->setModel(PCW10);
		if (!strcmp((char *)s, "10"   )) m_sys->setModel(PCW10);
		if (!strcmp((char *)s, "9512" )) m_sys->setModel(PCW9000);
		if (!strcmp((char *)s, "9512+")) m_sys->setModel(PCW9000P);
		xmlFree(s);
	}
	return true;
}



bool JoyceAsic::storeNode(xmlDoc *doc, xmlNs *ns, xmlNode *cur)
{
	int r = (int)(m_sys->m_cpu->getSpeed(true));
	storeInteger(doc, ns, cur, "cpuspeed", r);
	switch(m_sys->getModel())
	{
		case PCW8000:  xmlSetProp(cur, (xmlChar *)"model", (xmlChar *)"8000"); break;
		case PCW9000:  xmlSetProp(cur, (xmlChar *)"model", (xmlChar *)"9512"); break;
		case PCW9000P: xmlSetProp(cur, (xmlChar *)"model", (xmlChar *)"9512+"); break;
		case PCW10:    xmlSetProp(cur, (xmlChar *)"model", (xmlChar *)"9256"); break;
	}
	r = m_sys->getJDOSMode();
        storeInteger(doc, ns, cur, "jdos", r);

	return true;
}


bool JoyceAsic::hasSettings(SDLKey *key, string &caption)
{
	*key = SDLK_g;
	caption = "  General  ";
	return true;
}

UiEvent JoyceAsic::settings(UiDrawer *d)
{
	int sel = -1;
	UiEvent uie;
	char speed[50];
	PcwModel m;
	int r, r2;

	r = (int)(m_sys->m_cpu->getSpeed(true));
	m = m_sys->getModel();
	
	while (1)
	{
		UiMenu uim(d);
		sprintf(speed, "%d", r);	
	
		uim.add(new UiLabel("  General options  ", d));
		uim.add(new UiSeparator(d));
		uim.add(new UiLabel("  PCW model  ", d));
		uim.add(new UiSetting(SDLK_8, m == PCW8000,       "    8256 / 8512  ",  d));	
		uim.add(new UiSetting(SDLK_9, m == PCW9000,       "    9512   ",  d));	
		uim.add(new UiSetting(SDLK_0, m == PCW10,         "    9256 / 10  ",  d));
		uim.add(new UiSetting(SDLK_PLUS, m == PCW9000P,   "    9512+  ",  d));	
                uim.add(new UiTextInput(SDLK_s, "  Speed (%): ______  ",d));
                uim.add(new UiSeparator(d));
#ifdef WIN32
                uim.add(new UiTextInput(SDLK_j, "  JOYCE data directory: ________________________________  ",d));
                uim.add(new UiSeparator(d));
#endif
		uim.add(new UiCommand(SDLK_f, "  File access...", d));
                uim.add(new UiSeparator(d));

                uim.add(new UiCommand(SDLK_ESCAPE, "  EXIT  ", d));
                d->centreContainer(uim);
                uim.setSelected(sel);

                ((UiTextInput &)uim[7]).setText(speed);
#ifdef WIN32
                ((UiTextInput &)uim[9]).setText(getHomeDir());
#endif
                uie =  uim.eventLoop();

                if (uie == UIE_QUIT) return uie;
                sel = uim.getSelected();
                switch(uim.getKey(sel))
		{
			case SDLK_8: m = PCW8000; break;
			case SDLK_9: m = PCW9000; break;
			case SDLK_PLUS: m = PCW9000P; break;
			case SDLK_0: m = PCW10; break;
			case SDLK_s: 
				r2 = atoi(((UiTextInput &)uim[7]).getText().c_str());
				if (r2) r = r2;
				break;
			case SDLK_j:
				setHomeDir(((UiTextInput &)uim[9]).getText());
				break;
			case SDLK_f:
				uie = setFileAccess(d);
				if (uie == UIE_QUIT) return uie;
				break;
			case SDLK_ESCAPE:
				m_sys->setModel(m);
        			if (r) m_sys->m_cpu->setSpeed(r);
				return UIE_OK;
			default: break;
		}
	}
}

