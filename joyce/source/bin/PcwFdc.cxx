/************************************************************************

    JOYCE v2.1.8 - Amstrad PCW emulator

    Copyright (C) 1996, 2001, 2003, 2005  John Elliott <seasip.webmaster@gmail.com>

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
#include "PcwFolderChooser.hxx"
#include "Path.hxx"
#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif

static PcwMenuTerm *gl_termMenu;

static void gl_report_on(const char *progress)
{
	if (!gl_termMenu) return;
	gl_termMenu->popupRedraw(progress, 1);
}

static void gl_report_off()
{
	if (!gl_termMenu) return;
	gl_termMenu->popupOff();
}


PcwFDC::PcwFDC(PcwSystem *s) : PcwDevice("fdc")
{
	int n;
	XLOG("PcwFDC::PcwFDC()");

	m_sys = s;

	m_fdc = fdc_new();
	for (n = 0; n < 4; n++)
	{
		m_driveType[n] = FD_35;
		m_driveHeads[n] = 2;
		m_driveCyls[n] = 83;
		m_driveMap[n] = (n % 2);

		m_fdl[n]     = NULL;
		m_fd_none[n] = NULL;
		m_fd_9256[n] = NULL;
	}
	m_driveMap[2] = 10;	// nc9 model
	m_driveMap[3] = 11;
	m_bootRO = false;

	gl_termMenu = m_sys->m_termMenu;
	dsk_reportfunc_set(gl_report_on, gl_report_off);	
	reset();	
}

PcwFDC::~PcwFDC()
{
	dsk_reportfunc_set(NULL, NULL);
	// 2.1.6 Call eject methods on all 4 drives before
	// destroying them, in case 2 drives are aliased.
        for (int n = 0; n < 4; n++)
        {
                fd_eject(fdc_getdrive(m_fdc, n));
	}
        for (int n = 0; n < 4; n++)
        {
		fd_destroy(&m_fdl    [n]);
		fd_destroy(&m_fd_none[n]);
		fd_destroy(&m_fd_9256[n]);
        }
	fdc_destroy(&m_fdc);

}

static PcwFDC *theFDC;

static void PcwFdcIsr(FDC_PTR fdc, int status)
{
	theFDC->isr(fdc, status);
}


void PcwFDC::isr(FDC_PTR fdc, int status)
{
        if (m_fdcInt != status)
	{	
//                joyce_dprintf("FDC interrupt goes to %d\n", status);
		m_fdcInt = status;
	}
	if (m_sys->m_asic)
	{
        	m_sys->m_asic->fdcIsr(status);	
	}
}


void PcwFDC::setTerminalCount(bool b)
{
	m_terminalCount = b;
	fdc_set_terminal_count(m_fdc, b);
}

void PcwFDC::setMotor(byte b)
{
	fdc_set_motor(m_fdc, b);
}


void PcwFDC::writeData(byte b)
{
	fdc_write_data(m_fdc, b); 
}


void PcwFDC::writeDOR(int b)
{
	fdc_write_dor(m_fdc, b);
}

void PcwFDC::writeDRR(byte b)
{
	fdc_write_drr(m_fdc, b);
}


byte PcwFDC::readDIR()
{	
//	byte b = fdc_read_dir(m_fdc);
//	fprintf(stderr, "DIR=%d\n", b);
	return fdc_read_dir(m_fdc);
}

byte PcwFDC::readData(void)
{
	return fdc_read_data(m_fdc);
}


byte PcwFDC::readControl(void)
{
	return fdc_read_ctrl(m_fdc);
}


void PcwFDC::tick(void)
{
        fdc_tick(m_fdc);  /* Check for end of FDC operation */
}


void PcwFDC::insertA(const string filename, const char *type)
{
	int err;

// implicit	fd_eject       (m_fdl[0]);
	fdl_setfilename(m_fdl[0], filename.c_str());
	fdl_settype    (m_fdl[0], type);
}



void PcwFDC::insertB(const string filename, const char *type)
{
	int err;

// implicit	fd_eject       (m_fdl[1]);
	fdl_setfilename(m_fdl[1], filename.c_str());
	fdl_settype    (m_fdl[1], type);
}



bool PcwFDC::loadBootSector(const string filename, const char *type, byte *sector)
{
	int err;

// implicit	fd_eject       (m_fdl[0]);
	fdl_setfilename(m_fdl[0], filename.c_str());
	fdl_settype    (m_fdl[0], type);
        joyce_dprintf("Loading %s\n", filename.c_str());
	fd_setreadonly (m_fdl[0], m_bootRO);

	setMotor(0x0F);

       // if (!fd_isready(&m_fd[0].fdd)) return 0;
        if (!fd_isready(m_fdl[0])) return 0;

        joyce_dprintf("Image file %s opened OK.\n", filename.c_str());

        //err = fd_seek_cylinder(&m_fd[0].fdd, 0);
        err = fd_seek_cylinder(m_fdl[0], 0);

        if (err)
        {
                joyce_dprintf("Seek error while booting: %d\n", err);
                return false;       /* Track 0 */
        }
        //err = fd_read_sector(&m_fd[0].fdd, 0,0,0,1, sector, 512);
        int deleted = 0;
        err = fd_read_sector(m_fdl[0], 0,0,0,1, sector, 512, &deleted, 1, 1, 0);
        if (err)
        {
                joyce_dprintf("Read error while booting: %d\n", err);
                return false;       /* Could not read sector */
        }
	return true;
}





void PcwFDC::reset(void)
{
	// eject all drives (thus, hopefully, committing changes)
        for (int n = 0; n < 4; n++)
        {
                fd_eject(fdc_getdrive(m_fdc, n));
        }
	reset(0);
}

void PcwFDC::reset(int somedrives)
{
	int n, idx;

	if (!somedrives)
	{
		theFDC = this;
		m_fdcInt = false;
		fdc_reset(m_fdc);
		fdc_setisr(m_fdc, PcwFdcIsr);
		m_terminalCount = false;
	}
	for (n = somedrives; n < 4; n++)
	{
                //fdd_init      (&m_fdd[n]);
		if (m_fdl[n])     fd_destroy(&m_fdl[n]);
		if (m_fd_none[n]) fd_destroy(&m_fd_none[n]);
		if (m_fd_9256[n]) fd_destroy(&m_fd_9256[n]);
		     
		m_fdl[n]     = fd_newldsk();
		m_fd_none[n] = fd_new();
		m_fd_9256[n] = fd_newnc9(NULL);
      
                fd_settype (m_fdl[n], m_driveType[n]);
                fd_setheads(m_fdl[n], m_driveHeads[n]);
                fd_setcyls (m_fdl[n], m_driveCyls[n]);

		idx = m_driveMap[n];
		switch(idx)
		{
			case 0: case 1: case 2: case 3:
        		fdc_setdrive(m_fdc, n, m_fdl[idx]); break;
			case 4: case 5: case 6: case 7:
        		fdc_setdrive(m_fdc, n, m_fd_none[idx - 4]); break;
			case 8: case 9: case 10: case 11:
        		fdc_setdrive(m_fdc, n, m_fd_9256[idx - 8]); break;
			case 12: case 13: case 14: case 15:
        		fdc_setdrive(m_fdc, n, fdc_getdrive(m_fdc, idx - 12)); break;
		}	
	}
	// Make the 9256 disconnected drives 2,3 use unit 1 as their proxy.
	for (n = 2; n < 4; n++) 
		fd9_setproxy(m_fd_9256[n], fdc_getdrive(m_fdc, 1));
}


// Parsing XML configuration data...

bool PcwFDC::parseNode(xmlDocPtr doc, xmlNsPtr ns, xmlNodePtr cur)
{
        int bootro = 0;
        
	// Boot drive R/O setting
	char *s = (char *)xmlGetProp(cur, (xmlChar *)"bootro");

        if      (s && !strcmp(s,"1")) bootro = 1;
        else if (s && strcmp(s,"0")) 
		joyce_dprintf("FDC boot read-only must be 0 or 1 - not %s\n", s);
        if (s) xmlFree(s);

        m_bootRO = bootro;

	// Settings for individual drives
	cur = cur->xmlChildrenNode;
	while (cur != NULL)
	{
		if (!strcmp((char *)cur->name, "drive") && (cur->ns == ns))
		{
			if (!parseDriveNode(doc, ns, cur)) return false;
		}
		cur = cur->next;
	}
	return true;
}



bool PcwFDC::parseDriveNode(xmlDocPtr doc, xmlNsPtr ns, xmlNodePtr cur)
{
	char *s, *t, *type, *cyls, *heads;
	int id = -1, model = -1, as = -1;

	s = (char *)xmlGetProp(cur, (xmlChar *)"id");
        if (!s || s[1] != 0 || s[0] < '0' || s[0] > '3')
	{
        	joyce_dprintf("<drive> must have an id= field\n");
		return true;
	}
	id = atoi(s);
	if (s) xmlFree(s);	

	s = (char *)xmlGetProp(cur, (xmlChar *)"model");
	if (s && !strcmp(s, "dsk")) model = 1;
	if (s && !strcmp(s, "nc8")) model = 2;
	if (s && !strcmp(s, "nc9")) model = 3;
	if (s && !strcmp(s, "same")) model = 4;

	if (model < 1) joyce_dprintf("<drive> %d has unrecognised "
					"model type %s\n", s);
	else switch(model)
	{
		case 1: /* real drive */
		type  = (char *)xmlGetProp(cur, (xmlChar *)"type");
		cyls  = (char *)xmlGetProp(cur, (xmlChar *)"cylinders");
		heads = (char *)xmlGetProp(cur, (xmlChar *)"heads");
		fd_destroy(&m_fdl[id]);
		m_fdl[id] = fd_newldsk();
		fdc_setdrive(m_fdc, id, m_fdl[id]);
		m_driveMap[id] = id;

		if (cyls) 
		{ 
			fd_setcyls(m_fdl[id], m_driveCyls[id] = atoi(cyls));
		}
		if (heads) 
		{ 
			fd_setheads(m_fdl[id], m_driveHeads[id] = atoi(heads));
		}
		if (type)
		{
			if (!strcmp(type, "3.5"))  
				fd_settype(m_fdl[id], m_driveType[id] = FD_35);	
			if (!strcmp(type, "5.25")) 
				fd_settype(m_fdl[id], m_driveType[id] = FD_525);	
			if (!strcmp(type, "3.0"))  
				fd_settype(m_fdl[id], m_driveType[id] = FD_30);	
		}
		if (type)  xmlFree(type);
		if (heads) xmlFree(heads);
		if (cyls)  xmlFree(cyls);
		break;

		case 2: /* nc8 */
		m_driveMap[id] = id + 4;
		fd_destroy(&m_fd_none[id]);
		m_fd_none[id] = fd_new();
		fd_settype(m_fd_none[id], FD_NONE);
                fdc_setdrive(m_fdc, id, m_fd_none[id]);
		break;

		case 3:	/* nc9 */
		m_driveMap[id] = id + 8;
		fd_destroy(&m_fd_9256[id]);
		m_fd_9256[id] = fd_newnc9(NULL);
		fd_settype(m_fd_9256[id], FD_NC9256);
                fdc_setdrive(m_fdc, id, m_fd_9256[id]);
		break;

		case 4: /* Same as */
		t = (char *)xmlGetProp(cur, (xmlChar *)"ref");
		if (t) 
		{ 
			as = atoi(t);
			xmlFree(t);
		}
		if (as >= 0 && as <= 3) m_driveMap[id] = 12 + as;
		fdc_setdrive(m_fdc, id, fdc_getdrive(m_fdc, as));
		break;

	}
	if (s) xmlFree(s);
	return true;
}



bool PcwFDC::storeNode(xmlDocPtr doc, xmlNsPtr ns, xmlNodePtr cur)
{
	bool some;
	char buf[20];
	xmlNodePtr drv;

	xmlSetProp(cur, (xmlChar *)"bootro", (xmlChar *)(m_bootRO ? "1" : "0"));

	for (int n = 0; n < 4; n++)
	{
		some = false;
                sprintf(buf, "%d", n);
		// Search for existing matching <drive> node.
                for (drv = cur->xmlChildrenNode; drv; drv = drv->next)
                {
			if (strcmp((char *)drv->name, "drive")) continue;

			char *s = (char *)xmlGetProp(drv, (xmlChar *)"id");
			if (!s) continue;
			if (strcmp(s, buf)) 
			{
				xmlFree(s); 
				continue;
			}
			xmlFree(s);
			some = true;
			storeDriveNode(doc, ns, drv, n);
                }
		// No existing <drive> node; create one.
		if (!some)
		{
			drv = xmlNewNode(ns, (xmlChar *)"drive");
			xmlSetProp(drv, (xmlChar *)"id", (xmlChar *)buf);
			xmlAddChild(cur, drv);
			storeDriveNode(doc, ns, drv, n);
		}
	}
	return true;
}


bool PcwFDC::storeDriveNode(xmlDocPtr doc, xmlNsPtr ns, xmlNodePtr cur, int n)
{
	char buf[20];

	xmlSetProp(cur, (xmlChar *)"cylinders", NULL);
	xmlSetProp(cur, (xmlChar *)"heads",     NULL);
	xmlSetProp(cur, (xmlChar *)"type",      NULL);
	xmlSetProp(cur, (xmlChar *)"ref",       NULL);

	switch(m_driveMap[n])
	{
		case  0: case  1: case  2: case  3:
		xmlSetProp(cur, (xmlChar *)"model", (xmlChar *)"dsk");
		sprintf(buf, "%d", m_driveCyls[n]);
		xmlSetProp(cur, (xmlChar *)"cylinders", (xmlChar *)buf);
                sprintf(buf, "%d", m_driveHeads[n]);
                xmlSetProp(cur, (xmlChar *)"heads", (xmlChar *)buf);
                switch(m_driveType[n])
                {
                	case FD_35:  xmlSetProp(cur, (xmlChar *)"type", (xmlChar *)"3.5");  break;
			case FD_30:  xmlSetProp(cur, (xmlChar *)"type", (xmlChar *)"3.0");  break;
			case FD_525: xmlSetProp(cur, (xmlChar *)"type", (xmlChar *)"5.25"); break;
		}
		break;	

		case  4: case  5: case  6: case  7:
		xmlSetProp(cur, (xmlChar *)"model", (xmlChar *)"nc8");
		break;

		case  8: case  9: case 10: case 11:
		xmlSetProp(cur, (xmlChar *)"model", (xmlChar *)"nc9");
		xmlSetProp(cur, (xmlChar *)"ref",   (xmlChar *)"1");
		break;

		case 12: case 13: case 14: case 15:
		sprintf(buf, "%d", m_driveMap[n] - 12);
		xmlSetProp(cur, (xmlChar *)"model", (xmlChar *)"same");
		xmlSetProp(cur, (xmlChar *)"ref",   (xmlChar *)buf);
		break;
	}
	return true;
}



bool PcwFDC::hasSettings(SDLKey *key, string &caption)
{
        *key = SDLK_d;
        caption = "  Disc drives  ";
        return true;
}

UiEvent PcwFDC::settings(UiDrawer *d)
{
	UiEvent uie;
	bool is8256, is9256, is16, advanced;
	int sel = -1;
        int driveType[4];
        int driveHeads[4];
        int driveCyls[4];
        int driveMap[4];
	int n, changes;

	for (n = 0; n < 4; n++) 
	{
		driveType[n]  = m_driveType[n];
		driveHeads[n] = m_driveHeads[n];
		driveCyls[n]  = m_driveCyls[n];
		driveMap[n]   = m_driveMap[n];
	}
	
	while (1)
	{
		UiMenu uim(d);

		is16 = is9256 = is8256 = advanced = false;
		if (m_driveMap[2] == 10 && m_driveMap[3] == 11) is9256 = true;
		if (m_driveMap[2] == 12 && m_driveMap[3] == 13) is8256 = true; 
		if (m_driveMap[1] == 5 && m_driveMap[2] == 6 && m_driveMap[3] == 7) is16 = true;
		// Drives C and D are mapped in a nonstandard way...
		if (is8256 == 0 && is9256 == 0 && is16 == 0) advanced = true;

		uim.add(new UiLabel  ("         Disc drives         ", d));
		uim.add(new UiSeparator(d));
		if (!is16)
		{
			uim.add(new UiSetting(SDLK_w, !m_bootRO, "  Boot discs are read/Write  ",  d));
			uim.add(new UiSetting(SDLK_o,  m_bootRO, "  Boot discs are read-Only   ",  d));
			uim.add(new UiSeparator(d));
			uim.add(new UiSetting(SDLK_8, is8256, "  8256/8512/9512 controller  ",  d));
			uim.add(new UiSetting(SDLK_9, is9256, "  9256/9512+/10  controller  ",  d));
		}
		else
		{
			uim.add(new UiSetting(SDLK_6, is16, "  PCW16 system  ",  d));
		}
		uim.add(new UiSeparator(d));
		if (advanced) 
		{
			uim.add(new UiCommand(SDLK_0, "  Drive 0 settings... ", d, UIG_SUBMENU));	
			uim.add(new UiCommand(SDLK_1, "  Drive 1 settings... ", d, UIG_SUBMENU));	
			uim.add(new UiCommand(SDLK_2, "  Drive 2 settings... ", d, UIG_SUBMENU));	
			uim.add(new UiCommand(SDLK_3, "  Drive 3 settings... ", d, UIG_SUBMENU));	
		}
		else
		{
			uim.add(new UiCommand(SDLK_a, "  Drive A: settings... ", d, UIG_SUBMENU));	
			if (!is16) uim.add(new UiCommand(SDLK_b, "  Drive B: settings... ", d, UIG_SUBMENU));	
		}
		uim.add(new UiSeparator(d));
		uim.add(new UiCommand(SDLK_ESCAPE, "  EXIT  ", d));
		d->centreContainer(uim);
		uim.setSelected(sel);
		uie = uim.eventLoop();

		if (uie == UIE_QUIT) return uie;
		switch(uim.getKey(sel = uim.getSelected()))
		{
			case SDLK_w: m_bootRO = false; break;
			case SDLK_o: m_bootRO = true;  break;
			case SDLK_9: m_driveMap[2] = 10; m_driveMap[3] = 11; break;
			case SDLK_8: m_driveMap[2] = 12; m_driveMap[3] = 13; break;
			case SDLK_6: m_driveMap[1] = 5;  m_driveMap[2] = 6; m_driveMap[3] = 7; break;
			case SDLK_0:	
			case SDLK_a: uie = driveSettings(0, advanced, d); 
				     if (uie == UIE_QUIT) return uie;
				     break;
			case SDLK_1:	
			case SDLK_b: uie = driveSettings(1, advanced, d); 
				     if (uie == UIE_QUIT) return uie;
				     break;
			case SDLK_2: uie = driveSettings(2, advanced, d); 
				     if (uie == UIE_QUIT) return uie;
				     break;
			case SDLK_3: uie = driveSettings(3, advanced, d); 
				     if (uie == UIE_QUIT) return uie;
				     break;

			case SDLK_ESCAPE:
//
// See if settings have changed. If so, rewire the FDC. 
//
					changes = 0;
					for (n = 0; n < 4; n++) 
					{
						changes |= (driveType[n]  != m_driveType[n]);
		                                changes |= (driveHeads[n] != m_driveHeads[n]);
		                                changes |= (driveCyls[n]  != m_driveCyls[n]);
		                                changes |= (driveMap[n]   != m_driveMap[n]);
					}
					if (changes)
					{
        					for (n = 0; n < 4; n++)
                					fd_eject(fdc_getdrive(m_fdc, n));
						reset();
						// Update the PCW type in PcwSystem.
						int XXX;
//						m_sys->setModel(adaptModel(m_sys->getModel()));
					}
					return UIE_OK;
			default:	break;
		}	// end switch
	}		// end while
	return UIE_OK;
}

UiEvent PcwFDC::driveSettings(int unit, bool advanced, UiDrawer *d)
{
	char buf[40];
	int sel = -1;
	bool fd1, fd2, fd35, fdother, fdnc9, fdnc8, fdsameas;
	UiEvent uie;

	while (1)
	{	
		fd1 = fd2 = fd35 = fdother = fdnc9 = fdnc8 = fdsameas = false;
		if (m_driveMap[unit] < 4)
		{
			if (m_driveType[unit] == FD_30 && m_driveHeads[unit] == 1 &&
			    m_driveCyls[unit] >= 40 && m_driveCyls[unit] <= 43) fd1 = true;
			if (m_driveType[unit] == FD_30 && m_driveHeads[unit] == 2 &&
			    m_driveCyls[unit] >= 80 && m_driveCyls[unit] <= 86) fd2 = true;
			if (m_driveType[unit] == FD_35 && m_driveHeads[unit] == 2 &&
			    m_driveCyls[unit] >= 80 && m_driveCyls[unit] <= 86) fd35 = true;
		}
		if (m_driveMap[unit] >= 4  && m_driveMap[unit] < 8) fdnc8 = true;
		if (advanced)
		{
			if (m_driveMap[unit] >= 8  && m_driveMap[unit] < 12) fdnc9 = true;
			if (unit > 0 && m_driveMap[unit] >= 12 && m_driveMap[unit] < 16) fdsameas = true;
		}
		if (!fd35 && !fd2 && !fd1 && !fdnc9 && !fdnc8	
			&& !fdsameas) fdother = true;

		UiMenu uim(d);
	
		if (advanced) sprintf(buf, "   Unit %d settings   ", unit);
		else	      sprintf(buf, "  Drive %c: settings  ", unit + 'A');
	
		uim.add(new UiLabel(buf, d));
		uim.add(new UiSeparator(d));
		if (unit || advanced) 
			uim.add(new UiSetting(SDLK_n, fdnc8,   "  Not connected     ", d));
		uim.add(new UiSetting(SDLK_1, fd1,     "  180k 3\" drive    ", d));
		uim.add(new UiSetting(SDLK_2, fd2,     "  720k 3\" drive    ", d));
		uim.add(new UiSetting(SDLK_3, fd35,    "  720k 3.5\" drive  ", d));
		uim.add(new UiSetting(SDLK_o, fdother, "  Other...          ", d));
		if (advanced)
		{
			sprintf(buf, "  Same as unit %d  ", m_driveMap[unit] % 4);
			uim.add(new UiSetting(SDLK_9, fdnc9,    "  9256 not connected  ", d));
			if (unit > 0) uim.add(new UiSetting(SDLK_s, fdsameas, buf, d));
		}
		uim.add(new UiSeparator(d));
		uim.add(new UiCommand(SDLK_ESCAPE, "  EXIT  ", d));	
		d->centreContainer(uim);
		uim.setSelected(sel);
		uie = uim.eventLoop();
		if (uie == UIE_QUIT) return uie;
		sel = uim.getSelected();
		switch(uim.getKey(sel))
		{
			case SDLK_n: m_driveMap [unit] = unit + 4;
				     m_driveType[unit] = FD_NONE;
				     break;
			case SDLK_9: m_driveMap [unit] = unit + 8;
				     m_driveType[unit] = FD_NC9256;
				     break;
			case SDLK_1: m_driveMap  [unit] = unit;
				     m_driveType [unit] = FD_30;
		                     m_driveHeads[unit] = 1;
				     m_driveCyls [unit] = 43;
				     break;
			case SDLK_2: m_driveMap  [unit] = unit;
				     m_driveType [unit] = FD_30;
		                     m_driveHeads[unit] = 2;
				     m_driveCyls [unit] = 83;
				     break;
			case SDLK_3: m_driveMap  [unit] = unit;
				     m_driveType [unit] = FD_35;
		                     m_driveHeads[unit] = 2;
				     m_driveCyls [unit] = 83;
				     break;
			case SDLK_o:	// Set up custom geometry
				    uie = driveCustom(unit, advanced, d);
				    if (uie == UIE_QUIT) return uie;
				    break;
			case SDLK_a:	// Same as
				    uie = driveSameAs(unit, d);
				    if (uie == UIE_QUIT) return uie;	
				    break;
			case SDLK_ESCAPE: return UIE_CONTINUE;
			default:	break;
		}
	}	
	return UIE_CONTINUE;
}


UiEvent PcwFDC::driveCustom(int unit, bool advanced, UiDrawer *d)
{
	int sel = -1;
	char buf[20];
	UiEvent uie;

	while (1)
	{
		UiMenu uim(d);
		UiTextInput *t;

		bool b3, b35, b525, bs, bd;
		b3 = b35 = b525 = bs = bd = false;

		if (m_driveType[unit] == FD_30)  b3   = true;
		if (m_driveType[unit] == FD_35)  b35  = true;
		if (m_driveType[unit] == FD_525) b525 = true;
		if (m_driveHeads[unit] == 1) bs = true;		
		if (m_driveHeads[unit] == 2) bd = true;		

		uim.add(new UiSetting(SDLK_3,   b3,   "  3\" drive  ", d));
		uim.add(new UiSetting(SDLK_PERIOD, b35,  "  3.5\" drive  ", d));
		uim.add(new UiSetting(SDLK_5,   b525, "  5.25\" drive  ", d));
		uim.add(new UiSeparator(d));
		uim.add(t = new UiTextInput(SDLK_c, "  Number of cylinders: ___ ", d));
		uim.add(new UiSetting(SDLK_s,   bs, "  Single sided  ", d));
		uim.add(new UiSetting(SDLK_d,   bd, "  Double sided  ", d));
		uim.add(new UiSeparator(d));
		uim.add(new UiCommand(SDLK_ESCAPE, "  EXIT  ", d));

		sprintf(buf, "%d", m_driveCyls[unit]);
		t->setText(buf);	

		uim.setSelected(sel);
		d->centreContainer(uim);
		uie = uim.eventLoop();
		if (uie == UIE_QUIT) return uie;
		sel = uim.getSelected();
		switch(uim.getKey(sel))
		{
			case SDLK_3:   m_driveType[unit] = FD_30; break;
			case SDLK_PERIOD: m_driveType[unit] = FD_35; break;
			case SDLK_5:   m_driveType[unit] = FD_525; break;
			case SDLK_s:   m_driveHeads[unit] = 1; break;
			case SDLK_d:   m_driveHeads[unit] = 2; break;
			case SDLK_c:   m_driveCyls[unit] = atoi(t->getText().c_str()); break;
			case SDLK_ESCAPE: return UIE_CONTINUE;
			default: break;
		}	

	}
	return UIE_CONTINUE;
}


UiEvent PcwFDC::driveSameAs(int unit, UiDrawer *d)
{
	char buf[40];
	UiMenu uim(d);	
	UiEvent e;
	int sel;

	for (int n = 0; n < unit; n++)	
	{
		sprintf(buf, "  Unit %d  ", n);
		uim.add(new UiCommand((SDLKey)(SDLK_0 + n), buf, d));
	}
	sel = m_driveMap[unit] % 4;
	if (sel >= unit) sel = -1;

	uim.setSelected(sel);
	d->centreContainer(uim);
	e = uim.eventLoop();
	if (e != UIE_OK) return e;
	m_driveMap[unit] = 12 + uim.getSelected();	
	return e;
}

/*
PcwModel PcwFDC::adaptModel(PcwModel j)
{
	bool is8256, is9256;

	is9256 = is8256 = false;
	if (m_driveMap[2] == 10 && m_driveMap[3] == 11) is9256 = true;
	if (m_driveMap[2] == 12 && m_driveMap[3] == 13) is8256 = true; 

	if (is8256 && j == PCW10   ) return PCW8000;
	if (is8256 && j == PCW9000P) return PCW9000;
	if (is9256 && j == PCW8000 ) return PCW10;
	if (is9256 && j == PCW9000 ) return PCW9000P;
	return j;
}	


void PcwFDC::setModel(PcwModel j)
{
	bool is8256, is9256;
	int n;

	PcwDevice::setModel(j);

	is9256 = is8256 = false;
	if (m_driveMap[2] == 10 && m_driveMap[3] == 11) is9256 = true;
	if (m_driveMap[2] == 12 && m_driveMap[3] == 13) is8256 = true; 
	if (is8256 == 0 && is9256 == 0) return;	// Don't interfere with
						// custom setup.

	//
	// If there are changes, rewire drives 2 and 3 only.
	//
	if (is9256 && (j == PCW8000 || j == PCW9000))
	{
		m_driveMap[2] = 10; 
		m_driveMap[3] = 11;
        	for (n = 2; n < 4; n++) fd_eject(m_fdc.fdc_drive[n]);
		reset(2);
	}	
	if (is8256 && (j == PCW9000P || j == PCW10))
	{
		m_driveMap[2] = 12; 
		m_driveMap[3] = 13;
        	for (n = 2; n < 4; n++) fd_eject(m_fdc.fdc_drive[n]);
		reset(2);
	}	
		
}
*/


UiEvent PcwFDC::discsMenu(int x, int y, UiDrawer *d)
{
	UiEvent uie;
	int sel = -1;
	int nd;
	const char *dtitle[4] = { "  Drive A (", "  Drive B (", 
			    	  "  Unit 2 (",  "  Unit 3 (" };

	while (1)
	{
		UiMenu uim(d);

		for (nd = 0; nd < 4; nd++) if (m_driveMap[nd] < 4)
		{
			string title = dtitle[nd];
			if (nd) uim.add(new UiSeparator(d));
			if (fdl_getfilename(m_fdl[nd])[0]) 
			{
				title += displayName(fdl_getfilename(m_fdl[nd]), 40);
				title += ")  ";
				uim.add(new UiLabel(title, d));
				uim.add(new UiCommand((SDLKey)(SDLK_e + nd), "  Eject  ", d));
			}
			else
			{
				title += "Ejected)  ";
				uim.add(new UiLabel(title, d));
                         	uim.add(new UiCommand((SDLKey)(SDLK_i + nd), "  Insert...  ", d, UIG_SUBMENU));
			}
			uim.add(new UiSetting((SDLKey)(SDLK_r + nd), fd_getreadonly(m_fdl[nd]), "  Read only  ", d));
		}
		uim.add(new UiSeparator(d));
		uim.add(new UiCommand(SDLK_m, "  Disc management...  ", d, UIG_SUBMENU));
		uim.add(new UiSeparator(d));
		uim.add(new UiCommand(SDLK_ESCAPE, "  EXIT  ", d));	
		uim.setX(x);
		uim.setY(y);
		uim.setSelected(sel);
		uie = uim.eventLoop();
		if (uie == UIE_QUIT) return uie;
		sel = uim.getSelected();
		switch(uim.getKey(sel))
		{
			case SDLK_e: fd_eject(m_fdl[0]); break; 
			case SDLK_f: fd_eject(m_fdl[1]); break; 
			case SDLK_g: fd_eject(m_fdl[2]); break; 
			case SDLK_h: fd_eject(m_fdl[3]); break; 
			case SDLK_i:
			case SDLK_j:
			case SDLK_k:
			case SDLK_l:
			{
				nd = uim.getKey(sel) - SDLK_i;
				string str = fdl_getfilename(m_fdl[nd]);
				const char *type = fdl_gettype(m_fdl[nd]);
				if (str == "") str = findPcwDir(FT_DISC, false);
				uie = requestDsk(d, str, &type);
				if (uie == UIE_OK)
				{
// implicit				fd_eject(m_fdl[nd]); 
					fdl_setfilename(m_fdl[nd],str.c_str());
					fdl_settype(m_fdl[nd],type);	
				}				

			}
			break;
			case SDLK_m: uie = m_sys->discManager(d);
				     if (uie == UIE_QUIT) return uie;
				     break;
			case SDLK_r: fd_setreadonly(m_fdl[0], !fd_getreadonly(m_fdl[0])); break;
			case SDLK_s: fd_setreadonly(m_fdl[1], !fd_getreadonly(m_fdl[1])); break;
			case SDLK_t: fd_setreadonly(m_fdl[2], !fd_getreadonly(m_fdl[2])); break;
			case SDLK_u: fd_setreadonly(m_fdl[3], !fd_getreadonly(m_fdl[3])); break;


			case SDLK_ESCAPE: return UIE_OK;
			default: break;
		}	
	}

	return UIE_CONTINUE;
}




//
// When creating (or overriding) disc image file format, ask here.
// Note that there is a special version of this in PcwSetup.cxx with
// special-case code to read in MD3 boot discs.
//
UiEvent PcwFDC::getDskType(UiDrawer *d, int &fmt, bool create)
{
	UiEvent uie;
	int inMenu = 1;
	int sel = -1;
	int ofmt = fmt;

	while(inMenu)
	{
		UiMenu uim(d);

		uim.add(new UiLabel  ("      Drive type      ", d));
		uim.add(new UiSeparator(d));
		if (!create)
		{
			uim.add(new UiSetting(SDLK_0, (fmt == 0), "  Auto detect filetype  ", d));
		}
		uim.add(new UiSetting(SDLK_1, (fmt == 1), "  Floppy drive  ", d));
		uim.add(new UiSetting(SDLK_2, (fmt == 2), "  CPCEMU/JOYCE .DSK  ", d));
		uim.add(new UiSetting(SDLK_3, (fmt == 3), "  Raw disc image file  ", d));
		uim.add(new UiSetting(SDLK_4, (fmt == 4), "  MYZ80 hard drive  ", d));
		uim.add(new UiSetting(SDLK_5, (fmt == 5), "  .CFI - compressed raw   ", d));
		uim.add(new UiSetting(SDLK_6, (fmt == 6), "  Extended .DSK  ", d));
#ifdef HAVE_WINDOWS_H
		uim.add(new UiSetting(SDLK_7, (fmt == 7), "  Alternative floppy  ", d));
#endif
		uim.add(new UiSetting(SDLK_8, (fmt == 8), "  Folder  ", d));
		uim.add(new UiSeparator(d));
		uim.add(new UiCommand(SDLK_o, "  OK  ", d));
		uim.add(new UiCommand(SDLK_ESCAPE, "  Cancel  ", d));
		uim.setSelected(sel);
                d->centreContainer(uim);
		uie = uim.eventLoop();
		if (uie == UIE_QUIT || uie == UIE_CANCEL) 
		{
			fmt = ofmt;
			return uie;
		}
		sel = uim.getSelected();
		switch(uim.getKey(sel))
		{
			case SDLK_0: fmt = 0; break;
			case SDLK_1: fmt = 1; break;
			case SDLK_2: fmt = 2; break;
			case SDLK_3: fmt = 3; break;
			case SDLK_4: fmt = 4; break;
			case SDLK_5: fmt = 5; break;
			case SDLK_6: fmt = 6; break;
			case SDLK_7: fmt = 7; break;
			case SDLK_8: fmt = 8; break;
			case SDLK_ESCAPE: 
					fmt = ofmt;
					return UIE_CANCEL;
			case SDLK_o: inMenu = 0; break;
			default: break;
		}
	}
	return UIE_OK;
}

UiEvent PcwFDC::getFolderType(UiDrawer *d, int &fmt)
{
	UiEvent uie;
	int inMenu = 1;
	int sel = -1;

	fmt = 4;
	while(inMenu)
	{
		UiMenu uim(d);

		uim.add(new UiLabel  ("Setting up folder as PCW disc", d));
		uim.add(new UiLabel  (" What format should be used? ", d));
		uim.add(new UiSeparator(d));
		uim.add(new UiSetting(SDLK_1, (fmt == 0), "  173k PCW format  ", d));
		uim.add(new UiSetting(SDLK_2, (fmt == 1), "  169k CPC system  ", d));
		uim.add(new UiSetting(SDLK_3, (fmt == 2), "  178k CPC data  ", d));
		uim.add(new UiSetting(SDLK_4, (fmt == 3), "  192k PCW format  ", d));
		uim.add(new UiSetting(SDLK_5, (fmt == 4), "  706k PCW format  ", d));
		uim.add(new UiSetting(SDLK_6, (fmt == 5), "  784k PCW format  ", d));
		uim.add(new UiSeparator(d));
		uim.add(new UiCommand(SDLK_o, "  OK  ", d));
		uim.add(new UiCommand(SDLK_ESCAPE, "  Cancel  ", d));
		uim.setSelected(sel);
                d->centreContainer(uim);
		uie = uim.eventLoop();
		if (uie == UIE_QUIT || uie == UIE_CANCEL) 
		{
			return uie;
		}
		sel = uim.getSelected();
		switch(uim.getKey(sel))
		{
			case SDLK_1: fmt = 0; break;
			case SDLK_2: fmt = 1; break;
			case SDLK_3: fmt = 2; break;
			case SDLK_4: fmt = 3; break;
			case SDLK_5: fmt = 4; break;
			case SDLK_6: fmt = 5; break;
			case SDLK_ESCAPE: 
					return UIE_CANCEL;
			case SDLK_o: inMenu = 0; break;
			default: break;
		}
	}
	return UIE_OK;
}

typedef struct FolderFormat
{
	const char *name;
	int blockSize;
	int dirBlocks;
	int totalBlocks;
	int sysTracks;
	int bootType;
	int bootSidedness;
	int bootCyls;
	int bootSectors;
	int bootGapRW;
	int bootGapFMT;
} FolderFormat;

static const FolderFormat formats[] = 
{
	// Name     Blocksize  Dir blk  Blocks  Systrk  Boot vars
	{"pcw180",  1024,      2,       175,        1,  0,0x00,40, 9,0x2a,0x52},
	{"cpcsys",  1024,      2,       171,        2,  1,0x00,40, 9,0x2a,0x52},
	{"cpcdata", 1024,      2,       180,        0,  2,0x00,40, 9,0x2a,0x52},
	{"pcw200",  1024,      3,       195,        1,  0,0x00,40,10,0x0c,0x17},
	{"pcw720",  2048,      4,       357,        1,  3,0x81,80, 9,0x2a,0x52},
	{"pcw800",  4096,      2,       198,        1,  3,0x81,80,10,0x0c,0x17},
};


//
// Format a filesystem folder as a LibDsk disc image.
//
int PcwFDC::formatFolder(const string dirname, int fmt)
{
	byte bootBlock[512];
	string ininame = dirname + "/.libdsk.ini";
	FILE *fp;

#ifdef HAVE_WINDOWS_H
	if (mkdir(dirname.c_str()))
#else    
	if (mkdir(dirname.c_str(), 0755))
#endif
	{
		if (errno != EEXIST) return -1;
	}
	fp = fopen(ininame.c_str(), "w");
	if (!fp) return -1;
	fprintf(fp, ";File auto-generated by JOYCE %d.%d.%d\n",
			BCDVERS >> 8, ((BCDVERS >> 4) & 0x0F), BCDVERS & 0x0F);
	fprintf(fp, "[RCPMFS]\n");
	fprintf(fp, "Format=%s\n", formats[fmt].name);
	fprintf(fp, "BlockSize=%d\n", formats[fmt].blockSize);
	fprintf(fp, "DirBlocks=%d\n", formats[fmt].dirBlocks);
	fprintf(fp, "TotalBlocks=%d\n", formats[fmt].totalBlocks);
	fprintf(fp, "SysTracks=%d\n", formats[fmt].sysTracks);
	fprintf(fp, "Version=3\n");
	fclose(fp);
	// If the format has a boot track, write that.
	if (formats[fmt].sysTracks)
	{
		ininame = dirname + "/.libdsk.boot";
		fp = fopen(ininame.c_str(), "wb");
		if (!fp) return -1;

		memset(bootBlock, 0xE5, sizeof(bootBlock));
		memset(bootBlock, 0, 16);
		bootBlock[0] = formats[fmt].bootType;	
		bootBlock[1] = formats[fmt].bootSidedness;	
		bootBlock[2] = formats[fmt].bootCyls;
		bootBlock[3] = formats[fmt].bootSectors;
		bootBlock[4] = 2;	// 512-byte sectors
		bootBlock[5] = formats[fmt].sysTracks;
		switch(formats[fmt].blockSize)
		{
			case 1024: bootBlock[6] = 3; break;
			case 2048: bootBlock[6] = 4; break;
			case 4096: bootBlock[6] = 5; break;
		}
		bootBlock[7] = formats[fmt].dirBlocks;
		bootBlock[8] = formats[fmt].bootGapRW;
		bootBlock[9] = formats[fmt].bootGapFMT;
		if (fwrite(bootBlock, 1, sizeof(bootBlock), fp) < sizeof(bootBlock) || fclose(fp))
			return -1;
	}
	return 0;
}

static const char *dsktypes[] =
{
        NULL, 		// 0
	"floppy",	// 1
	"dsk", 		// 2
	"raw", 		// 3
	"myz80", 	// 4
	"cfi", 		// 5
	"edsk", 	// 6
	"ntwdm",	// 7
	"rcpmfs",	// 8
};



// Ask for a DSK file to open / create
UiEvent PcwFDC::requestDsk(UiDrawer *d, string &filename, const char **type)
{
	int sel = -1;
	bool a,b,dsk,folder;
	int fmt = 0;
	UiEvent uie;
	int inMenu = 1;
	
	while(inMenu)
	{
		UiMenu uim(d);

		dsk = false;
		folder = false;
		if (filename == A_DRIVE) a = true; else a = false;
		if (filename == B_DRIVE) b = true; else b = false;
		if (!(a || b)) 
		{
			if (Path::isDir(filename)) folder = true;
			else	dsk = true; 
		}

		uim.add(new UiLabel("  Enter filename for disc image  ", d));
		uim.add(new UiSeparator(d));
		uim.add(new UiLabel  ("  File/drive: " + displayName(filename, 40) + "  ", d));
		uim.add(new UiSetting(SDLK_a, a, "  Drive A: [" A_DRIVE "]  ",d));
		uim.add(new UiSetting(SDLK_b, b, "  Drive B: [" B_DRIVE "]  ",d));
		uim.add(new UiSetting(SDLK_d, dsk,    "  Disc file...  ",d));
		uim.add(new UiSetting(SDLK_f, folder, "  Folder...  ",d));
		uim.add(new UiSeparator(d));
		uim.add(new UiCommand(SDLK_v, "  Advanced...  ", d, UIG_SUBMENU));
		uim.add(new UiSeparator(d));
		uim.add(new UiCommand(SDLK_o, "  OK  ", d));
		uim.add(new UiCommand(SDLK_c, "  Cancel  ", d));
		uim.setSelected(sel);
                d->centreContainer(uim);
		uie = uim.eventLoop();
		if (uie == UIE_QUIT || uie == UIE_CANCEL) return uie;
		sel = uim.getSelected();
		switch(uim.getKey(sel))
		{
			case SDLK_a: filename = A_DRIVE; fmt = 1; break;
			case SDLK_b: filename = B_DRIVE; fmt = 1; break;
			case SDLK_c: return UIE_CANCEL;  break;
			case SDLK_o: inMenu = 0; break;
			case SDLK_d: 
                                {
                                    PcwFileChooser f("  OK  ", d);
                                    if (dsk)
                                            f.initDir(findPcwDir(FT_DISC, false));
                                    else    f.initDir(filename);

                                    uie = f.eventLoop();
                                    if (uie == UIE_QUIT) return uie;
                                    if (uie == UIE_OK) 
				    {
					filename = f.getChoice();
					fmt = 2;	
				    }
                                }
                                break;
			case SDLK_f: 
                                {
                                    PcwFolderChooser f("  OK  ", d);
                                    if (!folder)
                                            f.initDir(findPcwDir(FT_DISC, false));
                                    else    f.initDir(filename);

                                    uie = f.eventLoop();
                                    if (uie == UIE_QUIT) return uie;
                                    if (uie == UIE_OK) 
				    {
					filename = f.getChoice();
					string ini = filename + "/.libdsk.ini";
					FILE *fp = fopen(ini.c_str(), "r");
					if (!fp)
					{
						int fmt;
						uie = getFolderType(d, fmt);
						if (uie == UIE_QUIT) return uie;
						if (uie == UIE_OK)
						{
							formatFolder(filename, fmt);
						}
					}
					else fclose(fp);
					fmt = 8;	// Folder
				    }
                                }
                                break;
			case SDLK_v: 
				uie = getDskType(d, fmt, false);
                                if (uie == UIE_QUIT) return uie;
				break;
			default: break;
		}
	}
	// A filename has been entered. See if we can open the matching
	// DSK.
	DSK_PDRIVER drv;
	dsk_err_t err = dsk_open(&drv, filename.c_str(), dsktypes[fmt], NULL);
	// If we failed to open a DSK as DSK, try EDSK.
	if (fmt == 2 && err != 0)
	{
		err = dsk_open(&drv, filename.c_str(), "edsk", NULL);
		if (err) fmt = 6;
	}
	if (!err)	// It can be opened. Great.
	{
		*type = dsktypes[fmt];
		dsk_close(&drv);
		return UIE_OK; 
	}	
	// File can't be opened. 
	return askCreateDisc(d, filename, fmt, type);
}


UiEvent PcwFDC::askCreateDisc(UiDrawer *d, string filename, int fmt, const char **type)
{
	UiMenu uim(d);
	UiEvent uie;	
	dsk_err_t err;
	DSK_PDRIVER drv;

	string sl1;
	sl1 = "  Cannot open: ";
	sl1 += displayName(filename, 40);
	sl1 += "  ";

	uim.add(new UiLabel(sl1, d));
	uim.add(new UiLabel("", d));
	uim.add(new UiCommand(SDLK_c, "  Create it  ", d));
	uim.add(new UiCommand(SDLK_a, "  Abandon  ", d));
	d->centreContainer(uim);
	uie = uim.eventLoop();
	if (uie == UIE_QUIT || uie == UIE_CANCEL) return uie;
	if (uim.getKey(uim.getSelected()) != SDLK_c) return UIE_CANCEL;
	// We are at liberty to create it.

	if (!dsktypes[fmt])	// No type was specified
	{
		if (filename == A_DRIVE || filename == B_DRIVE) fmt = 1;
		else						fmt = 2;

		uie = getDskType(d, fmt, true);
		if (uie == UIE_QUIT || uie == UIE_CANCEL) return uie;
	}
	// Try to create a new blank disc image
	err = dsk_creat(&drv, filename.c_str(), dsktypes[fmt], NULL);
	if (!err)	// It can be created. Great.
	{
		*type = dsktypes[fmt];
		dsk_close(&drv);
		return UIE_OK; 
	}	
	UiMenu uim2(d);
		
	sl1 = "  Cannot open: ";
	sl1 += displayName(filename, 40);
	sl1 += "  ";

	uim2.add(new UiLabel(sl1, d));
	uim2.add(new UiCommand(SDLK_a, "  Abandon  ", d));
	d->centreContainer(uim2);
	uie = uim2.eventLoop();
	if (uie == UIE_QUIT || uie == UIE_CANCEL) return uie;

	return UIE_CANCEL;
}


