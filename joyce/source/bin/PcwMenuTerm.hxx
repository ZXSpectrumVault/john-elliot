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

#include "UiControl.hxx"

class UiMenu3;
class UiMenuBar;

class PcwMenuTerm : public PcwSdlTerm, public UiDrawer
{
public:
	PcwMenuTerm(PcwSystem *s);
	virtual ~PcwMenuTerm();

protected:
	virtual void menuDraw(void) = 0;
        virtual void drawKeyboard(void) = 0;
	virtual UiEvent aboutBox(void) = 0;
	virtual void onGainFocus();

	UiEvent cpuState(Z80 *R);
	UiEvent confirm(int x, int y, const std::string prompt);
	UiMenuBar *m_menuBar;

public:
	UiEvent yesno(const std::string prompt);
	virtual void mainMenu(Z80 *R) = 0;
	void exitMenu(void);

	UiEvent report(const std::string s, const std::string prompt);
// Overrides
        void setSysVideo(PcwSdlContext *s);
// Callbacks
        virtual SDL_Surface *getSurface();
        virtual SDL_Surface *saveSurface(SDL_Surface *s, SDL_Rect &rc);
        virtual SDL_Surface *saveSurface(SDL_Rect &rc);
        virtual void restoreSurface(SDL_Surface *s, SDL_Rect &rc);

        virtual void onTimer50(void);
        virtual UiEvent menuEvent(SDL_Event &e);
        virtual char getKey(void);

	virtual void centreContainer(UiContainer &c);

        virtual void setManualUpdate(bool b = true);
        virtual bool getManualUpdate(void);
	virtual void updateRect(SDL_Rect &rc);

        void popup(const std::string message, int time, int centred);
        void popupRedraw(const std::string message, int centred);
        void popupOff(void);
	virtual SDL_Surface *createPopup(void) = 0;
        virtual void setPopupText(SDL_Surface *s) = 0;

	// Draw a popup message. Returns 0: No popup. 1: Drawn. 2: Removed,
	// update screen behind it.
	int drawPopup(void);

	// Align text output on a character cell grid. 
	// XXX We should not need to do this. Fix SdlTerm.
	virtual int alignX(int x) = 0;
	virtual int alignY(int y) = 0;
// Popup support
protected:
        char   m_popup[80];
        time_t m_popupTimeout;
        SDL_Surface *m_popupBacking;
	SDL_Surface *m_popupContext;
        SDL_Rect m_popupRect;

private:
	bool	m_menuMutex;
};

