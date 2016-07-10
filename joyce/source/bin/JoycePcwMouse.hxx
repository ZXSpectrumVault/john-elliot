/************************************************************************

    JOYCE v2.2.8 - Amstrad PCW emulator

    Copyright (C) 1996, 2001-2, 2016  John Elliott <seasip.webmaster@gmail.com>

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

enum MouseType { MT_NONE, MT_AMX, MT_KEMPSTON, MT_ESPEN };


class JoycePcwMouse: public PcwDevice, public PcwInput
{
public:
	JoycePcwMouse(JoyceSystem *s);
	virtual ~JoycePcwMouse();
//
// Reset device
//
	virtual void reset(void);
//
// Implement PCW mouse
//
	virtual void poll(Z80 *R);
	virtual int handleEvent(SDL_Event &e);
	virtual byte in(word port);
	virtual void out(word address, byte value);

//
// Settings functions.
// See if this device has user-settable options. If it does, populates
// "key" and "caption" and returns true; else, returns false.
//
	virtual bool hasSettings(SDLKey *key, string &caption);
//
// Display settings screen
// 
	virtual UiEvent settings(UiDrawer *d);
//
// Implement the JOYCE "Assign key" calls
//
	virtual void edfe(Z80 *R);
//
// nb: Ability to dump state is not yet provided, but would go here.
//

protected:
        virtual bool parseNode(xmlDocPtr doc, xmlNsPtr ns, xmlNodePtr cur);
	virtual bool storeNode(xmlDocPtr doc, xmlNsPtr ns, xmlNodePtr cur);

	void updateSettings(void);
	void trackMouse(Uint16 x, Uint16 y);
	void checkPatch(void);

	// Get mouse position in PCW screen coordinates
	Uint8 getMousePos(int *, int *);
	int dy(int l);
	int dx(int l);
	int buttons(void);

	MouseType m_mouseType;
	bool	  m_autoPatch;
	int	  m_patchApplied;
	bool	  m_tracking;
	bool	  m_cursorShown;

	int	  m_ox, m_oy;
	unsigned  m_lpc;
	unsigned short m_lcount_last;
// Screen scaling things
//
// XXX These should be autodetected from the active terminal.
//
	int	m_xmin, m_xmax, m_ymin, m_ymax, m_xscale, m_yscale;
//
// Auto-patch pointers
//
        word m_mpmx, m_spmx, m_spmy, m_md3mx;
	word m_md3my, m_md3mf1, m_md3mf2;
};

