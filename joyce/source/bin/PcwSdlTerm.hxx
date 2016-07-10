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

//
// Common code for those terminal types that use SdlTerm for their output.
//

class PcwSdlTerm : public PcwTerminal
{
public:
	PcwSdlTerm(const char *id, PcwSystem *s);
	virtual ~PcwSdlTerm();

protected:
	PcwSystem *m_sys;
	PcwTerminal *m_oldTerm;
	SdlTerm *m_term;
	SDL_Colour m_clrFG, m_clrBG;
	int m_tick;

public:
	virtual void beepOn(void);
        virtual void onDebugSwitch(bool debugMode);
	virtual void reset(void);
	virtual void onScreenTimer(void);
        virtual void setForeground(byte r, byte g, byte b);
        virtual void setBackground(byte r, byte g, byte b);
	virtual bool storeNode(xmlDocPtr, xmlNsPtr, xmlNodePtr);
	virtual bool parseNode(xmlDocPtr, xmlNsPtr, xmlNodePtr);
	virtual PcwTerminal &operator << (unsigned char c);
	inline SdlTerm *getTerminal(void) { return m_term; }

};

