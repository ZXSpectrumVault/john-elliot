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

#include "GdiPrint.hxx"

#define DAISY_MAXCMD 500	/* Maximum length of command to write to
				 * the controller. XXX Use the STL. */

class JoyceDaisy : public PcwPrinter
{
public:
	JoyceDaisy(JoyceSystem *s);
	~JoyceDaisy();
	void reset(void);
	void tick(void);
	void out(word port, byte value);
	byte in(word port);
        virtual bool hasSettings(SDLKey *key, string &caption);
        virtual UiEvent settings(UiDrawer *d);
        virtual UiEvent control(UiDrawer *d);
protected:
	virtual bool parseNode(xmlDocPtr doc, xmlNsPtr ns, xmlNodePtr cur);
	virtual bool storeNode(xmlDocPtr doc, xmlNsPtr ns, xmlNodePtr cur);
	bool parsePrintWheel(xmlDocPtr doc, xmlNsPtr ns, xmlNodePtr cur);
	bool storePrintWheel(xmlDocPtr doc, xmlNsPtr ns, xmlNodePtr cur);
	virtual string getTitle(void);
	byte parInp(void);
	void interrupt(void);

        UiEvent daisySettings(UiDrawer *d);

	void addResult(byte b);

//
// PostScript output code
//
	void closePS(void);
	bool startPagePS(void);
	bool endPagePS(void);
	void charPS(char c, float xf, float yf);	
	bool startPage(void);
	bool endPage(void);
	void putChar(char c, float xf, float yf);	
	void checkEndForm(void);
	UiEvent getCustomPaper(UiDrawer *d);

	PrinterFormat m_format;
#ifdef HAVE_WINDOWS_H
        GdiPrint m_gdiPrint;
#endif

	byte m_result[DAISY_MAXCMD];
	byte m_resultLatch;
	int m_resultw, m_resultr;	
	byte m_cmd[DAISY_MAXCMD];
	int m_curcmd;
	bool m_daisyEnabled;
	int m_intTime;		// Number of instructions before interrupt.
	JoyceSystem *m_sys;
	bool m_intEnabled;	// Should we interrupt the CPU?
	bool m_interrupting;	// We are twiddling the CPU's interrupt line
				// right now
	bool m_intWaiting;	// We would normally be interrupting but 
				// daisy interrupts are disabled.
	float m_x, m_y;		// Coordinates. The daisy uses 1/192ths of
				// an inch vertically, 1/120ths of an inch
				// horizontally.
	float m_w, m_h;		// Paper size
	bool m_bailIn;		// Bail bar out?
	bool m_coverOpen;	// Cover open?
	bool m_ribbon;		// Ribbon present?
	string m_psdir;
	bool m_drawing_page;	// Page drawing in process
	int  m_sequence;	// Filename number for page (daisy%d.ps)
	char m_outFilename[PATH_MAX];
	FILE *m_fp;
	// Printwheel definition:
	string m_fontname;	// Font name
	int  m_ptsize;		// Font size (points)
	unsigned char m_wheel[128];	// Mapping of wheel pins to characters. 
};


