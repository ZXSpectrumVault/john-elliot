
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

#include "Anne.hxx"
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
#include "AnneFdc.hxx"

//
// Namespace for the hardware definitions
//
#define XMLNS             "http://www.seasip.demon.co.uk/annecfg.dtd"
#define HW_FILE           "annehw.xml" 
//
// Namespace for the boot ROM list
//
#define CFGNS ((xmlChar *)"http://www.seasip.demon.co.uk/annebt.dtd")
#define BOOT_FILE         "annebt.xml" 
//
//
// This class is supposed to encapsulate the entire PcW16 system. 
//

AnneSystem::AnneSystem(AnneArgs *a) : PcwSystem(a, HW_FILE, XMLNS),
				m_keyb(this),
				m_accel(this),
				m_mouse(this),
				m_com1(0x28, this),
				m_com3(0x20, this)
{
	m_cpu = new AnneZ80(this);
	m_mem = new AnneMemory();
// Need to init m_termMenu before m_fdc
	m_termMenu = new AnneMenuTermPcw(this);
	m_fdc = new AnneFDC(this);
	m_asic = new AnneAsic(this);
        m_defaultTerminal = new AnneTerminal(&m_beeper);
        m_activeKeyboard = &m_keyb;
	m_activeMouse    = &m_mouse;
	addDevice(m_fdc);
        addDevice(m_mem);
	addDevice(m_asic);
	addDevice(&m_accel);
        addDevice(m_defaultTerminal);
	addDevice(m_termMenu);
        addDevice(&m_keyb);
	addDevice(&m_mouse);
	addDevice(&m_com1);
	addDevice(&m_com3);
	addDevice(&m_lpt1);
	addDevice(&m_rtc);
	m_romGood = false;
        m_bootcfg = xmlNewDoc((xmlChar *)"1.0");
        if (m_bootcfg)
        {
                xmlNsPtr ns;
                xmlNodePtr root;

                root = xmlNewNode(NULL, (xmlChar *)"bootlist");
                xmlDocSetRootElement(m_bootcfg, root);

                ns = xmlNewNs(root, CFGNS, NULL);
        }
	m_fdc->disableDrive(1);
	m_fdc->disableDrive(2);
	m_fdc->disableDrive(3);
}

AnneSystem::~AnneSystem()
{
	/* Save the last known good ROM file */
	if (m_romGood)
	{
		long len = ((AnneMemory *)m_mem)->getFlashActual();
		FILE *fp = openPcwFile(FT_BOOT, "annelast.rom", "wb"); 
		fwrite(PCWFLASH, 1, len, fp);
		fclose(fp);
	}
	delete m_defaultTerminal;
}





bool AnneSystem::sysInit(void)
{
        loadHardware();
        parseHardware();
	loadBootList();
	parseBootList();
        int memsiz = m_mem->alloc(m_args);
        if (!memsiz)
        {
                alert("Not enough memory to run ANNE.");
                return false;
        }
        printf("ANNE will emulate a PcW 16 with %d Mb memory\n",
			memsiz);
        fflush(stdout);

	if (!PcwSystem::sysInit()) return false;

	if (!((AnneTerminal *)m_defaultTerminal)->loadBitmaps()) return false;

        if (m_args->m_aDrive != "")
        {
                m_fdc->insertA(m_args->m_aDrive);
                m_args->m_aDrive = "";
        }
	// Zap everything outside the boot ROM. If this is not done,
	// it will loop forever checksumming the entire 4Mb of RAM.
	//moved to AnneMemory.cxx
	//memset(PCWFLASH + 65536, 0x01, 1048576 - 65536);
// All "outside functions" are regulated in ANNE by a
// simulated 900Hz clock. For a 16MHz CPU, this is every
// 16e6/9e2 = 17777 cycles.
//

	m_cpu->setCycles(17777);
	m_cpu->reset();

	// If there was no -e option, then 
        if (m_bootList.size() == 0 && m_args->m_bootRom == "")
        {
                int nc;
                UiEvent uie;
                UiDrawer *d = m_termMenu;
                UiMenu uim(d);

                uim.add(new UiLabel("  No PCW16 boot ROMs have been set up.  ", d));
                uim.add(new UiSeparator(d));
                uim.add(new UiCommand(SDLK_s, "  Set up ANNE from the PCW16 recovery disc  ", d));
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
                                        m_termMenu->report("No boot ROMs added.", "  Continue  ");
                                }
                                if (nc < 0)
                                {
                                        deinitVideo();
                                        joyce_dprintf("ROM conversion cancelled.\n");
                                        return false;
                                }
				reset();
				break;
		}
	}
	if (m_args->m_bootRom != "") 
	{
		m_bootfile = m_args->m_bootRom;
	}
	else
	{
		UiEvent uie = chooseBootRom(m_termMenu, m_bootfile);
                if (uie == UIE_QUIT || uie == UIE_CANCEL) return false;
	}
	loadBoot(PCWFLASH, m_bootfile);
	m_accel.patch(PCWFLASH);		
	m_romGood = true;
	return true;
}


void AnneSystem::reset(void)
{
        PcwSystem::reset();
        if (m_bootfile != "") loadBoot(PCWFLASH, m_bootfile);
	m_accel.patch(PCWFLASH);		
}


void AnneSystem::powerPress(void)
{
	m_com1.setPower(true);
}


void AnneSystem::powerRelease(void)
{
	m_com1.setPower(false);
}



bool AnneSystem::loadBootList()
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



bool AnneSystem::parseBootList()
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
                if (!strcmp((char *)cur->name, "bootrom") && (cur->ns == ns))
                {
			s = xmlGetProp(cur, (xmlChar *)"id");
		        if (!s || s[0] < '0' || s[0] > '9')
		        {
                		joyce_dprintf("<bootrom> must have an id= field\n");
               			if (s) free(s);
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

bool AnneSystem::storeBootList()
{
	int n;
	for (n = 0; n < (int)m_bootList.size(); n++)
	{
		if (!storeBootEntry(n))	return false;
	}
	return true;
}


bool AnneSystem::storeBootEntry(int n)
{
	xmlNodePtr cur  = xmlDocGetRootElement(m_bootcfg);
	xmlNsPtr   ns   = xmlSearchNsByHref(m_bootcfg, cur, CFGNS);
	BootListEntry e = m_bootList[n];
	xmlChar *s;
	xmlNodePtr newText = xmlNewText((xmlChar *)e.m_title.c_str());
	xmlNodePtr newNode = xmlNewNode(ns, (xmlChar *)"bootrom");
	int found = 0;
	char ids[20];

	xmlAddChild(newNode, newText);
	sprintf(ids, "%d", e.m_id);
	xmlSetProp(newNode, (xmlChar *)"id", (xmlChar *)ids);

	if (!cur) return false;
        for (cur = cur->xmlChildrenNode; cur != NULL; cur=cur->next)
        {
                if (!strcmp((char *)cur->name, "bootrom") && (cur->ns == ns))
                {
			s = xmlGetProp(cur, (xmlChar *)"id");
		        if (!s || s[0] < '0' || s[0] > '9')
		        {
                		joyce_dprintf("<bootrom> must have an id= field\n");
               			if (s) free(s);
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


void AnneSystem::getBootID(Z80 *R)
{
	/* Special boot support not present. */
	if (R->b == 0xFF)
	{
		R->a = 0;
		return;
	}
        R->a = 0;
        for (int n = 0; n < m_bootList.size(); n++)
        {
                if (m_bootList[n].m_id == R->b)
                {
                        R->a = 1;
                        zputs(R->getHL(), m_bootList[n].m_title.c_str());
                }
        }
}



bool AnneSystem::saveBootList()
{
	string filename;

	if (!m_bootcfg) return false;

        filename = findPcwFile(FT_BOOT, BOOT_FILE, "w"); 

        if (xmlSaveFile(filename.c_str(), m_bootcfg) < 0) return 0;
        return 1;
}


UiEvent AnneSystem::chooseBootRom(UiDrawer *d, string &bootfile)
{
	UiMenu uim(d);
	UiEvent uie;
	SDLKey k;
	char nbr[20];
	string s;
	int sel, id, n;

	uim.add(new UiLabel("  Boot from:  ", d));
	uim.add(new UiSeparator(d));
	uim.add(new UiCommand(SDLK_EQUALS, "  Previous session", d));
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
	if (k == SDLK_EQUALS)
	{
		bootfile = findPcwFile(FT_BOOT, "annelast.rom", "rb");
		return UIE_OK;
	}
	if (k == SDLK_SPACE)
	{
		string filename = findPcwDir(FT_BOOT, false);
		PcwFileChooser f("  OK  ", d);
		f.initDir(filename);
                uie = f.eventLoop();
		if (uie == UIE_QUIT) return uie;
                bootfile = f.getChoice();
		return UIE_OK;
	}


	id = m_bootList[sel - 3].m_id;
	bootfile = idToFilename(id);

	return UIE_OK;
}



UiEvent AnneSystem::chooseBootID(UiDrawer *d, const string prompt, int *bootid)
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









string AnneSystem::idToFilename(int bootid)
{
	string bootfile;
	char nbr[20];

        sprintf(nbr, "%d", bootid);
        bootfile = "anne";
        bootfile += nbr;
        bootfile += ".rom";
        bootfile = findPcwFile(FT_BOOT, bootfile, "rb");
	return bootfile;
}

int AnneSystem::lookupId(int bootid)
{
	unsigned n;
	for (n = 0; n < m_bootList.size(); n++)
		if (m_bootList[n].m_id == bootid) return n;
	return -1;	
}

bool AnneSystem::deleteBootEntry(int bootid)
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
		if (strcmp((char *)cur->name, "bootrom")) continue;
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

