/************************************************************************

    JOYCE v2.1.8 - Amstrad PCW emulator

    Copyright (C) 1996, 2001-6  John Elliott <seasip.webmaster@gmail.com>

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

#define NO_PARITY 0x00
#define KB_PARITY 0x80
#define KB_STOP   0x40
#define KB_START  0x20
#define KB_BUSY   0x10
#define KB_RESET  0x04
#define KB_CLOCK  0x02
#define KB_TX     0x01

#define ST_NORMAL 0x00
#define ST_WRDATA 0x01
#define ST_STROBE 0x02
#define ST_OUTPUT 0x03

#define WAITING_DELAY 1 /*3*/

static unsigned char partable[256]={
	KB_PARITY, NO_PARITY, NO_PARITY, KB_PARITY, 
	NO_PARITY, KB_PARITY, KB_PARITY, NO_PARITY, 
	NO_PARITY, KB_PARITY, KB_PARITY, NO_PARITY,
	KB_PARITY, NO_PARITY, NO_PARITY, KB_PARITY,
	NO_PARITY, KB_PARITY, KB_PARITY, NO_PARITY, 
	KB_PARITY, NO_PARITY, NO_PARITY, KB_PARITY, 
	KB_PARITY, NO_PARITY, NO_PARITY, KB_PARITY, 
	NO_PARITY, KB_PARITY, KB_PARITY, NO_PARITY,
	NO_PARITY, KB_PARITY, KB_PARITY, NO_PARITY, 
	KB_PARITY, NO_PARITY, NO_PARITY, KB_PARITY, 
	KB_PARITY, NO_PARITY, NO_PARITY, KB_PARITY, 
	NO_PARITY, KB_PARITY, KB_PARITY, NO_PARITY,
	KB_PARITY, NO_PARITY, NO_PARITY, KB_PARITY, 
	NO_PARITY, KB_PARITY, KB_PARITY, NO_PARITY, 
	NO_PARITY, KB_PARITY, KB_PARITY, NO_PARITY, 
	KB_PARITY, NO_PARITY, NO_PARITY, KB_PARITY,
	NO_PARITY, KB_PARITY, KB_PARITY, NO_PARITY, 
	KB_PARITY, NO_PARITY, NO_PARITY, KB_PARITY, 
	KB_PARITY, NO_PARITY, NO_PARITY, KB_PARITY, 
	NO_PARITY, KB_PARITY, KB_PARITY, NO_PARITY,
	KB_PARITY, NO_PARITY, NO_PARITY, KB_PARITY, 
	NO_PARITY, KB_PARITY, KB_PARITY, NO_PARITY, 
	NO_PARITY, KB_PARITY, KB_PARITY, NO_PARITY, 
	KB_PARITY, NO_PARITY, NO_PARITY, KB_PARITY,
	KB_PARITY, NO_PARITY, NO_PARITY, KB_PARITY, 
	NO_PARITY, KB_PARITY, KB_PARITY, NO_PARITY, 
	NO_PARITY, KB_PARITY, KB_PARITY, NO_PARITY, 
	KB_PARITY, NO_PARITY, NO_PARITY, KB_PARITY,
	NO_PARITY, KB_PARITY, KB_PARITY, NO_PARITY, 
	KB_PARITY, NO_PARITY, NO_PARITY, KB_PARITY, 
	KB_PARITY, NO_PARITY, NO_PARITY, KB_PARITY, 
	NO_PARITY, KB_PARITY, KB_PARITY, NO_PARITY,
	NO_PARITY, KB_PARITY, KB_PARITY, NO_PARITY, 
	KB_PARITY, NO_PARITY, NO_PARITY, KB_PARITY, 
	KB_PARITY, NO_PARITY, NO_PARITY, KB_PARITY, 
	NO_PARITY, KB_PARITY, KB_PARITY, NO_PARITY,
	KB_PARITY, NO_PARITY, NO_PARITY, KB_PARITY, 
	NO_PARITY, KB_PARITY, KB_PARITY, NO_PARITY, 
	NO_PARITY, KB_PARITY, KB_PARITY, NO_PARITY, 
	KB_PARITY, NO_PARITY, NO_PARITY, KB_PARITY,
	KB_PARITY, NO_PARITY, NO_PARITY, KB_PARITY, 
	NO_PARITY, KB_PARITY, KB_PARITY, NO_PARITY, 
	NO_PARITY, KB_PARITY, KB_PARITY, NO_PARITY, 
	KB_PARITY, NO_PARITY, NO_PARITY, KB_PARITY,
	NO_PARITY, KB_PARITY, KB_PARITY, NO_PARITY, 
	KB_PARITY, NO_PARITY, NO_PARITY, KB_PARITY, 
	KB_PARITY, NO_PARITY, NO_PARITY, KB_PARITY, 
	NO_PARITY, KB_PARITY, KB_PARITY, NO_PARITY,
	KB_PARITY, NO_PARITY, NO_PARITY, KB_PARITY, 
	NO_PARITY, KB_PARITY, KB_PARITY, NO_PARITY, 
	NO_PARITY, KB_PARITY, KB_PARITY, NO_PARITY, 
	KB_PARITY, NO_PARITY, NO_PARITY, KB_PARITY,
	NO_PARITY, KB_PARITY, KB_PARITY, NO_PARITY, 
	KB_PARITY, NO_PARITY, NO_PARITY, KB_PARITY, 
	KB_PARITY, NO_PARITY, NO_PARITY, KB_PARITY, 
	NO_PARITY, KB_PARITY, KB_PARITY, NO_PARITY,
	NO_PARITY, KB_PARITY, KB_PARITY, NO_PARITY, 
	KB_PARITY, NO_PARITY, NO_PARITY, KB_PARITY, 
	KB_PARITY, NO_PARITY, NO_PARITY, KB_PARITY, 
	NO_PARITY, KB_PARITY, KB_PARITY, NO_PARITY,
	KB_PARITY, NO_PARITY, NO_PARITY, KB_PARITY, 
	NO_PARITY, KB_PARITY, KB_PARITY, NO_PARITY, 
	NO_PARITY, KB_PARITY, KB_PARITY, NO_PARITY, 
	KB_PARITY, NO_PARITY, NO_PARITY, KB_PARITY
   };


AnneKeyboard::AnneKeyboard(AnneSystem *s) :
        PcKeyboard(s)
{
        XLOG("AnneKeyboard::AnneKeyboard()");
        reset();
}


AnneKeyboard::~AnneKeyboard()
{

}

void AnneKeyboard::reset(void)
{
	// XXX Use m_typematic here
        SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);
	m_keysWaiting = 0;
	PcKeyboard::reset();
}

int AnneKeyboard::handleEvent(SDL_Event &e)
{
        int rv = 0;

        switch(e.type)
        {
                case SDL_QUIT: rv = -99; break;
                case SDL_KEYDOWN:
			if (e.key.keysym.sym == SDLK_MENU ||
/* 2.1.9: Make Meta+F9 bring up the menu. This is for Macs which may not have
 * a Menu key */
			   (e.key.keysym.sym == SDLK_F9 && 
			    (e.key.keysym.mod & KMOD_META)))	
			{
 				SDL_EnableKeyRepeat(0, 0);
				m_sys->m_termMenu->mainMenu(m_sys->m_cpu);
				// XXX Use m_typematic here
				SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);
                        	rv = 1;
			}
                        break;
        }
	if (!rv) return PcKeyboard::handleEvent(e);
	return rv;
}
//
// Settings functions.
// See if this device has user-settable options. If it does, populates
// "key" and "caption" and returns true; else, returns false.
//
bool AnneKeyboard::hasSettings(SDLKey *key, string &caption)
{
        *key     = SDLK_k;
        caption = "  Keyboard  ";
        return false;	// There is no keyboard settings screen
}
//
// Display settings screen
// 
UiEvent AnneKeyboard::settings(UiDrawer *d)
{
	return UIE_CONTINUE;
}
//
// XXX Implement the JOYCE "Assign key" calls
//
void AnneKeyboard::assignKey(Uint16 key, Uint8 l, Uint8 h)
{

}


void AnneKeyboard::edfe(Z80 *R)
{

}


void AnneKeyboard::clearKeys(void)
{
	m_queue.clear();
}


bool AnneKeyboard::parseNode(xmlDocPtr doc, xmlNsPtr ns, xmlNodePtr cur)
{
	return true;
}

bool AnneKeyboard::storeNode(xmlDocPtr doc, xmlNsPtr ns, xmlNodePtr cur)
{
	return true;
}

void AnneKeyboard::setRange(int *R, byte a, byte b, byte c, byte d)
{

}


void AnneKeyboard::keySet(void)
{

}


void AnneKeyboard::pushKey(byte kc)
{
	PcKeyboard::pushKey(kc);
//	fprintf(stderr, "Push keycode %02x %s\n", kc, (m_shift & (KB_CLOCK | KB_BUSY)) ? "(queued)" : "(ok)");
	if (m_shift & (KB_CLOCK | KB_BUSY)) 
	{
		/* If KB disabled or we're already interrupting, 
		 * try interrupting later */
		m_keysWaiting = WAITING_DELAY;
		return;
	}
	/* Else raise the flag of insurrection */
	((AnneAsic *)m_sys->m_asic)->keybIsr(true);
	m_shift |= KB_BUSY;
}


/* If there are keycodes in the queue, interrupt (polled at 900Hz) */

void AnneKeyboard::poll(Z80 *R)
{
	if (!m_keysWaiting) return;
//	if (m_shift & KB_CLOCK) 
	if (m_shift & (KB_CLOCK | KB_BUSY)) 
	{
//		fprintf(stderr, "No auto-ISR: KB_CLOCK\n");
		return;
	}
/*	if (m_shift & KB_BUSY) 
	{
		fprintf(stderr, "No auto-ISR: KB_BUSY\n");
		return;
	} */

	--m_keysWaiting;
	if (m_keysWaiting > 0) return; 

	// Interrupt for the next key in sequence
//	fprintf(stderr, "Auto-ISR\n");
	m_shift |= KB_BUSY;
	((AnneAsic *)m_sys->m_asic)->keybIsr(true);
}


byte AnneKeyboard::in(byte port)
{
	byte k = 0, b = 0;

	if (port == 0xF4)
	{
		m_shift &= ~KB_BUSY;
		((AnneAsic *)m_sys->m_asic)->keybIsr(false);
		if (m_queue.size()) 
		{
			k = m_queue.front();
			m_queue.pop_front();
		}
		// More than one key in the queue? If so, schedule
		// an interrupt for the next one. 
		if (m_queue.size()) 
		{
//			fprintf(stderr, "Auto-ISR queued\n");
			m_keysWaiting = WAITING_DELAY;
		}
		else m_keysWaiting = 0;
		return k;
	}
	if (port == 0xF5)
	{
		// XXX VDU pointer address bits 8,9 in bits 2,3 of b
		b = m_shift & (KB_STOP  | KB_START | 
			       KB_BUSY  | KB_CLOCK | KB_TX);

		if (m_queue.size())
		{
			k = m_queue.front();
			return partable[k] | KB_STOP | (b & 0x1F);
		}
		else
		{
			return b;
		}

	}
//	fprintf(stderr, "AnneKeyboard::in(%02x)\n", port);
	return 0xFF;
}

void AnneKeyboard::out(byte port, byte value)
{
//	fprintf(stderr, "AnneKeyboard::out(%02x, %02x) pc=%04x\n", port, value,
//		gl_sys->m_cpu->pc);
	if (port == 0xF4 && m_state == ST_OUTPUT)
	{
		m_command = value;
		return;
	}
	if (port == 0xF5)
	{
		m_shift = (m_shift & ~3) | (value & 3);
		value &= 0x87;
// Commands sent include:
//
// 0x07 / 0x87: Set TX state, reset interface, send start bit
		if (m_state == ST_OUTPUT && (value == 7 || value == 0x87))
		{
			m_state = ST_WRDATA;
			return;
		}
// 0x01: Release clock, but keep TX - byte sent to keyboard
		if (m_state == ST_WRDATA && (value == 1))
		{
			onCommand();
			m_state = ST_NORMAL;
			return;
		}
// 0x04: Reset interface to read mode
		if (value & KB_RESET) // reset interface (not keyboard)
		{
			m_shift = 0;
			m_state = ST_NORMAL;
/* If keys waiting, interrupt & send 
			if (m_queue.size() && !(m_shift & KB_CLOCK))
			{
				((AnneAsic *)m_sys->m_asic)->keybIsr(true);
				m_shift |= KB_BUSY;
			} */
			return;
		}
// 0x02: Force the clock line
		if (value & KB_CLOCK)
		{
			m_state = ST_OUTPUT;
			return;
		}
	}
//	fprintf(stderr, "AnneKeyboard::out(%02x, %02x)\n", port, value);
}
