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


class UiMenu3;
class UiMenuBar;

class AnneMenuTerm : public PcwMenuTerm
{
public:
	AnneMenuTerm(AnneSystem *s);
	virtual ~AnneMenuTerm();

protected:
	virtual void menuDraw(void) = 0; 
        virtual void drawKeyboard(void) = 0;
	virtual UiEvent aboutBox(void) = 0;
///	virtual void onGainFocus();

	UiEvent actionMenu(Z80 *R, int x, int y);
	UiEvent debugMenu(Z80 *R, int x, int y);
	UiEvent menuF1(int x, int y);
//	UiEvent confirm(int x, int y, const string prompt);


public:
	void mainMenu(Z80 *R);
	void exitMenu(void);

//        virtual UiEvent menuEvent(SDL_Event &e);
private:
	bool	m_menuMutex;
};

