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


class PcwVgaTerm : public PcwSdlTerm
{
public:
	PcwVgaTerm(PcwSystem *s);
	virtual ~PcwVgaTerm();

	//virtual int settings(void);
	inline SdlTerm &getMenuTerm()  { return (*m_menuTerm);  }
	inline SdlTerm &getVgaTerm()   { return (*m_term);      }
//
// Functions used for menus
//
//	int uiMenuBar(int y, int count, UI_MENUENTRY *entries);
//	int uiMenu   (int x, int y, int count, UI_MENUENTRY *entries, int selected);
	inline SDL_Colour& getMenuBg() { return m_menuBg; }
	inline SDL_Colour& getMenuFg() { return m_menuFg; }

        virtual void onGainFocus();     // Terminal is gaining focus

protected:
//
// Functions connected with state
//
        virtual bool parseNode(xmlDocPtr doc, xmlNsPtr ns, xmlNodePtr cur);
	virtual bool storeNode(xmlDocPtr doc, xmlNsPtr ns, xmlNodePtr cur);
//
// Settings that are persisted
//
	SDL_Colour m_menuFg,   m_menuBg;
	SDL_Colour m_shadowFg, m_shadowBg;
//
// State
//
// Note that we have three separate terminal contexts all on the same screen.
// m_term is used for CP/M input and output; m_menuTerm is used for menu
// operations; and m_debugTerm is used for debug output. They all work on
// the same screen, but each has its own cursor position, colours etc.
//
	SdlTerm    *m_menuTerm;  // Used for menu operations
//
// Variables for graphics
//
	SDL_Rect m_clip;

};


void flipXbm(int w, int h, Uint8* bits);



