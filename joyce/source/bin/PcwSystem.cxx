
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

#include "Pcw.hxx"
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
// This class is supposed to encapsulate the entire PCW system. 
//



PcwSystem::PcwSystem(PcwArgs *args, const char *hwfile, const char *xmlns) : 
			     m_termVga(this), 
			     m_termLogo(this), 
			     m_termGSX(this),
			     m_termDebug(this),
			     m_hdc(this),
			     m_args(args)
{
	XLOG("PcwSystem::PcwSystem()");

	m_hwfile   = hwfile;
	m_xmlns    = xmlns;
	m_bootfile = args->m_bootRom;
	m_jdosMode = JM_PROMPT;

	m_cpu  = NULL;
	m_mem  = NULL;
	m_asic = NULL;
	m_termMenu = NULL;
	m_activePrinter  = NULL;
	m_activeKeyboard = NULL;
	m_activeMouse    = NULL;
	m_activeTerminal = NULL;
	m_video          = NULL;
	m_bootRom        = NULL;
	m_bootRomLen     = 0;

	m_devices        = new PcwDevice *[10];
	m_devCount       = 0;
	m_devMax         = 10;
	m_resetPending   = false;
	m_model          = PCW8000;
	m_mouseX         = 0;
	m_mouseY         = 0;
	m_mouseBtn       = 0;
	// Create an empty DOM for hardware configuration
	m_settings = xmlNewDoc((xmlChar *)"1.0");
	if (m_settings)
	{
	        xmlNsPtr ns;
       		xmlNodePtr root;

	        root = xmlNewNode(NULL, (xmlChar *)"pcw");
       		xmlDocSetRootElement(m_settings, root);

        	ns = xmlNewNs(root, (xmlChar *)m_xmlns, NULL);
	}
	// Add all devices to generic list
	addDevice(&m_hdc);
	addDevice(&m_termLogo);
	addDevice(&m_termGSX);
	addDevice(&m_termVga);
	addDevice(&m_termDebug);
}


PcwSystem::~PcwSystem()
{
        if (m_settings) xmlFreeDoc(m_settings);
	if (m_bootRom)  delete m_bootRom;
	if (m_fdc) delete m_fdc;
	if (m_cpu) delete m_cpu;
	if (m_mem) delete m_mem;
	if (m_termMenu) delete m_termMenu;
}


void PcwSystem::addDevice(PcwDevice *d)
{
	int n;

	if (m_devCount >= m_devMax)
	{
		PcwDevice **t = new PcwDevice *[2 * m_devMax];
	
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
bool PcwSystem::loadHardware()
{
        string filename;
        xmlDocPtr doc;
        xmlNsPtr ns;
        xmlNodePtr cur;

        filename = findPcwFile(FT_NORMAL, m_hwfile, "r");
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
        ns = xmlSearchNsByHref(doc, cur, (xmlChar *)m_xmlns);
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



bool PcwSystem::parseHardware()
{
	xmlNodePtr root = xmlDocGetRootElement(m_settings);
	xmlNsPtr   ns   = xmlSearchNsByHref(m_settings, root, (xmlChar *)m_xmlns);
	bool okay = true;

	XLOG("PcwSystem::parseHardware()");
        for (int n = 0; n < m_devCount; n++) 
	{
	/*	printf("n=%d dev=%p\n", n, m_devices[n]); 
		fflush(stdout);
		printf("  %s:%s\n", m_devices[n]->getClass().c_str(),
				    m_devices[n]->getName().c_str());
		fflush(stdout); */
		okay = okay && m_devices[n]->parse(m_settings, ns, root);
	}
	return okay;
}


bool PcwSystem::storeHardware()
{
        xmlNodePtr root = xmlDocGetRootElement(m_settings);
        xmlNsPtr   ns   = xmlSearchNsByHref(m_settings, root, (xmlChar *)m_xmlns);
	bool okay = true;

        for (int n = 0; n < m_devCount; n++) 
        {
                okay = okay && m_devices[n]->store(m_settings, ns, root);
        }
	return okay;
}


bool PcwSystem::saveHardware()
{
	string filename;

	if (!m_settings) return false;

        filename = findPcwFile(FT_NORMAL,m_hwfile,"w"); 

        if (xmlSaveFile(filename.c_str(), m_settings) < 0) return 0;
        return 1;
}


///////////////////////////////////////////////////////////////////////////
// Video
//

void PcwSystem::selectTerminal(PcwTerminal *t)
{
	char buf[80];

	sprintf(buf, "PcwSystem::selectTerminal(%p -> %p)",
			m_activeTerminal, t);
	XLOG(buf);

	if (m_activeTerminal) m_activeTerminal->onLoseFocus();
	m_activeTerminal = t;
	XLOG("PcwSystem::selectTerminal() -> onGainFocus()");
	if (t) t->onGainFocus();
	XLOG("PcwSystem::selectTerminal() done\n");
}


//#define VT m_termMenu->setSysVideo(m_video); m_termMenu->setSysVideo(m_video);

bool PcwSystem::initVideo(void)
{
	try
	{
		if (m_video) delete m_video;
		m_video = new PcwSdlContext(m_args, 800, 600);
	}
	catch(PcwInitScrException *e)
	{
		joyce_dprintf("Error initialising screen: %s\n", e->getExplanation());
		delete e;
		return false;
	}
	m_defaultTerminal->setSysVideo(m_video);
	XLOG("Setting system video");
	m_termLogo. setSysVideo(m_video);
	m_termGSX.  setSysVideo(m_video);
        m_termMenu->setSysVideo(m_video);
        m_termVga.  setSysVideo(m_video);
	m_termDebug.setSysVideo(m_video);
	XLOG("Set system video");

	// General system reset
	XLOG("Resetting");
	reset();
	XLOG("After reset");
        m_termMenu->setSysVideo(m_video);
	XLOG("After setSysVideo");

	return true;
}


bool PcwSystem::deinitVideo(void)
{
	if (m_video) delete m_video;
	return true;
}


///////////////////////////////////////////////////////////////////////////////
//
// General reset
//
void PcwSystem::reset(void)
{
	char buf[50];
	m_resetPending = false;
	for (int n = 0; n < m_devCount; n++) 
	{
		sprintf(buf, "PcwSystem::reset(%d)", n);
		XLOG(buf);

		m_devices[n]->reset();
	}
	XLOG("Reset complete");
	// Switch to PCW terminal, that's what the boot ROM will be using
	selectTerminal(m_defaultTerminal);	
}

void PcwSystem::setResetFlag(bool b)
{
	m_resetPending = b;
}


//////////////////////////////////////////////////////////////////////////////
//
// Bring up settings pages
//
UiEvent  PcwSystem::settings(UiDrawer *d, int x, int y)
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
			uie = m_devices[map[n]]->settings((UiDrawer *)m_termMenu);
			if (uie == UIE_QUIT) return uie;
		}			
	} while(1);	
	return UIE_OK;
}

///////////////////////////////////////////////////////////////////////////////
//
// Event handling
//
int PcwSystem::eventLoop(void)
{
	SDL_Event e;
	int evid, rv;

	do
	{
		evid = SDL_PollEvent(&e);
		if (!evid) break;
/* Record mouse coords and button state */	
		switch(e.type)
		{
			case SDL_MOUSEMOTION:
				m_mouseX   = e.motion.x;
				m_mouseY   = e.motion.y;
				m_mouseBtn = e.motion.state;
				break;
			case SDL_MOUSEBUTTONDOWN:
				m_mouseBtn |= SDL_BUTTON(e.button.button);
				break;
			case SDL_MOUSEBUTTONUP:
				m_mouseBtn &= ~SDL_BUTTON(e.button.button);
				break;
		}

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
void PcwSystem::zputs(word W, const char *s)
{
        int n = 0;

        while (n < 16 && (*s))
        {
                if (*s < ' ') m_cpu->storeB(W++, ' ');
                else m_cpu->storeB(W++, *s);
                ++s;
                ++n;
        }
        while (n < 16)
        {
                m_cpu->storeB(W++, ' ');
                ++n;
        }
}



// Load the initial boot ROM (BOOTFILE.EMJ).
bool PcwSystem::loadBoot(byte *memory, string bootfile)
{
        FILE *fp;
/* PCW16s can have boot ROMs bigger than 64k */ 
        unsigned long address = 0;
        int c;
//
// Do we have a cached copy of the boot ROM?
//
	if (m_bootRom)
	{
		memcpy(memory, m_bootRom, m_bootRomLen);
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
        while ((c = getc(fp)) != EOF) memory[address++] = c;
	memcpy(m_bootRom, memory, m_bootRomLen);

        fclose(fp);
	return true;
}

///////////////////////////////////////////////////////////////////////////////
//
// Support for the various PCW models
//
void PcwSystem::setModel(PcwModel j)
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
void PcwSystem::setDebug(bool b)
{	
	if (m_debugMode == b) return;
	m_debugMode = b;
	m_activeTerminal->onDebugSwitch(b);
	if (b) m_termDebug.onGainFocus();
	else   m_termDebug.onLoseFocus();
}


///////////////////////////////////////////////////////////////////////////////
//
// System init
//
bool PcwSystem::sysInit(void)
{
	int memsiz;

	XLOG("PcwSystem::sysInit()");
	if (!initVideo())
	{
                alert("Sorry, could not select 800x600 graphics mode.\n"
                      "JOYCE needs at least 800x600 8-bit graphics.");
		return false;
	}
	XLOG("Setting debug mode");
	setDebug(m_args->m_debug);
	extern_stop = 0;	// XXX global
	XLOG("Resetting the CPU");
	m_cpu->reset();
	// Load the boot ROM.
	XLOG("Loading the boot ROM");
	m_mem->reset();
	XLOG("PcwSystem::sysInit() complete");
	return true;
}

