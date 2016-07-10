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

class AnneSystem : public PcwSystem
{
public:
	AnneSystem(AnneArgs *a);
	virtual ~AnneSystem();

	AnneKeyboard m_keyb;
	AnneAccel    m_accel;
	AnneRTC	   m_rtc;
	AnneSerial m_com1;
	AnneSerial m_com3;
	AnneSerialMouse m_mouse;
	AnnePrinter m_lpt1;
	bool	  m_romGood;
// overrides
	virtual UiEvent discManager(UiDrawer *d);
	virtual int autoSetup(void);
	virtual bool sysInit(void);
	virtual void reset(void);
// Booting
        bool loadBootList();
        bool parseBootList();
        bool storeBootList();
        bool storeBootEntry(int n);
        bool deleteBootEntry(int id);
        bool saveBootList();
        UiEvent chooseBootRom(UiDrawer *d, string &bootfile);
        UiEvent chooseBootID(UiDrawer *d, const string prompt, int *bootid);
        string idToFilename(int bootid);
        int lookupId(int bootid);
	void getBootID(Z80 *R);

	inline	AnneArgs *getArgs() { return (AnneArgs *)m_args; }
	
	void powerPress(void);
	void powerRelease(void);
};

