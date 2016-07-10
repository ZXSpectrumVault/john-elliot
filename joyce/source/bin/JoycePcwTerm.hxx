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

class JoycePcwTerm : public PcwTerminal
{
public:
	JoycePcwTerm(PcwBeeper *b);
	virtual ~JoycePcwTerm();

	virtual void onGainFocus();
	virtual void onLoseFocus();
	virtual void onScreenTimer();

	virtual void out(word port, byte value);
        virtual void setForeground(byte r, byte g, byte b);
        virtual void setBackground(byte r, byte g, byte b);
	virtual void onDebugSwitch(bool debugMode);
	inline Uint8 getRefresh()        { return m_scrRefresh; }
	inline void  setRefresh(Uint8 r) { m_scrRefresh = r; }

	void border(byte b);
	void enable(bool b = true);
        virtual void reset(void);
//
// XXX Native output (not yet implemented)
//
        virtual PcwTerminal &operator << (unsigned char c);

protected:
	void drawPcwScr(void);

	SDL_Colour m_screenBg, m_screenFg, m_beepFg, m_beepBg;
//
// Variables for drawing PCW video RAM
//
	bool	m_borderInvalid;	// Border needs redraw?
	int	m_visible;		// Screen visible?
	int	m_pcwWhite, m_pcwBlack;	  // Colours to use
	int	m_pcwDWhite, m_pcwDBlack; // 
	int	m_beepClr;		// Colour for border beep
	int	m_scrEnabled;		// 0 : Screen is border only
					// 1 : Screen is normal terminal
					// 2 : Screen is border only, but
					//    needs to be zapped.
	Uint8	m_colourReg;		// Colour register.
					// Bit 6 set if screen is visible.
	Uint8   m_prevColourReg;	// Previous value of colour register
	int	m_scrHeight;		// 256 lines or 200?
	Uint8	m_scrOrigin;		// Vertical origin
	Uint8	*m_rollerRam;		// PCW R-RAM
//
// Refresh settings
//
	int	m_scrCount, m_scrRefresh;
//
// XML settings
//
	bool parseNode(xmlDocPtr doc, xmlNsPtr ns, xmlNodePtr cur);
	bool storeNode(xmlDocPtr doc, xmlNsPtr ns, xmlNodePtr cur);
//
// Settings functions.
// See if this device has user-settable options. If it does, populates
// "key" and "caption" and returns true; else, returns false.
//
public:
        virtual bool hasSettings(SDLKey *key, string &caption);
//
// Display settings screen
// Return 0 if OK, -1 if quit message received.
// 
        virtual UiEvent settings(UiDrawer *d);
private:
	UiEvent customClrs(UiDrawer *d);

//
// Beeper support
//
public:
	virtual void beepOn(void);
	virtual void beepOff(void);
	void initSound(void);
	void deinitSound(void);
        virtual void tick(void);

protected:
	BeeperMode m_beepMode;
	PcwBeeper *m_beeper;
};





