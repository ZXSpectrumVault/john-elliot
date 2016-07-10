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

#include "Anne.hxx"
#include "UiContainer.hxx"
#include "UiLabel.hxx"
#include "UiCommand.hxx"
#include "UiMenu.hxx"

// XXX Use a proper proportional font
#define UI_CHAR_W 8
#define UI_CHAR_H 16

AnneMenuTermPcw::AnneMenuTermPcw(AnneSystem *s) : AnneMenuTerm(s)
{
	XLOG("AnneMenuTermPcw::AnneMenuTermPcw()");
}


AnneMenuTermPcw::~AnneMenuTermPcw()
{

}

void AnneMenuTermPcw::menuDraw(void)
{
	char menustr[101];
	double speed = m_sys->m_cpu->getSpeed(false);
	SDL_Rect &rc = getSysVideo()->getBounds();

	m_term->fillRect(0, 0, rc.w, UI_CHAR_H,
			 m_term->getBackground());
        sprintf(menustr,"ANNE v%d.%d.%d  Copyright 1996,2001-14, John Elliott  "
                        "Compiled with SDL version %d.%d.%d  Speed=%.2f%%",
                        BCDVERS >> 8,
                        (BCDVERS >> 4) & 0x0F,
                        BCDVERS & 0x0F,
                        SDL_MAJOR_VERSION,
                        SDL_MINOR_VERSION,
                        SDL_PATCHLEVEL,
			speed );
	m_term->setReverse(false);
	m_term->home();
	(*m_term) << menustr;
}

UiEvent AnneMenuTermPcw::aboutBox(void)
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


/* In xanne, the keyboard map is loaded from a BMP file */

void AnneMenuTermPcw::drawKeyboard(void)	/* Draw the keyboard map */
{
        SDL_Surface *image;
        SDL_Rect dest;
        string bmpFile;

        bmpFile = findPcwFile(FT_NORMAL, "pcwkey.bmp", "rb");
        image = SDL_LoadBMP(bmpFile.c_str());
        if ( image == NULL ) return;

        /* Blit onto the screen surface
         *         */
        dest.x = 80;
        dest.y = 60;
        dest.w = image->w;
        dest.h = image->h;
        SDL_BlitSurface(image, NULL, m_sysVideo->getSurface(), &dest);

        SDL_UpdateRect(m_sysVideo->getSurface(), dest.x, dest.y, image->w, image->h);
        SDL_FreeSurface(image);
}





void AnneMenuTermPcw::drawMenuBorder(SDL_Rect &rcOuter, SDL_Rect &rcInner)
{
        SDL_Rect rc;

	m_term->setForeground(m_clrFG);
        m_term->setBackground(m_clrBG);

        memcpy(&rc,       &rcOuter, sizeof(SDL_Rect));
	memcpy(&rcInner,  &rcOuter, sizeof(SDL_Rect));

        m_term->fillRect(rc, m_term->getBackground());
	m_term->rect    (rc, m_term->getForeground());

	rcInner.x += UI_CHAR_W;
	rcInner.y += UI_CHAR_H;
	rcInner.w -= 2 * UI_CHAR_W;
	rcInner.h -= 2 * UI_CHAR_H;
}



void AnneMenuTermPcw::setPopupText(SDL_Surface *s)
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


SDL_Surface *AnneMenuTermPcw::createPopup(void)
{
	SDL_Surface *s = saveSurface(m_sys->m_video->getSurface(), m_popupRect);
	if (!s) return NULL;
	setPopupText(s);
	return s;
}


// UiDrawer

void AnneMenuTermPcw::measureString(const string &s, Uint16 *w, Uint16 *h)
{
	*w = UI_CHAR_W * s.size();
	*h = UI_CHAR_H;
}

void AnneMenuTermPcw::measureMenuBorder(SDL_Rect &rcInner, SDL_Rect &rcOuter)
{
	rcOuter = rcInner;
	rcOuter.x -= UI_CHAR_W;
	rcOuter.y -= UI_CHAR_W;
	rcOuter.w += 2 * UI_CHAR_W;
	rcOuter.h += 2 * UI_CHAR_H;
}

void AnneMenuTermPcw::measureMenuBar(SDL_Rect &rc)
{
	SDL_Rect &rc2 = getSysVideo()->getBounds();

        rc.h = UI_CHAR_H;
	rc.w = rc2.w;
	rc.y = UI_CHAR_H;
	rc.x = 0;
}



void AnneMenuTermPcw::drawRect   (int selected, SDL_Rect &rc, SDL_Color *c)
{
	if (c)
	{
		Uint32 oldfg = m_term->getForeground();
		m_term->setForeground(*c);
		m_term->fillRect(rc, m_term->getForeground());
		m_term->setForeground(oldfg);	
	}
	else if (selected) m_term->fillRect(rc, m_term->getForeground());
	else               m_term->fillRect(rc, m_term->getBackground());
}

void AnneMenuTermPcw::drawMenuBar(int selected, SDL_Rect &rc)
{
	drawRect(selected, rc);
}

void AnneMenuTermPcw::drawSeparator(int selected, SDL_Rect &rc)
{
	rc.y += 6;
	rc.h  = 2;
	rc.w += 2 * UI_CHAR_W;
	rc.x -= UI_CHAR_W;
	drawRect(!selected, rc);
}


void AnneMenuTermPcw::drawString (int selected, Uint16 x, Uint16 y, const string s)
{
        x /= UI_CHAR_W;
        y /= UI_CHAR_H;

        if (selected) m_term->setReverse(true);
        else          m_term->setReverse(false);

        m_term->moveCursor(x, y);
        (*m_term) << s.c_str();
}

void AnneMenuTermPcw::drawCursor    (Uint16 x, Uint16 y, char ch)
{
        x /= UI_CHAR_W;
        y /= UI_CHAR_H;

        m_term->setReverse(false);

        m_term->moveCursor(x, y);
        (*m_term) << (unsigned char)ch;
}



void AnneMenuTermPcw::drawPicture(int selected, Uint16 x, Uint16 y, SDL_Surface *s)
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


void AnneMenuTermPcw::drawGlyph  (int selected, Uint16 x, Uint16 y, UiGlyph g) 
{
	unsigned char ch;

        x /= UI_CHAR_W;
        y /= UI_CHAR_H;

        if (selected) m_term->setReverse(true);
        else          m_term->setReverse(false);

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


int  AnneMenuTermPcw::alignX(int x)
{
        return x - (x % UI_CHAR_W);
}


int  AnneMenuTermPcw::alignY(int y)
{
        return y - (y % UI_CHAR_H);
}

