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

#include "Pcw.hxx"
#include "PcwFid.hxx"
#include "inline.hxx"

extern void gsx_cmd(word pb);

static void fid_puts(Z80 *R, word addr, const char *s)
{
        while (*s) R->storeB(addr++, *s++);
        R->storeB(addr++, 0x0D);
        R->storeB(addr++, 0x0A);
        R->storeB(addr++, 0xFF);
}


void fid_cems(Z80 *R)
{
/* This is called with: 

	A = 11
	B = flags, currently 0
	C = 5
	D = error:  0 => a device has been got
		    1 => hook failure, no more devices
		    2 => hook failure, device name in use
		    3 => version control error
	HL -> 93-byte message buffer

    returns: A=0 => ret NC
	     A=1 => ret C
*/

	switch (R->d)
	{
		case 3:			/* Version control */
		fid_puts(R, R->getHL(), "VGA.FID: Incorrect FID environment");
		R->a = 0;
		return;

		case 2:			/* Drive letter in use */
		fid_puts(R, R->getHL(), "VGA.FID: VGA driver already loaded");
		R->a = 0;
		return;

		case 1:			/* No free memory */
		fid_puts(R, R->getHL(), "VGA.FID: No character devices available");
		R->a = 0;
		return;

		case 0:			/* Device hooked OK */
		fid_puts(R, R->getHL(), "VGA.FID loaded OK");
		R->a = 1;
		return;
	}

}


void vga_capabilities(Z80 *R)	/* This function new in v1.20 */
{
	/* Sets the following bits in HL: */
	/* Bit 0: If 800x600 screen is available */
	/* Bit 1: If GSX is available */
	/* Bit 2: (v1.22) If LIOS is available */
	
	R->h = 0;
	R->l = 7;	/* All features */
	R->b = R->c = R->d = R->e = 0;
	R->ix = 0;
	R->iy = 0;
}



void fid_char(Z80 *R)
{
	PcwVgaTerm &t = gl_sys->m_termVga;
	switch(R->c)
	{
		case 0: gl_sys->selectTerminal(gl_sys->m_defaultTerminal);
			break;
		case 1: gl_sys->selectTerminal(&gl_sys->m_termVga);
			break;
		case 2:	t.getVgaTerm() << R->e; 
			break;
		case 3: t.getVgaTerm().writeSys(R->e);  break;
/* v2.1.7: Reintroduce GSX */
 		case 4: gl_sys->m_termGSX.command(R->getDE()); break;
		case 5: fid_cems(R); break;
		case 6: vga_capabilities(R); break;	/* v1.20 */
	}
}

/* FIDEMS for the COM port driver */

void com_ems(Z80 *R)
{
	char str[90];

/* This is called with: 

	A = 0Dh
	C = 7
	D = error:  0 => a device has been got
		    1 => hook failure, no more devices
		    2 => hook failure, device name in use
		    3 => version control error
	HL -> 93-byte message buffer

    returns: A=0 => ret NC
	     A=1 => ret C
*/
	switch (R->d)
	{
		case 3:			/* Version control */
		fid_puts(R, R->getHL(), "COMPORT.FID: Incorrect FID environment");
		R->a = 0;
		return;

		case 2:			/* Device name in use*/
		fid_puts(R, R->getHL(), "COMPORT.FID: A COM port driver is already loaded");
		R->a = 0;
		return;

		case 1:			/* No free memory */
		fid_puts(R, R->getHL(), "COMPORT.FID: No character devices available");
		R->a = 0;
		return;

		case 0:			/* Device hooked OK */
		sprintf(str,"COMPORT.FID is obsolete. Delete it. "); /* Using port COM%d", jset.comport + 1); */
		fid_puts(R, R->getHL(), str);
		R->a = 1;
		return;
	}
}
