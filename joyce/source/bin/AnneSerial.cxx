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
#include "UiScrollingMenu.hxx"
#include "PcwFileList.hxx"
#include "PcwFileChooser.hxx"

//
// Emulate the PCW16's 16550-compatible serial chipset.
// This is not a complete emulation; loopback mode and FIFOs are not 
// implemented, for instance. 
//
// The serial mouse emulation is done separately; AnneZ80.cxx routes calls to
// the mouse or the serial port depending on the mouse configuration.
//
AnneSerial::AnneSerial(byte basePort, AnneSystem *s) : PcwDevice("serial", 
		(basePort == 0x28) ? "com1" :
		(basePort == 0x20) ? "com3" : "unknown")
{
	m_sys = s;
	m_basePort = basePort;
	m_comms = newComms();
	m_z80tick = 0;
}


AnneSerial::~AnneSerial()
{

}



void AnneSerial::reset(void)
{
	m_dlab = 0;
	m_dll  = 0;
	m_dlh  = 0;
	m_rbr  = 0;
	m_ier  = 0;
	m_iir  = 1;
	m_lcr  = 0;
	m_mcr  = 0;
	m_lsr  = 0;
	m_msr  = 0;
	m_scr  = 0;
	m_thr  = 0;
	m_fcr  = 0;
	setPower(false);	
}


byte AnneSerial::in(byte port)
{
	if (port < m_basePort || port > (m_basePort + 7)) return 0xFF;
	
	port -= m_basePort;

	switch(port)
	{
		case 0: if (m_dlab) return m_dll;
			if (m_comms && m_comms->getRxCharacterAvailable())
			{	
				m_rbr = m_comms->read();
			}
			m_lsr &= ~1;	// Character has been read;
					// remove 'char waiting' from LSR
// If receiving char raised interrupt, reset it.
//			if (!(m_iir & 1))	
//			{
				m_iir |= 1;
				((AnneAsic *)m_sys->m_asic)->sioIsr(false);
//			}
			return m_rbr;
		case 1: if (m_dlab) return m_dlh;
			else	    return m_ier;
		case 2: return m_iir;
		case 3: return m_lcr;
		case 4:	return m_mcr;
		case 5: m_lsr = 0x20;	// Ready to transmit 
			if (m_comms)
			{
				if (m_comms->getAllSent()) 
					m_lsr |= 0x40;
				// Ready to receive
				if (m_comms->getRxCharacterAvailable())
					m_lsr |= 1;
				// Bits 1,2,3 are errors which m_comms
				// doesn't report.
				// Bit 4 is break which m_comms doesn't report.
			}
			return m_lsr;	// Line status
		case 6: m_msr &= 0x40;	// Clear all bits except Ring Indicator
			if (m_comms)
			{
				if (m_comms->getCTS()) m_msr |= 0x10;	// CTS	
			}
			return m_msr;	// Modem status
		case 7: return m_scr;	// Scratchpad
	}
	return 0xFF;
}

void AnneSerial::out(byte port, byte value)
{
	if (port < m_basePort || port > (m_basePort + 7)) return;
	port -= m_basePort;

	switch(port)
	{
// Divisor latch low, or Transmitter Holding Register
		case 0: if (m_dlab) 
			{
				m_dll = value;
				m_dlatch = (((word)m_dlh) << 8) | m_dll;
				m_baud = 115200 / m_dlatch;
				if (m_comms) m_comms->setTX(m_baud);
				if (m_comms) m_comms->setRX(m_baud);
			}
			else
			{
				m_thr = value; 
				if (m_comms) m_comms->write(value);
			}
			break;
// Divisor latch high, or Interrupt Enable Register
		case 1: if (m_dlab) 
			{
				m_dlh = value;
				m_dlatch = (((word)m_dlh) << 8) | m_dll;
				m_baud = 115200 / m_dlatch;
				if (m_comms) m_comms->setTX(m_baud);
				if (m_comms) m_comms->setRX(m_baud);
			}
			else	    m_ier = value;	
			break;
		case 2: m_fcr = value;	// FIFO control register
			break;
		case 3: m_lcr = value;	// Line control register
// Bit 7: Divisor Latch Access Bit
			m_dlab = (value & 0x80);
			if (!m_comms) break;
// Bits 0,1: Data bits
			m_comms->setTXbits((value & 3) + 5);
			m_comms->setRXbits((value & 3) + 5);
// Bit 2: Stop bits
			if (value & 4)
			{
				if (value & 3) m_comms->setStop(4);
				else	       m_comms->setStop(3);
			}
			else	m_comms->setStop(2);
// Bits 3-5: Parity
			if (value & 8) 
			{
				if (value & 4) m_comms->setParity(PE_EVEN);
				else	       m_comms->setParity(PE_ODD);
			}
			else	m_comms->setParity(PE_NONE);
// Bit 6: Break
			if (value & 0x40) m_comms->sendBreak();
			break;
		case 4: m_mcr = value;
			if (!m_comms) break;
			m_comms->setDTR(value & 1);
			m_comms->setRTS(value & 2);
			// Skip bits 2 & 3 (OUT1, OUT2)
			// Skip bit 4 (loopback)
			break;
		case 7: m_scr = value;	// Scratchpad
			break;
	}
}


void AnneSerial::tick(void)
{
	bool sendint;

        // How often should this be called? Once per Z80 instruction is 
        // excessive. Perhaps once every 100. It works in JOYCE, after all.
        if (++m_z80tick < 100) return;

	m_z80tick = 0;
	if (!isEnabled()) return;
	if (!m_comms) return;

	sendint = false;
	if (m_ier & 1)  sendint = m_comms->getRxCharacterAvailable();
	if (sendint) 
	{
		m_lsr |= 1;	// Char ready
		m_iir &= ~1;	// Interrupting;
		m_iir |= 4;	// Received data ready
		((AnneAsic *)m_sys->m_asic)->sioIsr(true);
	}

}

bool AnneSerial::parseNode(xmlDocPtr doc, xmlNsPtr ns, xmlNodePtr cur)
{
        parseFilename(doc, ns, cur, "sioout", m_comms->m_outfile);
        parseFilename(doc, ns, cur, "sioin",  m_comms->m_infile);
        m_comms->flush();
	return true;
}


bool AnneSerial::storeNode(xmlDocPtr doc, xmlNsPtr ns, xmlNodePtr cur)
{
	storeFilename(doc, ns, cur, "sioout", m_comms->m_outfile);
	storeFilename(doc, ns, cur, "sioin",  m_comms->m_infile);
	return true;
}

string AnneSerial::getTitle(void)
{
	return "  Serial port  ";
}


bool AnneSerial::hasSettings(SDLKey *key, string &caption)
{
	switch (m_basePort)
	{
		case 0x20: caption = "  COM3  "; *key = SDLK_3; break;
		case 0x28: caption = "  COM1  "; *key = SDLK_1; break;
		default:   caption = "  Serial  "; *key = SDLK_s; break;
	}
	return true;
}


// Power switch handling. In a PCW16, the power switch is hooked up to the
// Ring Indicator of the first serial port.
UiEvent AnneSerial::power(UiDrawer *d)
{
	return UIE_CONTINUE;
}

UiEvent AnneSerial::settings(UiDrawer *d)
{
	UiEvent e;
	SDLKey k;
	int sel;
	string cap;

	hasSettings(&k, cap);	// Get title
	while (1)
	{
		UiMenu uim(d);
		string sOut = "  Output: " + displayName(m_comms->m_outfile, 40);
		string sIn  = "  Input:  " + displayName(m_comms->m_infile,  40);

		uim.add(new UiLabel(cap, d));
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



void AnneSerial::setPower(bool b)
{
	if (b) m_msr &= ~0x40;
	else   m_msr |= 0x40;
}

bool AnneSerial::getPower()
{
	return ((m_msr & 0x40) == 0);
}






