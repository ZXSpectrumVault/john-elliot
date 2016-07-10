/************************************************************************

    JOYCE v2.1.7 - Amstrad PCW emulator

    Copyright (C) 1996, 2001-5  John Elliott <seasip.webmaster@gmail.com>

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

static int addBootEntry(JoyceSystem *s, int id, const string title)
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


static int addBootEntry(JoyceSystem *s, char *id, char *title)
{
	return addBootEntry(s, atoi(id), title);
}



/* Migrate the boot settings from a JOYCE 1.3x system */
static int migrateOldBoot(JoyceSystem *sys, FILE *fp)
{
	char buf[50];
	char buf2[50];
	char *s, *t;
	int ch;
	FILE *fpboot;
	UiEvent uie;
	int nb, count;

	count = 0;
	while (fgets(buf, 50, fp))
	{
		s = strchr(buf, '=');
		if (s)
		{
			*s = 0;
			++s;
			t = strchr(s, '\n'); if (t) *t = 0;
			t = strchr(s, '\r'); if (t) *t = 0;
			sprintf(buf2, "boot%s.dsk", buf);
			fpboot = openPcwFile(FT_OLDBOOT, buf2, "rb");
			if (!fpboot)
			{
				string err = "Cannot open ";
				err += buf2;
				uie = sys->m_termMenu->report(err, "  Skip this disc  ");
				if (uie == UIE_QUIT) return -1;
				continue;	
			}
			string doing = "Copying ";
			doing += buf2;
			doing += " (";
			doing += s;
			doing += ")";
			sys->m_termMenu->popupRedraw(doing, 1);
			/* Load the old DSK file into the TPA */
			nb = 0;
			while ((ch = fgetc(fpboot)) != EOF)
			{
				PCWRAM[nb++] = ch;
			}
			fclose(fpboot);
			/* Write out the new DSK file */
			fpboot = openPcwFile(FT_BOOT, buf2, "wb");
			if (!fpboot)
			{
				string err = "Cannot create ";
				err += buf2;
				uie = sys->m_termMenu->report(err, "  Skip this disc  ");
				if (uie == UIE_QUIT) return -1;
				sys->m_termMenu->popupOff();
				continue;	
			}
			for (ch = 0; ch < nb; ch++)
			{
				fputc(PCWRAM[ch], fpboot);
			}
			fclose(fpboot);
			/* Clear the memory it used */
			memset(PCWRAM, 0, nb);
			/* Add the entry to our new boot menu */
			if (addBootEntry(sys, buf, s)) ++count;
		}
	}	
	fclose(fp);
	sys->m_termMenu->popupRedraw("Saving settings", 0);
	sys->storeBootList();
	sys->saveBootList();
	sys->m_termMenu->popupOff();
	return count; 
}

static const char *dsktypes[] = 
{ 
	NULL, "floppy", "dsk", "raw", "myz80", "cfi", "edsk", "ntwdm"
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
		uim.add(new UiSetting(SDLK_2, (fmt == 2), "  CPCEMU/JOYCE .DSK  ", d));
		uim.add(new UiSetting(SDLK_3, (fmt == 3), "  Raw disc image file  ", d));
		uim.add(new UiSetting(SDLK_4, (fmt == 4), "  MYZ80 hard drive  ", d));
		uim.add(new UiSetting(SDLK_5, (fmt == 5), "  .CFI - compressed raw  ", d));
		uim.add(new UiSetting(SDLK_6, (fmt == 6), "  Extended .DSK   ", d));
#ifdef HAVE_WINDOWS_H
		uim.add(new UiSetting(SDLK_7, (fmt == 7), "  Alternative floppy  ", d));
#endif
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
                                else    f.initDir(filename);
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
	uim.add(new UiLabel      ("  to add '.dsk' to the end; JOYCE will  ", d));
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
	uim.add(new UiLabel      ("  JOYCE will refer to this start-of-  ", d));
	uim.add(t = new UiTextInput  (SDLK_HASH,"  day disc: _______________    ", d));
	t->setText(curname);
	d->centreContainer(uim);
	uie = uim.eventLoop();
	if (uie == UIE_QUIT || uie == UIE_CANCEL) return uie;
	name = t->getText();		

	return UIE_OK;
}


/* Ask for a DSK file to open/create */

/* Find an unused boot ID, and return it. */
string newBootFilename(JoyceSystem *sys, int &id)
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
	sprintf(newName, "boot%d.dsk", id);
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
static dsk_err_t dskTrans(JoyceSystem *sys, DSK_PDRIVER indr, DSK_PDRIVER outdr, 
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


UiEvent addBoot(JoyceSystem *sys, int *successes)
{
	DSK_PDRIVER drfrom, drto;
	DSK_GEOMETRY dg;
	UiEvent e;
	bool md3;
	int id;
	unsigned char secbuf[512], cksum;
	int ispcw;
	string why = "Not a PCW boot disc";
	string newName, shortName;
	UiDrawer *d = sys->m_termMenu;

	e = requestDsk(d, "  Add boot disc  ", &drfrom, &dg, false, md3);
	if (e != UIE_OK) return e;
	dsk_err_t err = 0;
	ispcw = 1;

	if (dg.dg_secsize != 512 || dg.dg_secbase != 1) 
	{
		ispcw = 0;
		if (dg.dg_secbase == 0x41) why = "This is a CPC system disc.";
		if (dg.dg_secbase == 0xC1) why = "This is a CPC data disc.";
	}
	else
	{
		err = dsk_pread(drfrom, &dg, secbuf, 0, 0, 1);
		if (!err) 
		{
			cksum = 0;
			for (int n = 0; n < 512; n++) cksum += secbuf[n];
			if (cksum != 0xFF && cksum != 1) ispcw = 0;
			if (cksum == 2) why = "This is a LocoScript install disc.";	
			if (cksum == 3) why = "This is a Spectrum +3 system disc.";	
			if (secbuf[0] != 0 && secbuf[0] != 3) ispcw = 0;
			if (secbuf[0] == 0xE9 || secbuf[0] == 0xEB)
				why = "This is a DOS disc.";			
		}
	}
	if (err || !ispcw)
	{
		dsk_close(&drfrom);
		e = showDskErr(err, why, d);
		if (e == UIE_QUIT) return e;
		return sys->m_termMenu->yesno("Add another boot disc?");
	}
	shortName = newBootFilename(sys, id);
	newName = findPcwFile(FT_BOOT, shortName, "wb");

	e = getBootName(d, shortName);	
	if (e != UIE_OK)
	{
		dsk_close(&drfrom);
		if (e == UIE_QUIT) return e;
		return sys->m_termMenu->yesno("Add another boot disc?");
	}
	err = dsk_creat(&drto, newName.c_str(), "dsk", NULL);
	if (!err) err = dskTrans(sys, drfrom, drto, dg, md3);		

	if (drfrom) dsk_close(&drfrom);
	if (drto  ) dsk_close(&drto);
	if (err) 
	{
		e = showDskErr(err, why, d);
		if (e == UIE_QUIT) return e;
	}
	else 
	{
		++(*successes);
		addBootEntry(sys, id, shortName);
	}
	// Now ask whether to do another.
	return sys->m_termMenu->yesno("Add another boot disc?");
}


UiEvent readDsk(JoyceSystem *sys)
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

UiEvent writeDsk(JoyceSystem *sys)
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



/* Returns: -1 to quit 
 * 0 if no discs read in
 * >0 if discs were read in */
int JoyceSystem::autoSetup()
{
	UiEvent e;
	int successes = 0;

	/* 1. See if there's an existing boot.cfg */
	FILE *fp = openPcwFile(FT_OLDBOOT, "boot.cfg", "r");
	if (fp)
	{
		e = m_termMenu->yesno(
			"Copy boot files from JOYCE 1.3x system?");
		if (e == UIE_OK) return migrateOldBoot(this, fp);
		fclose(fp);
	}
	/* 2. Add new... */
	do
	{
		e = addBoot(this, &successes);
		if (e == UIE_QUIT) return -1;
	} while (e != UIE_CANCEL);

	storeBootList();
	saveBootList();
	return successes;
}


UiEvent JoyceSystem::discManager(UiDrawer *d)
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

		uim.add(new UiLabel  ("      JOYCE disc management      ", d));
		uim.add(new UiSeparator(d));
		uim.add(new UiLabel  ("  Boot discs  ", d));
		uim.add(new UiCommand(SDLK_a, "    Add a boot disc  ", d, UIG_SUBMENU));
		uim.add(new UiCommand(SDLK_r, "    Rename a boot disc  ", d, UIG_SUBMENU));
		uim.add(new UiCommand(SDLK_d, "    Delete a boot disc  ", d, UIG_SUBMENU));
		uim.add(new UiSeparator(d));
		uim.add(new UiLabel  ("  All discs  ", d));
		uim.add(new UiCommand(SDLK_c, "    Convert floppy to JOYCE .DSK file   ", d, UIG_SUBMENU));
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
			case SDLK_r: uie = chooseBootID(d, "  Choose boot disc to rename  ", &bootid);
				     if (uie == UIE_QUIT) return uie;
				     bootno = lookupId(bootid); 
				     if (bootno < 0) break;
				     uie = getBootName(d, name, m_bootList[bootno].m_title);
				     if (uie == UIE_QUIT) return uie;
			             if (uie == UIE_OK) m_bootList[bootno].m_title = name;
				     break;
			case SDLK_d: uie = chooseBootID(d, "  Choose boot disc to delete  ", &bootid);
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
