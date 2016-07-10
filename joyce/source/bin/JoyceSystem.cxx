
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

#include "Joyce.hxx"
#include "UiContainer.hxx"
#include "UiLabel.hxx"
#include "UiSeparator.hxx"
#include "UiCommand.hxx"
#include "UiSetting.hxx"
#include "UiMenu.hxx"
#include "UiTextInput.hxx"
#include "UiScrollingMenu.hxx"
#include "PcwFileList.hxx"
#include "PcwFileChooser.hxx"

//
// Minimal bootstrap ROM to run a boot sector that's already loaded at
// 0F000h. 
//
static const byte initBoot[] = { 0xF3, 0x31, 0x00, 0xF0, 0xC3, 0x10, 0xF0 };

//
// Namespace for the hardware definitions
//
#define XMLNS             "http://www.seasip.demon.co.uk/joycecfg.dtd"
#define HW_FILE           "joycehw.xml" 
//
// Namespace for the boot disc list
//
#define CFGNS ((xmlChar *)"http://www.seasip.demon.co.uk/joycebt.dtd")
#define BOOT_FILE         "joycebt.xml" 

//
// This class is supposed to encapsulate the entire PCW system. 
//
JoyceSystem::JoyceSystem(JoyceArgs *args) : PcwSystem(args, HW_FILE, XMLNS),
		        m_keyb(this), 
			m_mouse(this),
                        m_daisy(this),
                        m_cps(this)
{
        m_cpu = new JoyceZ80(this);
	m_asic = new JoyceAsic(this);
	m_mem  = new JoyceMemory();
        if (m_bootfile == "")
                m_bootfile = findPcwFile(FT_BOOT, BOOT_SYS, "rb");
// Need to create m_termMenu before m_fdc
	m_termMenu       = new JoyceMenuTermPcw(this);
	m_fdc = new JoyceFDC(this);

	m_activeKeyboard = &m_keyb;	
	m_activeMouse    = &m_mouse;

	m_bootcfg = xmlNewDoc((xmlChar *)"1.0");
	if (m_bootcfg)
	{
	        xmlNsPtr ns;
       		xmlNodePtr root;

	        root = xmlNewNode(NULL, (xmlChar *)"bootlist");
       		xmlDocSetRootElement(m_bootcfg, root);

        	ns = xmlNewNs(root, CFGNS, NULL);
	}
	m_defaultTerminal = new JoycePcwTerm(&m_beeper);
	m_activePrinter  = &m_cen;
	addDevice(m_fdc);
	addDevice(m_asic);
	addDevice(m_mem);
	addDevice(m_defaultTerminal);
	addDevice(m_termMenu);
	addDevice(&m_joystick);
	addDevice(&m_keyb);
	addDevice(&m_mouse);
	addDevice(&m_locoLink);
        addDevice(&m_matrix);
        addDevice(&m_daisy);
        addDevice(&m_cen);
        addDevice(&m_cps);
}


JoyceSystem::~JoyceSystem()
{
	delete m_defaultTerminal;
        if (m_bootcfg)  xmlFreeDoc(m_bootcfg);
}
#if 0
JoyceSystem::JoyceSystem(JoyceArgs *args) : 
			     m_asic(this),
			     m_fdc(this),
			     m_daisy(this),
			     m_cps(this),
			     m_termVga(this), 
			     m_termLogo(this), 
                             m_termMenu(this), 
			     m_termDebug(this)
{
	XLOG("JoyceSystem::JoyceSystem()");

        m_cpu = new JoyceZ80(this); 
	m_bootfile = args->m_bootRom;
	m_args     = args;

	m_activeTerminal = NULL;
	m_video          = NULL;
	m_bootRom        = NULL;
	m_bootRomLen     = 0;

	m_devices        = new (JoyceDevice *)[10];
	m_devCount       = 0;
	m_devMax         = 10;
	m_resetPending   = false;
	m_model          = PCW8000;

	// Create an empty DOM for hardware configuration
	m_settings = xmlNewDoc((xmlChar *)"1.0");
	if (m_settings)
	{
	        xmlNsPtr ns;
       		xmlNodePtr root;

	        root = xmlNewNode(NULL, (xmlChar *)"pcw");
       		xmlDocSetRootElement(m_settings, root);

        	ns = xmlNewNs(root, XMLNS, NULL);
	}

	// Add all devices to generic list
	addDevice(&m_mem);
	addDevice(&m_fdc);
	addDevice(&m_hdc);
	addDevice(&m_matrix);
	addDevice(&m_daisy);
	addDevice(&m_cen);
	addDevice(&m_cps);
	addDevice(&m_termPcw);
	addDevice(&m_termLogo);
	addDevice(&m_termVga);
	addDevice(&m_termDebug);
	addDevice(&m_termMenu);
}


JoyceSystem::~JoyceSystem()
{
        if (m_settings) xmlFreeDoc(m_settings);
        if (m_bootcfg)  xmlFreeDoc(m_bootcfg);
	if (m_bootRom)  delete m_bootRom;
	delete m_cpu;
}


void JoyceSystem::addDevice(JoyceDevice *d)
{
	int n;

	if (m_devCount >= m_devMax)
	{
		JoyceDevice **t = new (JoyceDevice *)[2 * m_devMax];
	
		if (!t) return;
		for (n = 0; n < m_devCount; n++) t[n] = m_devices[n];
		delete m_devices;
		m_devices = t;
		m_devMax *= 2;
	}
	m_devices[m_devCount++] = d;
}

//
// Access to XML settings file
//
bool JoyceSystem::loadHardware()
{
        string filename;
        xmlDocPtr doc;
        xmlNsPtr ns;
        xmlNodePtr cur;

        filename = findPcwFile(FT_NORMAL, HW_FILE, "r");
//
// Do a lot of sanity checks on the hardware XML.
//
        doc = xmlParseFile(filename.c_str());
        if (doc == NULL)	// Can't open it
        {
                joyce_dprintf("Could not open %s\n", filename.c_str());
                return 0;
        }
        cur = xmlDocGetRootElement(doc);
        if (cur == NULL)	// No root element
        {
                joyce_dprintf("%s: File is empty\n", filename.c_str());
                xmlFreeDoc(doc);
                return 0;
        }
        ns = xmlSearchNsByHref(doc, cur, XMLNS);
        if (ns == NULL)		// No namespace
        {
                joyce_dprintf("%s: No JOYCE namespace found\n", filename.c_str());
                xmlFreeDoc(doc);
                return 0;
        }
        if (strcmp((char *)cur->name, "pcw")) // Wrong root
        {
                joyce_dprintf("%s: root node is not of type 'pcw'.", filename.c_str());
                xmlFreeDoc(doc);
                return 0;
        }
	// Newly-loaded DOM is okay.
	if (m_settings) xmlFreeDoc(m_settings);
	m_settings = doc; 
	return true;
}


#endif
bool JoyceSystem::parseHardware()
{
	bool okay = PcwSystem::parseHardware();

	// Allow the FDC to alter the PCW model type
	setModel(((JoyceFDC *)m_fdc)->adaptModel(getModel()));

	return okay;
}
#if 0

bool JoyceSystem::storeHardware()
{
        xmlNodePtr root = xmlDocGetRootElement(m_settings);
        xmlNsPtr   ns   = xmlSearchNsByHref(m_settings, root, XMLNS);
	bool okay = true;

        for (int n = 0; n < m_devCount; n++) 
        {
                okay = okay && m_devices[n]->store(m_settings, ns, root);
        }
	return okay;
}


bool JoyceSystem::saveHardware()
{
	string filename;

	if (!m_settings) return false;

        filename = findPcwFile(FT_NORMAL,HW_FILE,"w"); 

        if (xmlSaveFile(filename.c_str(), m_settings) < 0) return 0;
        return 1;
}


///////////////////////////////////////////////////////////////////////////
// Video
//

void JoyceSystem::selectTerminal(JoyceTerminal *t)
{
	if (m_activeTerminal) m_activeTerminal->onLoseFocus();
	m_activeTerminal = t;
	if (t) t->onGainFocus();
}


bool JoyceSystem::initVideo(void)
{
	try
	{
		if (m_video) delete m_video;
		m_video = new JoyceSdlContext(m_args);
	}
	catch(JoyceInitScrException *e)
	{
		joyce_dprintf("Error initialising screen: %s\n", e->getExplanation());
		delete e;
		return false;
	}
	m_termPcw.  setSysVideo(m_video);
	m_termLogo. setSysVideo(m_video);
        m_termMenu-> setSysVideo(m_video);
        m_termVga.  setSysVideo(m_video);
	m_termDebug.setSysVideo(m_video);

	// General system reset
	reset();
        m_termMenu-> setSysVideo(m_video);

	return true;
}
#endif


bool JoyceSystem::deinitVideo(void)
{
	// [v2.1.5] Similarly, shut down joysticks
	m_keyb.reset();
	m_joystick.reset();
	// [v1.9.3] Shut down sound before releasing SDL
        ((JoycePcwTerm *)m_defaultTerminal)->deinitSound();
	PcwSystem::deinitVideo();
	return true;
}

///////////////////////////////////////////////////////////////////////////////
//
// General reset
//
void JoyceSystem::reset(void)
{
	PcwSystem::reset();
	loadBoot(PCWRAM, m_bootfile);
}
#if 0


//////////////////////////////////////////////////////////////////////////////
//
// Bring up settings pages
//
UiEvent  JoyceSystem::settings(UiDrawer *d, int x, int y)
{
	vector<int> map;
	UiMenu uim(d);
	UiEvent uie;
	SDLKey k;
	int entries, n;
	string caption;

	for (n = entries = 0; n < m_devCount; n++)
	{
		if (m_devices[n]->hasSettings(&k, caption))
		{
			map.push_back(n);
			uim.add(new UiCommand(k, caption,d, UIG_SUBMENU));
		}
	}
	uim.add(new UiSeparator(d))
	   .add(new UiCommand(SDLK_s,      "  Save settings  ", d))
	   .add(new UiCommand(SDLK_ESCAPE, "  Exit", d));

	uim.setX(x);
	uim.setY(y);
	do
	{
		uie = uim.eventLoop();
		if (uie == UIE_QUIT || uie == UIE_CANCEL) return uie;

		n = uim.getSelected();
		if (n >= (int)map.size()) 
		{
			if (uim.getKey(n) == SDLK_ESCAPE) break;
			if (uim.getKey(n) == SDLK_s) 
			{
				storeHardware();
				if (saveHardware()) uie = m_termMenu->report("Settings were saved", "  OK");
				else		    uie = m_termMenu->report("Failed to save settings","  Cancel");
 
				if (uie == UIE_QUIT) return uie;
			}
		}
		else
		{
			uie = m_devices[map[n]]->settings((UiDrawer *)&m_termMenu);
			if (uie == UIE_QUIT) return uie;
		}			
	} while(1);	
	return UIE_OK;
}

///////////////////////////////////////////////////////////////////////////////
//
// Event handling
//
int JoyceSystem::eventLoop(void)
{
	SDL_Event e;
	int evid, rv;

	do
	{
		evid = SDL_PollEvent(&e);
		if (!evid) break;
		
		if (m_activeKeyboard) rv = m_activeKeyboard->handleEvent(e);
		if (rv < 0) goodbye(99);
		if (rv == 0 && m_activeMouse) rv = m_activeMouse->handleEvent(e);
		if (rv < 0) goodbye(99);
	}
	while (evid);
	return 0;
}

//////////////////////////////////////////////////////////////////////////////
//
// Boot disc management
//
static void zputs(word W, const char *s)
{
        int n = 0;

        while (n < 16 && (*s))
        {
                if (*s < ' ') store(W++, ' ');
                else store(W++, *s);
                ++s;
                ++n;
        }
        while (n < 16)
        {
                store(W++, ' ');
                ++n;
        }
}
#endif

bool JoyceSystem::loadBootList()
{
        string filename;
        xmlDocPtr doc;
        xmlNsPtr ns;
        xmlNodePtr cur;

        filename = findPcwFile(FT_BOOT, BOOT_FILE, "r");
//
// Do a lot of sanity checks on the XML.
//
        doc = xmlParseFile(filename.c_str());
        if (doc == NULL)	// Can't open it
        {
                joyce_dprintf("Could not open %s\n", filename.c_str());
                return 0;
        }
        cur = xmlDocGetRootElement(doc);
        if (cur == NULL)	// No root element
        {
                joyce_dprintf("%s: File is empty\n", filename.c_str());
                xmlFreeDoc(doc);
                return 0;
        }
        ns = xmlSearchNsByHref(doc, cur, CFGNS);
        if (ns == NULL)		// No namespace
        {
                joyce_dprintf("%s: No JOYCE namespace found\n", filename.c_str());
                xmlFreeDoc(doc);
                return 0;
        }
        if (strcmp((char *)cur->name, "bootlist")) // Wrong root
        {
                joyce_dprintf("%s: root node is not of type 'bootlist'.", filename.c_str());
                xmlFreeDoc(doc);
                return 0;
        }
	// Newly-loaded DOM is okay.
	if (m_bootcfg) xmlFreeDoc(m_bootcfg);
	m_bootcfg = doc; 
	return true;
}



bool JoyceSystem::parseBootList()
{
	xmlNodePtr cur  = xmlDocGetRootElement(m_bootcfg);
	xmlNsPtr   ns   = xmlSearchNsByHref(m_bootcfg, cur, CFGNS);
	BootListEntry e;
	xmlChar *s;

	if (!cur) return true;
	m_bootList.clear();
        cur = cur->xmlChildrenNode;
        while (cur != NULL)
        {
                if (!strcmp((char *)cur->name, "bootdisc") && (cur->ns == ns))
                {
			s = xmlGetProp(cur, (xmlChar *)"id");
		        if (!s || s[0] < '0' || s[0] > '9')
		        {
                		joyce_dprintf("<bootdisc> must have an id= field\n");
               			if (s) xmlFree(s);
				continue;
			} 
			e.m_id = atoi((char *)s);
      			xmlFree(s); 
			s = xmlNodeListGetString(m_bootcfg, cur->xmlChildrenNode, 1);
			if (!s) continue;
			e.m_title = (char *)s;
			xmlFree(s);
			m_bootList.push_back(e);				
                }
                cur = cur->next;
        }
        return true;

}

bool JoyceSystem::storeBootList()
{
	int n;
	for (n = 0; n < (int)m_bootList.size(); n++)
	{
		if (!storeBootEntry(n))	return false;
	}
	return true;
}


bool JoyceSystem::storeBootEntry(int n)
{
	xmlNodePtr cur  = xmlDocGetRootElement(m_bootcfg);
	xmlNsPtr   ns   = xmlSearchNsByHref(m_bootcfg, cur, CFGNS);
	BootListEntry e = m_bootList[n];
	xmlChar *s;
	xmlNodePtr newText = xmlNewText((xmlChar *)e.m_title.c_str());
	xmlNodePtr newNode = xmlNewNode(ns, (xmlChar *)"bootdisc");
	int found = 0;
	char ids[20];

	xmlAddChild(newNode, newText);
	sprintf(ids, "%d", e.m_id);
	xmlSetProp(newNode, (xmlChar *)"id", (xmlChar *)ids);

	if (!cur) return false;
        for (cur = cur->xmlChildrenNode; cur != NULL; cur=cur->next)
        {
                if (!strcmp((char *)cur->name, "bootdisc") && (cur->ns == ns))
                {
			s = xmlGetProp(cur, (xmlChar *)"id");
		        if (!s || s[0] < '0' || s[0] > '9')
		        {
                		joyce_dprintf("<bootdisc> must have an id= field\n");
               			if (s) xmlFree(s);
				continue;
			} 
			if (e.m_id != atoi((char *)s)) { free(s); continue; }
      			xmlFree(s); 
	
			++found;
			xmlReplaceNode(cur, newNode);
                }
        }
	if (!found)
	{
		cur  = xmlDocGetRootElement(m_bootcfg);
		xmlAddChild(cur, newNode);
	}
        return true;
}



bool JoyceSystem::saveBootList()
{
	string filename;

	if (!m_bootcfg) return false;

        filename = findPcwFile(FT_BOOT, BOOT_FILE, "w"); 

        if (xmlSaveFile(filename.c_str(), m_bootcfg) < 0) return 0;
        return 1;
}

UiEvent JoyceSystem::bootHelp()
{
	UiDrawer *d = m_termMenu;
	UiMenu uim(d);
	
	uim.add(new UiLabel("This screen lists up to 9 boot discs (start-of-day discs)", d));
	uim.add(new UiLabel("that you have read into JOYCE. If you have read in more ", d));
	uim.add(new UiLabel("than 9, the others will be listed if you press the [F3] ", d));
	uim.add(new UiLabel("key. ", d));
	uim.add(new UiLabel("  To read in more boot discs, press [F9] key, then [F2]", d));
	uim.add(new UiLabel("and choose 'Disc management' from the menu that appears.", d));
	uim.add(new UiLabel(" ", d));
	uim.add(new UiCommand(SDLK_o, "  OK  ", d));
	d->centreContainer(uim);
	return uim.eventLoop();
}


void JoyceSystem::getBootID(Z80 *R)
{
	unsigned n;
	UiEvent e;
	string bootfile;
	const char *type = NULL;

	// New in v1.5.0+: Extra boot image functions are overloaded in by
	// checking B == 255. 
	if (R->b == 0xFF) switch(R->c)
	{
		// Alternative boot 
		case 0:  m_keyb.clearKeys();
			 e = chooseBootDisc(m_termMenu, bootfile, &type);
			 if (e == UIE_QUIT) goodbye(99);
			 if (e != UIE_OK) { R->a = 1; return; }
			// Try to boot alternative image
			 if (fdcBoot(bootfile, type))
                         {
				memcpy(PCWRAM, initBoot, sizeof(initBoot));
				m_cpu->pc = 0x0000;
				m_cpu->iff1 = m_cpu->iff2 = 0;
				return;
                         }
			 memset(PCWRAM + 0xFFF0, 0, 16);
			 R->a = 1; return; 
		// Help with booting
		case 1:	 m_keyb.clearKeys(); 
			 e = bootHelp();
			 if (e == UIE_QUIT) goodbye(99);
			 memset(PCWRAM + 0xFFF0, 0, 16);
			 R->a = 1; return; 
		default: R->a = 0; return;	
	}
	R->a = 0;
	for (n = 0; n < m_bootList.size(); n++)
	{
		if (m_bootList[n].m_id == R->b)
		{
			R->a = 1;
			zputs(R->getHL(), m_bootList[n].m_title.c_str());	
		}
	}
}

UiEvent JoyceSystem::chooseBootDisc(UiDrawer *d, string &bootfile, const char **type)
{
	UiMenu uim(d);
	UiEvent uie;
	SDLKey k;
	char nbr[20];
	string s;
	int sel, id, n;

	uim.add(new UiLabel("  Boot from:  ", d));
	uim.add(new UiSeparator(d));
	for (n = 0; n < (int)m_bootList.size(); n++)
	{
		BootListEntry e = m_bootList[n];
		if (e.m_id < 10)
		{
			sprintf(nbr, "  %d. ", e.m_id);	
			s = nbr;
			k = (SDLKey)(SDLK_0 + e.m_id);
		}
		else if (e.m_id < 36)
		{
			sprintf(nbr, "  %c. ", e.m_id + 'A' - 10);
			s = nbr;
			k = (SDLKey)(SDLK_a - 10 + e.m_id);
		}
		else 
		{
			s = "     ";
			k = SDLK_UNKNOWN;
		}
		s += e.m_title;
		uim.add(new UiCommand(k, s, d));
	}
	uim.add(new UiCommand(SDLK_SPACE, "  Other...  ", d));
	d->centreContainer(uim);
	uie = uim.eventLoop();
	if (uie != UIE_OK) return uie;
	k = uim.getKey(sel = uim.getSelected());
	if (k == SDLK_SPACE)
	{
// Since the FDC can handle LIBDSK discs, we use requestDsk here.
//		string filename = findJoyceDir(FT_BOOT, false);
//		JoyceFileChooser f("  OK  ", d);
//		f.initDir(filename);
//              uie = f.eventLoop();

		uie = m_fdc->requestDsk(d, bootfile, type);
                if (uie != UIE_OK) return uie;
//                bootfile = f.getChoice();
		return UIE_OK;
	}


	id = m_bootList[sel - 2].m_id;
	bootfile = idToFilename(id);

	return UIE_OK;
}



UiEvent JoyceSystem::chooseBootID(UiDrawer *d, const string prompt, int *bootid)
{
	UiMenu uim(d);
	UiEvent uie;
	SDLKey k;
	char nbr[20];
	string s;
	int sel, id, n;

	uim.add(new UiLabel(prompt, d));
	uim.add(new UiSeparator(d));
	for (n = 0; n < (int)m_bootList.size(); n++)
	{
		BootListEntry e = m_bootList[n];
		if (e.m_id < 10)
		{
			sprintf(nbr, "  %d. ", e.m_id);	
			s = nbr;
			k = (SDLKey)(SDLK_0 + e.m_id);
		}
		else if (e.m_id < 36)
		{
			sprintf(nbr, "  %c. ", e.m_id + 'A' - 10);
			s = nbr;
			k = (SDLKey)(SDLK_a - 10 + e.m_id);
		}
		else 
		{
			s = "     ";
			k = SDLK_UNKNOWN;
		}
		s += e.m_title;
		uim.add(new UiCommand(k, s, d));
	}
	uim.add(new UiSeparator(d));
	uim.add(new UiCommand(SDLK_ESCAPE, "  Cancel  ", d));
	d->centreContainer(uim);
	uie = uim.eventLoop();
	if (uie != UIE_OK) return uie;
	k = uim.getKey(sel = uim.getSelected());
	if (k == SDLK_ESCAPE) return UIE_CANCEL;

	id = m_bootList[sel - 2].m_id;
	*bootid = id;

	return UIE_OK;
}









int JoyceSystem::fdcBoot(const string filename, const char *type)
{
	bool okay;
	byte cksum;
	int n, warn = 0;

	okay = m_fdc->loadBootSector(filename, type, PCWRAM + 0xF000);
	if (!okay) return 0;

	cksum = 0;
	for (n = 0xF000; n < 0xF200; n++)
	{
		cksum += PCWRAM[n];
	}
	switch(cksum)
	{
		case 0xFF:	// 8000 series
			if (m_model == PCW9000)  { warn = 1; setModel(PCW8000); }
			if (m_model == PCW9000P) { warn = 1; setModel(PCW10);   }
			break;
		case 0x01:	// 9000 series
			if (m_model == PCW8000)  { warn = 1; setModel(PCW9000);  }
			if (m_model == PCW10)    { warn = 1; setModel(PCW9000P); }
			break;
		case 0x02:
			m_termMenu->popup("Can't boot from LocoScript install disc", 4, 1);
			return 0;
		case 0x03:
			m_termMenu->popup("Can't boot from Spectrum +3 disc", 4, 1);
			return 0;
		default:
			m_termMenu->popup("Unknown type of boot disc", 4, 1);
			return 0;
			
		}
	if (warn) switch(m_model)
	{
		case PCW8000:  m_termMenu->popup("PCW type changed to 8256/8512", 6, 1); break;
		case PCW9000:  m_termMenu->popup("PCW type changed to 9512", 6, 1); break;
		case PCW9000P: m_termMenu->popup("PCW type changed to 9512+", 6, 1); break;
		case PCW10:    m_termMenu->popup("PCW type changed to 9256/10", 6, 1); break;
	}
        memset(PCWRAM, 0, 0xF000);
	// [v1.9.2] Insert B drive disc if specified
	if (m_args->m_bDrive != "")
	{
		m_fdc->insertB(m_args->m_bDrive);
		m_args->m_bDrive = "";
	}
        return 1;
}

int JoyceSystem::fdcBoot(int bootid)
{
	return fdcBoot(idToFilename(bootid));
}

string JoyceSystem::idToFilename(int bootid)
{
	string bootfile;
	char nbr[20];

        sprintf(nbr, "%d", bootid);
        bootfile = "boot";
        bootfile += nbr;
        bootfile += ".dsk";
        bootfile = findPcwFile(FT_BOOT, bootfile, "rb");
	return bootfile;
}

int JoyceSystem::lookupId(int bootid)
{
	unsigned n;
	for (n = 0; n < m_bootList.size(); n++)
		if (m_bootList[n].m_id == bootid) return n;
	return -1;	
}

bool JoyceSystem::deleteBootEntry(int bootid)
{
	int n = lookupId(bootid);
	if (n < 0) return false;

	// Remove disc from the boot list	
	m_bootList.erase(m_bootList.begin() + n);
	// Remove it from the XML	
	xmlNodePtr cur  = xmlDocGetRootElement(m_bootcfg);
	xmlNodePtr next;
	xmlNsPtr   ns   = xmlSearchNsByHref(m_bootcfg, cur, CFGNS);
	xmlChar *s;

	if (!cur) return true;
        for (cur = cur->xmlChildrenNode; cur != NULL; cur = next)
        {
		next = cur->next;
		if (strcmp((char *)cur->name, "bootdisc")) continue;
		if (cur->ns != ns) continue;
                
		s = xmlGetProp(cur, (xmlChar *)"id");
		if (!s) continue;

		if (bootid != atoi((char *)s)) { xmlFree(s); continue; }
		xmlFree(s);
                xmlUnlinkNode(cur);
                xmlFreeNode(cur);
	}
	return true;
}

#if 0

// Load the initial boot ROM (BOOTFILE.EMJ).
bool JoyceSystem::loadBoot(string bootfile)
{
        FILE *fp;
        word address = 0;
        int c;
//
// Do we have a cached copy of the boot ROM?
//
	if (m_bootRom)
	{
		memcpy(PCWRAM, m_bootRom, m_bootRomLen);
		return true;
	}

        if ((fp = fopen(bootfile.c_str(),"rb")) == NULL)
        {
                m_termMenu->report("Failed to open the boot ROM", "  Exit  ");
                deinitVideo();
                fprintf(stderr,"Open of %s failed!  \r\n", bootfile.c_str());
                diewith("Failed to open boot file\r\n",98);
		return false;
        }
	fseek(fp, 0, SEEK_END);
	m_bootRomLen = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	m_bootRom    = new byte[m_bootRomLen];
        /* Load the boot ROM into Z80 memory */
        while ((c = getc(fp)) != EOF) PCWRAM[address++] = c;
	memcpy(m_bootRom, PCWRAM, m_bootRomLen);

        fclose(fp);
	return true;
}

///////////////////////////////////////////////////////////////////////////////
//
// Support for the various PCW models
//
void JoyceSystem::setModel(JoyceModel j)
{
	if (m_model == j) return;
	m_model = j;
	for (int n = 0; n < m_devCount; n++) 
	{
		m_devices[n]->setModel(j);
	}
}


///////////////////////////////////////////////////////////////////////////////
//
// Debug support
//
void JoyceSystem::setDebug(bool b)
{	
	if (m_debugMode == b) return;
	m_debugMode = b;
	m_activeTerminal->onDebugSwitch(b);
	if (b) m_termDebug.onGainFocus();
	else   m_termDebug.onLoseFocus();
}

#endif
///////////////////////////////////////////////////////////////////////////////
//
// System init
//
bool JoyceSystem::sysInit(void)
{
        loadBootList();
        parseBootList();
	loadHardware();
	parseHardware();
        int memsiz = m_mem->alloc(m_args);
        if (!memsiz)
        {
                alert("Not enough memory to run JOYCE.");
                return false;
        }
        printf("JOYCE will emulate a PCW 8%d (or 9%d)\n", memsiz, memsiz);
	fflush(stdout);

	if (!PcwSystem::sysInit()) return false;

	loadBoot(PCWRAM, m_bootfile);

// All "outside functions" are regulated in JOYCE by a
// simulated 900Hz clock. For a 4MHz CPU, this is every
// 4e6/9e2 = 4444 cycles, or about 500 instructions. 
// On a PCW with a Sprinter, double this; but if the interrupts get 
// too infrequent, they will never occur when EI is on. I'm 
// using 4441 rather than 4444 because it's prime, and so we're 
// less likely to get problems caused by interrupts synchronising 
// exactly with a tight loop.
//
	m_cpu->setCycles(4441);
	m_cpu->reset();
	// If there is no BOOT.CFG, ask about adding some boot discs.
	// If we've been explicitly told to boot from a disc, then
	// it doesn't matter if any were set up or not.
	if (m_bootList.size() == 0 && m_args->m_aDrive == "")
	{
		int nc;
		UiEvent uie;
		UiDrawer *d = m_termMenu;
		UiMenu uim(d);

		uim.add(new UiLabel("  No JOYCE boot discs have been set up.  ", d));
		uim.add(new UiSeparator(d));
		uim.add(new UiCommand(SDLK_s, "  Set up boot discs properly  ", d));
		uim.add(new UiCommand(SDLK_a, "  Boot from " A_DRIVE , d)); 
		uim.add(new UiCommand(SDLK_b, "  Boot from " B_DRIVE , d));
		uim.add(new UiCommand(SDLK_q, "  Quit ", d));
	        d->centreContainer(uim);
	        uie = uim.eventLoop();
       		if (uie == UIE_QUIT || uie == UIE_CANCEL) return false;
	        switch (uim.getKey(uim.getSelected()))
		{
			case SDLK_q: return false;
			case SDLK_s:
				nc = autoSetup();
				if (!nc)
				{
					m_termMenu->report("No discs were converted",
                                                        "  Continue  ");
				}
				if (nc < 0) 
				{
					deinitVideo();
					joyce_dprintf("Disc conversion cancelled.\n");
					return false;
				}
				// Disc setup has used the RAM, so reset it again.
				reset();
				break;
			case SDLK_a:	m_args->m_aDrive = A_DRIVE; break;
			case SDLK_b:	m_args->m_aDrive = B_DRIVE; break;
			default:        return false;
		}
	}
	// If we have an auto-A drive, boot it
	if (m_args->m_aDrive != "")
	{
		if (m_args->m_aDrive[0] == '#')
		{
			int id = atoi(m_args->m_aDrive.c_str() + 1);
			if (fdcBoot(id))
			{
// The Z80 always starts at 0000h. Put a bootstrap there and a jump to it
// at the bottom of memory.
				memcpy(PCWRAM, initBoot, sizeof(initBoot));
				m_cpu->pc = 0x0000;
				m_cpu->iff1 = m_cpu->iff2 = 0;
				return true;
			}
			joyce_dprintf("Could not auto-boot disc %d\n", id);
		}
		else if (fdcBoot(m_args->m_aDrive))
		{
			memcpy(PCWRAM, initBoot, sizeof(initBoot));
			m_cpu->pc = 0x0000;
			m_cpu->iff1 = m_cpu->iff2 = 0;
			return true;
		}
	}
	return true;
}

