/************************************************************************

    JOYCE v2.2.7 - Amstrad PCW emulator

    Copyright (C) 1996, 2001-9, 2014  John Elliott <seasip.webmaster@gmail.com>

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
#include "UiCommand.hxx"
#include "UiMenu.hxx"

/* Dimensions of a character in the fixed font */
#define UI_CHAR_W 8
#define UI_CHAR_H 16

JoyceMenuTermPcw::JoyceMenuTermPcw(JoyceSystem *s) : JoyceMenuTerm(s)
{
	XLOG("JoyceMenuTermPcw::JoyceMenuTermPcw()");
}


JoyceMenuTermPcw::~JoyceMenuTermPcw()
{

}

void JoyceMenuTermPcw::menuDraw(void)
{
	char menustr[101];
	double speed = m_sys->m_cpu->getSpeed(false);
	SDL_Rect &rc = getSysVideo()->getBounds();

	m_term->fillRect(0, 0, rc.w, UI_CHAR_H,
			 m_term->getForeground());
        sprintf(menustr,"JOYCE v%d.%d.%d  Copyright 1996,2001-14, John Elliott  "
                        "Compiled with SDL version %d.%d.%d  Speed=%.2f%%",
                        BCDVERS >> 8,
                        (BCDVERS >> 4) & 0x0F,
                        BCDVERS & 0x0F,
                        SDL_MAJOR_VERSION,
                        SDL_MINOR_VERSION,
                        SDL_PATCHLEVEL,
			speed );
	m_term->setReverse();
	m_term->home();
	(*m_term) << menustr;
	m_term->setReverse(false);
}

UiEvent JoyceMenuTermPcw::aboutBox(void)
{
	UiMenu uim(this);

        uim.add(new UiLabel("This program is free software; you can redistribute it and/or modify it under the", this))
	   .add(new UiLabel("terms of the GNU General Public License as published by the Free Software ", this))
	   .add(new UiLabel("Foundation; either version 2 of the License, or (at your option) any later version.", this))
	   .add(new UiLabel("", this))
	   .add(new UiLabel("This program is distributed in the hope that it will be useful, but WITHOUT ANY", this))
	   .add(new UiLabel("WARRANTY; without even the implied warranty of MERCHANTABILITY or", this))
	   .add(new UiLabel("FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License ", this))
	   .add(new UiLabel("for more details.", this))
	   .add(new UiLabel("", this))
	   .add(new UiLabel("You should have received a copy of the GNU General Public License along with ", this))
	   .add(new UiLabel("this program; if not, write to the Free Software Foundation, Inc., 59 Temple Place, ", this))
	   .add(new UiLabel("Suite 330, Boston, MA  02111-1307  USA" , this))
	   .add(new UiLabel("", this))
           .add(new UiCommand(SDLK_o, "  OK  ", this));

	centreContainer(uim);
	return uim.eventLoop();
}


// XBM files from xpaint(1) define the image as an array of "unsigned char", 
// which compiles without warnings. XBM files from xv(1) define it as 
// "char", which causes lots of warnings. 

#include "pcwkey.xbm"	// X-Window bitmap, a graphic format designed to
			// be used in C source.

void JoyceMenuTermPcw::drawKeyboard(void)	/* Draw the keyboard map */
{
	int y,x,b,z=0;
	byte ch;
	Uint32 fg, bg;

	if (!m_term->lock()) return;
	fg = m_term->getForeground();
        bg = m_term->getBackground();

	for (y = 0; y < pcwkey_height; y++)
		for (x = 0; x < pcwkey_width; x+=8)
		{
			ch = pcwkey_bits[z++];
			for (b = 0; b < 8; b++)
			{
				m_term->dot(x+b, y+32, (ch & 1) ? fg : bg);
				ch = (ch >> 1);
			}
		}
	m_term->unlock();
	SDL_UpdateRect(m_term->getSurface(),
			0, 32, pcwkey_width, pcwkey_height);

}





void JoyceMenuTermPcw::drawMenuBorder(SDL_Rect &rcOuter, SDL_Rect &rcInner)
{
        SDL_Rect rc;

        m_term->setForeground(m_clrFG);
        m_term->setBackground(m_clrBG);

        memcpy(&rc,       &rcOuter, sizeof(SDL_Rect));
	memcpy(&rcInner,  &rcOuter, sizeof(SDL_Rect));

        m_term->fillRect(rc, m_term->getForeground());

        rc.x += 2; rc.y += 5; rc.w -= 4; rc.h -= 10;
        m_term->rect(rc,  m_term->getBackground());
        rc.x += 2; rc.y += 4; rc.w -= 4; rc.h -= 8;
        m_term->rect(rc,  m_term->getBackground());

	rcInner.x += UI_CHAR_W;
	rcInner.y += UI_CHAR_H;
	rcInner.w -= 2 * UI_CHAR_W;
	rcInner.h -= 2 * UI_CHAR_H;
}



void JoyceMenuTermPcw::setPopupText(SDL_Surface *s)
{
	SDL_Rect rc;
	SdlTerm t(s);
        
	t.setForeground(m_clrBG);
        t.setBackground(m_clrFG);

	memcpy(&rc, &m_popupRect, sizeof(SDL_Rect));
        rc.x = rc.y = 0;
	t.fillRect(rc, t.getBackground());

        rc.x += 2; rc.y += 5; rc.w -= 4; rc.h -= 10;
        t.rect(rc, t.getForeground());
        rc.x += 2; rc.y += 4; rc.w -= 4; rc.h -= 8;
        t.rect(rc, t.getForeground());
	t.setForeground(m_clrBG);
	t.setBackground(m_clrFG);

	t.moveCursor(1, 1);
	t << m_popup;
}


SDL_Surface *JoyceMenuTermPcw::createPopup(void)
{
	SDL_Surface *s = saveSurface(m_sys->m_video->getSurface(), m_popupRect);
	if (!s) return NULL;
	setPopupText(s);
	return s;
}


// UiDrawer

void JoyceMenuTermPcw::measureString(const string &s, Uint16 *w, Uint16 *h)
{
	*w = UI_CHAR_W * s.size();
	*h = UI_CHAR_H;
}

void JoyceMenuTermPcw::measureMenuBorder(SDL_Rect &rcInner, SDL_Rect &rcOuter)
{
	rcOuter = rcInner;
	rcOuter.x -= UI_CHAR_W;
	rcOuter.y -= UI_CHAR_W;
	rcOuter.w += 2 * UI_CHAR_W;
	rcOuter.h += 2 * UI_CHAR_H;
}

void JoyceMenuTermPcw::measureMenuBar(SDL_Rect &rc)
{
	SDL_Rect &rc2 = getSysVideo()->getBounds();

        rc.h = UI_CHAR_H;
	rc.w = rc2.w;
	rc.y = UI_CHAR_H;
	rc.x = 0;
}



void JoyceMenuTermPcw::drawRect   (int selected, SDL_Rect &rc, SDL_Color *c)
{
	if (c)
	{
		Uint32 oldfg = m_term->getForeground();
		m_term->setForeground(*c);
		m_term->fillRect(rc, m_term->getForeground());
		m_term->setForeground(oldfg);	
	}
	else if (selected) m_term->fillRect(rc, m_term->getBackground());
	else               m_term->fillRect(rc, m_term->getForeground());
}

void JoyceMenuTermPcw::drawMenuBar(int selected, SDL_Rect &rc)
{
	drawRect(selected, rc);
}

void JoyceMenuTermPcw::drawSeparator(int selected, SDL_Rect &rc)
{
	rc.x -= 4;
	rc.w += 8;
	rc.y += 6;
	rc.h  = 4;
	drawRect(!selected, rc);
}


void JoyceMenuTermPcw::drawString (int selected, Uint16 x, Uint16 y, const string s)
{
        x /= UI_CHAR_W;
        y /= UI_CHAR_H;

        if (selected) m_term->setReverse(false);
        else          m_term->setReverse(true);

        m_term->moveCursor(x, y);
        (*m_term) << s.c_str();
}

void JoyceMenuTermPcw::drawCursor    (Uint16 x, Uint16 y, char ch)
{
        x /= UI_CHAR_W;
        y /= UI_CHAR_H;

        m_term->setReverse(true);

        m_term->moveCursor(x, y);
        (*m_term) << (unsigned char)ch;
}



void JoyceMenuTermPcw::drawPicture(int selected, Uint16 x, Uint16 y, SDL_Surface *s)
{
	SDL_Rect rcFrom, rcTo;

	rcFrom.x = rcFrom.y = 0;
	rcFrom.w = s->w;
	rcFrom.h = s->h;
	rcTo.x = x;
	rcTo.y = y;	
	rcTo.w = s->w;
	rcTo.h = s->h;

        SDL_BlitSurface(s, &rcFrom,
			m_sysVideo->getSurface(), &rcTo);
}


void JoyceMenuTermPcw::drawGlyph  (int selected, Uint16 x, Uint16 y, UiGlyph g) 
{
	unsigned char ch;

        x /= UI_CHAR_W;
        y /= UI_CHAR_H;

        if (selected) m_term->setReverse(false);
        else          m_term->setReverse(true);

        m_term->moveCursor(x, y);
	switch(g)
	{
		case UIG_NONE:		ch = ' '; break;
                case UIG_CHECKED:	ch = 188; break;
                case UIG_UNCHECKED:	ch = 187; break;
		case UIG_COMMAND:	ch = 252; break;
		case UIG_SUBMENU:	ch = 128; break;
	}
        (*m_term) << ch;
}


int  JoyceMenuTermPcw::alignX(int x)
{
	return x - (x % UI_CHAR_W);
}


int  JoyceMenuTermPcw::alignY(int y)
{
	return y - (y % UI_CHAR_H);
}


