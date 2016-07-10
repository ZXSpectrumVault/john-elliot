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

class UiDrawCallback;
class UiDrawer;

class PcwHDC : public PcwDevice
{
public:
	PcwHDC(PcwSystem *sys);
	virtual ~PcwHDC();

	PcwSystem *m_sys;
//
// Settings functions.
// See if this device has user-settable options. If it does, populates
// "key" and "caption" and returns true; else, returns false.
//
	virtual bool hasSettings(SDLKey *key, std::string &caption);
//
// Display settings screen
// Return 0 if OK, -1 if quit message received.
// 
	virtual UiEvent settings(UiDrawer *d);
//
// nb: Ability to dump state is not yet provided, but would go here.
//

protected:
        virtual bool parseNode(xmlDocPtr doc, xmlNsPtr ns, xmlNodePtr cur);
	virtual bool storeNode(xmlDocPtr doc, xmlNsPtr ns, xmlNodePtr cur);
	virtual bool parseDriveNode(xmlDocPtr doc, xmlNsPtr ns, xmlNodePtr cur);	
	virtual bool storeDriveNode(xmlDocPtr doc, xmlNsPtr ns, xmlNodePtr cur, int n);	
	virtual void reset(void);
	word m_fidText;
	bool m_allowFloppy;
	byte m_lastDrv;
	std::string m_lastErr;
	std::string m_drvName[16];
	const char*  m_drvType[16];
	std::string m_userType[16];
	DSK_PDRIVER  m_drvUnit[16];
	DSK_GEOMETRY m_drvDg[16];
	bool m_isHD[16];
	std::string hooked, namebuf;

// exported FID functions
public:
	void fidLogin(Z80 *R);
	void fidRead(Z80 *R);
	void fidWrite(Z80 *R);
	void fidFlush(Z80 *R);
	void fidMessage(Z80 *R);
	void fidEms(Z80 *R);
private:
	unsigned char deblock[16][1024];
	bool deblockPending[16];
	long deblockSector[16];
	UiEvent setupDrive(UiDrawer *d, int drive);
	UiEvent getFolderType(UiDrawer *d, int &fmt, bool hd);
	int formatFolder(const string dirname, int fmt);

	bool deblockRead (int img, long sector);
	bool deblockWrite(int img, long sector);

	void makeDpb(word dpb, byte *format, int is_xdpb);
	void fidPuts(word addr, const std::string s);
	void setupPars(Z80 *R);
	int  openImage(int n);
	int  closeImage(int n);
	void ret_err(Z80 *R);
	void ret_ok(Z80 *R);
	bool isHard(const string filename, const char *type);	
};

