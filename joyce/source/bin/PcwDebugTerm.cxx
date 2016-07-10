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

//
// Debugger console
//

PcwDebugTerm::PcwDebugTerm(PcwSystem *s) : PcwSdlTerm("debug", s)
{
	XLOG("PcwDebugTerm::PcwDebugTerm()");
	m_clrFG.r = 0xFF; m_clrFG.g = 0x80; m_clrFG.b = 0;
	m_clrBG.r = 0x00; m_clrBG.g = 0x00; m_clrBG.b = 0;
}

PcwDebugTerm::~PcwDebugTerm()
{
	XLOG("PcwDebugTerm::~PcwDebugTerm()");
}


void PcwDebugTerm::setSysVideo(PcwSdlContext *c)
{
	SDL_Surface *s;

        if (m_term) delete m_term;
        m_term = NULL;
        if (c) 
	{
		SDL_Rect rc;

		s = c->getSurface();
		rc.x = 0; 
		rc.w = s->w;
		rc.h = s->h / 2;
		rc.y = s->h - rc.h;	
		m_term = new SdlTerm(s, &rc);
	}
        PcwSdlTerm::setSysVideo(c);
}


// Focus: Unlike the other terminals, the debugger has focus at the same time
// as another terminal.

void PcwDebugTerm::onGainFocus()     // Terminal is gaining focus
{
        if (m_term)
        {
                m_term->setForeground(m_clrFG);
                m_term->setBackground(m_clrBG);
		m_term->clearWindow();
		m_term->enableCursor();
        }
        if (m_framebuf && m_sysVideo)
        {
		SDL_Rect rc;
                SDL_Surface *s = m_sysVideo->getSurface();

		rc.x = 0;
		rc.w = s->w;
		rc.h = s->h / 2;
                rc.y = s->h - rc.h;

                SDL_BlitSurface(m_framebuf->getSurface(), &rc, s, &rc);
                SDL_UpdateRect(s, 0, 0, s->w, s->h);    
        }
}


void PcwDebugTerm::onLoseFocus()     // Terminal is about to lose focus
{
	if (m_term) m_term->enableCursor(false);
	PcwSdlTerm::onLoseFocus();
}



PcwTerminal &PcwDebugTerm::operator << (unsigned char c)
{
	if (c == '\n' && m_term) { m_term->cr(); m_term->lf(); return (*this); }

	return PcwSdlTerm::operator <<(c);
}

