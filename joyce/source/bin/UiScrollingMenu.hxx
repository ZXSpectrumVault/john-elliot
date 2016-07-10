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

class UiScrollingMenu : public UiMenu
{
public:
	UiScrollingMenu(int itemH, unsigned visCount, UiDrawer *d);
	virtual ~UiScrollingMenu();

	virtual void pack(void);
	virtual void draw(int selected);
	
	inline int getSelected(void)   { return m_selected; }
	void setSelected(int s);

	virtual void drawElement(int selected, int e);

	virtual UiEvent eventLoop(void);
	virtual UiEvent onKey(SDLKey k);
	virtual UiEvent onMouse(Uint16 x, Uint16 y);

protected:
	void showSelected(void);

	SDL_Rect m_rcInner;
	int m_itemH;
	unsigned m_visCount;
	int m_top;
	bool m_onscreen;
};






