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


AnneAsic::AnneAsic(AnneSystem *s) : PcwAsic(s)
{
	XLOG("AnneAsic::AnneAsic()");
	m_sys = s;
	reset();
}

AnneAsic::~AnneAsic()
{

}



void AnneAsic::outF8(byte b)
{
	switch(b & 0x0F)
	{
		case 1:	m_sys->setResetFlag(); break;	// Reset
		case 2: case 3: case 4: m_fdcIntMode = b; break; // FDC mode
		case 5: 
		case 6: m_sys->m_fdc->setTerminalCount((b == 5) ? 1 : 0); break;
		case 7: 
		case 8: ((AnneTerminal*)m_sys->m_defaultTerminal)->enable(b == 7); break;
		case 11: m_sys->m_activeTerminal->beepOn(); break;
		case 12: m_sys->m_activeTerminal->beepOff(); break;
			break;

		case 15:
			((AnneTerminal*)m_sys->m_defaultTerminal)->powerLED(b & 0x20);
			break;
	}
}
	
void AnneAsic::reset(void)
{
	m_fdcIntMode   = 4;
	m_timerCount   = 0;
	m_retraceCount = 0;
	m_inf8         = 0;
	m_inf7	       = 0;
}

//
// The FDC is interrupting. Pass this to the CPU (maybe)
//
// Other IRQs set different bits in this byte.
//
void AnneAsic::fdcIsr(int status)
{
	if (status) m_inf8 |= 0x40;
        else        m_inf8 &= (~0x40);
//	fprintf(stderr, "fdcIsr: status=%d -> m_inf8 = 0x%02x\n", status, m_inf8);
}

void AnneAsic::keybIsr(int status)
{
	if (status) m_inf8 |= 0x02;
        else        m_inf8 &= (~0x02);

//	fprintf(stderr, "keybIsr: status=%d -> m_inf8 = 0x%02x\n", status, m_inf8);
}

void AnneAsic::sioIsr(int status)
{
        if (status) m_inf8 |= 0x10;
        else        m_inf8 &= (~0x10);

//      fprintf(stderr, "sioIsr: status=%d -> m_inf8 = 0x%02x\n", status, m_inf
}



void AnneAsic::onAsicTimer(void)
{
	++m_timerCount;
	++m_retraceCount;

        if (m_retraceCount == 18) m_retraceCount = 0;
        if (m_retraceCount < 3)                // In the vertical retrace 
                m_inf8 |=   4; 
        else    m_inf8 &= (~4);

        if (m_timerCount >= 3) /* 300Hz signal */
        {
                m_timerCount = 0;
//
// The timer interrupt is a one-shot thing; if interrupts are disabled, it 
// is lost. To emulate this, only raise the interrupt line if we know the
// interrupt will be serviced. Compare the FDC interrupt, in which the FDC
// keeps holding the interrupt line high until told to stop.
//
		if ((m_inf7 & 0x0F) < 0x0F) m_inf7++;
		if (m_sys->m_cpu->iff1) 
		{
			m_inf8 |= 1;
			m_sys->m_cpu->Int(1);
		}
	}
	else	// Here we check if a peripheral has interrupted.  
	{	// Peripherals interrupt by holding the interrupt line
		// active until acknowledged; which we simulate by 
		// interrupting every 900Hz or so until the device stops
		// bothering us. 
      
		// FDC 
	 	if (m_inf8 & 0x40) switch (m_fdcIntMode)
        	{
               		case 2: m_sys->m_cpu->Nmi(true); break;
      		        case 3: m_sys->m_cpu->Int(true); break;
        	}
		// Keyboard, COM ports	
		if (m_inf8 & 0x12) m_sys->m_cpu->Int(true);
	}
}



bool AnneAsic::parseNode(xmlDoc *doc, xmlNs *ns, xmlNode *cur)
{
        int r = 0;

        parseInteger(doc, ns, cur, "cpuspeed", &r);
        if (r) m_sys->m_cpu->setSpeed(r);

        parseInteger(doc, ns, cur, "jdos", &r);
	if (r == JM_PROMPT || r == JM_ALWAYS || r == JM_NEVER)
	{
		m_sys->setJDOSMode((JDOSMode)r);
	}

	return true;
}



bool AnneAsic::storeNode(xmlDoc *doc, xmlNs *ns, xmlNode *cur)
{
	int r = (int)(m_sys->m_cpu->getSpeed(true));
	storeInteger(doc, ns, cur, "cpuspeed", r);
	r = m_sys->getJDOSMode();
        storeInteger(doc, ns, cur, "jdos", r);
	return true;
}


bool AnneAsic::hasSettings(SDLKey *key, string &caption)
{
	*key = SDLK_g;
	caption = "  General  ";
	return true;
}

UiEvent AnneAsic::settings(UiDrawer *d)
{
	int sel = -1;
	UiEvent uie;
	char speed[50];
	int r, r2;

	r = (int)(m_sys->m_cpu->getSpeed(true));
	
	while (1)
	{
		UiMenu uim(d);
		sprintf(speed, "%d", r);	
	
		uim.add(new UiLabel("  General options  ", d));
		uim.add(new UiSeparator(d));
                uim.add(new UiTextInput(SDLK_s, "  Speed (%): ______  ",d));
                uim.add(new UiSeparator(d));
#ifdef WIN32
                uim.add(new UiTextInput(SDLK_a, "  ANNE data directory: ________________________________  ",d));
                uim.add(new UiSeparator(d));
#endif
		uim.add(new UiCommand(SDLK_f, "  File access...", d));
                uim.add(new UiSeparator(d));

                uim.add(new UiCommand(SDLK_ESCAPE, "  EXIT  ", d));
                d->centreContainer(uim);
                uim.setSelected(sel);

                ((UiTextInput &)uim[2]).setText(speed);
#ifdef WIN32
                ((UiTextInput &)uim[4]).setText(getHomeDir());
#endif
                uie =  uim.eventLoop();

                if (uie == UIE_QUIT) return uie;
                sel = uim.getSelected();
                switch(uim.getKey(sel))
		{
			case SDLK_s: 
				r2 = atoi(((UiTextInput &)uim[2]).getText().c_str());
				if (r2) r = r2;
				break;
			case SDLK_a:
				setHomeDir(((UiTextInput &)uim[4]).getText());
				break;
			case SDLK_f:
				uie = setFileAccess(d);
				if (uie == UIE_QUIT) return uie;
				break;
			case SDLK_ESCAPE:
        			if (r) m_sys->m_cpu->setSpeed(r);
				return UIE_OK;
			default: break;
		}
	}
}


byte AnneAsic::inF8(void)
{
        return m_inf8;
}

byte AnneAsic::inF7(void)
{
	byte b = m_inf7;
	m_inf7 &= 0xF0;
	return b;
}

void AnneAsic::daisyIsr(int) {}
