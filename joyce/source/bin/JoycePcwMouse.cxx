/************************************************************************

    JOYCE v2.2.8 - Amstrad PCW emulator

    Copyright (C) 1996, 2001-2, 2016  John Elliott <seasip.webmaster@gmail.com>

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
#include "UiMenu.hxx"
#include "UiLabel.hxx"
#include "UiSeparator.hxx"
#include "UiSetting.hxx"
#include "UiCommand.hxx"

#define MAXLINE 288
#define MAXSTATE ((MAXLINE * 8) + 8)

JoycePcwMouse::JoycePcwMouse(JoyceSystem *s) :
        PcwDevice("mouse", "pcw"), PcwInput(s)
{
	XLOG("JoycePcwMouse::JoycePcwMouse()");
	m_mouseType = MT_AMX;
	m_autoPatch = true; 
	reset();
}


JoycePcwMouse::~JoycePcwMouse()
{
}


//
// Reset device
//
void JoycePcwMouse::reset(void)
{
	SDL_ShowCursor(1);
	m_tracking = false;
	m_cursorShown = true;
	m_patchApplied = 0;

// Set up for the PCW's 720x256 screen
	
	m_xmin = 40;
	m_xmax = 760;
	m_ymin = 44;
	m_ymax = 556;
// This scaling works well on normal JOYCE with Stop Press and MD3
	if (m_mouseType == MT_ESPEN) m_xscale = 1;
	else			     m_xscale = 2;
	m_yscale = 2;
	getMousePos(&m_ox, &m_oy);
// Disable patches

	m_mpmx = m_spmx = m_spmy = m_md3mx = m_md3my = m_md3mf1 = m_md3mf2 = 0;

// Reset lightpen counter. Start it off in the middle of a frame.
	m_lpc = MAXSTATE / 2;
	m_lcount_last = 0;

	updateSettings();
}


void JoycePcwMouse::updateSettings(void)
{

}

//
// Implement PCW mouse
//
void JoycePcwMouse::poll(Z80 *R)
{

}


Uint8 JoycePcwMouse::getMousePos(int *rx, int *ry)
{
	int x, y;
	Uint8 b;

	b = SDL_GetMouseState(&x, &y);

	if (x <  m_xmin) x = m_xmin;
	if (x >= m_xmax) x = m_xmax - 1;
	if (y <  m_ymin) y = m_ymin;
	if (y >= m_ymax) y = m_ymax - 1;

	x -= m_xmin; x /= m_xscale;
	y -= m_ymin; y /= m_yscale;

	*rx = x;
	*ry = y;
	return b;
}


int JoycePcwMouse::dx(int l)
{
	int x, y;

	getMousePos(&x, &y);

	y = x - m_ox;

	if (y > l)  y = l;
	if (y < -l) y = -l;
	
	m_ox += y;
	return y;	
}


int JoycePcwMouse::dy(int l)
{
	int x, y;

	getMousePos(&x, &y);
	
	x = y - m_oy;
	if (x > l)  x = l;
	if (x < -l) x = -l;

	m_oy += x;

	return x;	
}

int JoycePcwMouse::buttons(void)
{
	int x, y;
	return SDL_GetMouseState(&x, &y);
}




int JoycePcwMouse::handleEvent(SDL_Event &e)
{
	if (e.type != SDL_MOUSEMOTION) return 0;
	if (!m_tracking) return 0;

	trackMouse(e.motion.x, e.motion.y);
	return 1;
	
}
extern int tr;

byte JoycePcwMouse::in(word port)
{
	int y, x;

	port &= 0xFF;
	if (!isEnabled()) return 0xFF;

	if (m_autoPatch && (m_patchApplied < 2)) checkPatch();

//	printf("JoycePcwMouse::in(%x) mouseType=%d\n", port, m_mouseType);
//	tr = 2;

	switch(m_mouseType)
	{
/* According to the PCW Plus Tips Collection, AMX mouse interface is:
 *
 * IN A0 gives vertical movement -   low  nibble = no. of moves up
 *                                   high nibble = no. of moves down
 * IN A1 gives horizontal movement - low  nibble = no. of moves right
 *                                   high nibble = no. of moves left
 * IN A2 gives button state in bottom 3 bits, active low
 * 
 * Stop Press also does this:
 *  
 * OUT A3, 93h (initialise mouse?)
 * and repeatedly OUT A3,FF ! OUT A3,0  (reset mickey counters?)
 *
 * The Tips Collection then says that Kempston uses a "similar" technique.
 * From disassembling Stop Press, Kempston works like this:
 *
 * IN D0 gives X position, 0-255. } 
 * IN D1 gives Y position, 0-255. } These are "mickey counters" of some kind
 *                                 and can quite happily roll over. The address
 *                                 is incompletely decoded and the counters 
 *                                 also appear at IN D2 / IN D3.
 * IN D4 gives button state in bottom 2 bits, active low
 */

		case MT_AMX: /*printf("PC=%04x\n", jstate.m_sys->m_cpu.pc); */switch(port)
		{
			case 0xA0: y = dy(15);
				   if (y < 0) return (-y);
				   else       return ( y << 4);
			case 0xA1: x = dx(15);
				   if (x > 0) return (x);
				   else       return (-x) << 4;
			case 0xA2: x = 0xFF;
				   y = buttons();
		                   if (y & 1) x &= 0xFE;        /* L */
       			           if (y & 2) x &= 0xFD;        /* M */
            			   if (y & 4) x &= 0xFB;        /* R */
				   return x; 
			case 0xA3:
				   return 0;

		}
		break;	// Don't fall through to Kempston here. In theory
			// you could have both mice connected and working
			// in tandem, but this is not a good idea. The main
			// problem is that programs which auto-probe the 
			// mouse to use will get confused about what's there.
			// It also causes trouble with the MasterPaint 
			// auto-patch, because the auto-patch patches the
			// AMX driver, but MasterPaint will use the 
			// Kempston one.

		case MT_KEMPSTON: switch(port)
		{
			case 0xD0: 
			case 0xD2: dx(15); return (((3*m_ox)/2) & 0xFF);
			case 0xD1: 
			case 0xD3: dy(15); return (-m_oy) & 0xFF;
			case 0xD4: x = 0xFF;
				   y = buttons();
				   if (y & 1) x &= 0xFD;
				   if (y & 2) x &= 0xFE;
				   if (y & 4) x &= 0xFE;
				   return x;
		}
		break;

		case MT_ESPEN:	
			if (port == 0xA6 || port == 0xA7)
			{
			        int col, row;
				unsigned short count = 0;

        			getMousePos(&col, &row);

/* count0 gets updated when the lightpen is triggered. Otherwise it retains
 * its last value, m_lcount_last */

				count = m_lcount_last & 0xFFF;

/* m_lpc says what state we're in. 
 * 0-7: Start of frame. Return VSYNC set for the first 4 reads, then clear 
 *     for the next 4 */
				if (m_lpc < 4)
				{
					count |= 0x4000;
				}
				else if (m_lpc < 8)
				{
					/* do nothing */
				}
/* 8 - (MAXLINE * 8)+7: On a scanline. Return HSYNC set on the second read */
				else if (m_lpc < (MAXLINE * 8 + 8))
				{
					int y = ((m_lpc / 8) - 37);
					/* In horizontal retrace? */
					if ((m_lpc % 8) == 1)
					{
						count |= 0x2000;
/* If lightpen triggered on the current row, encode column */
						if (y == row)
						{
							count += (1568 - 2*col);
							count &= 0xFFF;
						}
					}
				}
/* (MAXLINE *8) + 8 - MAXSTATE: Back into VSYNC */
				else 
				{
					count |= 0x4000;
				}
				m_lcount_last = count & 0xFFF;
				++m_lpc;				
				if (m_lpc >= MAXSTATE) m_lpc = 0;

				if (port == 0xA6) 
					return (count & 0xff);
				if (port == 0xA7) 
					return (count >> 8);
			}

		default: break;
	}
	return 0xFF;
}




//
// Settings functions.
// See if this device has user-settable options. If it does, populates
// "key" and "caption" and returns true; else, returns false.
//
bool JoycePcwMouse::hasSettings(SDLKey *key, string &caption)
{
	*key     = SDLK_m;
	caption = "  Mouse / lightpen  "; 
	return true;
}
//
// Display settings screen
// Return 0 if OK, -1 if quit message received.
// 
UiEvent JoycePcwMouse::settings(UiDrawer *d)
{	
	int x,y, sel;
	UiEvent uie;

	x = y = 0;
	do
	{
		UiMenu uim(d);

		uim.add(new UiLabel(" Pointing device settings    ", d))
		   .add(new UiSeparator(d))
                   .add(new UiSetting(SDLK_n, true, "  No mouse connected   ",d))
	           .add(new UiSetting(SDLK_a, false,"  AMX-type mouse       ",d))
                   .add(new UiSetting(SDLK_k, false,"  Kempston mouse       ",d))
                   .add(new UiSetting(SDLK_e, false,"  Electric Studio lightpen  ",d));

		if (isEnabled())
		{
			((UiSetting &)uim[2]).setChecked(false);
			if (m_mouseType == MT_AMX)      
			{	
				((UiSetting &)uim[3]).setChecked(true);
				uim.add(new UiSeparator(d))
				   .add(new UiSetting(SDLK_p,  m_autoPatch, "  Automatic Patch on   ", d))
				   .add(new UiSetting(SDLK_o, !m_autoPatch, "  Automatic patch Off  ", d));
			}
			if (m_mouseType == MT_KEMPSTON) 
			{
				((UiSetting &)uim[4]).setChecked(true);
			}
			if (m_mouseType == MT_ESPEN) 
			{
				((UiSetting &)uim[5]).setChecked(true);
			}
		}
		uim.add(new UiSeparator(d))
                   .add(new UiCommand(SDLK_ESCAPE, "  EXIT  ", d));
	
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
		uie = uim.eventLoop();
		if (uie == UIE_QUIT) return uie;
		
		sel = uim.getSelected();	
		switch(uim[sel].getKey())
		{
			case SDLK_ESCAPE: updateSettings(); return UIE_OK; 
			case SDLK_n:   enable(false); break;
			case SDLK_a:   enable(true); 
				       m_mouseType = MT_AMX; 
				       break;
			case SDLK_k:   enable(true); 
				       m_mouseType = MT_KEMPSTON; 
				       m_autoPatch = false;
				       break;
			case SDLK_e:   enable(true); 
				       m_mouseType = MT_ESPEN;
				       m_autoPatch = false;
				       break;
			case SDLK_p:   m_autoPatch = true;  break;		
			case SDLK_o:   m_autoPatch = false; break;		
			default: break;
		}
	} while(1);
	return UIE_OK;
}
//
// Implement direct access to the mouse registers
//
// (v1.27)  New mouse capture code
// 
//               The idea of this code is to bypass all that tedious mucking 
//             about with I/O ports and mickeys, and to pass mouse coordinates
//             directly into the program.
//  
//                The problem is: there is no standard mouse driver interface.
//              What we have to do is trap a mouse read, and find out where the
//              program keeps its mouse driver code. We then patch it.


void JoycePcwMouse::edfe(Z80 *R)
{
        Uint8  btn;
        int col, row;

	// Get coordinates, scaled to PCW screen
        btn = getMousePos(&col, &row);
        if (m_md3mx)      /* MD3 patch active? */
        {
                row = 255 - row;
                if (row == fetch2(m_md3my) && (col == fetch2(m_md3mx)))
                {
                        R->a = 0;
                        return;
                }
                store2(m_md3mx,  col);
                store2(m_md3my,  row);
                ::store(m_md3mf1, 0xFF);
                ::store(m_md3mf2, 0xFF);
                R->a = 1;
        }
        if (m_spmx)
        {
                store2(m_spmx, col);
                store2(m_spmy, row);
        }
        if (m_mpmx)
        {
                store2(m_mpmx,   col);
                store2(m_mpmx+2, row);
        }

	
}
//
// nb: Ability to dump state is not yet provided, but would go here.
//

bool JoycePcwMouse::parseNode(xmlDocPtr doc, xmlNsPtr ns, xmlNodePtr cur)
{
        char *s = (char *)xmlGetProp(cur, (xmlChar *)"type");

        if (s)
        {
                if      (!strcmp(s, "amx"))      m_mouseType = MT_AMX;
                else if (!strcmp(s, "kempston")) m_mouseType = MT_KEMPSTON;
                else if (!strcmp(s, "es_pen"))   m_mouseType = MT_ESPEN;
                else joyce_dprintf("Mouse type '%s' is unknown - ignored\n",s);
		xmlFree(s);
        }
        s = (char *)xmlGetProp(cur, (xmlChar *)"patch");
        if (s)
        {
                if      (!strcmp(s, "1")) m_autoPatch = 1;
                else if (!strcmp(s, "0")) m_autoPatch = 0;
                else joyce_dprintf("Mouse patch setting must be 1 or 0, not %s\n", s);
		xmlFree(s);
        }

	return parseEnabled(doc, ns, cur);	
}


bool JoycePcwMouse::storeNode(xmlDocPtr doc, xmlNsPtr ns, xmlNodePtr cur)
{
        switch(m_mouseType)
        {
                case  MT_AMX:      xmlSetProp(cur, (xmlChar *)"type", (xmlChar *)"amx");      break;
                case  MT_KEMPSTON: xmlSetProp(cur, (xmlChar *)"type", (xmlChar *)"kempston"); break;
                case  MT_ESPEN:    xmlSetProp(cur, (xmlChar *)"type", (xmlChar *)"es_pen"); break;
    		default:	   break;
	}
	return storeEnabled(doc, ns, cur);
}

//
// Code supporting the automatic mouse patch. When auto-patching is on and
// the patch has worked as expected, then we hide the X mouse pointer, since
// we know that the PCW mouse pointer is going to be in the right place.
//
// This function hides the pointer if it's in the PCW screen.
//
void JoycePcwMouse::trackMouse(Uint16 x, Uint16 y)
{
	int nsc = m_cursorShown;

	if (!m_tracking) 
	{
		if (!m_cursorShown) 
		{
			m_cursorShown = 1; 
			SDL_ShowCursor(1); 
			return; 
		}
	}

        if (x < 40 || x >= 760 || y < 44 || y >= 556) nsc = 1;
	else nsc = 0;

	if (m_cursorShown != nsc) 
        {
                SDL_ShowCursor(nsc);
              	m_cursorShown = nsc; 
        }
}

//
// Patching helper: compare (count) bytes in Z80 memory at (addr), with 
// (count) bytes at (compare). Returns 0 if they're the same, -1 if not.
//
static int zmemcmp(word addr, byte *compare, int count)
{
        int n;

        for (n = 0; n < count; n++) if (fetch(addr++) != compare[n]) return -1;
        return 0;
}
//
// Write (count) bytes into Z80 memory at (addr).
//
static void zmemput(word addr, byte *replace, int count)
{
        int n;
        for (n = 0; n < count; n++) store(addr++, replace[n]);
}

//
// Check for the MD3, Stop Press or MicroDesign mouse drivers. If they are
// found, patch them to access the JOYCE mouse directly.
//
void JoycePcwMouse::checkPatch(void)
{
	Z80 *R = m_sys->m_cpu;

        int    patch_add = R->pc - 1;   /* Address of calling code */

        static byte md3_match1[5] = {0xDB, 0xA2, 0xF6, 0xF8, 0xC9};
        static byte md3_match2[6] = {0xCD, 0x00, 0x00, 0x28, 0x0F, 0x2A};
        static byte md3_patch2[6] = {0x3E, 0xFD, 0xED, 0xFE, 0xA7, 0xC9};
        static byte sp_match[9]   = {0xED, 0x78, 0xFE, 0x10, 0x20,
                                     0x0E, 0x01, 0x80, 0x81};
        static byte sp_patch[7]   = {0x3E, 0xFD, 0xED, 0xFE, 0xC3, 0x00, 0x00};
        static byte mp_match1[7]  = {0xDB, 0xA2, 0x3C, 0x28, 0x49, 0x3D, 0x21};
        static byte mp_match2[14] = {0xDB, 0xD0, 0x47, 0xDB, 0xD1, 0x4F, 0xDB,
                                     0xD4, 0x57, 0xA0, 0xA1, 0x3C, 0x28, 0x50};

        // Check for MasterPaint code. The first check triggers the patch
        // mechanism, but we're not ready to do the patch yet. So we put
        // it into patch mode 2 (watch permanently). The second check is
        // called when we are ready to do the patch. 

	m_patchApplied = 99;	// Don't check again

        if (!zmemcmp(patch_add, mp_match1, 7))
        {
                m_patchApplied = 1;
                m_sys->m_termMenu->popup("Detected MasterPaint...", 0, 1);
	}
        if (!zmemcmp(patch_add, mp_match2, 14))
        {
		m_sys->m_termMenu->popupOff();
                m_mpmx = fetch2(patch_add + 0x69);
                sp_patch[4] = 0xC9;
                zmemput(patch_add + 0x68, sp_patch, 5);
                sp_patch[4] = 0xC3;
                m_patchApplied = 2;
                m_sys->m_termMenu->popup("Applied MasterPaint mouse patch", 4, 1);
        }
	        /* Check for md3 code */
        if (!zmemcmp(patch_add, md3_match1, 5))
        {
                md3_match2[1] = (patch_add - 0x20) & 0xFF;
                md3_match2[2] = (patch_add - 0x20) >> 8;
                if (!zmemcmp(patch_add - 0x254, md3_match2, 6))
                {
                        zmemput(patch_add - 0x254, md3_patch2, 6);

                        m_md3my  = fetch2(patch_add - 0x24E);
                        m_md3mx  = fetch2(patch_add - 0x23B);
                        m_md3mf1 = fetch2(patch_add - 0x228);
                        m_md3mf2 = fetch2(patch_add - 0x225);

                        m_sys->m_termMenu->popup("Applied MD3 mouse patch", 4, 1);
			m_patchApplied = 3;
                }
        }
        /* Check if we can recognise the Stop Press mouse code; and if we can,
         *            hook it. */
	patch_add--;
        if (!zmemcmp(patch_add, sp_match, 9))
        {
                sp_patch[5] = (patch_add + 0x103) & 0xFF;
                sp_patch[6] = (patch_add + 0x103) >> 8;
                zmemput(patch_add + 0x88, sp_patch, 7);

                m_spmx = fetch2(patch_add + 0x25);
                m_spmy = fetch2(patch_add + 0x2b);
                m_sys->m_termMenu->popup("Applied Stop Press mouse patch", 3, 1);
		m_patchApplied = 4;
        }
	if (m_patchApplied == 99)
	{
		m_sys->m_termMenu->popup("No mouse patch applied", 3, 1);
	}
        if (m_patchApplied >= 2 && m_patchApplied < 99)   // Switch on tracking 
        {
                int x, y;

                SDL_GetMouseState(&x, &y);

                m_tracking = 1;
                trackMouse(x, y);
		m_xscale = 1;
        }


}


void JoycePcwMouse::out(word address, byte value)
{
// The AMX mouse driver likes outputting to ports 0xA3 (init) and 0xA2 
// (while running). Our emulation just ignores these.
}


