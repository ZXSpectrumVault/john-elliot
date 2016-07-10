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
#include "PcwMenuFont.hxx"

void joyce_dprintf(const char *s, ...)	/* printf() for the debugger console */
{
	va_list arg;

	va_start(arg, s);
	joyce_vdprintf(s, arg);
	va_end(arg);
}



void joyce_vdprintf(const char *s, va_list arg)
{
	if (!gl_sys || !gl_sys->getDebug() || !gl_sys->m_video)
					/* If debugger not active, or screen
					  not set up, write to log file */
	{
		if (fplog) 
		{
			vfprintf(fplog, s, arg);
			fflush(fplog);
		}
	}
	else
	{
		gl_sys->m_termDebug.vprintf(s, arg);
	}
}


