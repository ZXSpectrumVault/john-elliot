/************************************************************************

    JOYCE v2.1.7 - Amstrad PCW emulator

    Copyright (C) 1996, 2001-5  John Elliott <seasip.webmaster@gmail.com>

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
#include "inline.hxx"
#include "UiContainer.hxx"
#include "UiMenuBar.hxx"
#include "UiLabel.hxx"
#include "UiCommand.hxx"
#include "UiSetting.hxx"
#include "UiMenu.hxx"
#include "UiSeparator.hxx"
#include "UiMenuBarCommand.hxx"

PcwMenuTerm::PcwMenuTerm(PcwSystem *s) : PcwSdlTerm("menu", s)
{
	XLOG("PcwMenuTerm::PcwMenuTerm()");
	m_menuMutex = false;

        m_clrBG.r   = 0x00; m_clrBG.g   = 0x00; m_clrBG.b   = 0xAA;
        m_clrFG.r   = 0x55; m_clrFG.g   = 0xFF; m_clrFG.b   = 0xFF;

	m_popupBacking = NULL;
	m_popup[0] = 0;
	m_menuBar = NULL;
}


PcwMenuTerm::~PcwMenuTerm()
{
	XLOG("PcwMenuTerm::~PcwMenuTerm()");
	if (m_menuBar) delete m_menuBar;
}


void PcwMenuTerm::setSysVideo(PcwSdlContext *c)
{
	char buf[200];
	sprintf(buf, "PcwMenuTerm::setSysVideo(%p) : m_term=%p, this=%p",
			c, m_term, this);
	XLOG(buf);

	if (m_term) delete m_term;
	m_term = NULL;
	XLOG("PcwMenuTerm::setSysVideo() creating new terminal");
	if (c) m_term = new SdlTerm(c->getSurface());
	XLOG("PcwMenuTerm::setSysVideo() setting FG / BG");
	if (m_term)
	{
		m_term->setForeground(m_clrFG);
		m_term->setBackground(m_clrBG);
	}
	XLOG("PcwMenuTerm::setSysVideo() calling base");
	PcwSdlTerm::setSysVideo(c);
	XLOG("PcwMenuTerm::setSysVideo() done");
}

void PcwMenuTerm::onGainFocus()
{
	if (m_term)
	{
		m_term->setForeground(m_clrFG);
		m_term->setBackground(m_clrBG);
		m_term->enableCursor(false);
	}	
	PcwSdlTerm::onGainFocus();
}

/* Ask the user if they want to quit. Returns only if they don't */

UiEvent PcwMenuTerm::confirm(int x, int y, const string prompt)
{	
	int lpad, rpad, tlen;
	string title;
	UiEvent e;
	UiMenu mc(this);

	if (prompt != "") 
	{
		tlen = prompt.size();
		if (tlen >= 14) title = prompt.substr(0, 15);
		else
		{
			title = "";
			lpad = (15 - tlen) / 2;
			rpad = 15 - (tlen + lpad);
			while (lpad--) title += " ";
			title += prompt;
			while (rpad--) title += " ";	
		}
                mc.add(new UiLabel(title, this));
	}
	mc.add(new UiLabel(" Are you sure? ", this))
	  .add(new UiSeparator(this))
	  .add(new UiCommand(SDLK_n, "  No",  this))
	  .add(new UiCommand(SDLK_y, "  Yes", this));
	mc.setX(x);
	mc.setY(y);

	e = mc.eventLoop();

	if (e == UIE_QUIT) return e;

	if (mc.getKey(mc.getSelected()) == SDLK_y) return UIE_OK;
	return UIE_CANCEL;
}





UiEvent PcwMenuTerm::report(const string s, const string prompt)
{
	UiMenu uim(this);
	SDL_Rect &rc = getSysVideo()->getBounds();
	int x,y;
	Uint16 sw, sh, bw, bh;

	/* Let (sw, sh) be the union of the rectangles containing 
	 * s and prompt */
	measureString(s,      &sw, &sh);
	measureString(prompt, &bw, &bh);

	if (bw > sw) sw = bw;
	if (bh > sh) sh = bh;
	measureString("A", &bw, &bh);


	uim.add(new UiLabel(s, this))
	   .add(new UiSeparator(this))
	   .add(new UiCommand(SDLK_o, prompt, this));
        y = (rc.h -  6 *       bh) / 2;
        x = (rc.w -  bw - sw - bw) / 2;

	x = alignX(x);
	y = alignY(y);

	uim.setX(x);
	uim.setY(y);
	return uim.eventLoop();
}
	


void PcwMenuTerm::exitMenu(void)
{
	int x, y;
	UiEvent uie;
	SDL_Rect &rc = getSysVideo()->getBounds();
	Uint16 bw, bh, sw, sh;

	if (m_menuMutex) return;

	m_menuMutex = true;
        m_oldTerm = m_sys->m_activeTerminal;
	SDL_BlitSurface(m_sysVideo->getSurface(), NULL,
			m_framebuf->getSurface(), NULL);
        m_sys->selectTerminal(this);
	SDL_ShowCursor(1);

	measureString("A", &bw, &bh);
	measureString("  Leave JOYCE  ", &sw, &sh);

	y = (rc.h - sh - 5 * bh) / 2;
	x = (rc.w - sw - 2 * bw) / 2;

	y = alignY(y);
	x = alignX(x);
	
	uie = confirm(x, y, "Leave JOYCE");
	if (uie == UIE_OK || uie == UIE_QUIT) goodbye(99);

	m_sys->selectTerminal(m_oldTerm);
	m_menuMutex = 0;
}

/* For dumping the CPU state */

static void put_flags(char *s, byte F)
{
	strcat(s, F & 0x80 ? "S" : "s");
	strcat(s, F & 0x40 ? "Z" : "z");
	strcat(s, F & 0x10 ? "H" : "h");
	strcat(s, F & 0x04 ? "P" : "p");
	strcat(s, F & 0x02 ? "N" : "n");
	strcat(s, F & 0x01 ? "C" : "c");
}



UiEvent PcwMenuTerm::cpuState(Z80 *R)
{
	static char s[6][50];
	int n;
	word t[3];
	UiMenu uim(this);
	SDL_Rect &rc = getSysVideo()->getBounds();

	t[0] = R->fetchW(R->sp  );
	t[1] = R->fetchW(R->sp+2);
	t[2] = R->fetchW(R->sp+4);

	sprintf(s[0],"A=%2.2x BC=%4.4x DE=%4.4x HL=%4.4x  ",
			R->a, R->getBC(), R->getDE(), R->getHL());
	put_flags(s[0], R->f);

        sprintf(s[1],"A'%2.2x BC'%4.4x DE'%4.4x HL'%4.4x  ",
                        R->a1, R->getBC1(), R->getDE1(), R->getHL1());
	put_flags(s[1], R->f1);

	sprintf(s[2],"IX=%4.4x IY=%4.4x PC=%4.4x SP=%4.4x I=%2.2x IFF2=%2.2x",
		  R->ix, R->iy, R->pc, R->sp, R->i, R->iff2);

	sprintf(s[3], "Stack: %4.4x %4.4x %4.4x %4.4x %4.4x %4.4x", 
		t[0], t[1], t[2], t[3], t[4], t[5]);
	sprintf(s[4]," %4.4x: %2.2x %2.2x %2.2x %2.2x <> %2.2x %2.2x "
                  "%2.2x %2.2x %2.2x %2.2x %2.2x", R->pc - 4,
                  R->fetchB(R->pc-4), 
                  R->fetchB(R->pc-3), R->fetchB(R->pc-2), R->fetchB(R->pc-1), 
                  R->fetchB(R->pc), R->fetchB(R->pc+1), R->fetchB(R->pc+2),
		  R->fetchB(R->pc+3), R->fetchB(R->pc+4), R->fetchB(R->pc+5), 
                  R->fetchB(R->pc+6) );

	sprintf(s[5], "Memory banks: %02x %02x %02x %02x",
			m_sys->m_mem->getPage(0),
			m_sys->m_mem->getPage(1),
			m_sys->m_mem->getPage(2),
			m_sys->m_mem->getPage(3));

	uim.add(new UiLabel("                  Z80 CPU State                  ", this))
	   .add(new UiSeparator(this));
	
        for (n = 0; n < 6; n++) 
	{
		uim.add(new UiLabel(s[n], this));
	}
	uim.add(new UiSeparator(this))
	   .add(new UiCommand  (SDLK_ESCAPE,  "  OK  ", this));
	centreContainer(uim);
	if (uim.eventLoop() == UIE_QUIT) return UIE_QUIT;
	return UIE_OK;
}



SDL_Surface *PcwMenuTerm::getSurface()
{
	return m_term->getSurface();
}


SDL_Surface *PcwMenuTerm::saveSurface(SDL_Surface *s, SDL_Rect &rc)
{
        SDL_Rect rcto;
        SDL_Surface *s2;

        rcto.x = rcto.y = 0;
        rcto.w = rc.w;
        rcto.h = rc.h;

        s2 = SDL_AllocSurface(SDL_SWSURFACE, rcto.w, rcto.h,
				s->format->BitsPerPixel, 0, 0, 0, 0);

        if (!s2) return NULL;
	SDL_SetColours(s2, m_sysVideo->m_colours, 0, 256);
	SDL_BlitSurface(s, &rc, s2, &rcto);
	return s2;
}

SDL_Surface *PcwMenuTerm::saveSurface(SDL_Rect &rc)
{
	return saveSurface(getSurface(), rc);
}


void PcwMenuTerm::restoreSurface(SDL_Surface *s, SDL_Rect &rc)
{
	SDL_Surface *scr = getSurface();

        SDL_BlitSurface(s, NULL, scr, &rc);
        SDL_FreeSurface(s);
        SDL_UpdateRects(scr, 1, &rc);
}





void PcwMenuTerm::centreContainer(UiContainer &c)
{
        unsigned int sx, sy;
	SDL_Rect &rc = getSysVideo()->getBounds();

	c.pack();

        sx = ((rc.w - c.getWidth ()) / 2);
	sy = ((rc.h - c.getHeight()) / 2);

	c.setX(alignX(sx));
	c.setY(alignY(sy));
}



void PcwMenuTerm::popup(const string message, int t, int centred)
{
	SDL_Rect &rc = getSysVideo()->getBounds();
	Uint16 sx, sy, bx, by;

        strncpy(m_popup, message.c_str(), 79); m_popup[79] = 0;

        if (t)
        {
                time(&m_popupTimeout);
                m_popupTimeout += t;
        }
        else    m_popupTimeout = 0;

	measureString(m_popup, &sx, &sy);
	measureString("A",     &bx, &by);

        m_popupRect.w = bx + sx + bx;
	m_popupRect.x = (rc.w - m_popupRect.w) / 2;
        m_popupRect.h = by + sy + by;

        m_popupRect.y = rc.h - m_popupRect.h;
	if (centred) m_popupRect.y /= 2;

	m_popupRect.y = alignY(m_popupRect.y);
	m_popupRect.x = alignY(m_popupRect.x);

	m_popupBacking = saveSurface(m_sys->m_video->getSurface(), m_popupRect);
	m_popupContext = createPopup();
	drawPopup();
}



void PcwMenuTerm::popupRedraw(const string message, int centred)
{
	char cmsg[80];
	
	memset(cmsg, ' ', 79);
	cmsg[79] = 0;
	if (message.length() > 79) memcpy(cmsg, message.c_str(), 79); 
	else
	{
		int x = (79 - message.length()) / 2;
		memcpy(cmsg + x, message.c_str(), message.length());
	}

	if (!m_popup[0])
	{
		popup(cmsg, 0, centred);
		return;	
	}
	strcpy(m_popup, cmsg);
	setPopupText(m_popupContext);
	drawPopup();
}


int PcwMenuTerm::drawPopup(void)
{
	SDL_Rect rc;

	if (!m_popup[0]) return 0;

        memcpy(&rc, &m_popupRect, sizeof(SDL_Rect));
        rc.x = rc.y = 0;

        SDL_BlitSurface(m_popupContext, &rc, m_sys->m_video->getSurface(), &m_popupRect);
        SDL_UpdateRects(m_sys->m_video->getSurface(), 1, &m_popupRect);

        if (m_popupTimeout)
        {
                time_t tm;
                time (&tm);
                if (tm == m_popupTimeout) popupOff();
        	return 2;
	}
        return 1;
}

void PcwMenuTerm::popupOff(void)
{
        SDL_Rect rc;
	if (!m_popup[0]) return;

        m_popupTimeout = 0;
        m_popup[0] = 0;

	memcpy(&rc, &m_popupRect, sizeof(SDL_Rect));
	rc.x = rc.y = 0;

        SDL_BlitSurface(m_popupBacking, &rc, m_sys->m_video->getSurface(), &m_popupRect);
	SDL_UpdateRects(m_sys->m_video->getSurface(), 1, &m_popupRect);
	SDL_FreeSurface(m_popupBacking);
	m_popupBacking = NULL;
}


void PcwMenuTerm::onTimer50(void)
{
	m_term->onTimer50();
}

UiEvent PcwMenuTerm::menuEvent(SDL_Event &e)
{
        int r = m_term->onEvent(e);

        switch(r)
        {
                case -2: return UIE_QUIT;
                case 0:  return UIE_CONTINUE;
                case 1:  return UIE_OK;
        }
        return UIE_CONTINUE;
}


char PcwMenuTerm::getKey(void)
{
        if (!m_term->kbhit()) return 0;
        return m_term->read();
}


void PcwMenuTerm::setManualUpdate(bool b)
{
	m_term->setManualUpdate(b);	
}

bool PcwMenuTerm::getManualUpdate(void)
{
	return m_term->getManualUpdate();
}

void PcwMenuTerm::updateRect(SDL_Rect &rc)
{
	SDL_UpdateRects(m_term->getSurface(), 1, &rc);
}


UiEvent PcwMenuTerm::yesno(const string prompt)
{
	UiEvent uie;
	UiMenu mc(this);

	if (m_menuMutex) return UIE_CONTINUE;

	m_menuMutex = true;
        m_oldTerm = m_sys->m_activeTerminal;
	SDL_BlitSurface(m_sysVideo->getSurface(), NULL,
			m_framebuf->getSurface(), NULL);
        m_sys->selectTerminal(this);
	SDL_ShowCursor(1);

	mc.add(new UiLabel(prompt, this))
	  .add(new UiSeparator(this))
	  .add(new UiCommand(SDLK_y, "  Yes  ", this))
	  .add(new UiCommand(SDLK_n, "  No  ",  this));
	centreContainer(mc);
	uie = mc.eventLoop();

	m_sys->selectTerminal(m_oldTerm);
	m_menuMutex = 0;

	if (uie == UIE_QUIT) return uie;
	if (mc.getKey(mc.getSelected()) == SDLK_y) return UIE_OK;
	return UIE_CANCEL;
}




