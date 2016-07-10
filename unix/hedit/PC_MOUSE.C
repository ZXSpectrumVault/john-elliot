/***************************************************************************
 *                                                                         *
 *    HHEDIT: Hungry Horace editor                                         *
 *    Copyright (C) 2009 John Elliott <jce@seasip.demon.co.uk>             *
 *                                                                         *
 *    This program is free software; you can redistribute it and/or modify *
 *    it under the terms of the GNU General Public License as published by *
 *    the Free Software Foundation; either version 2 of the License, or    *
 *    (at your option) any later version.                                  *
 *                                                                         *
 *    This program is distributed in the hope that it will be useful,      *
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of       *
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the        *
 *    GNU General Public License for more details.                         *
 *                                                                         *
 *    You should have received a copy of the GNU General Public License    *
 *    along with this program; if not, write to the Free Software          *
 *    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.            *
 *                                                                         *
 ***************************************************************************/

#include <dos.h>
#include "pc_mouse.h"

static unsigned st_have_mouse;

extern int gl_mxscale, gl_myscale;

int pc_mouse_init()
{
	unsigned long far *pvector = MK_FP(0, 0x33 * 4);
	union REGS rg;

	if (pvector[0] == 0) 
	{
		st_have_mouse = 0;
		return 0;	/* No mouse */
	}

	rg.x.ax = 0;
	int86(0x33, &rg, &rg);
	if (rg.x.ax) st_have_mouse = 1;
	return 1;
}

void pc_show_pointer(void)
{
	union REGS rg;

	if (!st_have_mouse) return;
	rg.x.ax = 1;
	int86(0x33, &rg, &rg);	
}


void pc_hide_pointer(void)
{
	union REGS rg;

	if (!st_have_mouse) return;
	rg.x.ax = 2;
	int86(0x33, &rg, &rg);	
}

int pc_poll_mouse(int *x, int *y, int *button)
{
	union REGS rg;

	if (!st_have_mouse) return 0;
	
	rg.x.ax = 3;
	int86(0x33, &rg, &rg);	
	if (gl_mxscale < 0) 
		*x = (rg.x.cx / -gl_mxscale); 
	else	*x = (rg.x.cx *  gl_mxscale);
	if (gl_myscale < 0) 
		*y = (rg.x.dx / -gl_myscale); 
	else	*y = (rg.x.dx *  gl_myscale);
	*button = rg.x.bx;
	return 1;
}



static int gl_idle = 0;

void pc_idle_init()
{
	unsigned long *ptr = MK_FP(0, 4 * 0x2F);	/* Is INT2F there? */

	if (*ptr) gl_idle = 1;
}

/* Release the CPU */
void idle()
{
	if (gl_idle)
	{
		union REGS rg;
		rg.x.ax = 0x1680;
		int86(0x2F, &rg, &rg);
	}
}
