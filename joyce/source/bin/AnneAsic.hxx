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


class AnneAsic : public PcwAsic
{
public:
	AnneAsic(AnneSystem *s);
	virtual ~AnneAsic();

	void outF8(byte b);
	byte inF8();	
	byte inF7();	
	void reset(void);

	void onAsicTimer(void);		 // Called by the CPU at 900Hz
	virtual void fdcIsr(int status); // Called by the FDC when its 
					 // interrupt line does things
	virtual void keybIsr(int status); // Keyboard interrupt 
	virtual void sioIsr(int status);  // Serial port interrupt
	virtual void daisyIsr(int status); // dummy

        virtual bool hasSettings(SDLKey *key, string &caption);
        virtual UiEvent settings(UiDrawer *d);

protected:
	byte m_inf8;
	byte m_inf7;
	byte m_fdcIntMode;
	bool m_timerInt;
	int m_timerCount, m_retraceCount;
	AnneSystem *m_sys;

	virtual bool parseNode(xmlDoc *doc, xmlNs *ns, xmlNode *ptr);
	virtual bool storeNode(xmlDoc *doc, xmlNs *ns, xmlNode *ptr);
};
