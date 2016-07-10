/************************************************************************

    JOYCE v2.1.3 - Amstrad PCW emulator

    Copyright (C) 1996, 2001-3  John Elliott <seasip.webmaster@gmail.com>

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
#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif
#ifdef HAVE_UTIME_H
#include <utime.h>
#endif
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
#ifdef HAVE_SYS_MOUNT_H
#include <sys/mount.h>
#endif
#ifdef HAVE_SYS_VFS_H
#include <sys/vfs.h>
#endif
#include "UiContainer.hxx"
#include "UiMenu.hxx"
#include "UiLabel.hxx"
#include "UiSeparator.hxx"
#include "UiSetting.hxx"
#include "UiCommand.hxx"

// These locations appear to be hardcoded in Rosanne 1.12, and the file 
// selector accesses them directly to tell if it's talking to a DOS, CP/M or
// Rosanne floppy. Our own os_change_disk must fake them, or the file selector
// will refuse to work.
#define FLOPPY_BANK	0x0A	// RAM bank containing these data

#define FLOPPY_ADDR(x)  (PCWRAM + ((FLOPPY_BANK * 0x4000) + (x & 0x3FFF)))

#define FLOPPY_MEDIA	FLOPPY_ADDR(0xBC28)
			// Media byte: 0FFh for CP/M, 0F9h or 0F0h for DOS
#define FLOPPY_ANNEFLAG	FLOPPY_ADDR(0xBDE8)
			// Nonzero if floppy has Rosanne structure

extern int gl_cputrace;

// Names of BIOS calls
#define bios_setink		32
#define	bios_getink		33
#define bios_setmode		34
#define bios_getmode		35
#define bios_setpatt		36
#define bios_getpatt		37
#define	bios_pixel		38
#define bios_getpxl		39
#define bios_line		40
#define bios_rectang		41
#define bios_block		42
#define bios_patblock		43
#define bios_storebitmap	44
#define bios_restorebitmap	45
#define bios_plotbitmap 	46
#define bios_plotmaskedbitmap	47
#define bios_plotvaribitmap	48

#define getBDE() (((unsigned)R->b) << 16 | *pde)
#define getCHL() (((unsigned)R->c) << 16 | *phl)



AnneAccel::AnneAccel(AnneSystem *s) :
        PcwDevice("accel", "anne")
{
	m_sys = s;
	m_fss.m_fpread = NULL;
	m_fss.m_fpwrite = NULL;
	m_fss.m_fsroot = findPcwDir(FT_ANNEFSROOT, false);
        XLOG("AnneAccel::AnneAccel()");
        reset();

	m_recursion = false;
	// Default to everything being disabled
	m_overrideGraphics = false;
	m_overrideFS       = false;
	m_overrideRamTest  = false;
}


AnneAccel::~AnneAccel()
{

}

void AnneAccel::reset(void)
{
//	enable(false);
	os_dos_init();
}


// Settings functions.
// See if this device has user-settable options. If it does, populates
// "key" and "caption" and returns true; else, returns false.
//
bool AnneAccel::hasSettings(SDLKey *key, string &caption)
{
        *key     = SDLK_a;
        caption = "  Rosanne acceleration  ";
	return true;
}
//
// Display settings screen
// 
UiEvent AnneAccel::settings(UiDrawer *d)
{
        int x,y, sel;
        UiEvent uie;

        x = y = 0;
        do
        {
                UiMenu uim(d);

                uim.add(new UiLabel("   Rosanne acceleration     ", d))
                   .add(new UiSeparator(d))
                   .add(new UiSetting(SDLK_e, isEnabled(),  "  Enabled       ",d));
		
		if (isEnabled())
		{
	                uim.add(new UiSeparator(d))
			   .add(new UiSetting(SDLK_m, m_overrideRamTest,  "  Memory test  ", d))
			   .add(new UiSetting(SDLK_f, m_overrideFS,       "  Floppy disc  ", d))
			   .add(new UiSetting(SDLK_g, m_overrideGraphics, "  Graphics operations  ", d));
		}
                uim.add(new UiSeparator(d))
		   .add(new UiCommand(SDLK_ESCAPE,  "  EXIT  ", d));
                // Only centre the menu once; we don't want it jumping about
		// the screen as options appear and disappear.
                if (!x)
                {
                        d->centreContainer(uim);
                        x = uim.getX();
                        y = uim.getY();
                }
                else
                {
                        uim.setX(x);
                        uim.setY(y);
                }

                if (sel >= (int)uim.size()) { uim.setSelected(-1); }
		else		uim.setSelected(sel);
                uie = uim.eventLoop();
                if (uie == UIE_QUIT) return uie;

                sel = uim.getSelected();
                switch(uim[sel].getKey())
                {
                        case SDLK_ESCAPE: patch(PCWRAM); return UIE_OK;
                        case SDLK_e:      enable(!isEnabled()); break;
			case SDLK_f:	  m_overrideFS       = !m_overrideFS; break;
			case SDLK_g:	  m_overrideGraphics = !m_overrideGraphics; 
					  patchBios();
					  break;
			case SDLK_m:	  m_overrideRamTest  = !m_overrideRamTest; break;
			default: break;
		}
	}
	while (1);
	return UIE_OK;
}


/* The ED FD operation gets inserted into PCW16 memory at 80:0043h, replacing 
 * a LD H,0 operation. HL is on the stack. L = A = Rosanne call number. */
void AnneAccel::edfd(Z80 *R)
{
	if (R->pc == 0x01DC || R->pc == 0x41DC)
	{
		fastboot(R);
		return;
	}
	if (R->pc >= 0x80 && R->pc <= 0x13D)
	{
		bios(R);
		return;
	}

	word vhl = R->fetchW(R->sp);
	word vde = R->getDE();
	word vbc = R->getBC();

	/* If call handled, force an os_nop but with no change to flags */
	if (override(R->l, &vbc, &vde, &vhl, R)) 
	{
/* Jump over the bit to calculate which call we're doing; it changes flags,
 * which we don't want. */
		R->pc += 0x0E;
		R->storeB(R->pc + 3, 0xC9);
	}
	R->storeW(R->sp,   vhl);
	R->storeW(R->sp+2, vde);
	R->storeW(R->sp+4, vbc);
	
	R->h = 0;	// The operation that was replaced
}



//
// Debugging helper. This prints out all of the Rosanne calls intercepted
// by the accelerator, with register dumps, state of the memory paging, 
// etcetera. 
//
void AnneAccel::showRosanneCall(byte function, word *pbc, word *pde, word *phl, Z80 *R)
{
	switch(function)
	{
		static byte prevdev;

		case 0: case 1: break;

		case 0x4C:	{
					unsigned chl = getCHL();
					printf("AnneAccel::override os_alert ");
					char *ptr = (char *)addr24(chl);
					do
					{
						printf("%s\\n", ptr);
						ptr += 1 + strlen(ptr);
					}
					while (*ptr);	
					printf("\n");
				}
				break;	
		case 0x4D:	{
					unsigned chl = getCHL();
					printf("AnneAccel::override os_report ");
					char *ptr = (char *)addr24(chl);
					do
					{
						printf("%s\\n", ptr);
						ptr += 1 + strlen(ptr);
					}
					while (*ptr);	
					printf("\n");
				}
				break;	
		case 0x4E:	printf("AnneAccel::override os_report_remove\n");
				break;
		case 0x51:	{
					unsigned chl = getCHL();
					printf("AnneAccel::override os_dialog_string ");
					char *ptr = (char *)addr24(chl);
					printf("%s\n", ptr + 8);
				}
				break;
		case 0x53:	printf("AnneAccel::override os_typo_init\n");
				break;
		case 0x54:	printf("AnneAccel::override os_typo_getlist\n");
				break;
		case 0x55:	printf("AnneAccel::override os_typo_select %d\n", *pde & 0xFF);
				break;
		case 0x56:	printf("AnneAccel::override os_typo_getselect\n");
				break;
		case 0x57:	printf("AnneAccel::override os_typo_def %d %dpt %x\n", *phl, *pde & 0xFF, *pde >> 8);
				break;
		case 0x58:	printf("AnneAccel::override os_typo_clr\n");
				break;
		case 0x59:	printf("AnneAccel::override os_typo_getdef\n");
				break;
		case 0x5A:	printf("AnneAccel::override os_typo_info\n");
				break;
		case 0x5B:	printf("AnneAccel::override os_typo_charinfo %d\n", R->c);
				break;
		case 0x5C:	printf("AnneAccel::override os_typo_charwidth %d\n", R->c);
				break;
		case 0x5D:	printf("AnneAccel::override os_typo_charscreen %d\n", R->c);
				break;
		case 0x7F: 	printf("AnneAccel::override os_set_device %02x -> %02x sp->%04x %04x %04x %04x %04x %04x "
					"%02x%02x%02x%02x\n",
					prevdev,
					R->b, 
					R->fetchW(R->sp + 6),
					R->fetchW(R->sp + 8),
					R->fetchW(R->sp + 10),
					R->fetchW(R->sp + 12),
					R->fetchW(R->sp + 14),
					R->fetchW(R->sp + 16),
					anneMemIO[0], anneMemIO[1], anneMemIO[2], anneMemIO[3]); 
					prevdev = R->b;
				break;
		case 0x8C:	{
					string fname = filename(*phl, *pde);
					printf("AnneAccel::override os_find_first b=%d dev=%d name=%s "
				               "SP->%04x %04x %04x %02x%02x%02x%02x\n", 
					R->b, prevdev, fname.c_str(),  
					R->fetchW(R->sp + 6),
					R->fetchW(R->sp + 8),
					R->fetchW(R->sp + 10),
					anneMemIO[0], anneMemIO[1], anneMemIO[2], anneMemIO[3]); 
				}
				break;
		case 0x90:	{
					string fname = filename(*phl, *pde);
					printf("AnneAccel::override os_open_file b=%d dev=%d name=%s "
				               "SP->%04x %04x %04x %02x%02x%02x%02x\n", 
						R->b, prevdev, fname.c_str(),  
						R->fetchW(R->sp + 6),
						R->fetchW(R->sp + 8),
						R->fetchW(R->sp + 10),
						anneMemIO[0], anneMemIO[1], anneMemIO[2], anneMemIO[3]); 
				}
				break;
		case 0x91:	{
					printf("AnneAccel::override os_close_file b=%d dev=%d "
				               "SP->%04x %04x %04x %02x%02x%02x%02x\n", 
						R->b, prevdev,
						R->fetchW(R->sp + 6),
						R->fetchW(R->sp + 8),
						R->fetchW(R->sp + 10),
						anneMemIO[0], anneMemIO[1], anneMemIO[2], anneMemIO[3]); 
				}
				break;
		case 0x93:	{
					unsigned chl = getCHL();
					unsigned bde = getBDE();
					printf("AnneAccel::override os_read_file: addr=%x len=%x dev=%d "
				               "SP->%04x %04x %04x %02x%02x%02x%02x\n", 
						chl, bde, prevdev,
						R->fetchW(R->sp + 6),
						R->fetchW(R->sp + 8),
						R->fetchW(R->sp + 10),
						anneMemIO[0], anneMemIO[1], anneMemIO[2], anneMemIO[3]); 
				}
				break;
/*		case 34: 	if (m_recursion) return false;
                   		m_recursion = true;
				printf("os_get_mem(b=%d e=%x)", R->b, R->e);
				cb_get_mem(R);
				printf(": B=%d HL=%x\n", R->b, R->getHL());
				*phl = R->getHL();
				*pde = R->getDE();
				*pbc = R->getBC();
				for (int j = 0; j < R->b; j++) printf("%02x ", fetch(R->getHL() + j));
       				m_recursion = false;
				return true;  */
		default: 	printf("AnneAccel::override %02x %04x %04x %04x sp->%04x %04x %04x %02x%02x%02x%02x\n", 
					function, *pbc, *pde, *phl,
					R->fetchW(R->sp + 6),
					R->fetchW(R->sp + 8),
					R->fetchW(R->sp + 10),
					anneMemIO[0], anneMemIO[1], anneMemIO[2], anneMemIO[3]); 
				break;
	}
}


/* Handle a Rosanne call. Return true to return from the call with the 
 * values as given (note that BC, DE, HL values were pushed onto the stack 
 * by now and so must be changed using the word pointers). The Z80's HL 
 * is currently set to the function number, so for true HL values you
 * must use the word pointer. */
bool AnneAccel::override(byte function, word *pbc, word *pde, word *phl, Z80 *R)
{
	//showRosanneCall(function, pbc, pde, phl, R);

	if (m_overrideFS)
	{
		if (overrideFS(function, pbc, pde, phl, R)) return true;
	}
	if (m_overrideGraphics)
	{
		patchBios();
		if (overrideGraphics(function, pbc, pde, phl, R)) return true;
	}
	return false;
}


                ///////////////////////////////////
////////////////// GRAPHICS OPERATION OVERRIDES  //////////////////////////////
                ///////////////////////////////////
inline unsigned parword(byte **p)
{
	unsigned w = p[0][1];
	w = (w << 8) | p[0][0];
	p[0] += 2;
	return w;
}


byte *AnneAccel::paraddr(unsigned width, unsigned height, byte **p)
{
	unsigned wb     = (width+7)/8;
	unsigned buflen = wb * height;
	byte *data      = new byte[buflen];
	unsigned cur 	= 0;
	unsigned addr = p[0][1];
	addr = (addr << 8) | p[0][0];
	int bank = p[0][2];
	byte *src;
	unsigned partlen;

	memset(data, 0xA3, buflen);
	/* Collect the data from each PcW16 bank in turn. The list of banks
	 * woll not be contiguous; hence we copy the bit contained in
	 * the selected bank, and call back to Z80 code to get the next
	 * bank. */
	while (cur < buflen)
	{
		src = addr24(bank, addr);
		partlen = 0x4000 - (addr & 0x3FFF);

		if (partlen > (buflen - cur)) partlen = buflen - cur;
		memcpy(data + cur, src, partlen);
		addr += partlen;
		cur  += partlen;
		addr &= 0x3FFF;
		bank = cb_get_next_mem(bank);
		if (bank < 0) break;
	}
	p[0] += 3;
	return data;
}


/* If a BIOS call is not hooked, hook it */
void AnneAccel::hook(int call, byte *jb)
{
	unsigned code;
	byte *pCode;

	if (jb[call*3] == 0xC3)
	{
		// Find out where the current jump is pointing.
		code =  jb[call*3 + 2];
		code =  (code << 8);
		code |= jb[call*3 + 1];
		pCode = addr16(code);

		// Save the jump, and replace it with ED/FD/C9.
		memcpy(m_oldJB + call * 3, jb + call * 3, 3);
		jb[call*3  ] = 0xED;
		jb[call*3+1] = 0xFD;
		jb[call*3+2] = 0xC9;

		// Just to be annoying, some Rosanne internal functions 
		// ignore the jumpblock and call the BIOS routines directly.
		// to catch these, put a jump from the internal function
		// down to the BIOS jumpblock.
		//
		memcpy(m_indJB + call * 3, pCode, 3);
		code = call * 3 + 0x80;

		pCode[0] = 0xC3;
		pCode[1] = (code & 0xFF);
		pCode[2] = (code >> 8);

	}
}


/* If a BIOS call is hooked, unhook it */
void AnneAccel::unhook(int call, byte *jb)
{
	unsigned code;
	byte *pCode;

	if (jb[call*3] == 0xED && m_oldJB[call*3] == 0xC3)
	{
		memcpy(jb + call * 3, m_oldJB + call * 3, 3);

		// And restore the first 3 bytes of the function.
		code =  jb[call*3 + 2];
		code =  (code << 8);
		code |= jb[call*3 + 1];
		pCode = addr16(code);
		memcpy(pCode, m_indJB + call * 3, 3);
	}
}


void AnneAccel::patchBios()
{
	if (!PCWRAM) return;	// No BIOS loaded yet

	byte *jumpblock = PCWRAM + 0x80;

	if (m_overrideGraphics)
	{
		hook(bios_block,            jumpblock);	
		hook(bios_patblock,         jumpblock);	
		hook(bios_plotbitmap,       jumpblock);	
		hook(bios_plotmaskedbitmap, jumpblock);	
		hook(bios_plotvaribitmap,   jumpblock);
	}
	else
	{
		unhook(bios_block,            jumpblock);	
		unhook(bios_patblock,         jumpblock);	
		unhook(bios_plotbitmap,       jumpblock);	
		unhook(bios_plotmaskedbitmap, jumpblock);	
		unhook(bios_plotvaribitmap,   jumpblock);
	}
}

void AnneAccel::bios(Z80 *R)
{
	int callno = (R->pc - 0x80) / 3;
	word x, y, w, h, count, ink;
	byte *param, *data, *mask;
	unsigned offset;

	switch(callno)
	{
		case bios_block:
			param = addr16(R->getHL());
			x = parword(&param);
			y = parword(&param);
			w = parword(&param);
			h = parword(&param);
			ink = cb_bios_getink();
			m_sys->m_activeTerminal->doRect(x,y,w,h,ink ? 8 : 0, cb_bios_getmode());
			break;

		case bios_patblock:
			param = addr16(R->getHL());
			x = parword(&param);
			y = parword(&param);
			w = parword(&param);
			h = parword(&param);
			ink = cb_bios_getink();
			m_sys->m_activeTerminal->doRect(x,y,w,h,8, 
				cb_bios_getmode(), cb_bios_getpatt());
			break;
	
		case bios_plotbitmap:
			param = addr16(R->getHL());
			x = parword(&param);
			y = parword(&param);
			w = parword(&param);
			h = parword(&param);
			data = paraddr(w, h, &param);
			m_sys->m_activeTerminal->doBitmap(x,y,w,h,data);
			delete data;
			break;

		case bios_plotmaskedbitmap:
			param = addr16(R->getHL());
			x = parword(&param);
			y = parword(&param);
			w = parword(&param);
			h = parword(&param);
			data = paraddr(w, h, &param);
			mask = paraddr(w, h, &param);
			m_sys->m_activeTerminal->doBitmap(x,y,w,h,data, mask);
			delete mask;
			delete data;
			break;
		case bios_plotvaribitmap:
			param = addr16(R->getHL());
			count = R->getDE();
			x = parword(&param);
			y = parword(&param);
			w = parword(&param);
			h = parword(&param);
			data = paraddr(count*16+8, count*16+8, &param);
			mask = paraddr(count*16+8, count*16+8, &param);
			m_sys->m_activeTerminal->doBitmap(x,y,w,h,data, mask, count);
			delete mask;
			delete data;
			break;
		default:
			printf("AnneAccel::bios(%d)\n", callno);
			break;
	}
}


bool AnneAccel::overrideGraphics(byte function, word *pbc, word *pde, word *phl, Z80 *R)
{
	switch(function)
	{
		case 117: return os_scr_direct(getCHL(), R);
	}
	return false;
}


bool AnneAccel::os_scr_direct(unsigned chl, Z80 *R)
{
	byte *params = addr24(chl);
	bool v;

	switch(params[0])
	{
		case 1: // solid white rectangle
		case 2: // solid black rectangle
		case 3: // solid grey rectangle
		case 4: // xor grey rectangle
			v = dg_blk_fill(params);
			if (v) R->a = 0;
			return v;
		case 6: // blit from place to place, then blank original
			v = dg_blk_copy(params, true);
			if (v) R->a = 0;
			return v;
		case 10: // blit from place to place
			v = dg_blk_copy(params, false);
			if (v) R->a = 0;
			return v;
	}

	return false;
}

bool AnneAccel::dg_blk_copy(byte *params, bool move)
{
	unsigned x, y, w, h, x2, y2;
	bool b;

	++params;
	x  = parword(&params);
	y  = parword(&params);
	w  = parword(&params);
	h  = parword(&params);
	x2 = parword(&params);
	y2 = parword(&params);

	if (x  > 639 || y  > 479 || (x+w ) > 639 || (y+h)  > 639 ||
	   x2 > 639 || y2 > 479 || (x2+w) > 639 || (y2+h) > 639) return false;
	/* Prepare to BitBlit. */
	cb_mouse_off();
	b = m_sys->m_activeTerminal->doBlit(x,y,w,h,x2,y2);		
	if (move && b)
	{
		/* If moving, blank out previous rectangle */
		b = m_sys->m_activeTerminal->doRect(x,y,w,h, 8, 0);
	}
	cb_mouse_on();
	return b;
}


bool AnneAccel::dg_blk_fill(byte *params)
{
	unsigned x, y, w, h;
	byte filltype = params[0];
	bool b;
	byte raster = 0;

	++params;
	x  = parword(&params);
	y  = parword(&params);
	w  = parword(&params);
	h  = parword(&params);
	switch(filltype)
	{
		case 1: filltype = 8; break;
		case 2: filltype = 0; break;
		case 3: filltype = params[0]; break;
		case 4: filltype = params[0]; raster = 3; break;
	}
	if (x  > 639 || y  > 479 || (x+w ) > 639 || (y+h)  > 639) return false;
	cb_mouse_off();
	b = m_sys->m_activeTerminal->doRect(x,y,w,h,filltype, raster);
	cb_mouse_on();
	return b;
}



void AnneAccel::cb_mouse_off()
{
	unsigned char *addr;

	Z80 *R = new AnneZ80(m_sys);
	// Call native bios_mousoff
	z80Callback(R, 0x80 + 3 * 7, 0);
	delete R;
}


void AnneAccel::cb_mouse_on()
{
	unsigned char *addr;

	Z80 *R = new AnneZ80(m_sys);
	// Call native bios_mouson
	z80Callback(R, 0x80 + 3 * 8, 0);
	delete R;
}




               /////////////////////////////////////
///////////////// FILESYSTEM OPERATION OVERRIDES  /////////////////////////////
               /////////////////////////////////////
//
// This function provides native versions of all the Rosanne filesystem calls
// for the floppy drive (FlashFS and LocoLinkFS are unaffected).
//
bool AnneAccel::overrideFS(byte function, word *pbc, word *pde, word *phl, Z80 *R)
{
	AnneFileSystemState fssSave;
	
	switch(function)
	{
//
// Handle os_typo_init. In its native incarnation this uses its own
// copy of the filesystem state so it can load fonts. We must replicate this.
// But for performance's sale, do it only if files are open.
//
		case 83:	if (m_recursion) return false;
				if (m_fss.m_fpwrite || m_fss.m_fpread)
				{
                  	 		m_recursion = true;
					saveFSState(fssSave);
					cb_typo_init(R);
					restoreFSState(fssSave);
      					m_recursion = false;
					return true;
				}
				break;
//
// Same thing for os_typo_def. Except that we also have to close any 
// files we're using so that os_typo_def doesn't try to use them.
//
		case 87: 	if (m_recursion) return false;
				if (m_fss.m_fpwrite || m_fss.m_fpread)
				{
             	      			m_recursion = true;
					saveFSState(fssSave);
					if (m_fss.m_fpwrite) fclose(m_fss.m_fpwrite);
					if (m_fss.m_fpread)  fclose(m_fss.m_fpread);
					m_fss.m_fpread  = NULL;
					m_fss.m_fpwrite = NULL;
					cb_typo_def(R, *pde, *phl);
					restoreFSState(fssSave);
       					m_recursion = false;
					return true; 
				}
				break;
		case 126:	os_dos_init(); break;
		case 127:	os_set_device(R->b); break;
		case 128:	// os_get_device
				if (m_device == 1)
				{
// v2.1.3: Can't change registers in the CPU; need to use pbc / pde / phl.
					// R->b = 1;
					*pbc &= 0xFF;
					*pbc |= 0x100;
					return true;
				}
				break;
		// case 129:	os_park_heads. No equivalent.
		case 130:	// os_change_disk
				if (m_device == 1)
				{
			/* Always OK */
					m_fss.m_discChanged = 0;
					*FLOPPY_MEDIA	 = 0xF9;
					*FLOPPY_ANNEFLAG = 1;
					R->f &= ~1;
					return true;
				}
				break;
		case 131:	// os_check_disk - disk never changes
				if (m_device == 1)
				{
					if (m_fss.m_discChanged) 
					{
						R->f |= 1;
						R->a = 0x22;
					}
					else R->f &= ~1;
					return true;
				}
		case 132:	if (m_device == 1)
				{
					os_disk_space(pbc, phl);
					*pde = 0;
					R->f &= ~1;
					return true;
				}
				break;
		// case 133:	os_flash_percent, flash only
		case 134:	if (m_device == 1)
				{
					os_get_cwd(*phl);
					R->f &= ~1;
					return true;
				}
				break;
		case 135:	if (m_device == 1) 
				{
				   os_set_cwd(*phl, R);	
				   return true;
				}
				break;
		case 140:	if (m_device == 1)
				{ 
				  os_find_first(*phl, *pde, R);
				  return true;
				}
				break;
		case 141:	if (m_device == 1)
				{ 
				  os_find_next(R);
				  return true;
				}
				break; 
/* os_get_fileinfo - always handled natively
		case 142:	{
				static int lock = 0; 
				if (!lock)
				{
					lock = 1;
					cb_get_fileinfo();
					lock = 0;
				}}
				break; 
*/
		case 143: 	if (m_device == 1)
				{
				  os_set_filedate(*phl, *pde, R);
				  return true;
				}
		case 144:	if (m_device == 1)
				{ 
				  os_open_file(*phl, *pde, R->b, R);
				  return true;
				}
				break; 
// These must be possible whether or not this is the currently selected device.
		case 145:	return os_close_file(R->b, R);
		case 146:	return os_write_file(getCHL(), getBDE(), R);
		case 147: 	if (!os_read_file(getCHL(), getBDE(), R)) return false;
				*pde = R->getDE();
				*pbc = R->getBC();
				return true;

		case 148: 	if (!os_copy_file(getBDE(), R)) return false;
				*pde = R->getDE();
				*pbc = R->getBC();
				return true;

		case 149:	if (m_device == 1)
				{ 
				  os_set_attrib(*phl, *pde, R->b, R);
				  return true;
				}
				break; 
		case 150:	if (m_device == 1)
				{ 
				  os_create_dir(*phl, R);
				  return true;
				}
				break; 
		case 151:	if (m_device == 1)
				{
				  os_structure(R);
				  return true;
				}
				break;
		case 152:	if (m_device == 1)
				{ 
				  os_remove_dir(*phl, R);
				  return true;
				}
				break; 
		case 153:	if (m_device == 1)
				{ 
				  os_delete_file(*phl, *pde, R);
				  return true;
				}
				break; 
		case 154:	if (m_device == 1)
				{ 
				  os_rename_file(*phl, *pde, R->ix, R->iy, R);
				  return true;
				}
				break; 
		case 155:	if (m_device == 1)
				{ 
				  os_get_ptr(R);
				  *pde = R->getDE();
				  *pbc = R->getBC();
				  return true;
				}
				break; 
		case 156:	if (m_device == 1)
				{ 
				  os_set_ptr(getBDE(), R);
				  return true;
				}
				break; 
		// The others relate to flash, or to the file selector.
	}
	return false;
}

void AnneAccel::os_dos_init()
{
	if (m_fss.m_fpread) fclose(m_fss.m_fpread);
	if (m_fss.m_fpwrite) fclose(m_fss.m_fpwrite);
	m_fss.m_fpread = NULL;
	m_fss.m_fpwrite = NULL;
	m_device = 0;
	m_fss.m_filename = "";
	m_fss.m_fscwd = m_fss.m_fsroot;
	m_fss.m_discChanged = true;
}


void AnneAccel::os_set_device(byte device)
{
//	printf("os_set_device: %d -> %d\n", m_device, device);
	m_device = device;
}


string AnneAccel::pathname(word vhl, word vde)
{
	char *fbuf = (char *)(PCWRAM + vhl);
	char *tbuf = NULL;
	string filename;
	int idx;

	if (vde) tbuf = (char *)(PCWRAM + vde);

	filename = m_fss.m_fscwd + "/" + fbuf;

	idx = filename.find(0x83);
	//printf("pathname: fbuf=%s filename='%s' idx=%d\n", fbuf, filename.c_str(), idx);
	if (idx >= 0)	/* Mangled <filename.type> */
	{
		/* Strip trailing spaces */
		while(filename[filename.length() - 1] == ' ')
			filename = filename.substr(0, filename.length() - 1);
		while(  (idx = filename.find(0x83)) >= 0)
		{
			filename[idx] = '.';
		}
	}
	else if (tbuf) filename += tbuf;
	
	//printf("pathname(%s,%s)->'%s'\n", fbuf, tbuf?tbuf:"", filename.c_str());
	return filename;	
}


string AnneAccel::filename(word vhl, word vde)
{
	char *fbuf = (char *)(PCWRAM + vhl);
	char *tbuf = NULL;
	string fname;
	int idx;

	if (vde) tbuf = (char *)(PCWRAM + vde);

	fname = fbuf;

	idx = fname.find(0x83);
	if (idx >= 0)	/* Mangled <filename.type> */
	{
		/* Strip trailing spaces */
		while(fname[fname.length() - 1] == ' ')
			fname = fname.substr(0, fname.length() - 1);
		while(  (idx = fname.find(0x83)) >= 0)
		{
			fname[idx] = '.';
		}
	}
	else if (tbuf) fname += tbuf;
	return fname;
}




void AnneAccel::os_find_first(word vhl, word vde, Z80 *R)
{
	m_fss.m_filename = pathname(vhl, vde);
	m_fss.m_fileno = 0;	
	m_fss.m_searchtype = R->b;

	/* On a directory search, ensure that all the PCW16FS directories
	 * are present as they should be. */
	if (R->b == 0x10) checkStructure(); 

	//printf("os_find_first: %s searchtype=%x\n", m_fss.m_filename.c_str(), R->b);
	
	if (findfile()) 
	{
		R->f &= ~1;	 
		//printf("  okay\n");
	}
	else
	{
		R->f |= 1;
		R->a = 2;	// File not found
		//printf("  failed\n");
	}
}


void AnneAccel::os_get_cwd(word vhl)
{
	char *fbuf = (char *)(PCWRAM + vhl);
	int idx;

	string strdir = m_fss.m_fscwd.substr( m_fss.m_fsroot.size() );
	//printf("os_get_cwd: strdir=%s\n", strdir.c_str());
	while ((idx = strdir.find('/')) >= 0)
	{
		strdir[idx] = '\\';
	}
	//printf("os_get_cwd: strdir=%s\n", strdir.c_str());
	strncpy(fbuf, strdir.c_str(), 64);
	fbuf[64] = 0;
}



void AnneAccel::os_set_cwd(word vhl, Z80 *R)
{
	string fbuf = (char *)(PCWRAM + vhl);
	int idx;

	//printf("os_set_cwd: DOS path %s\n", fbuf.c_str());

	while ((idx = fbuf.find('\\')) >= 0)
	{
		fbuf[idx] = '/';
	}
	//printf("os_set_cwd: UNIX path %s\n", fbuf.c_str());

	if (fbuf == ".") 	/* Stay exactly where we are */
	{
		
	}
	else if (fbuf == "..")	/* Up one */
	{
		idx = m_fss.m_fscwd.rfind('/');
		if (idx >= 0) m_fss.m_fscwd = m_fss.m_fscwd.substr(0, idx);
#ifdef HAVE_WINDOWS_H	// Windows findfirst/next
		if (m_fss.m_fscwd.size() == 2 && m_fss.m_fscwd[1] == ':') m_fss.m_fscwd += '/';
#endif
		if (m_fss.m_fscwd.size() == 0) m_fss.m_fscwd = '/';
	}
	else if (fbuf[0] == '/')	/* Absolute path */
	{
		m_fss.m_fscwd = m_fss.m_fsroot + fbuf;
	}
	else m_fss.m_fscwd = m_fss.m_fscwd + "/" + fbuf;	/* Relative path */
	//printf("os_set_cwd: New CWD = %s\n", m_fss.m_fscwd.c_str());
	
	R->f &= ~1;
}

void AnneAccel::os_find_next(Z80 *R)
{
	if (m_fss.m_filename == "")
	{
		//printf("os_find_next: Returning error, no findfirst done\n");
		R->a = 0x1;
		R->f |= 1;
		return;
	}
	if (findfile()) 
	{
		//printf("os_find_next: Returning OK\n");
		R->f &= ~1;	 
	}
	else
	{
		R->f |= 1;
		R->a = 0x12;	// No more files found
		//printf("os_find_next: Returning error, nothing found\n");
		m_fss.m_filename = "";	// End of search
	}
}


void AnneAccel::os_open_file (word hlv, word dev, byte b, Z80 *R)
{
	string filename = pathname(hlv, dev);
	FILE **ptr = (b ? &m_fss.m_fpwrite : &m_fss.m_fpread);

	printf("os_open_file %d %s\n", b, filename.c_str());
	if (*ptr)
	{
		R->a = 0x04;	/* File already open */
		R->f |= 1;
		printf("  already open\n");
		return;
	}
	*ptr = fopen(filename.c_str(), b ? "wb" : "rb");
	if (!*ptr)
	{
		R->a = b ? 0x05 : 0x02;	/* Access denied / file not found */
		R->f |= 1;
		printf("  failed\n");
		return;
	}
	if (b)
	{	
		m_fss.m_fnameWrite = filename;
		m_fss.m_fpwdate = 0;	// Date/time to write back to the 
		m_fss.m_fpwtime = 0;	// file being written
	}
	else	m_fss.m_fnameRead = filename;
	printf("  Okay\n");
	R->f &= ~1;
}


bool AnneAccel::os_close_file(byte b, Z80 *R)
{
	FILE **ptr = (b ? &m_fss.m_fpwrite : &m_fss.m_fpread);

	//printf("os_close_file(%d)\n", b);
	if (!*ptr) 
	{
// Either the file isn't open, or it's on a different device. Let Rosanne
// deal with it.
		return false;
	}
	fclose(*ptr);
	*ptr = NULL;
#ifdef HAVE_UTIME_H
	if (b && (m_fss.m_fpwdate || m_fss.m_fpwtime))
	{
// Update timestamps
		struct utimbuf buf;
		struct tm newtime;

		newtime.tm_sec = (m_fss.m_fpwtime &   0x1F) * 2;
		newtime.tm_min = (m_fss.m_fpwtime &  0x7E0) >> 5;
		newtime.tm_hour= (m_fss.m_fpwtime & 0xF800) >> 11; 
		newtime.tm_mday= (m_fss.m_fpwdate &   0x1F);
		newtime.tm_mon = (m_fss.m_fpwdate &  0x1E0) >> 5;
		newtime.tm_year=((m_fss.m_fpwdate & 0xFE00) >> 9) + 80;
		newtime.tm_isdst= -1;
		time(&buf.actime);
		buf.modtime = mktime(&newtime);
		utime(m_fss.m_fnameWrite.c_str(), &buf);
	}
#endif
	R->f &= ~1;
	return true;
}


bool AnneAccel::os_write_file(unsigned chl, unsigned bde, Z80 *R)
{
	byte *ptr = addr24(chl);

	// XXX Check RAM to write from
	if (!m_fss.m_fpwrite) return false;
//	printf("  os_write_file %d bytes\n", bde);
	if (fwrite(ptr, 1, bde, m_fss.m_fpwrite) < bde)
	{
		R->a = 0xA3;
		R->f |= 1;
	//	printf("  failed\n");
		return true;
	}
//	printf("  okay\n");
	R->f &= ~1;
	return true;
}


bool AnneAccel::os_read_file(unsigned chl, unsigned bde, Z80 *R)
{
	byte *ptr = addr24(chl);

	// XXX Check RAM to read into
	if (!m_fss.m_fpread) 
	{
		// Must be reading from Flash.
		return false;
	}
	//printf("os_read_file bde=%x chl=%x\n", bde, chl);
	bde -= fread(ptr, 1, bde, m_fss.m_fpread);
	//printf("   read_file bde=%x chl=%x\n", bde, chl);

	if (bde)
	{	
		R->setDE(bde & 0xFFFF);
		R->b  = (bde >> 16) & 0xFF;
		R->a = 0xA1;
		R->f |= 1;
		//printf("  failed\n");
		return true;
	}
	//printf("  %02x %02x %02x %02x %02x okay\n",
	//		ptr[0], ptr[1], ptr[2], ptr[3], ptr[4]);
	R->f &= ~1;
	return true;
}



bool AnneAccel::os_copy_file(unsigned bde, Z80 *R)
{
	if (!m_fss.m_fpread && !m_fss.m_fpwrite)	/* Copying from cabinet to cabinet */
		return false;
	
	//printf("os_copy_file begins\n");
	m_flashRead.clear();
	m_flashWrite.clear();	
	while (bde)
	{
		int ch;

		R->f &= ~1;
		if (m_fss.m_fpread) ch = fgetc(m_fss.m_fpread);
		else	      ch = cb_read_byte(R);

		if (R->f & 1) return true;	/* Error */
		if (ch == EOF)			/* Premature EOF */
		{
			//printf("Stopping, ch = EOF, bde=%d\n", bde);
			if (cb_flush(R) == EOF) return true;
			R->setDE(bde & 0xFFFF);
			R->b  = (bde >> 16) & 0xFF;
			R->a = 0xA1;	/* Premature EOF */
			R->f |= 1;
			return true;
		}
		R->f &= ~1;
		if (m_fss.m_fpwrite) ch = fputc(ch, m_fss.m_fpwrite);
		else	       ch = cb_write_byte(R, ch);

		if (R->f & 1) return true;	/* Error */
		if (ch == EOF)
		{
			if (cb_flush(R) == EOF) return true;
			R->setDE(bde & 0xFFFF);
			R->b  = (bde >> 16) & 0xFF;
			R->a = 0xA3;	/* Disc full */
			R->f |= 1;
			return true;
		}
		--bde;
	}
	//printf("os_copy_file ends\n");
	R->f &= ~1;
	cb_flush(R);
	return true;
}


void AnneAccel::os_set_attrib(word hlv, word dev, byte b, Z80 *R)
{
	R->f |= 1;
	R->a = 5;
}


void AnneAccel::os_set_filedate(word hlv, word dev, Z80 *R)
{
	if (!m_fss.m_fpwrite)
	{
		R->f |= 1;
		R->a = 2;
		return;
	}
	m_fss.m_fpwtime = hlv;
	m_fss.m_fpwdate = dev;
	R->f &= ~1;
}


void AnneAccel::os_create_dir(word hlv, Z80 *R)
{
	string filename = pathname(hlv);
	
#ifdef HAVE_WINDOWS_H
	if (mkdir(filename.c_str()))
#else
	if (mkdir(filename.c_str(), 0755))
#endif
	{
		if (errno == EEXIST) R->a = 0x50;
		else		     R->a = 0x13;
		R->f |= 1;
	}
	else R->f &= ~1;
}


// Check that the proper directory structure exists. This is not done using
// os_structure(), because rosanne only does that after formatting a floppy,
// and I don't want to get into hooking the BIOS format calls.
void AnneAccel::checkStructure()
{
	string s;
	int n;
	char buf[20];

	checkExistsDir(m_fss.m_fsroot);
	s = m_fss.m_fsroot + "/PCW";
	checkExistsDir(s);
// When creating the \PCW directory under Windows, do it forwards.
#ifdef HAVE_WINDOWS_H
	s = m_fss.m_fsroot + "/PCW/SYSTEM";
	checkExistsDir(s);
	for (n = 1; n <= 16; n++)
	{
		sprintf(buf, "/FOLDER%02d", n);
		s = m_fss.m_fsroot + "/PCW";
		s += buf;
		checkExistsDir(s);
	}	
#else

//
// On Linux+ext3, this creation order plus the GLOB_NOSORT below is necessary 
// to make the system folder appear first in the list. Don't ask me why.
//
	for (n = 16; n > 0; n--)
	{
		sprintf(buf, "/FOLDER%02d", n);
		s = m_fss.m_fsroot + "/PCW";
		s += buf;
		checkExistsDir(s);
	}	
	s = m_fss.m_fsroot + "/PCW/SYSTEM";
	checkExistsDir(s);
#endif
}


void AnneAccel::os_structure(Z80 *R)
{
	R->a = 5;
	R->f |= 1;
}


void AnneAccel::os_remove_dir(word hlv, Z80 *R)
{
	string filename = pathname(hlv);
	
	if (rmdir(filename.c_str()))
	{
		if (errno == ENOENT) R->a = 0x02;
		else		     R->a = 0x05;
		R->f |= 1;
	}
	else R->f &= ~1;
}


void AnneAccel::os_delete_file(word hlv, word dev, Z80 *R)
{
	string filename = pathname(hlv, dev);

	// XXX populate FILEINFO	
	if (remove(filename.c_str()))
	{
		if (errno == ENOENT) R->a = 0x02;
		else		     R->a = 0x05;
		R->f |= 1;
	}
	else R->f &= ~1;
}


void AnneAccel::os_rename_file(word hlv, word dev, word ixv, word iyv, Z80 *R)
{
	string oldname = pathname(hlv, dev);
	string newname = pathname(ixv, iyv);

	// XXX populate FILEINFO	
	if (rename(oldname.c_str(), newname.c_str()))
	{
		if      (errno == EISDIR) R->a = 0x50;
		else if (errno == EEXIST) R->a = 0x50;
		else if (errno == ENOENT) R->a = 0x02;
		else		          R->a = 0x05;
		R->f |= 1;
	}
	else R->f &= ~1;

}


void AnneAccel::os_disk_space(word *pbc, word *phl)
{
#ifdef HAVE_WINDOWS_H
	DWORD secclus, bps, freeclus, totclus;

	// XXX Check this is a full path.
	string drive = m_fss.m_fsroot.substr(0,3);
	if (GetDiskFreeSpace(drive.c_str(), &secclus, &bps, &freeclus, &totclus))
	{
		long bshift = (secclus * bps) / 512;
		long bfree  = freeclus * bshift;

		*phl = (bfree & 0xFFFF);
		*pbc &= 0xFF00;
		*pbc |= (bfree >> 16) & 0xFF;
		return;
	}

#endif	// def HAVE_WINDOWS_H

#if defined(HAVE_SYS_VFS_H) || defined (HAVE_SYS_MOUNT_H)
	struct statfs buf;

	if (!statfs(m_fss.m_fsroot.c_str(), &buf))
	{
		long bshift = buf.f_bsize / 512;
		long bfree  = buf.f_bavail * bshift;

		*phl = (bfree & 0xFFFF);
		*pbc &= 0xFF00;
		*pbc |= (bfree >> 16) & 0xFF;
		return;
	}
#endif	// def HAVE_SYS_VFS_H
	*pbc |= 0xFF;	// Default: Infinite amount of space
	*phl = 0xFFFF;
}

void AnneAccel::os_get_ptr(Z80 *R)
{
	if (!m_fss.m_fpread)
	{
		R->f |= 1;
		R->a = 0xC8;	/* File not open */
		return;
	}
	long offs = ftell(m_fss.m_fpread);

	R->setDE(offs & 0xFFFF);
	R->b = (offs >> 16) & 0xFF;
	R->f &= ~1;
}


void AnneAccel::os_set_ptr(unsigned bde, Z80 *R)
{
	if (!m_fss.m_fpread)
	{
		R->f |= 1;
		R->a = 0xC8;	/* File not open */
		return;
	}
	if (fseek(m_fss.m_fpread, bde, SEEK_SET)) 
	{
		R->f |= 1;
		R->a = 0xA1;	/* Seek failed */
		return;
	}
	R->f &= ~1;
}



////////////////////// FILEINFO functions ////////////////////////////////
// Populate various bits of FILEINFO.
//
void AnneAccel::clearFileinfo(unsigned char *finfo)
{
	static const char default_finfo[80] = 
	{
		 ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
		 '.', ' ', ' ', ' ',0x00,0x00,0x2A,0x2A,
		0x2A,0x2A,0x2A,0x2A,0x2A,0x2A,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		 '*', '*', '*', '*', '*', '*', '*', '*',
		 '*', '*', '*', '*', '*', '*', '*', '*',
		 '*', '*', '*', '*', '*', '*', '*', '*',
		 '*', '*', '*', '*', '*', '*', '*', '*',
		 '*', '*', '*', '*', '*', '*', '*', '*',
		 '*', '*', '*', '*', '*', '*', '*', '*'
	};
	memcpy(finfo,default_finfo, 80);
}

//
// Long and short filenames.
void AnneAccel::setFilename(unsigned char *finfo, const char *filename)
{
	string longname, namebit, typebit;
	int idx;

	namebit = filename;
/* Strip off trailing slash, if any */
	if (namebit[namebit.size() -1] == '/')
		namebit = namebit.substr(0, namebit.size() - 1);
/* Strip off any path */
	idx = namebit.rfind('/');
	if (idx >= 0)
	{
		namebit = namebit.substr(idx + 1);
	}
	longname = namebit;
	idx = namebit.rfind('.');

	if (idx >= 0)
	{
		typebit = namebit.substr(idx + 1);
		namebit = namebit.substr(0, idx);
	}
	else	
	{
		typebit = "   ";	
	}
// Mangle filename.typ (and remember to unmangle it in pathname() )
	do	
	{
		idx = longname.find('.');
		if (idx >= 0) 
		{
			longname[idx] = 0x83;
		}	
	} while (idx >= 0);
	sprintf((char *)finfo,    "%-8.8s.%-3.3s", namebit.c_str(), typebit.c_str());
	sprintf((char *)finfo+32, "%-32.32s",      longname.c_str());
	memset(finfo+64, '*', 16);
}


void AnneAccel::setFiledate(unsigned char *finfo, time_t tt)
{
	struct tm *tm = localtime(&tt);
	word date, time;

	date = (tm->tm_mday)  | ((tm->tm_mon + 1) << 5) | ((tm->tm_year - 80) << 9);
	time = (tm->tm_sec/2) | ( tm->tm_min      << 5) | ( tm->tm_hour       << 10);
	
	finfo[0x16] = time & 0xFF;
	finfo[0x17] = time >> 8;
	finfo[0x18] = date & 0xFF;
	finfo[0x19] = date >> 8;
}

void AnneAccel::setAttrib  (unsigned char *finfo, unsigned attrib)
{
	finfo[0x0D] = attrib;
}

void AnneAccel::setSize    (unsigned char *finfo, unsigned long size)
{
	finfo[0x1C] = (size      ) & 0xFF;
	finfo[0x1D] = (size >>  8) & 0xFF;
	finfo[0x1E] = (size >> 16) & 0xFF;
	finfo[0x1F] = (size >> 24) & 0xFF;
}


void AnneAccel::setInode   (unsigned char *finfo, unsigned inode)
{
	finfo[0x1A] = (inode     ) & 0xFF;
	finfo[0x1B] = (inode >> 8) & 0xFF;
}


void AnneAccel::setDirNo   (unsigned char *finfo, unsigned dirno)
{
	finfo[0x14] =  dirno & 0xFF;
	finfo[0x15] = (dirno >> 8) & 0xFF;
}


void AnneAccel::setXword1  (unsigned char *finfo, unsigned xword)
{
	finfo[0x0F] =  xword & 0xFF;
	finfo[0x10] = (xword >> 8) & 0xFF;
}
////////////////////// FILEINFO functions end ////////////////////////////


bool AnneAccel::findfile()
{
	int fileno;
	int n;
	unsigned char *finfo;
	bool isdir;

#ifdef HAVE_WINDOWS_H	// Windows findfirst/next
	bool bFindDirs;
	HANDLE hFind;
	WIN32_FIND_DATA wfd;
	FILETIME   tmLocal;

	bFindDirs = ((m_fss.m_searchtype & 0x10) != 0);

	hFind = FindFirstFile(m_fss.m_filename.c_str(), &wfd);
	if (hFind == INVALID_HANDLE_VALUE) return false;

	n = 0;
	isdir = ((wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0);
	/* We never include files starting '.', ie. "." and ".."
	 * We also skip directories when searching for normal files,
	 * and vice versa. */
	if (wfd.cFileName[0] == '.' || (bFindDirs != isdir)) --n;
//	printf("Found %s searching for %s  n=%d\n", 
//		wfd.cFileName,
//		m_fss.m_filename.c_str(), n);
	for (; n < m_fss.m_fileno; n++)
	{
		if (!FindNextFile(hFind, &wfd))
		{
			printf("Ran dry\n");
			FindClose(hFind);
			return false;
		}
		isdir = ((wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0);
		/* Skip over ., .. and files that are/aren't directories */
		if (wfd.cFileName[0] == '.' || (bFindDirs != isdir)) --n;
//		printf("Found %s searching for %s  n=%d\n", 
//			wfd.cFileName,
//			m_fss.m_filename.c_str(), n);
//		fflush(stdout);
	}
	FindClose(hFind);
	if (n < 0) return false;
//	printf("Finally reached %s searching for %s  n=%d\n", 
//		wfd.cFileName,
//		m_fss.m_filename.c_str(), n);
	finfo = cb_get_fileinfo();

	clearFileinfo(finfo);
	setFilename(finfo, wfd.cFileName);
	setAttrib  (finfo, wfd.dwFileAttributes);
// Windows gives us a shortcut to convert file time into DOS format. 
// Make use of it.
	tmLocal = wfd.ftLastWriteTime;
	FileTimeToDosDateTime(&wfd.ftLastWriteTime,
		(LPWORD)(finfo + 0x18), (LPWORD)(finfo + 0x16));	
	setSize(finfo, wfd.nFileSizeLow);
	setInode   (finfo, m_fss.m_fileno + 1);	// Fake cluster
	setXword1  (finfo, 0xFF01);
	setDirNo   (finfo, m_fss.m_fileno);
	++m_fss.m_fileno;
	return true;

#else	// Unix glob
	glob_t results;
	unsigned attrib;
	struct stat st;
	
	//printf("Doing glob(%s)\n", m_fss.m_filename.c_str());
/* Note GLOB_NOSORT. The SYSTEM folder must come first in the list despite
 * its name sorting to the end. */
        if (glob(m_fss.m_filename.c_str(), GLOB_NOSORT | GLOB_MARK, NULL, &results)) return false;
	//printf("... OK\n");
        for (n = fileno = 0; n < (int)results.gl_pathc; n++)
        {
                char *s = results.gl_pathv[n];

                /* Skip directories */
                if (s[0] && s[strlen(s) - 1] == '/') isdir = 1;
		else	isdir = 0;
		if (stat(s, &st))
		{
			joyce_dprintf("Can't stat %s\n", st);
			continue;
		}
		if (m_fss.m_searchtype & 0x10)
		{
			if (!S_ISDIR(st.st_mode)) continue;
		}
		if (fileno == m_fss.m_fileno)
		{
			finfo = cb_get_fileinfo();
		
			attrib = 0;
// Directory
			if (S_ISDIR(st.st_mode)) attrib |= 0x10;
// Read-only
			if (access(s, W_OK))     attrib |= 0x01;	
			//printf("Found %s\n", s);
			clearFileinfo(finfo);
			setFilename(finfo, s);
			setAttrib  (finfo, attrib);
			setFiledate(finfo, st.st_mtime);
			setSize    (finfo, st.st_size);
			setInode   (finfo, st.st_ino);
			setXword1  (finfo, 0xFF01);
			setDirNo   (finfo, fileno);
			globfree(&results);
			++m_fss.m_fileno;
			return true;
		}
		++fileno;
        }
	/* Nothing found */
	globfree(&results);
	//printf("Found nothing\n");
	return false;
#endif
}


void AnneAccel::saveFSState(AnneFileSystemState &save)
{
	save = m_fss;

	//printf("Saving FS state\n");
	if (save.m_fpread)
	{
		save.m_lastPosRead = ftell(save.m_fpread);
		//printf("Read file open: %s, at %d\n", save.m_fnameRead.c_str(), save.m_lastPosRead);
	}
	if (save.m_fpwrite)
	{
		save.m_lastPosWritten = ftell(save.m_fpwrite);
		//printf("Write file open: %s, at %d\n", save.m_fnameWrite.c_str(), save.m_lastPosWritten);
	}
}


void AnneAccel::restoreFSState(AnneFileSystemState &save)
{
// Three possibilities:
// 1. Since the save, file handles have not been changed.
// 2. Since the save, files were closed but not reopened.
// 3. Since the save, files have been closed and reopened.
//
// Since we can't tell between states 1 and 3, close the files if they're
// open, and then reopen them.
	//printf("Restoring FS state\n");
	if (m_fss.m_fpwrite) fclose(m_fss.m_fpwrite);
	if (m_fss.m_fpread)  fclose(m_fss.m_fpread);
	m_fss = save;
	if (save.m_fpread)
	{
		//printf("Reopening for read: %s @ %x\n", save.m_fnameRead.c_str(),
		//	save.m_lastPosRead);
		m_fss.m_fpread = fopen(save.m_fnameRead.c_str(), "rb");
		if (m_fss.m_fpread) fseek(m_fss.m_fpread, save.m_lastPosRead, SEEK_SET);
	}
	if (save.m_fpwrite)
	{
	//	printf("Reopening for write: %s @ %x\n", 
	//		save.m_fnameWrite.c_str(),
	//		save.m_lastPosWritten);
		m_fss.m_fpwrite = fopen(save.m_fnameWrite.c_str(), "r+b");
		if (m_fss.m_fpwrite) fseek(m_fss.m_fpwrite, save.m_lastPosWritten, SEEK_SET);
	}
}




// Patch the OS to call our code.
// Note: Patching the OS means that the checksum must be updated. Strictly
// we should only do this if the checksum was correct in the first place.
//
// At absolute address 0x10000 is a byte counter. This is decremented and 
// loaded into E.
//
// The checksum is a bytewise sum into IXDA'. In all banks up to E, all 16k
// is summed.
//
//
unsigned long AnneAccel::checksum(byte *rosanne, byte **pchksum)
{
	int off = 0x10000;
	int len = ((rosanne[off] - 4) * 16384) - 4;
	int n;
	unsigned long sum = 0;

	for (n = 0; n < len; n++) sum += rosanne[off + n];
	*pchksum = rosanne + off + len;	
	return sum;
}


void AnneAccel::patch(byte *rosanne)
{
	int n;
	int fixsum = 0;
	static unsigned char match[] = { 0xfe, 0xaa, 0x3f, 0xd8, 0xc5, 0xd5,
					 0xe5, 0x6f };
	byte *chksum;
	unsigned long sum;

	for (n = 0; n < 1048576 - sizeof match; n++)
	{
		if (!memcmp(rosanne + n, match, sizeof match))
		{

// See if the current ROM checksum is correct. 
			sum = ~checksum(rosanne, &chksum);
			if ((chksum[0] == (sum      ) & 0xFF) &&
			    (chksum[1] == (sum >>  8) & 0xFF) &&
			    (chksum[2] == (sum >> 16) & 0xFF) &&
			    (chksum[3] == (sum >> 24) & 0xFF)) fixsum = 1; 
// Poke in the enable / disable sequence.	
			if (isEnabled())
			{
				rosanne[n + sizeof match] = 0xED;
				rosanne[n + sizeof match+1] = 0xFD;
			}
			else
			{
				rosanne[n + sizeof match] = 0x26;
				rosanne[n + sizeof match+1] = 0x00;
			}
		}	// end if memcmp
	}	// end loop
// Patch for fast boot
	if (isEnabled())
	{
		rosanne[0x01DA] = 0xED;
		rosanne[0x01DB] = 0xFD;
	}
	else
	{
		rosanne[0x01DA] = 0x3E;
		rosanne[0x01DB] = 0x80;
	}

// If the checksum was correct before, make it correct now.
	if (fixsum)
	{
		//printf("Correcting the checksum\n");
		sum = ~checksum(rosanne, &chksum);
		chksum[0] = (sum      ) & 0xFF;
		chksum[1] = (sum >>  8) & 0xFF;
		chksum[2] = (sum >> 16) & 0xFF;
		chksum[3] = (sum >> 24) & 0xFF;
	}
}


bool AnneAccel::parseNode(xmlDocPtr doc, xmlNsPtr ns, xmlNodePtr cur)
{
        char *s = (char *)xmlGetProp(cur, (xmlChar *)"ramtest");
	if (s && !strcmp(s, "yes")) m_overrideRamTest = true;
	else			    m_overrideRamTest = false;
	if (s) xmlFree(s);

        s = (char *)xmlGetProp(cur, (xmlChar *)"filesys");
	if (s && !strcmp(s, "yes")) m_overrideFS = true;
	else			    m_overrideFS = false;
	if (s) xmlFree(s);

        s = (char *)xmlGetProp(cur, (xmlChar *)"graphics");
	if (s && !strcmp(s, "yes")) m_overrideGraphics = true;
	else			    m_overrideGraphics = false;
	if (s) xmlFree(s);
	patchBios();
	return parseEnabled(doc, ns, cur);
}

bool AnneAccel::storeNode(xmlDocPtr doc, xmlNsPtr ns, xmlNodePtr cur)
{
	xmlSetProp(cur, (xmlChar *)"ramtest", (xmlChar *)(m_overrideRamTest ? "yes" : "no"));
	xmlSetProp(cur, (xmlChar *)"filesys", (xmlChar *)(m_overrideFS ? "yes" : "no"));
	xmlSetProp(cur, (xmlChar *)"graphics", (xmlChar *)(m_overrideGraphics ? "yes" : "no"));
	return storeEnabled(doc, ns, cur);
}

////////////////////////////// CALLBACKS //////////////////////////////////
// These functions are calls into the Z80 code from native. In general they
// use their own copy of the Z80 processor.
#define PCODEBASE 0x2000

unsigned char *AnneAccel::cb_get_fileinfo()
{
	unsigned char *addr;

	Z80 *R = new AnneZ80(m_sys);
	// Call native os_get_fileinfo
	z80Callback(R, 0x3B, 142);
	//printf("cb_get_fileinfo: C=%02x HL=%04x\n", R->c, R->getHL());

	addr = addr24(R->c, R->getHL());
	delete R;
/* Dump the FILEINFO structure that we're returning 
	int n, m;
	for (n = 0; n < 128; n++)
	{
		if ((n & 0x0F) == 0x00) printf("%04x: ", n);
	//	printf("%02x ", addr[n]);
		if ((n & 0x0F) == 0x0F) 
		{
			for (m = 15; m >= 0; m--)
			{
				if (isprint(addr[n-m])) putchar(addr[n-m]);
				else	putchar('.');
			}
			putchar('\n');
		}
	}
*/
	return addr;	
}

int AnneAccel::cb_read_byte(Z80 *R)
{
	unsigned char backup[512];
	int rv, n;
	int buflen;

	R->f &= ~1;	/* Assume success unless stated otherwise */
	if (m_flashRead.empty())	/* fill it */
	{
		/* Backup the buffer we will use for data read. */
		memcpy(backup, PCWRAM + PCODEBASE + 512, 512);

		Z80 *R2 = new AnneZ80(m_sys);
		// Call native os_read_file - load 510 bytes at 
		z80Callback(R2, 0x3B, 147, 0x80, 510, PCODEBASE + 512);

		if (R2->f & 1)	/* Carry set - error */
		{
			/* Fatal error? */
			if (R2->a != 0xA1) 
			{
				R->a = R2->a;
				R->f |= 1;
				delete R2;
				memcpy(PCWRAM + PCODEBASE + 512, backup, 512);
				return EOF;
			}
			/* EOF */
			buflen = 510 - R2->getDE();
		}
		else buflen = 510;
		for (n = 0; n < buflen; n++)
		{
			m_flashRead.push_back(PCWRAM[PCODEBASE + 512 + n]);	
		}
		if (buflen < 510) m_flashRead.push_back(EOF);
		memcpy(PCWRAM + PCODEBASE + 512, backup, 512);
		delete R2;

	}
	rv = m_flashRead.front();
	m_flashRead.pop_front();	
	return rv;	
}

int AnneAccel::cb_flush(Z80 *R)
{
	unsigned char backup[512];
	int rv, n;
	int buflen;

	if (m_flashWrite.empty()) return 0;

	/* Backup the buffer we will use for data write. */
	memcpy(backup, PCWRAM + PCODEBASE + 512, 512);

	buflen = m_flashWrite.size();
	for (n = 0; n < buflen; n++)
	{
		PCWRAM[PCODEBASE + 512 + n] = m_flashWrite.front();
		m_flashWrite.pop_front();
	}

	Z80 *R2 = new AnneZ80(m_sys);
	// Call native os_write_file - write bytes
	z80Callback(R2, 0x3B, 146, 0x80, buflen, PCODEBASE + 512);
	memcpy(PCWRAM + PCODEBASE + 512, backup, 512);

	if (R2->f & 1)	/* Carry set - error */
	{
		R->a = R2->a;
		R->f |= 1;
		delete R2;
		return EOF;
	}
	delete R2;
	return 0;
}


int AnneAccel::cb_write_byte(Z80 *R, int ch)
{
	m_flashWrite.push_back(ch);
	if (m_flashWrite.size() == 510) return cb_flush(R);
	return ch;
}

void AnneAccel::cb_get_mem(Z80 *R0)
{
	unsigned char *addr;

	Z80 *R = new AnneZ80(m_sys);
	// Call native os_get_mem
	z80Callback(R, 0x3B, R0->a, R0->getBC(), R0->getDE(), R0->getHL());
	R0->f = R->f;
	R0->b = R->b;
	R0->h = R->h;
	R0->l = R->l;
	delete R;
}

// Given a memory bank, find the next one in the group.
int AnneAccel::cb_get_next_mem(byte mem)
{
	Z80 *R = new AnneZ80(m_sys);
	// Call native os_get_mem
	z80Callback(R, 0x3B, 0x28, 0, mem, 0);
	
	byte rv = R->e;
	byte f  = R->f;
	delete R;	
	if (f & 1) return -1;

	return rv;
}



void AnneAccel::cb_typo_init(Z80 *R)
{
	Z80 *R2 = new AnneZ80(m_sys);
	// Call native os_typo_init()
	z80Callback(R2, 0x3B, 0x53);

	R->a = R2->a;
	R->f = R2->f;
	delete R2;
}



void AnneAccel::cb_typo_def(Z80 *R, word vde, word vhl)
{
	Z80 *R2 = new AnneZ80(m_sys);
	// Call native os_typo_def()
	z80Callback(R2, 0x3B, 0x57, R->getBC(), vde, vhl);

	R->a = R2->a;
	R->f = R2->f;
	delete R2;
}

// Retrieve BIOS raster operation
byte AnneAccel::cb_bios_getmode()
{
	Z80 *R = new AnneZ80(m_sys);
	// Call native bios_getmode
	z80Callback(R, 0x80 + 3 * bios_getmode, 0);
	byte rv = R->a;	
	delete R;
	return rv;
}

// Retrieve BIOS ink colour
byte AnneAccel::cb_bios_getink()
{
	Z80 *R = new AnneZ80(m_sys);
	// Call native os_get_mem
	z80Callback(R, 0x80 + 3 * bios_getink, 0);
	byte rv = R->a;	
	delete R;
	return rv;
}

// Retrieve BIOS pattern definition
byte *AnneAccel::cb_bios_getpatt()
{
	Z80 *R = new AnneZ80(m_sys);
	// Call native os_get_mem
	z80Callback(R, 0x80 + 3 * bios_getpatt, 0);
	byte *rv = addr16(R->getHL());
	delete R;
	return rv;
}


/* Make a call from native code into emulated Rosanne. */

void AnneAccel::z80Callback(Z80 *R, word addr, byte a, word vbc, word vde, word vhl)
{
	byte backup[256];
	word oldpc, oldsp, pcode = PCODEBASE;

	memcpy(backup, PCWRAM + PCODEBASE, 256);

	PCWRAM[pcode++] = 0x3E;		/* A register */
	PCWRAM[pcode++] = a;
	PCWRAM[pcode++] = 0x01;		/* BC registers */
	PCWRAM[pcode++] = (vbc & 0xFF);
	PCWRAM[pcode++] = (vbc >> 8);
	PCWRAM[pcode++] = 0x11;		/* DE registers */
	PCWRAM[pcode++] = (vde & 0xFF);
	PCWRAM[pcode++] = (vde >> 8);
	PCWRAM[pcode++] = 0x21;		/* HL registers */
	PCWRAM[pcode++] = (vhl & 0xFF);
	PCWRAM[pcode++] = (vhl >> 8);
	PCWRAM[pcode++] = 0xCD;		/* Call to target address */
	PCWRAM[pcode++] = (addr & 0xFF);
	PCWRAM[pcode++] = (addr >> 8);
	PCWRAM[pcode++] = 0xED;		/* Return from emulation to native */
	PCWRAM[pcode++] = 0xFF;

	// Start with designated PC and SP
	R->mainloop(PCODEBASE, PCODEBASE+0x100);	
	memcpy(PCWRAM + PCODEBASE, backup, 256);
}


byte *AnneAccel::addr16(word hlv)
{
	return anneMem[hlv >> 14] + hlv;	
}


byte *AnneAccel::addr24(byte c, word hlv)
{
	byte *addr;

	if (c & 0x80)
		addr = PCWRAM   + ((c & 0x7F)*16384) + (hlv & 0x3FFF);
	else	addr = PCWFLASH + ((c & 0x7F)*16384) + (hlv & 0x3FFF);
	return addr;
}

byte *AnneAccel::addr24(unsigned chl)
{
	return addr24( ((chl >> 16) & 0xFF), chl & 0xFFFF);
}

////////////////////////////////////////////////////////////////////////////
// Skip all that tedious memory test stuff
//
extern int gl_cputrace;

bool AnneAccel::fastboot(Z80 *R)
{
	int n, v;
	long memsize = m_sys->m_mem->sizeAllocated();

	if (!m_overrideRamTest)
	{
		R->a = 0x80;
		return false;
	}

// These are for a 2Mb PCW.
	//printf("Doing fastboot...\n");
	memset(PCWRAM, 0, 0x10000);
// Exactly replicate what PCWRAM will hold after a successful memory check.
	PCWRAM[0x0007] = memsize / 16384L;
	PCWRAM[0x2000] = 0x80;
	PCWRAM[0x3FFA] = 0x4A;
	PCWRAM[0x3FFB] = 0x3F;
	PCWRAM[0x3FFC] = 0xCF;
	PCWRAM[0x3FFD] = 0x02;
	PCWRAM[0x3FFE] = 0x30;
	PCWRAM[0x3FFF] = 0x47;
	for (n = 0xFC00; n < 0xFFE0; n+=2) 
	{
		PCWRAM[n  ] = 0x00;
		PCWRAM[n+1] = 0x0C; 	
	}
	for (n = 0xFC08, v = 5; n < 0xFC4A; n+=2) 
	{
		PCWRAM[n] = v;
		v += 5;
	}
	memset(PCWRAM + 0x10000, 0xFF, memsize - 0x20000);
	memset(PCWRAM + memsize - 0x10000, 0, 0x10000);	
	R->a = 0;
	R->setBC(0xC0B5);
	R->setDE(0xFD30);
	R->setHL(0);
	R->ix = 0x2CF;
	R->iy = 0x423E;	
	R->pc = 0x4018;	// Warm boot function 472D;	// Immediately after the memory check // was 0x4018, warm boot
	m_sys->m_mem->out(0x81, 0x00);	// Select ROM at 4000h
//	gl_cputrace = 1;
	return true;
}

