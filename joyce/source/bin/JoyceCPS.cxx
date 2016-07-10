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
#include "UiScrollingMenu.hxx"
#include "PcwFileList.hxx"
#include "PcwFileChooser.hxx"

//
// Emulation of the two Z80 DART channels
//

void Z80Dart::reset()
{
	m_regmode = REGMODE_REGISTER;
	m_regno = 0;
	m_latch = 0;
	memset(m_reg,  0, sizeof(m_reg));
	memset(m_rreg, 0, sizeof(m_rreg));
}

void Z80Dart::outCtrl(byte b)
{
	switch(m_regmode)
	{
		case REGMODE_REGISTER:
			m_regno     = (b & 7);
			if (m_regno) m_regmode = REGMODE_DATA;
			else	     out(0, b);
			break;
		case REGMODE_DATA:
			out(m_regno, b);
			m_regmode = REGMODE_REGISTER;
			break;
	}
}


byte Z80Dart::inCtrl()
{	
	switch(m_regmode)
	{
		case REGMODE_REGISTER:
			return in(0);
		case REGMODE_DATA:
			m_regmode = REGMODE_REGISTER;
			return in(m_regno);
	}
	return 0xFF;
}


byte Z80Dart::inData()
{
	return m_latch;
}

void Z80Dart::outData(byte b)
{
	m_latch = b;
}

void Z80Dart::out(byte reg, byte value)
{
	if (reg == 0) switch(value >> 3)
	{
		case 0: break;	  // Null code
		case 1: break;	  // Not used
//		case 2: break;	  // Reset interrupts
		case 3: reset();  // Reset everything
			break;
//		case 4: break;	  // XXX Switch to interrupt mode?
//		case 5: break;	  // XXX Reset TX int pending
//		case 6: break;	  // Error reset
//		case 7: break;	  // RETI
		joyce_dprintf("Unsupported DART command %d\n", (value>>3));
	}
	m_reg[reg & 7] = value;
}

byte Z80Dart::in(byte reg)
{
	if (reg  < 2) return m_rreg[reg];
	if (reg == 2) return m_reg[2];
	return 0xFF;
}

///////////////////////////////////////////////////////////////////////////////
// Special code for channel A: Hook up the DART to a JoyceComms object
// 

byte Z80DartA::in(byte reg)
{
	switch(reg)
	{
		case 0:	m_rreg[0] &= ~(0x25);
			if (m_comms->getRxCharacterAvailable()) m_rreg[0] |= 1;
			if (m_comms->getCTS()) m_rreg[0] |= 0x20;
			if (m_comms->getTxBufferEmpty()) m_rreg[0] |= 4;
			break;
		case 1: m_rreg[1] &= 0x01;
			if (m_comms->getAllSent()) m_rreg[1] |= 1;
			break;
	}	
	return Z80Dart::in(reg);
}

#define COMP(n,m)  ((value & m) != (m_reg[n] & m))

void Z80DartA::out(byte reg, byte value)
{
//
// When values change wrt the stored values, pass them through to the comms 
// layer
//
	if (reg == 5)
	{
		// bit 1 unused
		if  COMP(5, 0x02)  m_comms->setRTS(value & 2);
		// bit 3 unused
		// bit 4: tx enable
		if (COMP(5, 0x10) && (value & 0x10)) m_comms->sendBreak();
		if  COMP(5, 0x60)  m_comms->setTXbits(((value >> 5) & 3) + 5);
		if  COMP(5, 0x80)  m_comms->setDTR(value & 0x80);
	}
	if (reg == 4)
	{
		if COMP(4, 3) switch(value & 3)
		{
			case 0: case 2: m_comms->setParity(PE_NONE); break;
			case 1:		m_comms->setParity(PE_ODD);  break;
			case 3:		m_comms->setParity(PE_EVEN); break;
		}
		if COMP(4,0x0C) switch(value & 0x0C)
		{
			case 4:  m_comms->setStop(2); break;
			case 8:  m_comms->setStop(3); break;
			case 12: m_comms->setStop(4); break;
		}
		// bits 4-5: unused
		// bits 6-7: clock multiplier
	}
	if (reg == 3)
	{
		// bit 0: rx enable
		// bits 1-4: reserved, must be 0
		// bit 5: auto enable
		if  COMP(3, 0xC0)  m_comms->setRXbits(((value >> 6) & 3) + 5);
	}
	// reg 1: Bits 3,4 are 0 for polled mode, 0x18 for interrupt mode.
	return Z80Dart::out(reg, value);
}


byte Z80DartA::inData()
{
	m_latch = m_comms->read();
	return Z80Dart::inData();
}

void Z80DartA::outData(byte b)
{
	Z80Dart::outData(b);
	m_comms->write(m_latch);
}

///////////////////////////////////////////////////////////////////////////////
// Special code for channel B:
// 
// CTS is the printer's busy line
// DTR is the printer's strobe line
// Ring Indicator and RTS are used by the clock on the SCA Mk2 interface,
// which we don't implement.
//
byte Z80DartB::in(byte reg)
{
	if (reg == 0)	// Set CTS from printer status	
	{
		m_rreg[0] &= ~(0x20);
		m_rreg[0] |= (m_printer->isBusy() ? 0x00 : 0x20);
	}
	return Z80Dart::in(reg);
}


void Z80DartB::out(byte reg, byte value)
{
	byte strobe = m_reg[5] & 0x80;
	Z80Dart::out(reg, value);

	// If STROBE has just dropped, let the data latch know.
	if (reg == 5 && strobe)
	{
		if ((m_reg[5] & 0x80) != strobe) m_printer->dropStrobe();
	}		
}

//////////////////////////////////////////////////////////////////////////
JoyceCPS::JoyceCPS(JoyceSystem *s) : PcwPrinter("cps8256"), 
					m_dartA(m_comms = newComms()),
					m_dartB(this)
{
	XLOG("JoyceCPS::JoyceCPS()");
	m_sys = s;
	reset();
}

JoyceCPS::~JoyceCPS()
{

}

void JoyceCPS::reset(void)
{
	m_z80tick = 0;
	m_baudmode = 0;
	m_txbaud = m_rxbaud = 9600;
	m_cench = -1;
	m_dartA.reset();
	m_dartB.reset();
	PcwPrinter::reset();
}


byte JoyceCPS::in(byte port)
{
	if (!isEnabled()) return 0xFF;
	
	switch(port & 0x0F)
	{
		case 0: return m_dartA.inData();
		case 1: return m_dartA.inCtrl();
		case 2: return m_dartB.inData();
		case 3: return m_dartB.inCtrl();

	}
	return 0xFF;
}


//
// The DART has just dropped its DTR (STROBE) line...
//
void JoyceCPS::dropStrobe(void)
{
	writeChar(m_cench);	
}


void JoyceCPS::out(byte port, byte value)
{
	if (!isEnabled()) return;
	switch(port & 0x0F)
	{
		case 0: m_dartA.outData(value); break;
		case 1: m_dartA.outCtrl(value); break;
		case 2: m_dartB.outData(value); break;
		case 3: m_dartB.outCtrl(value); break;	

		case 4:	// 8253 timer
		case 5:
			switch(m_baudmode)
			{
				unsigned int calcv;

				case 1: 
				case 3: m_baudbuf = value; ++m_baudmode; break;

				case 2: 
				case 4: calcv = 256 * value + m_baudbuf; 
					if (calcv) calcv = 125000 / calcv;
					if (m_baudmode == 2) 
					{
						m_txbaud = calcv;
						m_comms->setTX(m_txbaud);
					}
					else
					{
						m_rxbaud = calcv;
						m_comms->setRX(m_rxbaud);
					}
					m_baudmode = 0;
					break;
			}
			break;

		case 7: // 8253 timer control
			if      (value == 0x36) m_baudmode = 1;	// Set TX
			else if (value == 0x76) m_baudmode = 3;	// Set RX
			else joyce_dprintf("OUT %x, %02x\n", port, value);
			break;
		case 8: m_cench = value; break;
	}
}




bool JoyceCPS::hasSettings(SDLKey *key, string &caption)
{
        *key = SDLK_8;
        caption = "  CPS8256 interface  ";
        return true;
}


string JoyceCPS::getTitle(void)
{
        return "  CPS8256 parallel port  ";
}

//
// Support the PS8256 being in interrupt mode.
//
void JoyceCPS::tick(void)
{
	// How often should this be called? Once per Z80 instruction is 
	// excessive. Perhaps once every 100. MAIL232 seems quite happy
	// with this.
	if (++m_z80tick < 100) return;

	if (!isEnabled()) return;
	// Is the SIO in interrupt mode?
	if (!(m_dartA.m_reg[1] & 0x18)) return; 

	bool b = m_comms->getRxCharacterAvailable();
	if (b)
	{
		// Interrupt, and keep the interrupt line high until someone
		// comes to deal with it.
		(m_sys->m_cpu)->Int(1);
		m_dartA.m_rreg[0] |= 3;
	}
	else	m_dartA.m_rreg[0] &= ~3;
	m_z80tick = 0;	
}


UiEvent JoyceCPS::settings(UiDrawer *d)
{
	UiEvent e;

        while (1)
        {
                UiMenu uim(d);
                bool bEnabled;
		int sel;

                bEnabled = isEnabled();
                uim.add(new UiLabel("  CPS8256 interface  ", d));
                uim.add(new UiSeparator(d));
                uim.add(new UiSetting(SDLK_d, !bEnabled, "  Disconnected  ", d));
                uim.add(new UiSetting(SDLK_c,  bEnabled, "  Connected  ",    d));
		if (bEnabled)
		{
			uim.add(new UiSeparator(d));
		        uim.add(new UiCommand(SDLK_p, "  Parallel port  ", d, UIG_SUBMENU));
			uim.add(new UiCommand(SDLK_s, "  Serial port  ",   d, UIG_SUBMENU));
		}
	       	uim.add(new UiSeparator(d));
	        uim.add(new UiCommand(SDLK_ESCAPE, "  EXIT  ", d));
		d->centreContainer(uim);	
		e = uim.eventLoop();
                if (e == UIE_QUIT) return e;
                sel = uim.getSelected();
                switch(uim.getKey(sel))
		{
			case SDLK_d: enable(false); break;
			case SDLK_c: enable();      break;
			case SDLK_ESCAPE: return UIE_CONTINUE;
			case SDLK_s: e = serialSettings(d);
				     if (e == UIE_QUIT) return e;
				     break;
			case SDLK_p: e = PcwPrinter::settings(d);
				     if (e == UIE_QUIT) return e;
				     break;
			default:	break;
		}
	}
	return UIE_CONTINUE;
}

UiEvent JoyceCPS::serialSettings(UiDrawer *d)
{
	UiEvent e;
	SDLKey k;
	int sel;

	while (1)
	{
		UiMenu uim(d);
		string sOut = "  Output: " + displayName(m_comms->m_outfile, 40);
		string sIn  = "  Input:  " + displayName(m_comms->m_infile,  40);

		uim.add(new UiLabel("  CPS8256 serial port  ", d));
                uim.add(new UiSeparator(d));
                uim.add(new UiCommand(SDLK_i, sIn,  d, UIG_SUBMENU));
                uim.add(new UiCommand(SDLK_o, sOut, d, UIG_SUBMENU));
#ifdef WIN32
                uim.add(new UiSeparator(d));
		uim.add(new UiCommand(SDLK_1, "  Use COM1:  ",d ));
		uim.add(new UiCommand(SDLK_2, "  Use COM2:  ",d ));
		uim.add(new UiCommand(SDLK_3, "  Use COM3:  ",d ));
		uim.add(new UiCommand(SDLK_4, "  Use COM4:  ",d ));
#endif
                uim.add(new UiSeparator(d));
                uim.add(new UiCommand(SDLK_ESCAPE, "  EXIT  ", d));
                d->centreContainer(uim);
                e = uim.eventLoop();
		if (e == UIE_QUIT) return e;
	
		sel = uim.getSelected();	
		switch((k = uim.getKey(sel)))
		{
			case SDLK_1: m_comms->m_infile = m_comms->m_outfile = "COM1:"; m_comms->flush(); break;
			case SDLK_2: m_comms->m_infile = m_comms->m_outfile = "COM2:"; m_comms->flush(); break;
			case SDLK_3: m_comms->m_infile = m_comms->m_outfile = "COM3:"; m_comms->flush(); break;
			case SDLK_4: m_comms->m_infile = m_comms->m_outfile = "COM4:"; m_comms->flush(); break;
			case SDLK_i: 
			case SDLK_o:
			{
				string s = m_comms->m_infile;
				UiEvent e;
				PcwFileChooser f("  OK  ", d);

				if (k == SDLK_o) s = m_comms->m_outfile;
				f.initDir(s);
				e = f.eventLoop();
				if (e == UIE_QUIT) return e;
				if (e == UIE_OK)
				{
					if (k == SDLK_o) m_comms->m_outfile = f.getChoice();
					if (k == SDLK_i) m_comms->m_infile  = f.getChoice();
 					m_comms->flush();
				} 
			}
			break;
			case SDLK_ESCAPE: return UIE_OK;
			default: break;
		}	
	}
	return UIE_OK;
} 

//
// parseNode() and storeNode() only cover the serial settings. The base class
// does parallel.
//
bool JoyceCPS::parseNode(xmlDocPtr doc, xmlNsPtr ns, xmlNodePtr cur)
{
	parseFilename(doc, ns, cur, "sioout", m_comms->m_outfile);
	parseFilename(doc, ns, cur, "sioin",  m_comms->m_infile);
	m_comms->flush();
	return PcwPrinter::parseNode(doc, ns, cur);
}

bool JoyceCPS::storeNode(xmlDocPtr doc, xmlNsPtr ns, xmlNodePtr cur)
{
	storeFilename(doc, ns, cur, "sioout", m_comms->m_outfile);
	storeFilename(doc, ns, cur, "sioin",  m_comms->m_infile);
	return PcwPrinter::storeNode(doc, ns, cur);
}

