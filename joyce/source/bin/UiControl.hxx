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

class UiContainer;

//
// NOTA BENE: Coordinates are in pixels.
//
class UiDrawer
{
public:
	virtual void measureString(const std::string &s, Uint16 *w, Uint16 *h) = 0;
	virtual void measureMenuBorder(SDL_Rect &rcInner, SDL_Rect &rcOuter) = 0;
	virtual void measureMenuBar(SDL_Rect &rc) = 0;

	virtual void drawRect   (int selected, SDL_Rect &rc, SDL_Colour *c = NULL) = 0;
	virtual void drawString (int selected, Uint16 x, Uint16 y, const std::string s) = 0;
	virtual void drawPicture(int selected, Uint16 x, Uint16 y, SDL_Surface *s) = 0;
	virtual void drawGlyph  (int selected, Uint16 x, Uint16 y, UiGlyph g) = 0;
	virtual void drawMenuBorder(SDL_Rect &rc, SDL_Rect &rcInner) = 0;
        virtual void drawMenuBar   (int selected, SDL_Rect &rc) = 0;
        virtual void drawSeparator (int selected, SDL_Rect &rc) = 0;
	virtual void drawCursor    (Uint16 x, Uint16 y, char ch) = 0;

	virtual bool getManualUpdate(void) = 0;
	virtual void setManualUpdate(bool b = true) = 0;
	virtual void updateRect(SDL_Rect &rc) = 0;

        virtual SDL_Surface *saveSurface(SDL_Rect &rc) = 0;
        virtual void restoreSurface(SDL_Surface *s, SDL_Rect &rc) = 0;
	virtual void centreContainer(UiContainer &c) = 0;

        virtual UiEvent menuEvent(SDL_Event &e) = 0; 
        virtual char getKey(void) = 0;
	virtual void onTimer50(void) = 0;
};


class UiControl
{
private:
	SDL_Rect m_rect;
	SDL_Rect m_bounds;
	SDLKey   m_id;
protected:
	UiDrawer *m_drawer;
public:
	UiControl(UiDrawer *d);
	virtual ~UiControl();

	virtual void draw(int selected);

        UiEvent eventLoop(void);
	virtual UiEvent onEvent(SDL_Event &e);
	virtual UiEvent onKey(SDLKey k);
	virtual UiEvent onMouse(Uint16 x, Uint16 y);
	virtual UiEvent onQuit(void);
	virtual UiEvent onSelected(void);
	virtual bool canFocus(void);
	bool hitTest(Uint16 x, Uint16 y);
	virtual bool wantAllKeys(void);

	virtual Uint16 getHeight() = 0;
	virtual Uint16 getWidth()  = 0;
	inline Sint16 getX()		{ return m_rect.x; }
	inline Sint16 getY()		{ return m_rect.y; }
	inline void   setX(int x)	{ m_rect.x = x; }
	inline void   setY(int y)	{ m_rect.y = y; }
	inline SDLKey getKey(void)	{ return m_id; }
	inline void   setKey(SDLKey k)	{ m_id = k;    }
	SDL_Rect getRect();

	// The "rect" is the true area of the control. The "bounds" cover
	// space that the parent has allocated to the control.

	inline SDL_Rect getBounds()     { return m_bounds; }
	inline void setBounds(SDL_Rect &rc) { m_bounds = rc; }
};




