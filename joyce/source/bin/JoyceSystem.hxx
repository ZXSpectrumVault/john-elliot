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


class JoyceSystem : public PcwSystem
{
public:
	JoyceSystem(JoyceArgs *a);
	virtual ~JoyceSystem();

// Keyboard & mouse
	JoycePcwKeyboard m_keyb;        // The keyboard
	JoycePcwMouse    m_mouse;       // The mouse
	JoyceJoystick	 m_joystick;	// Joysticks
// Printers and comms
        JoyceMatrix     m_matrix;       // Matrix printer
        JoyceDaisy      m_daisy;        // Daisywheel printer
        JoyceCenPrinter m_cen;          // Centronics printer
        JoyceCPS        m_cps;          // CPS8256
	JoyceLocoLink    m_locoLink;	// LocoLink interface

// Overrides
        virtual bool sysInit(); // Initialise the system
	virtual bool parseHardware();
        // Disc management & initial setup
	virtual UiEvent discManager(UiDrawer *d);
	virtual int autoSetup(void);
	virtual void reset(void);

// Booting
        bool loadBootList();
        bool parseBootList();
        bool storeBootList();
        bool storeBootEntry(int n);
        bool deleteBootEntry(int id);
        bool saveBootList();
        void getBootID(Z80 *R);
        UiEvent chooseBootDisc(UiDrawer *d, string &bootfile, const char **type);
        UiEvent chooseBootID(UiDrawer *d, const string prompt, int *bootid);
        int fdcBoot(const string filename, const char *type = NULL);
        int fdcBoot(int bootid);
        string idToFilename(int bootid);
        int lookupId(int bootid);
	virtual bool deinitVideo(void);

protected:
        UiEvent bootHelp(void);
};

