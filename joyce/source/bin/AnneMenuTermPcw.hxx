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

class AnneMenuTermPcw : public AnneMenuTerm
{
public:
	AnneMenuTermPcw(AnneSystem *s);
	virtual ~AnneMenuTermPcw();

        virtual SDL_Surface *createPopup(void);
        virtual void setPopupText(SDL_Surface *s);
	virtual UiEvent aboutBox(void);
// UiDrawer

        virtual void measureString(const string &s, Uint16 *w, Uint16 *h);
	virtual void measureMenuBorder(SDL_Rect &rcInner, SDL_Rect &rcOuter);
	virtual void measureMenuBar(SDL_Rect &);

        virtual void drawRect   (int selected, SDL_Rect &rc, SDL_Color *c = NULL);
        virtual void drawString (int selected, Uint16 x, Uint16 y, const string s);
        virtual void drawPicture(int selected, Uint16 x, Uint16 y, SDL_Surface *s);
        virtual void drawGlyph  (int selected, Uint16 x, Uint16 y, UiGlyph g);
	virtual void drawMenuBorder(SDL_Rect &rc, SDL_Rect &rcInner);
	virtual void drawMenuBar   (int selected, SDL_Rect &rc);
	virtual void drawSeparator (int selected, SDL_Rect &rc);
        virtual void drawCursor    (Uint16 x, Uint16 y, char ch);

        virtual int  alignX(int x);
        virtual int  alignY(int y);

protected:
	virtual void menuDraw(void);
	virtual void drawKeyboard(void);
};

