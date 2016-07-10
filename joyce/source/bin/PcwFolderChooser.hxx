/************************************************************************

    JOYCE v2.1.8 - Amstrad PCW emulator

    Copyright (C) 1996, 2001, 2005  John Elliott <seasip.webmaster@gmail.com>

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

class Path;

class PcwFolderChooser: public UiContainer
{
protected:
	Path *m_path;
	PcwFileList   m_menuL;
	UiMenu          m_menuR;

	UiEvent selectR(void);
	UiEvent selectL(void);
	void changeDir(string s, bool relative = false);
	
	string m_choice;
	string m_dir;
	int m_pane;
	int m_w, m_h;
	int m_lwi, m_lwo;	// Left menu: inner & outer widths
public:
	PcwFolderChooser(const string okCaption, UiDrawer *d);
	virtual ~PcwFolderChooser();

	string getChoice();
	string constructPath(string dir, string file);

	void initDir(string name);
        virtual void pack(void);
        virtual void draw(int selected);

	virtual UiEvent onEvent(SDL_Event &e);
	UiEvent onKey(SDL_Event &e);
	virtual UiEvent eventLoop(void);

	virtual Uint16 getWidth(void);
	virtual Uint16 getHeight(void);
};


