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


class JoyceFDC : public PcwFDC
{
public:
	JoyceFDC(JoyceSystem *s);
	virtual ~JoyceFDC();
/*
	void reset(void);
	void reset(int somedrives);
	void setTerminalCount(bool b);
	void setMotor(byte b);
	void writeData(byte b);
	byte readData(void);
	byte readControl(void);
	void isr(FDC_765 *, int);
	bool loadBootSector(const string filename, char *type, byte *sector);
	void insertB(const string filename);
	void tick(void);
	virtual bool hasSettings(SDLKey *key, string &caption);
        virtual UiEvent settings(UiDrawer *d);
	UiEvent discsMenu(int x, int y, UiDrawer *d);
*/
	// Set PCW type (8256/8512/9512 or 9256/9512+/10
	virtual void setModel(PcwModel j);

	PcwModel adaptModel(PcwModel j);
/*
protected:
        virtual bool parseNode(xmlDoc *doc, xmlNs *ns, xmlNode *ptr);
        virtual bool parseDriveNode(xmlDoc *doc, xmlNs *ns, xmlNode *ptr);
        virtual bool storeNode(xmlDoc *doc, xmlNs *ns, xmlNode *ptr);
	virtual bool storeDriveNode(xmlDoc *doc, xmlNs *ns, xmlNode *ptr, int n);

	UiEvent driveSettings(int drive, bool advanced, UiDrawer *d);
	UiEvent driveCustom  (int drive, bool advanced, UiDrawer *d);
	UiEvent driveSameAs  (int drive, UiDrawer *d);
	UiEvent getDskType   (UiDrawer *d, int &fmt, bool create);
public:
	UiEvent requestDsk   (UiDrawer *d, string &filename, char **type);
protected:
	FDC_765 m_fdc;                    // (v1.37) floppy controller 
//	DSK_FLOPPY_DRIVE m_fd[4];         // (v1.37) floppy drives A: and B:    
	LIBDSK_FLOPPY_DRIVE m_fdl[4];     // [v1.9.3] Move to LIBDSK for floppies 
	FLOPPY_DRIVE m_fd_none[4];        // (v1.37) disconnected A: and B:
	NC9_FLOPPY_DRIVE m_fd_9256[4];    // (v1.37) disconnected C: and D: 
	
	byte m_terminalCount;             // (v1.37) FDC's TC line  
	bool m_fdcInt;

	// Default settings, from XML.
	bool m_bootRO;
	int m_driveType[4];
	int m_driveHeads[4];
	int m_driveCyls[4];
	int m_driveMap[4];

	JoyceSystem *m_sys;
*/
}; 
