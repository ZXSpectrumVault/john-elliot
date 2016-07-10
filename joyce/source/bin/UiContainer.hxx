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

class UiContainer : public UiControl
{
public:
	UiContainer(UiDrawer *d);
	virtual ~UiContainer();

	inline UiContainer &add(UiControl *c) { m_entries.push_back(c); return *this; } 
	inline UiControl &operator[] (int c) { return *m_entries[c]; }
        inline unsigned size()  { return m_entries.size(); }
	inline SDLKey getKey(unsigned c) 
	{ 
	return (c < m_entries.size()) ? m_entries[c]->getKey() : SDLK_UNKNOWN;
	}
	virtual void pack(void) = 0;
	void clear(void);
        virtual UiEvent onMouse(Uint16 x, Uint16 y);

protected:
	std::vector<UiControl *> m_entries;
};






