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
#include "AnneFdc.hxx"


static const char *dsktypes[] =
{
        NULL, "floppy", "dsk", "raw", "myz80", "cfi"
};

static unsigned char bpb720[] =
{
	0xEB,0x40,0x90, 'I' , 'B', 'M', ' ', ' ' , '3', '.', '3',0x00 ,0x02,0x02,0x01,0x00,  //.@.IBM  3.3.....
	0x02,0x70,0x00,0xA0 ,0x05,0xF9,0x03,0x00 ,0x09,0x00,0x02,0x00 ,0x00,0x00,0x00,0x00,  //................
	0x00,0x00,0x00,0x00 ,0x00,0x00,0x29,0x96 ,0x19,0x03,0x19,0x50 ,0x43,0x57,0x31,0x36,  //......)....PCW16
	0x5F,0x31,0x31,0x31 ,0x20,0x20,0x46,0x41 ,0x54,0x31,0x32,0x20 ,0x20,0x20,0x00,0x00,  //_111  FAT12   ..
	0x18,0x4D,0x33,0xC0 ,0x8E,0xD0,0xBC,0x00 ,0x7C,0xBE,0x61,0x7C ,0x2E,0xAC,0x0A,0xC0,  //.M3.....|.a|....
	0x75,0x06,0x33,0xC0 ,0xCD,0x16,0xCD,0x19 ,0xB4,0x0E,0xBB,0x07 ,0x00,0xCD,0x10,0xEB,  //u.3.............
	0xEB,0x54,0x68,0x69 ,0x73,0x20,0x69,0x53 ,0x20,0x61,0x20,0x50 ,0x43,0x57,0x20,0x64,  //.This is a PCW d
	0x69,0x73,0x63,0x21 ,0x0D,0x0A,0x52,0x65 ,0x6D,0x6F,0x76,0x65 ,0x20,0x26,0x20,0x70,  //isc!..Remove & p
	0x72,0x65,0x73,0x73 ,0x20,0x61,0x20,0x6B ,0x65,0x79,0x2E,0x0D ,0x0A,0x0A,0x00
};


static unsigned char bpb1440[] =
{
	0xEB,0x40,0x90, 'I' , 'B', 'M', ' ', ' ' , '3', '.', '3',0x00 ,0x02,0x01,0x01,0x00,  //.@.IBM  3.3.....
	0x02,0xE0,0x00,0x40 ,0x0B,0xF0,0x09,0x00 ,0x12,0x00,0x02,0x00 ,0x00,0x00,0x00,0x00,  //...@............
	0x00,0x00,0x00,0x00 ,0x00,0x00,0x29,0x96 ,0x19,0x03,0x19,0x50 ,0x43,0x57,0x31,0x36,  //......)....PCW16
	0x5F,0x31,0x31,0x31 ,0x20,0x20,0x46,0x41 ,0x54,0x31,0x32,0x20 ,0x20,0x20,0x00,0x00,  //_111  FAT12   ..
	0x18,0x4D,0x33,0xC0 ,0x8E,0xD0,0xBC,0x00 ,0x7C,0xBE,0x61,0x7C ,0x2E,0xAC,0x0A,0xC0,  //.M3.....|.a|....
	0x75,0x06,0x33,0xC0 ,0xCD,0x16,0xCD,0x19 ,0xB4,0x0E,0xBB,0x07 ,0x00,0xCD,0x10,0xEB,  //u.3.............
	0xEB,0x54,0x68,0x69 ,0x73,0x20,0x69,0x53 ,0x20,0x61,0x20,0x50 ,0x43,0x57,0x20,0x64,  //.This is a PCW d
	0x69,0x73,0x63,0x21 ,0x0D,0x0A,0x52,0x65 ,0x6D,0x6F,0x76,0x65 ,0x20,0x26,0x20,0x70,  //isc!..Remove & p
	0x72,0x65,0x73,0x73 ,0x20,0x61,0x20,0x6B ,0x65,0x79,0x2E,0x0D ,0x0A,0x0A,0x00
};

UiEvent AnneFDC::askCreateDisc(UiDrawer *d, string filename, int fmt, const char **type)
{
	UiMenu uim(d);
	SDLKey key;	
	DSK_GEOMETRY dg;
	DSK_PDRIVER drv;
	char progress[80];
	unsigned char sector[512], *bpb;
	UiEvent uie;
	dsk_err_t err;
	int lsect;
	
	string sl1, sl2;
	sl1 = "  Cannot open: ";
	sl1 += displayName(filename, 40);
	sl1 += "  ";

	uim.add(new UiLabel(sl1, d));
	uim.add(new UiLabel("", d));
	uim.add(new UiCommand(SDLK_c, "  Create it (new 1.4M disc image)  ", d));
	uim.add(new UiCommand(SDLK_7, "  Create it (new 720k disc image)  ", d));
	uim.add(new UiCommand(SDLK_a, "  Abandon  ", d));
	d->centreContainer(uim);
	uie = uim.eventLoop();
	if (uie == UIE_QUIT || uie == UIE_CANCEL) return uie;
	key = uim.getKey(uim.getSelected());

	if (key != SDLK_7 && key != SDLK_c) return UIE_CANCEL;
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
	if (!err) err = dg_stdformat(&dg, (key == SDLK_7) ? FMT_720K : FMT_1440K, NULL, NULL);
	if (!err)
	{
		for (dsk_pcyl_t cyl = 0; cyl < dg.dg_cylinders; cyl++)
		{
			for (dsk_phead_t head = 0; head < dg.dg_heads; head++)
			{
				sprintf(progress, "Cyl %02d/%02d Head %d/%d",
					cyl+1,  dg.dg_cylinders, 
					head+1, dg.dg_heads);
				m_sys->m_termMenu->popupRedraw(progress, 1);
				err = dsk_apform(drv, &dg, cyl, head, 0xF6);
				if (err) break;
			}
			if (err) break;
		}
//
// Create a complete blank MS-DOS filesystem on the disc. This means the 
// emulated OS doesn't have to do any formatting.
//
		if (!err)
		{
			int n, m;

			// Create boot sector.
			memset(sector, 0, sizeof(sector));
			if (key == SDLK_7) bpb = bpb720;
			else		   bpb = bpb1440;
			memcpy(sector, bpb,  sizeof(bpb720));
			err = dsk_lwrite(drv, &dg, sector, 0);

			// Create FATs
			lsect = bpb[0x0E];		// Reserved sectors
			for (n = 0; n < bpb[0x10]; n++)	// FAT copies
			{
				memset(sector, 0, sizeof(sector));
				sector[0] = bpb[0x15];	// Media byte
				sector[1] = 0xFF;
				sector[2] = 0xFF;
				for (m = 0; m < bpb[0x16]; m++)
				{
					if (!err) err = dsk_lwrite(drv, &dg, sector, lsect++);
					if (sector[0]) memset(sector, 0, sizeof(sector));
				}
			}
			// Create the root directory
			memset(sector, 0, sizeof(sector));
			for (n = 0; n < bpb[0x11]; n += 16)
			{
				if (!err) err = dsk_lwrite(drv, &dg, sector, lsect++);
			}
			
		}			
		m_sys->m_termMenu->popupOff();
	}
	if (!err)	// Disk has been created. Great.
	{
		*type = dsktypes[fmt];
		dsk_close(&drv);
		return UIE_OK; 
	}	
	UiMenu uim2(d);
		
	sl1 = "  Cannot open: ";
	sl1 += displayName(filename, 40);
	sl1 += "  ";
	sl2 = dsk_strerror(err);

	uim2.add(new UiLabel(sl1, d));
	uim2.add(new UiLabel(sl2, d));
	uim2.add(new UiLabel("", d));
	uim2.add(new UiCommand(SDLK_a, "  Abandon  ", d));
	d->centreContainer(uim2);
	uie = uim2.eventLoop();
	if (uie == UIE_QUIT || uie == UIE_CANCEL) return uie;

	return UIE_CANCEL;
}


