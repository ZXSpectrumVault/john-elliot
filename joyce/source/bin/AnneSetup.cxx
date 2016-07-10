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
#include "DosFile.hxx"
#include "DosDisk.hxx"

static string dskName;
static const char *dskType;

static int addBootEntry(AnneSystem *s, int id, const string title)
{
	BootListEntry e;

	e.m_id = id;
	if (e.m_id)
	{
		e.m_title = title;
		s->m_bootList.push_back(e);
		return 1;
	}
	return 0;
}


static int addBootEntry(AnneSystem *s, char *id, char *title)
{
	return addBootEntry(s, atoi(id), title);
}



static const char *dsktypes[] = 
{ 
	NULL, "floppy", "dsk", "raw", "myz80", "cfi"
};

UiEvent getDskType(UiDrawer *d, int &fmt, bool create, bool &md3)
{
	UiEvent uie;
	int inMenu = 1;
	int sel = -1;
	int ofmt = fmt;
	bool omd3 = md3;

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
		uim.add(new UiSetting(SDLK_2, (fmt == 2), "  CPCEMU/ANNE .DSK  ", d));
		uim.add(new UiSetting(SDLK_3, (fmt == 3), "  Raw disc image file  ", d));
		uim.add(new UiSetting(SDLK_4, (fmt == 4), "  MYZ80 hard drive  ", d));
		uim.add(new UiSetting(SDLK_5, (fmt == 5), "  .CFI - compressed raw  ", d));
		uim.add(new UiSeparator(d));
		uim.add(new UiSetting(SDLK_n, !md3, "  Normal disc    ", d));
		uim.add(new UiSetting(SDLK_m,  md3, "  MicroDesign program disc  ", d));
		uim.add(new UiSeparator(d));
		uim.add(new UiCommand(SDLK_o, "  OK  ", d));
		uim.add(new UiCommand(SDLK_ESCAPE, "  Cancel  ", d));
		uim.setSelected(sel);
                d->centreContainer(uim);
		uie = uim.eventLoop();
		if (uie == UIE_QUIT || uie == UIE_CANCEL) 
		{
			md3 = omd3; 
			fmt = ofmt;
			return uie;
		}
		sel = uim.getSelected();
		switch(uim.getKey(sel))
		{
			case SDLK_m: md3 = true; break;
			case SDLK_n: md3 = false; break;
			case SDLK_0: fmt = 0; break;
			case SDLK_1: fmt = 1; break;
			case SDLK_2: fmt = 2; break;
			case SDLK_3: fmt = 3; break;
			case SDLK_4: fmt = 4; break;
			case SDLK_5: fmt = 5; break;
			case SDLK_6: fmt = 6; break;
			case SDLK_ESCAPE: 
					md3 = omd3;
					fmt = ofmt;
					return UIE_CANCEL;
			case SDLK_o: inMenu = 0; break;
			default: break;
		}
	}
	return UIE_OK;
}


/* Ask for a DSK file to open/create */
UiEvent requestDsk(UiDrawer *d, const string prompt, DSK_PDRIVER *dr, 
		DSK_GEOMETRY *dg, bool create, bool &md3)
{
	int sel = -1;
	bool a,b,dsk;
	string filename = A_DRIVE;
	int fmt = create ? 2 : 0;
	UiEvent uie;
	int inMenu = 1;
	
	md3 = false;
	while(inMenu)
	{
		UiMenu uim(d);

		if (filename == A_DRIVE) a = true; else a = false;
		if (filename == B_DRIVE) b = true; else b = false;
		if (!(a || b)) dsk = true; else dsk = false;

		uim.add(new UiLabel(prompt, d));
		uim.add(new UiSeparator(d));
		uim.add(new UiLabel  ("  File/drive: " + displayName(filename, 40) + "  ", d));
		uim.add(new UiSetting(SDLK_a, a, "  Drive A: [" A_DRIVE "]  ",d));
		uim.add(new UiSetting(SDLK_b, b, "  Drive B: [" B_DRIVE "]  ",d));
		uim.add(new UiSetting(SDLK_d, dsk, "  Disc file...  ",d));
		uim.add(new UiSeparator(d));
		uim.add(new UiCommand(SDLK_v, "  Advanced...  ", d, UIG_SUBMENU));
		uim.add(new UiSeparator(d));
		uim.add(new UiCommand(SDLK_c, "  Continue  ", d));
		uim.add(new UiCommand(SDLK_ESCAPE, "  EXIT  ", d));
		uim.setSelected(sel);
                d->centreContainer(uim);
		uie = uim.eventLoop();
		if (uie == UIE_QUIT || uie == UIE_CANCEL) return uie;
		sel = uim.getSelected();
		switch(uim.getKey(sel))
		{
			case SDLK_a: filename = A_DRIVE; if (create) fmt = 1; break;
			case SDLK_b: filename = B_DRIVE; if (create) fmt = 1; break;
			case SDLK_c: inMenu = 0; break;
			case SDLK_d: 
                            {
                                PcwFileChooser f("  OK  ", d);
				if (filename == A_DRIVE || filename == B_DRIVE)
					f.initDir(findPcwDir(FT_BOOT, false));
                                else	f.initDir(filename);
                                uie = f.eventLoop();
                                if (uie == UIE_QUIT) return uie;
                                if (uie == UIE_OK) filename = f.getChoice();
				if (create) fmt = 2;
                            }
                            break;
			case SDLK_v: 
				uie = getDskType(d, fmt, create, md3);
                                if (uie == UIE_QUIT) return uie;
				break;
			case SDLK_ESCAPE: return UIE_CANCEL;
			default: break;
		}
	}
	
	/* OK. We got to here, so "filename" is likely to be valid */
	dsk_err_t err = 1;

	dskName = filename;
	dskType = dsktypes[fmt];
	if (create) err = dsk_creat(dr, filename.c_str(), dsktypes[fmt], NULL);
	if (err)    err = dsk_open(dr, filename.c_str(), dsktypes[fmt], NULL);
	if (!err && !create) err = dsk_getgeom(*dr, dg);
	if(err)
	{
		UiMenu uim(d);
		
		string sl1,sl2;
		sl1 = "  Cannot open: ";
		sl1 += displayName(filename, 40);
		sl1 += "  ";
		sl2 = "    ";
		sl2 += dsk_strerror(err);
		sl2 += "  ";
 
		uim.add(new UiLabel(sl1, d));
		uim.add(new UiLabel("", d));
		uim.add(new UiLabel(sl2, d));
		uim.add(new UiLabel("", d));
		uim.add(new UiCommand(SDLK_ESCAPE, "  Cancel", d));
                d->centreContainer(uim);
		uie = uim.eventLoop();
		if (uie == UIE_QUIT) return uie;
		return UIE_CANCEL;	
	}
	return UIE_OK;
}

UiEvent getDiscName(UiDrawer *d, string &name, const string curname = "")
{
	UiEvent uie;
	UiTextInput *t; 

	UiMenu uim(d);

	uim.add(new UiLabel      ("  Enter the filename to which this   ", d));
	uim.add(new UiLabel      ("  disc should be saved (you don't need  ", d));
	uim.add(new UiLabel      ("  to add '.dsk' to the end; ANNE will  ", d));
	uim.add(new UiLabel      ("  do that automatically).   ", d));
	uim.add(t = new UiTextInput  (SDLK_HASH,"  Name: _______________    ", d));
	t->setText(curname);
	d->centreContainer(uim);
	uie = uim.eventLoop();
	if (uie == UIE_QUIT || uie == UIE_CANCEL) return uie;
	name = t->getText();
	if (!strchr(name.c_str(), '.')) name += ".dsk";
	return UIE_OK;
}



UiEvent getBootName(UiDrawer *d, string &name, const string curname = "")
{
	UiEvent uie;
	UiTextInput *t; 

	UiMenu uim(d);

	uim.add(new UiLabel      ("  Please enter a short name by which  ", d));
	uim.add(new UiLabel      ("  ANNE will refer to this boot ROM:  ", d));
	uim.add(t = new UiTextInput  (SDLK_HASH,"  _______________    ", d));
	t->setText(curname);
	d->centreContainer(uim);
	uie = uim.eventLoop();
	if (uie == UIE_QUIT || uie == UIE_CANCEL) return uie;
	name = t->getText();		

	return UIE_OK;
}


/* Ask for a DSK file to open/create */

/* Find an unused boot ID, and return it. */
string newBootFilename(AnneSystem *sys, int &id)
{
	char newName[20];
	int n;

	for (n = 0, id = 1; n < (int)sys->m_bootList.size(); n++)
	{
                if (sys->m_bootList[n].m_id == id) 
		{
			id++;
			n = -1;
		} 
	}
	sprintf(newName, "anne%d.rom", id);
	return newName;
}


UiEvent showDskErr(dsk_err_t err, string why, UiDrawer *d)
{
	UiMenu uim(d);
	string sl1,sl2;
	sl1 = "  Cannot use this disc  ";
	if (err) why = dsk_strerror(err);
	
	sl2 = "    ";
	sl2 += why;
	sl2 += "  ";
 
	uim.add(new UiLabel(sl1, d));
	uim.add(new UiLabel("", d));
	uim.add(new UiLabel(sl2, d));
	uim.add(new UiLabel("", d));
	uim.add(new UiCommand(SDLK_ESCAPE, "  Cancel", d));
	d->centreContainer(uim);
	UiEvent uie = uim.eventLoop();
	if (uie == UIE_QUIT) return uie;
	return UIE_CANCEL;	
}

// [v1.9.3] now supports MD3 discs
static dsk_err_t dskTrans(AnneSystem *sys, DSK_PDRIVER indr, DSK_PDRIVER outdr, 
			DSK_GEOMETRY &dg, bool md3)
{
	dsk_err_t e = DSK_ERR_OK;
	dsk_pcyl_t cyl;
	dsk_phead_t head;
	dsk_psect_t sec;
	string op;
	void *buf;
	char progress[80];

	buf = malloc(dg.dg_secsize);
	if (!buf) return DSK_ERR_NOMEM;

	for (cyl = 0; cyl < dg.dg_cylinders; ++cyl)
	{
		for (head = 0; head < dg.dg_heads; ++head)
		{	
			if (md3)
			{
				// entering 256-byte sector zone
				if (head == 1 && cyl == 1)
				{
					dg.dg_secsize = 256;
				}
				if (head == 0 && cyl == 2)
				{
					dg.dg_secsize = 512;
				}
			}	
                        op = "Formatting";
                        if (!e) e = dsk_apform(outdr, &dg, cyl, head, 0xE5);

                        if (!e) for (sec = 0; sec < dg.dg_sectors; ++sec)
                        {
                                sprintf(progress,
		"Cylinder %02d/%02d Head %d/%d Sector %03d/%03d", 
                cyl +1, dg.dg_cylinders,
                head+1, dg.dg_heads,
                sec+dg.dg_secbase, dg.dg_sectors + dg.dg_secbase - 1); 
				sys->m_termMenu->popupRedraw(progress, 1);
                        
                                op = "Reading"; 
                                e = dsk_pread(indr, &dg, buf, cyl,head,sec + dg.dg_secbase);
				// in MD3 mode ignore errors on security track
				if (md3 && head == 1 && cyl == 1 && e == DSK_ERR_DATAERR) e = DSK_ERR_OK;
			
                                if (e) break;
                                op = "Writing";
                                e = dsk_pwrite(outdr,&dg,buf,cyl,head, sec + dg.dg_secbase);
                                if (e) break;
                        }
                        if (e) break;
                }
                if (e) break;
        }
	sys->m_termMenu->popupOff();
	free(buf);
	return e;       
}


UiEvent addBoot(AnneSystem *sys, 
		string name, byte *data, int len, int *successes)
{
	UiEvent e;
	int id;
	FILE *fp;
	string newName, shortName;
	UiDrawer *d = sys->m_termMenu;

	shortName = newBootFilename(sys, id);
	fp = openPcwFile(FT_BOOT, shortName, "wb");
	if (!fp) return UIE_CANCEL;
	fwrite(data, 1, len, fp);
	fclose(fp);
	shortName = name;
		
	++(*successes);
	addBootEntry(sys, id, shortName);

	return UIE_OK;
}



UiEvent addBoot(AnneSystem *sys, int *successes)
{
	UiEvent uie;
	string bootfile, filename;
	FILE *fp;
	long size;
	byte *buf;

	filename = findPcwDir(FT_BOOT, false);
	PcwFileChooser f("  OK  ", sys->m_termMenu);
	f.initDir(filename);
      	uie = f.eventLoop();

	bootfile = f.getChoice();

	fp = fopen(bootfile.c_str(), "rb");
	if (!fp)
        {
                string err = "Cannot open ";
                err += bootfile;
		return sys->m_termMenu->report(err, "  Cancel  ");
	}
	fseek(fp, 0, SEEK_END);
	size = ftell(fp);
	fseek(fp, 0, SEEK_SET);	
	
	buf = new byte[size];
	if (fread(buf, 1, size, fp) < size)
	{
		delete buf;
                string err = "Cannot read all of ";
                err += bootfile;
		return sys->m_termMenu->report(err, "  Cancel  ");
	}
	fclose(fp);
	uie = getBootName(sys->m_termMenu, bootfile);
	if (uie != UIE_OK) { delete buf; return uie; }
	uie = addBoot(sys, bootfile, buf, size, successes);
	delete buf;
	return uie;
}

UiEvent saveBoot(AnneSystem *sys, int *successes)
{
	UiEvent uie;
	string bootfile;
	uie = getBootName(sys->m_termMenu, bootfile);
	if (uie != UIE_OK) return uie;
	uie = addBoot(sys, bootfile, PCWFLASH, ((AnneMemory *)(sys->m_mem))->getFlashActual(), successes);
	return uie;
}


UiEvent readDsk(AnneSystem *sys)
{
	DSK_PDRIVER drfrom, drto;
	DSK_GEOMETRY dg;
	UiEvent e = UIE_OK;
	bool md3;
	int ispcw;
	string why = "Not a PCW disc";
	string newName, shortName;
	UiDrawer *d = sys->m_termMenu;

	e = requestDsk(d, "  Read in a disc  ", &drfrom, &dg, false, md3);
	if (e != UIE_OK) return e;
	dsk_err_t err = 0;
	ispcw = 1;

	e = getDiscName(d, shortName);
	if (e != UIE_OK) return e;
	newName   = findPcwFile(FT_DISC, shortName, "wb");
	err = dsk_creat(&drto, newName.c_str(), "dsk", NULL);
	if (!err) err = dskTrans(sys, drfrom, drto, dg, md3);		

	if (drfrom) dsk_close(&drfrom);
	if (drto  ) dsk_close(&drto);
	if (err) 
	{
		e = showDskErr(err, why, d);
		if (e == UIE_QUIT) return e;
	}
	return e;
}

UiEvent writeDsk(AnneSystem *sys)
{
	DSK_PDRIVER drfrom, drto;
	DSK_GEOMETRY dg;
	UiEvent e = UIE_OK;
	bool md3;
	int ispcw;
	string why = "Failed to open DSK file";
	string oldName;
	UiDrawer *d = sys->m_termMenu;

	PcwFileChooser f("  OK  ", d);
	f.initDir(findPcwDir(FT_DISC, false));
	e = f.eventLoop();
        if (e != UIE_OK) return e;
  
	oldName = f.getChoice();

	dsk_err_t err = dsk_open(&drfrom, oldName.c_str(), NULL, NULL);
	if (!err) err = dsk_getgeom(drfrom, &dg);
	if (err)
	{
		e = showDskErr(err, why, d);
		return e;
	}
	e = requestDsk(d, "  Write DSK to  ", &drto, &dg, true, md3);
	if (e != UIE_OK) 
	{
		dsk_close(&drfrom);
		return e;
	}
	ispcw = 1;
	err = dskTrans(sys, drfrom, drto, dg, md3);		

	if (drfrom) dsk_close(&drfrom);
	if (drto  ) dsk_close(&drto);
	if (err) 
	{
		e = showDskErr(err, why, d);
		if (e == UIE_QUIT) return e;
	}
	return e;
}


UiEvent findRescueDisc(DSK_PDRIVER &rescue, DSK_GEOMETRY &dg, AnneSystem *sys)
{
	bool md3 = false;

	UiDrawer *d = sys->m_termMenu;
	UiEvent e = requestDsk(d, "  Locate the PCW16 Rescue Disc  ", &rescue,
			&dg, false, md3);
        sys->m_fdc->insertA(dskName.c_str(), dskType);
	return e;	
}


/* Returns: -1 to quit 
 * 0 if no discs read in
 * >0 if discs were read in */
int AnneSystem::autoSetup()
{
	UiEvent e;
	dsk_err_t err;
	int successes = 0;
	int n, len;
	DosDisk rec;
	DosFile pcwDir, sysDir, catalog, sysFile;
	/* Load recovery disc */
	char folder[20];
	char file[20];

	do
	{
		byte *dirent;
		e = findRescueDisc(rec.m_rescue, rec.m_dg, this);
		if (e == UIE_QUIT) return -1;
		if (e != UIE_OK) continue;

		e = rec.loadBasics(m_termMenu);
		if (e == UIE_OK) continue; if (e != UIE_CONTINUE) return -1;
		// Load the FAT
	
		dirent = rec.m_root.lookup("PCW", true);	
		if (!dirent)
		{
			e = m_termMenu->yesno("Cannot find PCW folder. Choose another disc?");
			if (e != UIE_OK) return -1; else continue;
		}
		// Load \PCW subdirectory
		e = rec.loadFile(dirent, m_termMenu, pcwDir);
		if (e == UIE_OK) continue; if (e != UIE_CONTINUE) return -1;
			
		dirent = pcwDir.lookup("CATALOG TXT", false);
		if (!dirent)
		{
			e = m_termMenu->yesno("Cannot find CATALOG.TXT. Choose another disc?");
			if (e != UIE_OK) return -1; else continue;
		}
		// Load CATALOG.TXT 
		e = rec.loadFile(dirent, m_termMenu, catalog);
		if (e == UIE_OK) continue; if (e != UIE_CONTINUE) return -1;

		dirent = catalog.lookup16("ROSANNE Operating System");
		if (!dirent)	
		{
			e = m_termMenu->yesno("Cannot find the ROSANNE Operating System file. Choose another disc?");
			if (e != UIE_OK) return -1; else continue;
		}
		sprintf(folder, "%-8.8s", dirent);
		sprintf(file,   "%-6.6s  %-3.3s", dirent + 19, dirent+26);

		dirent = pcwDir.lookup(folder, true);
		if (!dirent)
		{
			e = m_termMenu->yesno("Cannot find system folder. Choose another disc?");
			if (e != UIE_OK) return -1; else continue;
		}
		e = rec.loadFile(dirent, m_termMenu, sysDir);
		if (e == UIE_OK) continue; if (e != UIE_CONTINUE) return -1;
		dirent = sysDir.lookup(file, false);	
		if (!dirent)
		{
			e = m_termMenu->yesno("Cannot find system file. Choose another disc?");
			if (e != UIE_OK) return -1; else continue;
		}
		e = rec.loadFile(dirent, m_termMenu, sysFile);
		if (e == UIE_OK) continue; if (e != UIE_CONTINUE) return -1;
/* Finally, we have PCW045.SYS loaded - I hope. */	
		addBoot(this, "Empty system (Boot ROM only)", sysFile.getData(),
				65536, &successes);
		FILE *fp = openPcwFile(FT_BOOT, "annelast.rom", "rb");
/* Create annelast.rom so that if the user chooses 'last session' it works */
		if (fp) fclose(fp);
		else	
		{
			fp = openPcwFile(FT_BOOT, "annelast.rom", "wb");
			if (fp) 
			{
				fwrite(sysFile.getData(), 1, 65536, fp);
				fclose(fp);
			}
		}
		e = UIE_CANCEL;
	} while (e != UIE_CANCEL);

	storeBootList();
	saveBootList();
	return successes;
}


UiEvent AnneSystem::discManager(UiDrawer *d)
{
	UiEvent uie;
	int inMenu = 1;
	int sel = -1;
	int successes; 
	int bootid, bootno;
	string name;

	while(inMenu)
	{
		UiMenu uim(d);

		uim.add(new UiLabel  ("      ANNE disc/ROM management      ", d));
		uim.add(new UiSeparator(d));
		uim.add(new UiLabel  ("  Boot ROMs  ", d));
		uim.add(new UiCommand(SDLK_a, "    Add a boot ROM from a file  ", d, UIG_SUBMENU));
		uim.add(new UiCommand(SDLK_s, "    Save current ROM as boot ROM  ", d, UIG_SUBMENU));
		uim.add(new UiCommand(SDLK_r, "    Rename a boot ROM  ", d, UIG_SUBMENU));
		uim.add(new UiCommand(SDLK_d, "    Delete a boot ROM  ", d, UIG_SUBMENU));
		uim.add(new UiSeparator(d));
		uim.add(new UiLabel  ("  Discs  ", d));
		uim.add(new UiCommand(SDLK_c, "    Convert floppy to ANNE .DSK file   ", d, UIG_SUBMENU));
		uim.add(new UiCommand(SDLK_w, "    Write a .DSK file back out to floppy  ", d, UIG_SUBMENU));
		uim.add(new UiSeparator(d));
		uim.add(new UiCommand(SDLK_ESCAPE, " EXIT  ", d));
		uim.setSelected(sel);
                d->centreContainer(uim);
		uie = uim.eventLoop();
		if (uie == UIE_QUIT || uie == UIE_CANCEL) return uie;
		
		sel = uim.getSelected();
		switch(uim.getKey(sel))
		{
			case SDLK_a: uie = addBoot(this, &successes);
				     if (uie == UIE_QUIT) return uie;
				     break;
			case SDLK_s: uie = saveBoot(this, &successes);
				     if (uie == UIE_QUIT) return uie;
				     break;
			case SDLK_r: uie = chooseBootID(d, "  Choose boot ROM to rename  ", &bootid);
				     if (uie == UIE_QUIT) return uie;
				     bootno = lookupId(bootid); 
				     if (bootno < 0) break;
				     uie = getBootName(d, name, m_bootList[bootno].m_title);
				     if (uie == UIE_QUIT) return uie;
			             if (uie == UIE_OK) m_bootList[bootno].m_title = name;
				     break;
			case SDLK_d: uie = chooseBootID(d, "  Choose boot ROM to delete  ", &bootid);
				     if (uie == UIE_QUIT) return uie;
				     deleteBootEntry(bootid);
				     break;
			case SDLK_c: uie = readDsk(this);
				     if (uie == UIE_QUIT) return uie;
				     break;
			case SDLK_w: uie = writeDsk(this);
				     if (uie == UIE_QUIT) return uie;
				     break;
			case SDLK_ESCAPE: 
				m_termMenu->popupRedraw("Saving settings", 0);
				storeBootList();
				saveBootList();
				m_termMenu->popupOff();
				return UIE_OK;
			default:     break;
		}
	}
	return UIE_CONTINUE;
}
