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

/* JOYCE video model:
 * 
 *    One PcwSdlContext, which holds the following:
 *       * The master SdlSurface. 
 *       * The 256-colour palette, and functions to manage it.
 *       * The application icon.
 *
 *    A number of PcwTerminals; at any time, one of them is switched in
 *    and the others aren't, cf CardLayout in the Java AWT. 
 *    JoyceSystem::m_activeTerminal holds the current terminal; it should
 *    be set with JoyceSystem::setActiveTerminal().
 */	

#include "SdlTerm.hxx"
#include "SdlVt52.hxx"

class PcwTerminal : public PcwDevice
{
	friend class JoyceSystem;
	friend class AnneSystem;
	friend class PcwSystem;
	friend class PcwSdlTerm;

public:
	PcwTerminal(const char *devname);
	virtual ~PcwTerminal();

// State
protected:
	SdlContext *m_framebuf;
	PcwSdlContext *m_sysVideo;
	bool	m_debugMode;
	bool	m_beepStat;
// Interface

protected:				// JoyceSystem is a friend and can
					// access these functions.
	virtual void onGainFocus();	// Terminal is gaining focus
	virtual void onLoseFocus();	// Terminal is about to lose focus
	virtual void onDebugSwitch(bool debugMode);
					// System is entering/leaving debug
					// mode.

public:
	virtual void onScreenTimer();	// Called at ~900Hz.

	void setSysVideo(PcwSdlContext *s);
	inline PcwSdlContext *getSysVideo(void) { return m_sysVideo; }

	virtual void out(word port, byte value);
	virtual byte in (word port);

	virtual void reset(void);

	inline  PcwTerminal &operator << (signed char c) 
				{ return operator << ((unsigned char)c); }
	virtual PcwTerminal &operator << (unsigned char c) = 0;
	PcwTerminal & operator << (const char *s);
	PcwTerminal & operator << (int i);
	PcwTerminal & vprintf(const char *s, va_list args);
	PcwTerminal & printf(const char *s, ...);

	virtual void beepOn(void);
	virtual void beepOff(void);

	virtual void setForeground(byte r, byte g, byte b) = 0;
	virtual void setBackground(byte r, byte g, byte b) = 0;

	virtual void tick(void);

// These virtual functions are provided for ANNE, to accelerate the PCW16's
// rather slow graphics operations.
	virtual bool doBlit(unsigned xfrom, unsigned yfrom, unsigned w, 
				unsigned h, unsigned xto, unsigned yto);
	virtual bool doRect(unsigned x, unsigned y, unsigned w, unsigned h, 
				byte fill, byte rasterOp, byte *pattern = NULL);
        virtual bool doBitmap(unsigned x, unsigned y, unsigned w, unsigned h,
                                byte *data, byte *mask = NULL, int slice = 0);
};


void flipXbm(int w, int h, Uint8* bits);




