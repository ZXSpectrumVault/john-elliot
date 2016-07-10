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

class UiDrawCallback;
class UiDrawer;

//
// Generic interface to all the devices in a JOYCE emulated system.
//
class PcwDevice
{
public:
	PcwDevice(string sClass = "", string sName = "");
	virtual ~PcwDevice();
//
// Handle SDL events
//
	int handleEvent(SDL_Event &e);
//
// Load settings from JOYCEHW.XML
//
	bool parse(xmlDocPtr doc, xmlNsPtr ns, xmlNodePtr parent);
//
// Save settings to JOYCEHW.XML
//
	bool store(xmlDocPtr doc, xmlNsPtr ns, xmlNodePtr parent);

	virtual bool isEnabled(void);
	virtual void enable(bool b = true);
//
// Reset device
//
	virtual void reset(void) = 0;
//
// Settings functions.
// See if this device has user-settable options. If it does, populates
// "key" and "caption" and returns true; else, returns false.
//
	virtual bool hasSettings(SDLKey *key, string &caption);
//
// Display settings screen
// Return 0 if OK, -1 if quit message received.
// 
	virtual UiEvent settings(UiDrawer *d);
//
// Set PCW model
//
	virtual void setModel(PcwModel j);
//
// nb: Ability to dump state is not yet provided, but would go here.
//

protected:
        virtual bool parseNode(xmlDocPtr doc, xmlNsPtr ns, xmlNodePtr cur) = 0;
	virtual bool storeNode(xmlDocPtr doc, xmlNsPtr ns, xmlNodePtr cur) = 0;
	bool parseEnabled(xmlDocPtr doc, xmlNsPtr ns, xmlNodePtr cur);
	bool storeEnabled(xmlDocPtr doc, xmlNsPtr ns, xmlNodePtr cur);
	bool parseFilename(xmlDocPtr doc, xmlNsPtr ns, xmlNodePtr cur, 
			   const string nodename, string &name);
	bool storeFilename(xmlDocPtr doc, xmlNsPtr ns, xmlNodePtr cur, 
			   const string nodename, string name);
	bool parseColour(xmlDocPtr doc, xmlNsPtr ns, xmlNodePtr cur,
                           const string nodename, SDL_Colour &fg, SDL_Colour &bg);
	bool storeColour(xmlDocPtr doc, xmlNsPtr ns, xmlNodePtr cur,
                           const string nodename, SDL_Colour &fg, SDL_Colour &bg);
	bool parseInteger(xmlDocPtr doc, xmlNsPtr ns, xmlNodePtr cur,
                           const string nodename, int *i);
	bool storeInteger(xmlDocPtr doc, xmlNsPtr ns, xmlNodePtr cur,
                           const string nodename, int i);
		
	
private:
	void putColour  (xmlNodePtr cur, const string attrname, SDL_Colour &clr);
	void parseColour(xmlNodePtr node, const string attrname, SDL_Colour &clr);

public:
	inline string getClass() { return m_class; }
	inline string getName()  { return m_name; }	

protected:
	string m_class, m_name;
	PcwModel m_model;
private:
	bool m_enabled;
};

