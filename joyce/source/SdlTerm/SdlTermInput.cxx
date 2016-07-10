/************************************************************************

    SdlTerm v1.00 - Terminal emulation in an SDL framebuffer

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

#include <SDL.h>
#include "SdlContext.hxx"
#include "SdlTerm.hxx"

////////////////////////////////////////////////////////////////////////////
// Input
////////////////////////////////////////////////////////////////////////////

void SdlTerm::resetInput(void)
{
	m_inpBuf = "";

	for (int n = 0; n < SDLK_LAST; n++)
	{
		m_keyState[n] = false;
		for (int m = 0; m < 8; m++) (*m_keyMap[m][n]) = "";
	}
	resetKeyMap();
}

static const char gl_ukshifts[] = ")!\"£$%^&*(";

void SdlTerm::resetKeyMap(void)
{
	int n,m;
	// Set up a default key map...
	for (n = SDLK_FIRST; n <= SDLK_DELETE; n++)
	{
		for (m = 0; m < SH_ACSHIFT; m++) 
		{
			char c = n;
			if (c) (*m_keyMap[m][n]) = c;
		}
	}

	// Letter keys
	for (n = 0; n < 26; n++)
	{
		(*m_keyMap[SH_NONE   ][SDLK_a + n]) = 'a' + n;
		(*m_keyMap[SH_SHIFT  ][SDLK_a + n]) = 'A' + n;
		(*m_keyMap[SH_CTRL   ][SDLK_a + n]) =  
		(*m_keyMap[SH_CSHIFT ][SDLK_a + n]) =  1  + n;
// XXX ALT keys not assigned... yet
	}
	// Number keys
	for (n = 0; n < 10; n++)
	{
		(*m_keyMap[SH_NONE   ][SDLK_0 + n]) = '0' + n;
		(*m_keyMap[SH_SHIFT  ][SDLK_0 + n]) = gl_ukshifts[n];
// XXX Again, not assigned yet

// Numeric keypad
		(*m_keyMap[SH_NONE   ][SDLK_KP0 + n]) = '0' + n;
		(*m_keyMap[SH_SHIFT  ][SDLK_KP0 + n]) = '0' + n;
		(*m_keyMap[SH_CTRL   ][SDLK_KP0 + n]) = '0' + n;
		(*m_keyMap[SH_CSHIFT ][SDLK_KP0 + n]) = '0' + n;
		(*m_keyMap[SH_ALT    ][SDLK_KP0 + n]) = '0' + n;
		(*m_keyMap[SH_ASHIFT ][SDLK_KP0 + n]) = '0' + n;
		(*m_keyMap[SH_ACTRL  ][SDLK_KP0 + n]) = '0' + n;
		(*m_keyMap[SH_ACSHIFT][SDLK_KP0 + n]) = '0' + n;

	}
	for (n = 0; n < SH_ACSHIFT; n++) 
	{
		(*m_keyMap[n][SDLK_BACKSPACE])   = 127;
		(*m_keyMap[n][SDLK_KP_PERIOD])   = '.';
		(*m_keyMap[n][SDLK_KP_DIVIDE])   = '/';
		(*m_keyMap[n][SDLK_KP_MULTIPLY]) = '*';
		(*m_keyMap[n][SDLK_KP_MINUS])    = '-';
		(*m_keyMap[n][SDLK_KP_PLUS])     = '+';
		(*m_keyMap[n][SDLK_KP_ENTER])    = 'M' - '@';
		(*m_keyMap[n][SDLK_KP_EQUALS])   = '=';

		(*m_keyMap[n][SDLK_UP])          = '_' - '@';
		(*m_keyMap[n][SDLK_DOWN])        = '^' - '@';
		(*m_keyMap[n][SDLK_LEFT])        = 'A' - '@';
		(*m_keyMap[n][SDLK_RIGHT])       = 'F' - '@';
		(*m_keyMap[n][SDLK_INSERT])      = 'V' - '@';
		(*m_keyMap[n][SDLK_HOME])        = "\006\002\002";
		(*m_keyMap[n][SDLK_END])         = "\006\002";
		(*m_keyMap[n][SDLK_PAGEUP])      = 'R' - '@';
		(*m_keyMap[n][SDLK_PAGEDOWN])    = 'C' - '@';
	
		(*m_keyMap[n][SDLK_PRINT])       = 'P' - '@';
	}
	(*m_keyMap[SH_SHIFT][SDLK_HASH])   = '~';
	(*m_keyMap[SH_SHIFT][SDLK_QUOTE])  = '@';
	(*m_keyMap[SH_SHIFT][SDLK_EQUALS]) = '+';
	(*m_keyMap[SH_SHIFT][SDLK_COMMA])  = '<';
	(*m_keyMap[SH_SHIFT][SDLK_MINUS])  = '_';
	(*m_keyMap[SH_SHIFT][SDLK_PERIOD]) = '>';
	(*m_keyMap[SH_SHIFT][SDLK_SLASH])  = '?';
	(*m_keyMap[SH_SHIFT][SDLK_SEMICOLON])    = ':';
	(*m_keyMap[SH_SHIFT][SDLK_LEFTBRACKET])  = '{';
	(*m_keyMap[SH_SHIFT][SDLK_BACKSLASH])    = '|';
	(*m_keyMap[SH_SHIFT][SDLK_RIGHTBRACKET]) = '}';
	(*m_keyMap[SH_SHIFT][SDLK_BACKQUOTE])    = '¬';
	(*m_keyMap[SH_CTRL ][SDLK_PAUSE])        = 'C' - '@';
	(*m_keyMap[SH_CSHIFT][SDLK_PAUSE])       = 'C' - '@';
}


bool SdlTerm::pullChar(unsigned char *c)
{
	if (m_inpBuf.length() == 0) return false;

	*c = (m_inpBuf.data())[0];
	m_inpBuf = m_inpBuf.substr(1);
	return true;
}

void SdlTerm::pushChar(unsigned char c)
{
	m_inpBuf += c;
}

void SdlTerm::pushChar(std::string &s)
{
	m_inpBuf += s;
}


// To consider:
// Key translation tables (SDL to ASCII) and defaults
// Load/save layout as XML
// Key repeat

int SdlTerm::onKeyDown(SDL_KeyboardEvent &k)
{
	int shifts = 0;
	SDLMod m = SDL_GetModState();

	if (m & KMOD_SHIFT) shifts |= SH_SHIFT;
	if (m & KMOD_CTRL ) shifts |= SH_CTRL;
	if (m & KMOD_ALT  ) shifts |= SH_ALT;
	m_keyState[k.keysym.sym] = 1;
	
	pushChar(*m_keyMap[shifts][k.keysym.sym]);
	return 1;	
}


int SdlTerm::onKeyUp(SDL_KeyboardEvent &k)
{
	m_keyState[k.keysym.sym] = 0;

	return 1;
}


// Note: SdlTerm may not be the object that does the SDL_PollEvent().
//      If you are doing SDL_PollEvent()s elsewhere, then pass unhandled
//      events through to SdlTerm::OnEvent(). This will return 1 if the
//      event was handled, 0 if it wasn't, -2 if the event was SDL_Quit.
//
//      You could also call SdlTerm::poll(). This calls SDL_PollEvent()
//      and passes the result to SdlTerm::onEvent(). Return values are
//      the same, plus -1 if there was no event waiting.
//
int  SdlTerm::onEvent(SDL_Event &ev)
{
	if (ev.type == SDL_QUIT) return -2;

	if (ev.type == SDL_KEYDOWN) return onKeyDown(ev.key);
	if (ev.type == SDL_KEYUP)   return onKeyUp  (ev.key);

	return 0;
}


unsigned char SdlTerm::read(void)
{
	int ret;
	unsigned char c;

	if (pullChar(&c)) return c;	// Character in the buffer - okay.
	while(1)
	{
		ret = poll();
		if (ret == -2) return 0;	// SDL_Quit
		if (ret == -1)			// Nothing happened
		{
			SDL_Delay(20);
			onTimer50();
		}
		// Key was pressed, and ASCII is in the buffer
		if (ret == 1 && pullChar(&c)) return c;
	}
	return 0;
}


//
// Poll for SDL event. Returns:
// 1.  Event receieved and handled
// 0.  Event received and ignored
// -1. No event received.
// -2. SDL_Quit received.
//
int SdlTerm::poll(void)
{
        SDL_Event ev;

	int err = SDL_PollEvent(&ev);
	if (err) return onEvent(ev);
	return -1;
}


bool SdlTerm::kbhit(void)
{
	if (m_inpBuf.length() == 0) return false;
	return true;
}

