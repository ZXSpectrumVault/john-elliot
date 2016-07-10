/************************************************************************

    JOYCE v2.1.3 - Amstrad PCW emulator

    Copyright (C) 1996, 2001, 2003  John Elliott <seasip.webmaster@gmail.com>

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
#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif
#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif

/* DPB for an 8Mb hard drive. Note that this doesn't match the geometry
 * used by LIBDSK, which has 1024-byte sectors. A little deblocking is called
 * for. 
 *
 * This DPB has 16k/track (32 512 byte sectors), and 512 tracks. LIBDSK uses
 *             128k/track (128 1k sectors) and 64 tracks.
 * 
 * SPT=128
 * BSH=5
 * BLM=0x1F
 * EXM=1
 * DSM=0x7FF (2047)
 * DRM=0x3FF (1023)
 * */
static const byte mzdpb[] = { 0x80, 0x00, 0x05, 0x1F, 0x01, 0xFF, 0x07, 0xFF,
			      0x03, 0xFF, 0x00, 0x00, 0x80, 0x00, 0x00, 0x02,
			      0x03 };
// CPC system format 
static const byte sysdpb[]= { 0x24, 0x00, 0x03, 0x07, 0x00, 0xAA, 0x00, 0x3F, 
	                      0x00, 0xC0, 0x00, 0x10, 0x00, 0x02, 0x00, 0x02,
			      0x03 };
// CPC data format
static const byte datadpb[]={ 0x24, 0x00, 0x03, 0x07, 0x00, 0xB3, 0x00, 0x3F, 
	                      0x00, 0xC0, 0x00, 0x10, 0x00, 0x00, 0x00, 0x02,
			      0x03 };
			 



PcwHDC::PcwHDC(PcwSystem *sys) : PcwDevice("hdc")
{
	int n;
	char buf[20];
		
	m_sys = sys;	
	m_allowFloppy = false;
	for (n = 0; n < 16; n++) 
	{
		sprintf(buf, "%c.dsk", n + 'a');
		m_drvName[n] = findPcwFile(FT_DISC, buf, "rb"); 
		m_drvUnit[n] = NULL;
		m_drvType[n] = "myz80";
		deblockSector [n] = -1;
		deblockPending[n] = 0;
		m_isHD[n] = 0;
	}
}


PcwHDC::~PcwHDC()
{
	for (int n = 0; n < 16; n++) 
	{
		closeImage(n);
	}
}

void PcwHDC::reset(void)
{
	for (int n = 0; n < 16; n++) 
	{
		closeImage(n);
	}
}

UiEvent PcwHDC::getFolderType(UiDrawer *d, int &fmt, bool hd)
{
	UiEvent uie;
	int inMenu = 1;
	int sel = -1;

	fmt = hd ? 6 : 4;
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
		uim.add(new UiSetting(SDLK_6, (fmt == 5), "  784k PCW format  ", d));
		uim.add(new UiSetting(SDLK_7, (fmt == 6), "  8Mb MYZ80 format  ", d));
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
			case SDLK_7: fmt = 6; break;
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
	{"myz80",   4096,      8,      2048,        0,  0,0x00, 0, 0,0x00,0x00},
};


//
// Format a filesystem folder as a LibDsk disc image.
//
int PcwHDC::formatFolder(const string dirname, int fmt)
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



bool PcwHDC::hasSettings(SDLKey *key, string &caption)
{
	*key = SDLK_e;
	caption = "  Extra drives  ";	
	return true;
}

UiEvent PcwHDC::setupDrive(UiDrawer *d, int drive)
{
        int sel = -1;
        bool a,b,dsk,folder,myz80,autoType;
        string filename = m_drvName[drive];
        UiEvent uie;
        int inMenu = 1;
	char prompt[50];

	sprintf(prompt, "  Options for drive %c:  ", drive + 'A');

        while(inMenu)
        {
                UiMenu uim(d);

		dsk = folder = false;
                if (filename == A_DRIVE) a = true; else a = false;
                if (filename == B_DRIVE) b = true; else b = false;
		if (!(a || b)) 
		{
			struct stat st;
			if (!stat(filename.c_str(), &st) && S_ISDIR(st.st_mode))
				folder = true;
			else	dsk = true; 
		}

		autoType = (m_drvType[drive] == NULL);
		if (!autoType) myz80 = !strcmp(m_drvType[drive], "myz80");
		else	       myz80 = false;

/* If it couldn't detect whether this is supposed to be a floppy or a hard
 * drive, and it's a folder, then guess from the simulated geometry. */
		if (autoType == false && myz80 == false && folder == true)
		{
			if (isHard(filename.c_str(), "rcpmfs"))
				myz80    = true;
			else	autoType = true;
		}
                uim.add(new UiLabel(prompt, d));
                uim.add(new UiSeparator(d));
                uim.add(new UiLabel  ("  File/drive: " + displayName(filename, 40) + "  ", d));
                uim.add(new UiSetting(SDLK_a, a, "  Drive A: [" A_DRIVE "]  ",d));
                uim.add(new UiSetting(SDLK_b, b, "  Drive B: [" B_DRIVE "]  ",d));
		uim.add(new UiSetting(SDLK_d, dsk, "  Disc file...  ",d));
		uim.add(new UiSetting(SDLK_f, folder, "  Folder...  ",d));
		uim.add(new UiSeparator(d));
		uim.add(new UiSetting(SDLK_l, autoType, "  fLoppy drive  ", d));
		uim.add(new UiSetting(SDLK_h, myz80,    "  Hard drive  ", d));
		uim.add(new UiSeparator(d));
		uim.add(new UiCommand(SDLK_ESCAPE, "  EXIT  ", d));
		uim.setSelected(sel);
		d->centreContainer(uim);
		uie = uim.eventLoop();
		sel = uim.getSelected();
		switch(uim.getKey(sel))
		{
			case SDLK_a: filename = A_DRIVE; m_drvType[drive] = NULL; break;
			case SDLK_b: filename = B_DRIVE; m_drvType[drive] = NULL; break;
			case SDLK_d:
                             {
                             PcwFileChooser f("  OK  ", d);
                             if (filename == A_DRIVE || filename == B_DRIVE)
                                     f.initDir(findPcwDir(FT_DISC, false));
                             else    f.initDir(filename);
                             uie = f.eventLoop();
                             if (uie == UIE_QUIT) return uie;
                             if (uie == UIE_OK) 
				{
				filename = f.getChoice();
				m_drvType[drive] = "myz80";
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
						uie = getFolderType(d, fmt, myz80);
						if (uie == UIE_QUIT) return uie;
						if (uie == UIE_OK)
						{
							formatFolder(filename, fmt);
						}
					}
					else fclose(fp);
					m_drvType[drive] = "rcpmfs";
				    }
                                }
                                break;

			case SDLK_l: m_drvType[drive] = NULL; break;
			case SDLK_h: if (folder) m_drvType[drive] = "rcpmfs";
				     else        m_drvType[drive] = "myz80"; 
				     break;
			case SDLK_ESCAPE:
        			m_drvName[drive] = filename;
				return UIE_OK;
			default: break;	
		}
	}
	return UIE_CONTINUE;
}


UiEvent PcwHDC::settings(UiDrawer *d)
{
	UiEvent uie;
	int n;
	int sel = -1;
	while(1)
	{
		UiMenu uim(d);
		uim.add(new UiLabel("  Emulated extra drives  ", d));	
		uim.add(new UiSeparator(d));
		for (n = 2; n < 16; n++)
		{
			SDLKey k = (SDLKey)(SDLK_a + n);
			char buf[30];

			if (n == 12) continue;
			sprintf(buf, "  Settings for drive %c:  ", n + 'A');
			uim.add(new UiCommand(k, buf, d));
		}
		uim.add(new UiSeparator(d));
		uim.add(new UiCommand(SDLK_ESCAPE, "  EXIT  ", d));
                d->centreContainer(uim);
                uim.setSelected(sel);
                uie = uim.eventLoop();

                if (uie == UIE_QUIT) return uie;
                switch(uim.getKey(sel = uim.getSelected()))
                {
			case SDLK_ESCAPE:
				reset();
				return UIE_OK;
			default:
				n = uim.getKey(sel) - SDLK_a;
				uie = setupDrive(d, n);
                		if (uie == UIE_QUIT) return uie;
				break;
		}
	}
	return UIE_CONTINUE;
}


bool PcwHDC::parseDriveNode(xmlDocPtr doc, xmlNsPtr ns, xmlNodePtr cur)
{
	char *s;
	int id;

	s = (char *)xmlGetProp(cur, (xmlChar *)"letter");
	if (!s || s[1] != 0 || s[0] < 'a' || s[0] > 'p')
        {
                joyce_dprintf("<drive> must have a letter= field\n");
                return true;
        }
	id = s[0] - 'a';
	xmlFree(s);
	parseFilename(doc, ns, cur, "filename", m_drvName[id]);
	s = (char *)xmlGetProp(cur, (xmlChar *)"type");
	if (s)
	{
		if (!strcmp(s, "auto")) m_drvType[id] = NULL;
		else
		{
			m_userType[id] = s;
			m_drvType[id]  = m_userType[id].c_str();
		}
		xmlFree(s);
	}
	return true;	
}



bool PcwHDC::parseNode(xmlDocPtr doc, xmlNsPtr ns, xmlNodePtr cur)
{
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

bool PcwHDC::storeDriveNode(xmlDocPtr doc, xmlNsPtr ns, xmlNodePtr cur, int n)
{
	char buf[3];

	sprintf(buf, "%c", n + 'a');
	xmlSetProp(cur, (xmlChar *)"letter", (xmlChar *)buf);
	storeFilename(doc, ns, cur, "filename", m_drvName[n]);
	if (m_drvType[n]) xmlSetProp(cur, (xmlChar *)"type", (xmlChar *)m_drvType[n]);
	else		  xmlSetProp(cur, (xmlChar *)"type", (xmlChar *)"auto");
	return true;
}


bool PcwHDC::storeNode(xmlDocPtr doc, xmlNsPtr ns, xmlNodePtr cur)
{
        bool some;
        char buf[20];
        xmlNodePtr drv;

        for (int n = 0; n < 16; n++)
        {
                some = false;
                sprintf(buf, "%c", n + 'a');
                for (drv = cur->xmlChildrenNode; drv; drv = drv->next)
                {
                        if (strcmp((char *)drv->name, "drive")) continue;

                        char *s = (char *)xmlGetProp(drv, (xmlChar *)"letter");
                        if (!s) continue;
                        if (strcmp(s, buf))
                        {
                                xmlFree(s);
                                continue;
                        }
                        some = true;
                        storeDriveNode(doc, ns, drv, n);
			xmlFree(s);
		}
                // No existing <drive> node; create one.
                if (!some)
                {
                        drv = xmlNewNode(ns, (xmlChar *)"drive");
                        xmlSetProp(drv, (xmlChar *)"letter", (xmlChar *)buf);
                        xmlAddChild(cur, drv);
                        storeDriveNode(doc, ns, drv, n);
                }
        }
        return true;
}




void PcwHDC::fidPuts(word addr, const string s)
{
	for (unsigned n = 0; n < s.length(); n++)
	{
		m_sys->m_cpu->storeB(addr++, s[n]);
	}
	m_sys->m_cpu->storeB(addr++, 0xFF);
}


int PcwHDC::openImage(int img)
{
	string dname;
	dsk_err_t err;

	if (m_drvUnit[img]) return 1;

	dname = findPcwFile(FT_DISC, m_drvName[img], "r+b");
	// Try to open existing file. If that fails, create a new one.
	err = dsk_open(&m_drvUnit[img], m_drvName[img].c_str(), m_drvType[img], NULL);
	if (err) err = dsk_creat(&m_drvUnit[img], m_drvName[img].c_str(), m_drvType[img], NULL);
	if (!err) err = dsk_getgeom(m_drvUnit[img], &m_drvDg[img]);
	if (err)
	{
		joyce_dprintf("Failed to open %s: %s\n", 
				m_drvName[img].c_str(), dsk_strerror(err));
		return 0;
	}
	return 1;		/* File opened */
}

int PcwHDC::closeImage(int img)
{
	if (m_drvUnit[img])
	{
		if (deblockPending[img]) deblockWrite(img, deblockSector[img]);

 		dsk_close(&m_drvUnit[img]);
	}
	return 1;
}


bool PcwHDC::deblockRead (int img, long sector)
{
	dsk_err_t err;

	if (sector == deblockSector[img]) return true;

	if (deblockPending[img]) deblockWrite(img, deblockSector[img]);
	if ((err = dsk_lread(m_drvUnit[img], &m_drvDg[img], deblock[img], sector)))
	{
		m_lastErr = dsk_strerror(err);	
		return false;
	}
	deblockSector[img] = sector;
	return true;
}

bool PcwHDC::deblockWrite(int img, long sector)
{
	dsk_err_t err;

	if ((err = dsk_lwrite(m_drvUnit[img], &m_drvDg[img], deblock[img], sector)))
	{
		m_lastErr = dsk_strerror(err);	
		return false;
	}
	deblockPending[img] = false;
	return true;
}


void PcwHDC::ret_err(Z80 *R)
{
	R->a = 1;
	R->f &= ~(FLG_C | FLG_H);	/* No Carry, No Halfcarry, Zero */
	R->f |= FLG_Z;
}

void PcwHDC::ret_ok(Z80 *R)
{
	m_lastErr = "";
	R->a = 0;
	R->f |= FLG_C;		/* Carry set, no Halfcarry */
	R->f &= ~FLG_H;
}


void PcwHDC::fidLogin(Z80 *R)
{
	int nx;
	byte drv = R->b;
	dsk_err_t err;
	
	m_lastDrv = drv;
	if (!openImage(drv)) 
	{ 
		m_lastErr = "Could not open drive";
		ret_err(R); 
		return; 
	}
	// The LibDsk geometry probe has now populated m_drvDg[drv].
	// Use it to detect CPC-format discs and MYZ80-format discs, 
	// neither of which have boot blocks as such.
	if (m_drvDg[drv].dg_sectors == 128 && m_drvDg[drv].dg_secsize == 1024)
	{
		// rcpmfs in myz80 format, because it has 128 
		// 1k sectors.
		for (nx = 0; nx < 17; nx++) 
		{
			m_sys->m_cpu->storeB(R->ix + nx, mzdpb[nx]);
		}
		ret_ok(R);
		m_isHD[drv] = true;
		return;		
	}
	m_isHD[drv] = false;
	if (!err && m_drvDg[drv].dg_secbase == 0x41)	// CPC system
	{
		for (nx = 0; nx < 17; nx++) 
		{
			m_sys->m_cpu->storeB(R->ix + nx, sysdpb[nx]);
		}
		ret_ok(R);
		return;
	}
	if (!err && m_drvDg[drv].dg_secbase == 0xC1)	// CPC data
	{
		for (nx = 0; nx < 17; nx++) 
		{
			m_sys->m_cpu->storeB(R->ix + nx, datadpb[nx]);
		}
		ret_ok(R);
		return;
	}
	// Anything else, treat as a floppy with a PCW boot spec.
	if ((err = dsk_lread(m_drvUnit[drv], &m_drvDg[drv], deblock[drv], 0)))
	{
		m_lastErr = dsk_strerror(err);	
		ret_err(R);
		return;	
	}
        // v1.31: Support PCW16 extended formats 
        if (deblock[drv][0] == 0xEB || deblock[drv][0] == 0xE9)
        {
                if (!memcmp(deblock[drv] + 0x2B, "CP/M", 4) &&
                    !memcmp(deblock[drv] + 0x33, "DSK",  3) &&
                    !memcmp(deblock[drv] + 0x7C, "CP/M", 4))
                {
                        memcpy(deblock[drv] + 0, deblock[drv] + 0x80, 16);
                }
        }       // end v1.31 change 
        makeDpb(R->ix, deblock[drv], 0);
	ret_ok(R);
}

void PcwHDC::fidRead(Z80 *R)	
{
	byte *data;
	byte img = R->b;
	unsigned trk = R->getHL();
	unsigned sec = R->getDE();
	m_lastDrv = img;
	long lsect, dlsect;
	word spt;
	dsk_err_t err;

	if (!openImage(img)) 
	{ 
		m_lastErr = "Could not open drive";
		ret_err(R); 
		return; 
	}
	//printf("Read drive %d, track %04x, sector %04x\n", img, trk, sec);
	
	spt = m_sys->m_cpu->fetchW(R->ix);			// Sectors/track [DPB]

	// lsect = address of sector (in CP/M's 512-byte units) 
	lsect = ((spt/4) * trk) + sec;
	// dlsect = address of sector in the drive's native sectors
	dlsect = (lsect * 512) / m_drvDg[img].dg_secsize;
	// Physical sector is smaller. Combine blocks.
	if (m_drvDg[img].dg_secsize < 512)
	{
		data = deblock[img];
		for (int ns = 0; ns < 512; ns += m_drvDg[img].dg_secsize)
		{
			err = dsk_lread(m_drvUnit[img], &m_drvDg[img], 
				data + ns, dlsect);
			if (err)
			{
				m_lastErr = dsk_strerror(err);
				ret_err(R);
				return;
			}
			++dlsect;
		}
	}	
	// Sector sizes match
	if (m_drvDg[img].dg_secsize == 512)
	{
		err = dsk_lread(m_drvUnit[img], &m_drvDg[img], deblock[img], lsect);
		if (err)
		{
			m_lastErr = dsk_strerror(err);
			ret_err(R);
			return;
		}
		data = deblock[img];
	}
	// Physical sector size is bigger. Deblock.
	if (m_drvDg[img].dg_secsize > 512)
	{
		if (!deblockRead(img, dlsect)) { ret_err(R); return; }
		if (lsect & 1) data = deblock[img] + 512;
		else	       data = deblock[img];
	}

	for (int nx = 0; nx < 512; nx++) 
	{
		m_sys->m_cpu->storeB(R->iy++, data[nx]);
	}
	ret_ok(R);
}


void PcwHDC::fidWrite(Z80 *R)	
{
	byte *data;
	byte img = R->b;
	unsigned trk = R->getHL();
	unsigned sec = R->getDE();
	m_lastDrv = img;
	long lsect, dlsect;
	word spt;
	dsk_err_t err;

	if (!openImage(img)) 
	{ 
		m_lastErr = "Could not open drive";
		ret_err(R); 
		return; 
	}
	
	spt = m_sys->m_cpu->fetchW(R->ix);			// Sectors/track [DPB]

	// lsect = address of sector (in CP/M's 512-byte units) 
	lsect = ((spt/4) * trk) + sec;
	// dlsect = address of sector in the drive's native sectors
	dlsect = (lsect * 512) / m_drvDg[img].dg_secsize;

	// Copy the data from Z80 memory into the buffer.
	data = deblock[img];
	// If we have to deblock, make sure the buffer has the correct
	// record in it.
	if (m_drvDg[img].dg_secsize > 512)
	{
		if (!deblockRead(img, dlsect)) { ret_err(R); return; }
 		if (lsect & 1) data += 512;
	}
	for (int nx = 0; nx < 512; nx++) 
	{
		data[nx] = m_sys->m_cpu->fetchB(R->iy++);
	}

	// Physical sector is smaller. Write multiple sectors.
	if (m_drvDg[img].dg_secsize < 512)
	{
		data = deblock[img];
		for (int ns = 0; ns < 512; ns += m_drvDg[img].dg_secsize)
		{
			err = dsk_lwrite(m_drvUnit[img], &m_drvDg[img], 
				data + ns, dlsect);
			if (err)
			{
				m_lastErr = dsk_strerror(err);
				ret_err(R);
				return;
			}
			++dlsect;
		}
	}	
	// Sector sizes match
	if (m_drvDg[img].dg_secsize == 512)
	{
		err = dsk_lwrite(m_drvUnit[img], &m_drvDg[img], deblock[img], lsect);
		if (err)
		{
			m_lastErr = dsk_strerror(err);
			ret_err(R);
			return;
		}
		data = deblock[img];
	}
	// Physical sector size is bigger. Deblock.
	if (m_drvDg[img].dg_secsize > 512)
	{
		deblockPending[img] = true;
		deblockSector[img]  = dlsect;
		if (R->c == 1)	// Immediate write
			if (!deblockWrite(img, dlsect)) { ret_err(R); return; }
	}
	ret_ok(R);
}


void PcwHDC::fidFlush(Z80 *R)	
{
	int img = R->b;
// [2.1.8] This can't be risked with rcpmfs. It can be called when
// the filesystem is in an inconsistent state, but rcpmfs will lose 
// data if the image is closed with an inconsistent FS. 
// 	closeImage(img);
	ret_ok(R);	
}

void PcwHDC::fidMessage(Z80 *R)	/* (v1.22) expand disc error message */
{
	char msg[98];

	sprintf(msg,"%c: %-92s", m_lastDrv + 'A', m_lastErr.c_str());

	R->setHL(m_fidText);
	R->f |= FLG_C;
	fidPuts(m_fidText, msg);			
}


void PcwHDC::setupPars(Z80 *R)
	// v1.22: Determine whether a drive is floppy 
{	// or hard, and set HL IX IY accordingly 
	int img = R->b;

	if (isHard(m_drvName[img], m_drvType[img]))	// Fixed
	{
		R->setHL(513);
		R->ix = 0;
		R->iy = 0;
	}
	else	// Floppy
	{
		R->setHL(91);
		R->ix = 64;
		R->iy = 1024;
	}
}



void PcwHDC::fidEms(Z80 *R)
{
	static int nhooked;
	static byte cdrv;
	char str[90];

/* This is called with: 

	A = 7
	B = flags, currently 0
	C = drive letter, currently 0FFh
	D = error:  0 => a drive has been hooked, E is its letter
		    1 => hook failure, no more memory
		    2 => hook failure, drive letter in use
		    3 => version control error
		    4 => start of fid_ems routine
	HL -> 93-byte message buffer

    returns: A=0 => ret NC
	     A=1 => ret C
	     A=2 => attempt to hook another disc drive
		      B=new drive letter

*/

	switch (R->d)
	{
		case 4:			/* Start of FIDEMS */
		if (R->c != 0xFF) m_allowFloppy = true;
		hooked = "";	/* No drives hooked yet */
		nhooked = 0;
		cdrv = 2;	/* Start trying to hook at C: */
		R->b = cdrv;
		R->a = 2;
		m_fidText = R->getHL();	/* (v1.22) save text buffer */
		setupPars(R);
		return;
	
		case 3:			/* Version control */
		fidPuts(R->getHL(), "JOYCE driver: Incorrect FID environment\r\n");
		R->a = 0;
		return;

		case 2:			/* Drive letter in use */
		if (cdrv < 0x0f)	/* (v1.22) Failed to get C: - O: */
		{			/* This is because that */
					/* letter's in use, so try the */
			R->a = 2;	/* next letter */
			++cdrv;
			R->b = cdrv;
			setupPars(R);
		}
		else if (nhooked)
		{
			sprintf(str,"JOYCE driver on drives %s\r\n", hooked.c_str());
			fidPuts(R->getHL(),str);
			R->a = 1;
		}
		else
		{
			fidPuts(R->getHL(), "JOYCE driver: No free drive letters\r\n");
			R->a = 0;
		}
		return;
		case 1:			/* No free memory */
		if (nhooked)
		{
			sprintf(str,"JOYCE driver on drives %s\r\n", hooked.c_str());
			fidPuts(R->getHL(),str);
			R->a = 1;
			return;
		}
		else
		{
			fidPuts(R->getHL(), "JOYCE driver: No memory available\r\n");
			R->a = 0;
			return;
		}
		case 0:			/* Drive hooked OK */
		nhooked++;
		sprintf(str,"%c: ", R->e + 'A'); 
		hooked += str;
		R->a = 2;
		++cdrv;
		R->b = cdrv;
		setupPars(R);
		return;
	}

}





static word to_al0(word blocks)
{
	word al0 = 0;

	while (blocks--) al0 = (al0 >> 1) | 0x8000;
	return al0;
}


static byte tomask(byte b)
{
	byte mask = 0;

	while (b--) mask = (mask << 1) | 1;
	return mask;
}


static word sizefrom(byte b)
{
	word W = 128;
	
	while (b--) W *= 2;

	return W;
}


void PcwHDC::makeDpb(word dpb, byte *format, int is_xdpb)
{
	word BLOCK_SIZE, OFF, NUM_TRACKS, NUM_SECTORS, DIR_BLOCKS,
	     SEC_SIZE;

	word SPT, BSH, BLM, DSM, DRM, AL0, AL1, EXM, CKS, PSH, PHM;

	BLOCK_SIZE  = sizefrom(format[6]);
	OFF         = format[5];
	NUM_TRACKS  = format[2]; if (format[1] & 3) NUM_TRACKS *= 2;
	NUM_SECTORS = format[3];
	DIR_BLOCKS  = format[7];
	SEC_SIZE    = sizefrom(format[4]);

	SPT = NUM_SECTORS * SEC_SIZE  / 128; 	
	BSH = format[6];	
	BLM = tomask(format[6]);
	DSM = ((NUM_TRACKS - OFF) * NUM_SECTORS * SEC_SIZE / BLOCK_SIZE) - 1;
	DRM = (DIR_BLOCKS * BLOCK_SIZE / 32) - 1;
	AL0 = (to_al0(DIR_BLOCKS) >> 8);
	AL1 = (to_al0(DIR_BLOCKS) & 0xFF);
	if (DSM < 256) EXM = (BLOCK_SIZE >> 10) - 1;
	else	       EXM = (BLOCK_SIZE >> 11) - 1;
	CKS = (DRM + 1) / 4;
	PSH = format[4];
	PHM = tomask(format[4]);

	m_sys->m_cpu->storeW(dpb,     SPT);
	m_sys->m_cpu->storeB(dpb + 2, BSH);
	m_sys->m_cpu->storeB(dpb + 3, BLM);
	m_sys->m_cpu->storeB(dpb + 4, EXM);
	m_sys->m_cpu->storeW(dpb + 5, DSM);
	m_sys->m_cpu->storeW(dpb + 7, DRM);
	m_sys->m_cpu->storeB(dpb + 9, AL0);
	m_sys->m_cpu->storeB(dpb +10, AL1);
	m_sys->m_cpu->storeW(dpb +11, CKS);
	m_sys->m_cpu->storeW(dpb +13, OFF);
	m_sys->m_cpu->storeB(dpb +15, PSH);
	m_sys->m_cpu->storeB(dpb +16, PHM);
}

// Take a quick look at a disc image to see if it's in MYZ80 format (and 
// therefore a hard drive) or not.
bool PcwHDC::isHard(const string filename, const char *type)
{
	DSK_PDRIVER pdr;
	DSK_GEOMETRY dg;
	int err;

	if (type && !strcmp(type, "myz80")) 	// All MYZ80 images are HD
		return true;
	if (type && strcmp(type, "rcpmfs"))	// All non-rcpmfs types aren't
		return false;

	err = dsk_open(&pdr, filename.c_str(), type, NULL);
	if (!err)
	{
		err = dsk_getgeom(pdr, &dg);
		if (err == 0 && dg.dg_sectors == 128 && dg.dg_secsize == 1024)
		{
			dsk_close(&pdr);
			return true;
		}
		dsk_close(&pdr);
	}
	return false;
}
