/************************************************************************

    JOYCE v2.1.2 - Amstrad PCW emulator

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
	


/* State variables for the filesystem. Some Rosanne calls need to use the FS
 * themselves, and do it by storing the user's FS state on entry and 
 * retrieving it on return. We have to mimic this. */
class AnneFileSystemState
{
public:
	string m_filename;
	string m_fsroot;
	string m_fscwd;
	int m_fileno;
	int m_searchtype;
	bool m_discChanged;
	FILE *m_fpread, *m_fpwrite;
	word m_fpwdate, m_fpwtime;
	string m_fnameRead, m_fnameWrite;
	long   m_lastPosRead, m_lastPosWritten;
};


class AnneAccel : public PcwDevice
{
public:
	AnneAccel(AnneSystem *s);
	virtual ~AnneAccel();

	virtual void reset();
	virtual bool hasSettings(SDLKey *key, string &caption);
	virtual UiEvent settings(UiDrawer *d);
	void patch(byte *rosanne);
	void edfd(Z80 *R);
	void bios(Z80 *R);
	virtual bool parseNode(xmlDocPtr doc, xmlNsPtr ns, xmlNodePtr cur);
	virtual bool storeNode(xmlDocPtr doc, xmlNsPtr ns, xmlNodePtr cur);
protected:
	bool fastboot(Z80 *R);
	unsigned long checksum(byte *rosanne, byte **pchksum);	
	bool override(byte function, word *pbc, word *pde, word *phl, Z80 *R);
	bool overrideFS(byte function, word *pbc, word *pde, word *phl, Z80 *R);
	bool overrideGraphics(byte function, word *pbc, word *pde, word *phl, Z80 *R);

// For debugging: Show what's going on in a Rosanne call.
	void showRosanneCall(byte function, word *pbc, word *pde, word *phl, Z80 *R);

// Rosanne override functions
//   Graphics
	bool os_scr_direct(unsigned chl, Z80 *R);
	bool dg_blk_copy(byte *params, bool move);
	bool dg_blk_fill(byte *params);
	void patchBios(void);
//   File management
	void os_dos_init();
	void os_set_device(byte device);
	void os_set_filedate(word dostime, word dosdate, Z80 *R);
	void os_open_file (word hlv, word dev, byte b, Z80 *R);
	bool os_close_file(byte b, Z80 *R);
	bool os_write_file(unsigned chl, unsigned bde, Z80 *R);
	bool os_read_file(unsigned chl, unsigned bde, Z80 *R);
	bool os_copy_file(unsigned bde, Z80 *R);
	void os_set_attrib(word hlv, word dev, byte b, Z80 *R);
	void os_create_dir(word hlv, Z80 *R);
	void os_structure(Z80 *R);
	void os_remove_dir(word hlv, Z80 *R);
	void os_delete_file(word hlv, word dev, Z80 *R);
	void os_rename_file(word hlv, word dev, word ixv, word iyv, Z80 *R);
	void os_get_ptr(Z80 *R);
	void os_set_ptr(unsigned bde, Z80 *R);
	void os_find_first(word hlv, word dev, Z80 *R);
	void os_find_next(Z80 *R);
	void os_set_cwd(word hlv, Z80 *R);
	void os_get_cwd(word hlv);
	void os_disk_space(word *pbc, word *phl);
	void checkStructure(void);
// Callback from native to Z80
	void z80Callback(Z80 *R, word addr, byte a, word vbc = 0, word vde = 0, word vhl = 0);
	unsigned char *cb_get_fileinfo();
	void cb_get_mem(Z80 *R);
	int cb_get_next_mem(byte mem);
	void cb_typo_init(Z80 *R);
	void cb_typo_def(Z80 *R, word vde, word vhl);
	int cb_read_byte (Z80 *R);
	int cb_write_byte(Z80 *R, int value);
	int cb_flush(Z80 *R);
	byte *cb_bios_getpatt();
	byte cb_bios_getink();
	byte cb_bios_getmode();
	void cb_mouse_off();
	void cb_mouse_on();
	byte *addr16(word hlv);
	byte *addr24(unsigned chl);
	byte *addr24(byte c, word hlv);
	byte *paraddr(unsigned w, unsigned h, byte **p);
	
// Populate bits of fileinfo
 	void clearFileinfo(unsigned char *finfo);
	void setFilename(unsigned char *finfo, const char *filename);
	void setFiledate(unsigned char *finfo, time_t updated);
	void setAttrib  (unsigned char *finfo, unsigned attrib);	
	void setSize    (unsigned char *finfo, unsigned long size);	
	void setInode   (unsigned char *finfo, unsigned inode);	
	void setDirNo   (unsigned char *finfo, unsigned dirno);	
	void setXword1  (unsigned char *finfo, unsigned xword1);	
// Convert Rosanne filename/type to full pathname
	string pathname (word hlv, word dev = 0);
// Convert Rosanne filename/type to filename (no path)
	string filename (word hlv, word dev = 0);
	bool findfile();
// Hook / unhook a BIOS function
	void hook(int call, byte *jb);
	void unhook(int call, byte *jb);

	void saveFSState(AnneFileSystemState &save);
	void restoreFSState(AnneFileSystemState &save);

private:
	byte m_device;
	byte m_oldJB[0x200];
	byte m_indJB[0x200];
	AnneSystem *m_sys;
	AnneFileSystemState m_fss;

	list<int> m_flashRead;
	list<int> m_flashWrite;
	bool m_recursion;	// Stop this function intercepting its own calls
//
// Granular overrides of different aspects of the OS.
//
	bool m_overrideGraphics;
	bool m_overrideFS;
	bool m_overrideRamTest;
};

