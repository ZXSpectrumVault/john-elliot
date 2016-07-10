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

class BootListEntry
{
public:
        string m_title;
        int m_id;
};

enum JDOSMode
{
	JM_PROMPT = 0,
	JM_ALWAYS = 1,
	JM_NEVER  = 2
};

//
// This class is supposed to encapsulate the entire PCW system. 
//

class PcwSystem
{
protected:		// The other modules need these to initialise
	PcwArgs *m_args;
public:
//
// The actual hardware "modules"
//
	Z80		*m_cpu;		// CPU
	PcwMemory	*m_mem;		// Memory controls
	PcwAsic		*m_asic;	// ASIC
	PcwFDC		*m_fdc;		// Disc subsystem
	PcwHDC		m_hdc;		// Hard drive controller
	PcwBeeper	m_beeper;	// The beeper.
// Terminals
	PcwVgaTerm      m_termVga;	// 800x600 text screen
	PcwLogoTerm	m_termLogo;	// Support for DR Logo
	PcwMenuTerm     *m_termMenu;	// Menu terminal
	PcwDebugTerm  m_termDebug;	// Debugger terminal
	PcwGSX          m_termGSX;	// GSX terminal
//
// Resources that belong to the emulator, not the emulated machine
//
	PcwSdlContext *m_video;	// Things like palettes and 
					// the SDL icon
//
// Internal state variables
//
	PcwPrinter  *m_activePrinter;	 // Current printer for LST intercept
	PcwTerminal *m_activeTerminal;   // Current screen
	PcwTerminal *m_defaultTerminal;	 // The terminal that will be chosen on reset
	PcwInput    *m_activeKeyboard;   // Current keyboard
	PcwInput    *m_activeMouse;	 // Current mouse
        xmlDocPtr     m_settings;        // The XML DOM containing the settings 
protected:
	string        m_bootfile;
	byte	     *m_bootRom;
	int	      m_bootRomLen;
	JDOSMode      m_jdosMode;

	const char *m_hwfile;
	const char *m_xmlns;
//
// Functions
//
public:
	PcwSystem(PcwArgs *a, const char *hwfile, const char *xmlns);
	virtual ~PcwSystem();

	virtual bool sysInit();	// Initialise the system

	bool loadHardware();	// Load the hardware description XML file.
	virtual bool parseHardware();	// Allow the modules to parse the XML DOM.
	bool storeHardware();	// Ask the modules to save their state to the DOM
	bool saveHardware();	// Save the hardware XML file.

	bool loadBoot(byte *memory, string bootFile);
				// Switch video over to a different terminal.
	bool initVideo(void);	// Start up video
	virtual bool deinitVideo(void);	// Shut down video
	void selectTerminal(PcwTerminal *t);

	UiEvent settings(UiDrawer *d, int x, int y);
	void setDebug(bool debugMode = true);
	inline bool getDebug(void) { return m_debugMode; }

	// Handle SDL events
	int eventLoop(void);
	// System reset handling
	virtual void reset(void);
	void setResetFlag(bool b = true);
	inline bool getResetFlag() { return m_resetPending; }

	// PCW type handling
	inline PcwModel getModel() { return m_model; }
	void setModel(PcwModel j);

	inline JDOSMode getJDOSMode() { return m_jdosMode; }
	void setJDOSMode(JDOSMode m)  { m_jdosMode = m ; }

	// Disc management & initial setup
	virtual UiEvent discManager(UiDrawer *d) = 0;
	virtual int autoSetup(void) = 0;
	
	inline int getMouseX()   { return m_mouseX; }	
	inline int getMouseY()   { return m_mouseY; }	
	inline int getMouseBtn() { return m_mouseBtn; }	
protected:
	void addDevice(PcwDevice *dev);
	UiEvent bootHelp(void);
	PcwDevice **m_devices;
	int m_devCount, m_devMax;
	bool m_debugMode;
	bool m_resetPending;
	PcwModel m_model;
	void zputs(word W, const char *s);
	int m_mouseX, m_mouseY, m_mouseBtn;
public:
        xmlDocPtr     m_bootcfg;         // The XML DOM containing BOOT.CFG
        vector<BootListEntry> m_bootList;

};

