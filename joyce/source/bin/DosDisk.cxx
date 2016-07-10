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
#include "DosFile.hxx"
#include "DosDisk.hxx"


UiEvent DosDisk::loadBasics(UiDrawer *d)
{
// Load boot sector
	static byte spec144[] = 
	{ 0x00, 0x02, 0x01, 0x01, 0x00, 0x02, 0xE0, 0x00, 0x40, 0x0B,
	  0xF0, 0x09, 0x00, 0x12, 0x00, 0x02, 0x00 };	
	// Load the boot sector
	dsk_err_t err = dsk_lread(m_rescue, &m_dg, m_bootsec, 0);
	if (err) return showDskErr(err, "", d);
	int n;
	byte rdir[14*512];

	if (memcmp(m_bootsec + 11, spec144, sizeof(spec144)))	
	{
		UiEvent uie;
		UiMenu uim(d);
		uim.add(new UiLabel("  This is not a PCW16 recovery disc.  ", d));
		uim.add(new UiSeparator(d));
		uim.add(new UiCommand(SDLK_t,      "  Try another disc  ", d));
		uim.add(new UiCommand(SDLK_ESCAPE, "  Cancel  ", d));
		uim.setSelected(-1);
		d->centreContainer(uim);
		uie = uim.eventLoop();
		if (uie == UIE_QUIT || uie == UIE_CANCEL) return uie;
                if (uim.getKey(uim.getSelected()) == SDLK_t)
			return UIE_OK; 
		return UIE_CANCEL;
	}
	// Load FAT
	err  =0;
	for (n = 0; n < 9; n++)
	{
		err = dsk_lread(m_rescue, &m_dg, m_fat + 512*n, n+1);
		if (err) break;	
	}
	// Load the root directory
	if (!err) for (n = 0; n < 14; n++)
	{
		err = dsk_lread(m_rescue, &m_dg, rdir + 512*n, n+19);
		if (err) break;	
	}
	if (err) return showDskErr(err, "", d);
	m_root.load(rdir, 14*512);

	return UIE_CONTINUE;	
}

int DosDisk::nextCluster(int cluster)
{
	byte *chain = m_fat + ((cluster / 2) * 3);
	if (cluster & 1) 
		return (chain[2] << 4) | (chain[1] >> 4);
	else	return chain[0] | ((chain[1] & 0x0F) << 8);
}

int DosDisk::fileSize(byte *dirEnt)
{
	int cluster = dirEnt[26] + 256 * dirEnt[27];
	int siz = 0;

	if (!cluster) return 0;
	while (cluster < 0xFF0) 
	{
		cluster = nextCluster(cluster);
		siz += 512;
	}
	return siz;
}

UiEvent DosDisk::loadFile(byte *dirEnt, UiDrawer *d, DosFile &file)
{
	dsk_err_t err;
	int cluster = dirEnt[26] + 256 * dirEnt[27];
	int len = 0;
	if (!cluster) return UIE_CONTINUE;
	
	byte *data = new byte[len = fileSize(dirEnt)];
	byte *ptr = data;	
	while (cluster < 0xFF0) 
	{
		err = dsk_lread(m_rescue, &m_dg, ptr, 31 + cluster);
		if (err)
		{
			delete data;
			return showDskErr(err, "", d);
		}
		cluster = nextCluster(cluster);
		ptr += 512;
	}
	file.load(data, len);
	delete data;
	return UIE_CONTINUE;	
}	



